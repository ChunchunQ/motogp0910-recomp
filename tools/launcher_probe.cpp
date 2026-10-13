// Windows integration checks for settings preservation and actual child launch.
#include "../src/launcher/core.h"
#include <rex/cvar.h>
#include "../src/audio_settings.h"
#include <shellapi.h>
#include <iostream>
using namespace launcher;
int checks=0;
void Check(bool ok,const char* name) {
  if(!ok) throw std::runtime_error(name);
  ++checks;
}
void Write(const fs::path& path,std::string_view bytes) {
  std::ofstream file(path,std::ios::binary);file<<bytes;
  if(!file) throw std::runtime_error("Cannot write test fixture");
}
int wmain(int argc,wchar_t** argv) {
  // The same PE file is renamed motogp0910.exe for a real CreateProcessW test.
  if(argc>1 && std::wstring_view(argv[1]).starts_with(L"--game_data_root=")) {
    std::ofstream args("launch-arguments.txt",std::ios::binary);
    for(int i=1;i<argc;++i) args<<Utf8(argv[i])<<'\n';
    Write("launch-cwd.txt",Utf8(fs::current_path().wstring()));
    return 17;
  }
  try {
    if(argc!=2) throw std::runtime_error("Pass the real extracted game root");
    auto base=fs::current_path()/L"launcher-test-files";
    if(fs::exists(base)) throw std::runtime_error("Use a clean test working folder");
    fs::create_directories(base);
    const std::wstring unicode=L"Folder Türkçe & spaces\\";
    Check(Wide(Utf8(unicode))==unicode,"UTF-8 round trip");
    std::vector<std::wstring> quoted{L"program.exe",L"",L"a b",L"quotes\"inside",L"trailing\\",unicode,L"& % \" \\"};
    std::wstring command;
    for(const auto& arg:quoted) { if(!command.empty()) command+=L" ";command+=Quote(arg); }
    int count=0;auto** parsed=CommandLineToArgvW(command.c_str(),&count);
    Check(parsed && count==quoted.size(),"Quote argument count");
    for(int i=0;i<count;++i) Check(parsed[i]==quoted[i],"Quote argument value");
    LocalFree(parsed);
    auto file=base/L"motogp0910.toml";
    const std::string original="# Keep my settings\nuser_data_root = 'D:\\Existing saves'\ncustom_flag = 42\nkeybind_a = 'K'\n[window]\nwidth = 1600\nheight = 900\n[audio]\nmute = true\n[custom]\nitems = [1, 2, 3]\n";
    Write(file,original); Config config(file);
    Check(Settings::Load(config.table).width==1600,"Nested SDK key loading");
    Check(Settings::Load(config.table).mute,"Nested audio loading");
    Settings settings=Settings::Load(config.table);settings.width=1920;settings.height=1080;settings.fullscreen=false;
    settings.backend="xinput";settings.keyboard=false;settings.mute=true;
    settings.effect="fsr";settings.scale_x=2;settings.scale_y=3;settings.volume=37;
    settings.fsr_reduction=0.54;settings.anisotropic=5;settings.post_effect="fxaa";settings.adapter=0;
    settings.Store(config.table);Put(config.table,"game_data_root",Utf8(unicode));config.Save();
    Config restored(file);auto actual=Settings::Load(restored.table);
    Check(actual.width==1920 && actual.height==1080,"Window setting round trip");
    Check(!actual.fullscreen && actual.vsync && !actual.keyboard && actual.mute && actual.backend=="xinput","Boolean/input round trip");
    Check(actual.effect=="fsr" && actual.scale_x==2 && actual.scale_y==3 && actual.volume==37 && actual.anisotropic==5 && actual.post_effect=="fxaa", "Advanced settings round trip");
    Check(actual.adapter==0,"GPU index round trip");
    Check(std::abs(actual.fsr_reduction-0.54)<0.00001, "FSR strength round trip");
    Check(ValidBinding("Ctrl+K,Space") && ValidBinding("Shift+Up") && ValidBinding(""), "Valid SDK mapping syntax");
    Check(!ValidBinding("Shift") && !ValidBinding("enter") && !ValidBinding("Ctrl+") && !ValidBinding("Space,"), "Silent or malformed mappings rejected");
    auto invalid_settings=actual;invalid_settings.bindings[0]="enter";
    bool invalid_rejected=false;try{toml::table temporary;invalid_settings.Store(temporary);}catch(const Error&){invalid_rejected=true;}
    Check(invalid_rejected,"Invalid mappings cannot be saved");
    Check(Get(restored.table,"custom_flag",int64_t(0))==42,"Unknown key preserved");
    Check(Get(restored.table,"keybind_a",std::string{})=="K","Custom key binding preserved");
    Check(Get(restored.table,"user_data_root",std::string{})=="D:\\Existing saves","Save path preserved");
    Check(restored.table["custom"]["items"].as_array()->size()==3,"Unknown table/array preserved");
    Check(Get(restored.table,"game_data_root",std::string{})==Utf8(unicode),"Unicode path config round trip");
    Check(ReadFile(fs::path(file.wstring()+L".bak"))==original,"Exact original backup");
    config.Save();Check(ReadFile(fs::path(file.wstring()+L".bak"))==original,"Existing backup never overwritten");
    auto gpu_flags=LoadLibraryW((ExecutableFolder()/L"rexgpu-xenos.dll").c_str());
    Check(gpu_flags!=nullptr,"GPU plugin cvars loaded without presentation");
    rex::cvar::LoadConfig(file);
    Check(rex::cvar::Query<int32_t>("window_width")==1920 && rex::cvar::Query<int32_t>("window_height")==1080,"Actual SDK window flags");
    Check(!rex::cvar::Query<bool>("fullscreen") && rex::cvar::Query<bool>("vsync"),"Actual SDK display flags");
    Check(rex::cvar::Query<std::string>("input_backend")=="xinput" && !rex::cvar::Query<bool>("mnk_mode"),"Actual SDK input flags");
    Check(rex::cvar::Query<bool>("audio_mute"),"Actual SDK mute flag");
    Check(rex::cvar::Query<std::string>("present_effect")=="fsr" && rex::cvar::Query<int32_t>("anisotropic_override")==5, "Actual SDK graphics flags");
    Check(rex::cvar::Query<int32_t>("draw_resolution_scale_x")==2 && rex::cvar::Query<int32_t>("draw_resolution_scale_y")==3, "Actual SDK render scales");
    Check(rex::cvar::Query<int32_t>("d3d12_adapter")==0,"Actual SDK GPU selection flag");
    Check(rex::cvar::Query<bool>("native_2x_msaa"), "Actual SDK native MSAA flag");
    Check(rex::cvar::Query<std::string>("swap_post_effect")=="fxaa" && rex::cvar::Query<bool>("store_shaders"), "Actual SDK FXAA/cache flags");
    Check(rex::cvar::Query<int32_t>("audio_master_volume")==37, "Game volume cvar loaded");
    Write(file,"custom_flag = 77\n"); bool rejected=false;
    try{config.Save();}catch(const Error&){rejected=true;}
    Check(rejected && ReadFile(file)=="custom_flag = 77\n","Concurrent edit protected");
    Write(base/L"invalid.toml","bad = [\n");rejected=false;
    try{Config invalid(base/L"invalid.toml");}catch(const toml::parse_error&){rejected=true;}
    Check(rejected && ReadFile(base/L"invalid.toml")=="bad = [\n","Invalid config protected");
    Write(base/L"locked.toml","vsync = false\n");Config locked(base/L"locked.toml");
    Write(base/L"locked.toml.tmp","existing temporary file");rejected=false;
    try{locked.Save();}catch(const Error&){rejected=true;}
    Check(rejected && ReadFile(locked.path)=="vsync = false\n","Failed atomic save protected");
    Check(ReadFile(base/L"locked.toml.tmp")=="existing temporary file","Temporary file not clobbered");
    Config language(base/L"motogp0910-launcher.toml");
    Check(Get(language.table,"language",std::string("en"))=="en","Default language English");
    Put(language.table,"language",std::string("tr"));language.Save();
    Check(Get(Config(language.path).table,"language",std::string{})=="tr","Turkish language persisted");
    Put(language.table,"language",std::string("en"));language.Save();
    Check(Get(Config(language.path).table,"language",std::string{})=="en","English language persisted");
    toml::table roots;Check(ResolveGameRoot(base,roots)==base/L"game","Missing game default");
    Write(base/L"default.xex","test");Check(ResolveGameRoot(base,roots)==base,"Legacy flat fallback");
    fs::create_directories(base/L"game");Write(base/L"game/default.xex","test");
    Check(ResolveGameRoot(base,roots)==base/L"game","Nested game preferred");
    Put(roots,"game_data_root",std::string("Selected Folder"));
    Check(ResolveGameRoot(base,roots)==base/L"Selected Folder","Configured relative root");
    auto args=Arguments(base/L"Game Türkçe & spaces",settings,restored.table);
    for(const auto& [name,expected]:std::array<std::pair<const wchar_t*,const wchar_t*>,7>{{
        {L"draw_resolution_scale_x",L"2"},{L"draw_resolution_scale_y",L"3"},
        {L"resolution_scale",L"1"},{L"d3d12_adapter",L"0"},{L"audio_master_volume",L"37"},
        {L"anisotropic_override",L"5"},{L"present_fsr_sharpness_reduction",L"0.540000"}}}) {
      auto argument=L"--"+std::wstring(name)+L"="+expected;
      Check(std::count(args.begin(),args.end(),argument)==1,"Numeric settings keep their type in actual launch arguments");
      Check(rex::cvar::SetFlagByName(Utf8(name),Utf8(expected)),"Real SDK parses numeric launch values");
    }
    Check(std::find(args.begin(),args.end(),L"--user_data_root=D:\\Existing saves")!=args.end(),"Custom save root forwarded");
    Check(std::none_of(args.begin(),args.end(),[](const auto& arg){return arg.starts_with(L"--cache_root=");}),"Default cache left to SDK");
    // Real known user-supplied files; no synthetic XEX/patch or SDK stubs.
    auto program=ExecutableFolder();Validate(program,fs::path(argv[1]));++checks;
    auto broken=base/L"broken-game";fs::create_directories(broken);rejected=false;
    try{Validate(program,broken);}catch(const Error&){rejected=true;}
    Check(rejected,"Missing XEX blocked");Write(broken/L"default.xex","incorrect XEX");rejected=false;
    try{Validate(program,broken);}catch(const Error&){rejected=true;}
    Check(rejected && ReadFile(broken/L"default.xex")=="incorrect XEX","Incorrect XEX blocked and preserved");
    fs::copy_file(fs::path(argv[1])/L"default.xex",broken/L"default.xex",fs::copy_options::overwrite_existing);
    Write(broken/L"default.xexp","incorrect patch");rejected=false;
    try{Validate(program,broken);}catch(const Error&){rejected=true;}
    Check(rejected && ReadFile(broken/L"default.xexp")=="incorrect patch","Incorrect patch blocked and preserved");
    auto mock=base/L"Process Türkçe & spaces";fs::create_directories(mock);
    std::wstring self(32768,L'\0');self.resize(GetModuleFileNameW(nullptr,self.data(),self.size()));
    fs::copy_file(self,mock/L"motogp0910.exe");
    for(const auto& entry:fs::directory_iterator(program)) if(entry.path().extension()==L".dll") fs::copy_file(entry.path(),mock/entry.path().filename());
    auto process=Start(mock,args);Check(WaitForSingleObject(process,15000)==WAIT_OBJECT_0,"Child process completed");
    DWORD exit=0;GetExitCodeProcess(process,&exit);CloseHandle(process);
    Check(exit==17,"Child exit status");
    std::string expected;for(const auto& arg:args) expected+=Utf8(arg)+'\n';
    Check(ReadFile(mock/L"launch-arguments.txt")==expected,"Real child received all Unicode arguments");
    Check(ReadFile(mock/L"launch-cwd.txt")==Utf8(mock.wstring()),"Game working directory is executable folder");
    std::cout<<checks<<" launcher checks passed\n";return 0;
  }catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n';return 1;}
}
