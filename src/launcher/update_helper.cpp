// Standalone /MT helper: copied to TEMP before launch, so updating bin does not
// replace a running image or rely on the runtime DLLs being replaced.
#include "update_protocol.h"
#include "../../third_party/nlohmann/json.hpp"
#include <chrono>
#include <exception>
#include <set>
#include <vector>

using namespace launcher::updates::detail;
namespace {
constexpr wchar_t kExtractScript[]=L"extract.ps1";
// Windows PowerShell 5/.NET provides ZIP deflate without shipping another
// runtime or a third-party archiver. Names are validated before any extraction.
constexpr char kExtract[]=R"PS(param([string]$Archive,[string]$Destination,[string]$CancelName)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$cancel = [Threading.EventWaitHandle]::OpenExisting($CancelName)
$zip = [IO.Compression.ZipFile]::OpenRead($Archive)
try {
  if ($zip.Entries.Count -gt 4096) { throw 'Too many update archive entries' }
  $base = [IO.Path]::GetFullPath($Destination).TrimEnd('\') + '\'
  [IO.Directory]::CreateDirectory($base) | Out-Null
  $seen = New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
  [long]$total = 0
  foreach ($entry in $zip.Entries) {
    if ($cancel.WaitOne(0)) { throw 'Update cancelled' }
    $name = $entry.FullName.Replace('\','/')
    if (!$name -or $name.StartsWith('/') -or $name.Contains(':') -or $name.Length -gt 1024) { throw 'Invalid archive path' }
    $parts = $name.TrimEnd('/').Split('/')
    foreach ($part in $parts) {
      if (!$part -or $part -eq '.' -or $part -eq '..' -or $part -match '[<>:"|?*\x00-\x1f]' -or $part.EndsWith('.') -or $part.EndsWith(' ') -or $part -match '^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\.|$)') { throw 'Unsafe archive path' }
    }
    if ((($entry.ExternalAttributes -shr 16) -band 0xf000) -eq 0xa000 -or ($entry.ExternalAttributes -band 0x400)) { throw 'Archive links are unsupported' }
    $path = [IO.Path]::GetFullPath([IO.Path]::Combine($base, $name.Replace('/','\')))
    if (!$path.StartsWith($base,[StringComparison]::OrdinalIgnoreCase) -or !$seen.Add($path)) { throw 'Duplicate or escaping archive path' }
    $total += $entry.Length
    if ($entry.Length -gt 1073741824 -or $total -gt 2147483648) { throw 'Extracted update exceeds the size limit' }
  }
  $buffer = New-Object byte[] 65536
  foreach ($entry in $zip.Entries) {
    if ($cancel.WaitOne(0)) { throw 'Update cancelled' }
    $name = $entry.FullName.Replace('/','\')
    $path = [IO.Path]::Combine($base,$name)
    if ($name.EndsWith('\')) { [IO.Directory]::CreateDirectory($path) | Out-Null; continue }
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path)) | Out-Null
    $entryStream = $entry.Open()
    try {
      $output = [IO.File]::Open($path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
      try {
        [long]$written = 0
        while (($count = $entryStream.Read($buffer,0,$buffer.Length)) -gt 0) {
          if ($cancel.WaitOne(0)) { throw 'Update cancelled' }
          $written += $count
          if ($written -gt $entry.Length) { throw 'Invalid extracted file size' }
          $output.Write($buffer,0,$count)
        }
        if ($written -ne $entry.Length) { throw 'Truncated extracted update' }
        $output.Flush($true)
      } finally { $output.Dispose() }
    } finally { $entryStream.Dispose() }
  }
} finally { $zip.Dispose(); $cancel.Dispose() }
)PS";

bool turkish=false,installation_authorized=false;
HWND progress_window{};
std::wstring progress_text;
LRESULT CALLBACK ProgressProc(HWND window,UINT message,WPARAM wp,LPARAM lp){
  if(message==WM_CLOSE)return 0;
  if(message==WM_PAINT){
    PAINTSTRUCT ps;auto dc=BeginPaint(window,&ps);RECT r;GetClientRect(window,&r);
    SetDCBrushColor(dc,RGB(23,26,33));FillRect(dc,&r,static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    RECT line{0,0,r.right,3};SetDCBrushColor(dc,RGB(255,106,19));FillRect(dc,&line,static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(242,244,247));auto font=CreateFontW(-17,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    auto old=SelectObject(dc,font);r.left+=24;r.right-=24;DrawTextW(dc,progress_text.c_str(),-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    SelectObject(dc,old);DeleteObject(font);EndPaint(window,&ps);return 0;
  }
  return DefWindowProcW(window,message,wp,lp);
}
void Progress(std::wstring text){
  progress_text=std::move(text);
  if(!progress_window){
    WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=ProgressProc;cls.lpszClassName=L"MotoGPUpdateInstaller";cls.hCursor=LoadCursorW(nullptr,IDC_WAIT);RegisterClassW(&cls);
    RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    progress_window=CreateWindowExW(WS_EX_TOPMOST,cls.lpszClassName,L"MotoGP 09/10",WS_POPUP|WS_BORDER,(work.left+work.right-440)/2,(work.top+work.bottom-110)/2,440,110,nullptr,nullptr,cls.hInstance,nullptr);
    ShowWindow(progress_window,SW_SHOWNOACTIVATE);
  }
  InvalidateRect(progress_window,nullptr,FALSE);UpdateWindow(progress_window);
  MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}
}
void NoLinks(const fs::path& path){
  for(auto current=path;!current.empty();){
    auto attributes=GetFileAttributesW(current.c_str());
    if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Update paths must not contain links or junctions");
    auto parent=current.parent_path();if(parent==current)break;current=std::move(parent);
  }
}
bool Below(const fs::path& file,const fs::path& directory){
  auto prefix=PathKey(directory);if(!prefix.ends_with(L"\\"))prefix+=L'\\';auto candidate=PathKey(file);
  return candidate==PathKey(directory)||candidate.starts_with(prefix);
}
bool PackageOwned(const fs::path& relative){
  const auto name=Fold(relative.generic_wstring());
  if(name==Fold(kLauncher))return true;
  const std::set<std::wstring> binaries{
      L"bin/motogp0910.exe",L"bin/motogp0910-ui.dll",L"bin/motogp0910-crash-monitor.exe",L"bin/motogp0910-update-helper.exe",L"bin/motogp0910.map",
      L"bin/rexruntime.dll",L"bin/rexgpu-xenos.dll",L"bin/amd_fidelityfx_dx12.dll",L"bin/msvcp140.dll",L"bin/msvcp140_atomic_wait.dll",L"bin/vcruntime140.dll",L"bin/vcruntime140_1.dll",
      L"bin/gamecontrollerdb.txt",L"bin/runtime-manifest.json"};
  if(binaries.contains(name))return true;
  if(name==L"resources/controller/ps3-controls.bin"||name==L"resources/controller/ps3-controls.json"||name==L"resources/controller/ps3-prompts.bin"||name==L"resources/controller/ps3-prompts.json")return true;
  if(name==L"resources/localization/tr/manifest.json"||name==L"resources/ui/opensans-semibold.ttf")return true;
  if(name==L"resources/archive/7z.exe"||name==L"resources/archive/7z.dll"||name==L"resources/archive/7zip-source.tar.xz")return true;
  if(name==L"resources/steam/icon.png"||name==L"resources/steam/logo.png"||name==L"resources/steam/banner.png"||name==L"resources/steam/cover.png"||name==L"resources/steam/wide.png")return true;
  return name.starts_with(L"resources/localization/tr/overlay/")&&(name.ends_with(L".font")||name.ends_with(L".lua")||name.ends_with(L".msg"));
}
struct File {fs::path relative;bool existed=false;bool attempted=false;};
void RetryCopy(const fs::path& from,const fs::path& to){
  for(int attempt=0;attempt<20;++attempt){if(CopyFileW(from.c_str(),to.c_str(),FALSE))return;if(progress_window)Progress(progress_text);Sleep(250);}
  throw std::runtime_error("Cannot copy update file. Check permissions, free space and running applications");
}
void AtomicCopy(const fs::path& from,const fs::path& to,const fs::path& scratch){
  NoLinks(to);fs::create_directories(to.parent_path());NoLinks(to.parent_path());RetryCopy(from,scratch);
  {Handle file(CreateFileW(scratch.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
    if(file.value==INVALID_HANDLE_VALUE||!FlushFileBuffers(file.value))throw std::runtime_error("Cannot flush update file");}
  for(int attempt=0;attempt<20;++attempt){if(MoveFileExW(scratch.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return;if(progress_window)Progress(progress_text);Sleep(250);}
  throw std::runtime_error("Cannot replace update file. The previous version will be restored");
}
void Extract(const fs::path& stage,HANDLE cancel,std::wstring_view cancel_name){
  const auto script=stage/kExtractScript;
  {std::ofstream output(script,std::ios::binary);output<<kExtract;output.close();if(!output)throw std::runtime_error("Cannot prepare ZIP extraction");}
  std::array<wchar_t,32768> buffer{};const auto count=GetSystemDirectoryW(buffer.data(),static_cast<UINT>(buffer.size()));
  if(!count||count>=buffer.size())throw std::runtime_error("Cannot locate Windows PowerShell");
  auto powershell=fs::path(buffer.data())/L"WindowsPowerShell/v1.0/powershell.exe";
  auto child=Spawn(powershell,L"-NoProfile -NonInteractive -ExecutionPolicy Bypass -File "+Quote(script.wstring())+L" -Archive "+Quote((stage/L"update.zip").wstring())+L" -Destination "+Quote((stage/L"files").wstring())+L" -CancelName "+Quote(cancel_name),stage);
  const auto deadline=GetTickCount64()+150000;
  while(WaitForSingleObject(child.value,100)==WAIT_TIMEOUT){
    if(WaitForSingleObject(cancel,0)==WAIT_OBJECT_0||GetTickCount64()>deadline){TerminateProcess(child.value,1);WaitForSingleObject(child.value,5000);throw std::runtime_error("Update extraction cancelled or timed out");}
  }
  DWORD code{};if(!GetExitCodeProcess(child.value,&code)||code)throw std::runtime_error("Could not extract update ZIP. The archive is invalid or Windows PowerShell is unavailable");
}
struct Restart {
  Handle process;
  Handle health;
  Handle permission;
};
Restart Launch(const fs::path& root,std::wstring_view token){
  Handle health(CreateEventW(nullptr,TRUE,FALSE,EventName(token,L"healthy").c_str()));
  Handle permission(CreateEventW(nullptr,TRUE,TRUE,EventName(token,L"launch").c_str()));
  if(!health.value||!permission.value)throw std::runtime_error("Cannot prepare launcher restart");
  auto process=Spawn(root/kLauncher,L"--update-ack "+Quote(token),root,false);
  return {std::move(process),std::move(health),std::move(permission)};
}
bool Healthy(Restart& restart){
  // Missing startup acknowledgement or an early exit triggers the fallback.
  HANDLE handles[]{restart.health.value,restart.process.value};
  const auto deadline=GetTickCount64()+30000;
  bool acknowledged=false;
  while(GetTickCount64()<deadline){
    const auto status=WaitForMultipleObjects(2,handles,FALSE,100);
    if(status==WAIT_OBJECT_0){acknowledged=true;break;}
    if(status!=WAIT_TIMEOUT)return false;
    if(progress_window)Progress(progress_text);
  }
  if(!acknowledged)return false;
  const auto stable=GetTickCount64()+2000;
  while(GetTickCount64()<stable){
    if(WaitForSingleObject(restart.process.value,100)!=WAIT_TIMEOUT)return false;
    if(progress_window)Progress(progress_text);
  }
  return true;
}
void StopFailedRestart(Restart& restart){
  if(WaitForSingleObject(restart.process.value,0)==WAIT_TIMEOUT){
    // This is only the new launcher process started by this helper. The game
    // is never terminated. An unresponsive launcher cannot retain updated DLLs.
    TerminateProcess(restart.process.value,1);WaitForSingleObject(restart.process.value,10000);
  }
}
void Install(const fs::path& job_path){
  const auto stage=fs::canonical(job_path.parent_path());
  std::array<wchar_t,32768> path{};GetModuleFileNameW(nullptr,path.data(),static_cast<DWORD>(path.size()));
  if(PathKey(fs::path(path.data()).parent_path())!=PathKey(stage))throw std::runtime_error("Invalid updater working directory");
  const std::wstring prefix=L"MotoGP0910-update-";auto name=stage.filename().wstring();
  if(!name.starts_with(prefix)||!Token(std::wstring_view(name).substr(prefix.size())))throw std::runtime_error("Invalid updater token");
  const auto token=name.substr(prefix.size());
  std::array<wchar_t,32768> temp{};const auto count=GetTempPathW(static_cast<DWORD>(temp.size()),temp.data());
  if(!count||count>=temp.size()||PathKey(stage.parent_path())!=PathKey(temp.data()))throw std::runtime_error("Updater must run from Windows TEMP");
  NoLinks(stage);
  if(job_path.filename()!=L"install.json"||fs::file_size(job_path)>65536)throw std::runtime_error("Invalid update job");
  std::ifstream input(job_path);const auto job=nlohmann::json::parse(input);
  auto root=fs::canonical(fs::path(WideText(job.at("root").get<std::string>())));NoLinks(root);
  if(!fs::is_regular_file(root/kLauncher))throw std::runtime_error("Installed launcher not found");
  turkish=job.at("turkish").get<bool>();
  Handle lock(CreateMutexW(nullptr,FALSE,LockName(root).c_str()));
  const bool already=GetLastError()==ERROR_ALREADY_EXISTS;
  if(!lock.value||already)throw std::runtime_error("Another update is already running");
  Handle ready(OpenEventW(EVENT_MODIFY_STATE,FALSE,EventName(token,L"ready").c_str()));
  Handle commit(OpenEventW(SYNCHRONIZE,FALSE,EventName(token,L"commit").c_str()));
  Handle cancel(OpenEventW(SYNCHRONIZE,FALSE,EventName(token,L"cancel").c_str()));
  if(!ready.value||!commit.value||!cancel.value)throw std::runtime_error("Original launcher is no longer available");
  Handle parent(OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,job.at("parent").get<DWORD>()));
  FILETIME created{},exit{},kernel{},user{};
  if(!parent.value||!GetProcessTimes(parent.value,&created,&exit,&kernel,&user)||
      ((static_cast<uint64_t>(created.dwHighDateTime)<<32)|created.dwLowDateTime)!=job.at("parent_created").get<uint64_t>())throw std::runtime_error("Original launcher identity changed");
  if(FileHash(stage/L"update.zip")!=Utf8Text(Fold(WideText(job.at("sha256").get<std::string>()))))throw std::runtime_error("Update ZIP checksum mismatch");
  Extract(stage,cancel.value,EventName(token,L"cancel"));
  auto payload=stage/L"files";
  if(!fs::is_regular_file(payload/kLauncher)){
    std::vector<fs::path> children;for(auto& entry:fs::directory_iterator(payload))children.push_back(entry.path());
    if(children.size()!=1||!fs::is_directory(children[0]))throw std::runtime_error("ZIP does not contain a MotoGP release");
    payload=children[0];
  }
  for(const auto* required:{kLauncher,L"bin/motogp0910-ui.dll",L"bin/motogp0910.exe",L"bin/motogp0910-update-helper.exe",L"bin/rexruntime.dll",L"bin/rexgpu-xenos.dll",L"bin/amd_fidelityfx_dx12.dll",L"bin/msvcp140.dll",L"bin/msvcp140_atomic_wait.dll",L"bin/vcruntime140.dll",L"bin/vcruntime140_1.dll"})
    if(!fs::is_regular_file(payload/required))throw std::runtime_error("Update ZIP is missing required MotoGP program files");
  std::vector<fs::path> protected_roots;
  for(auto& item:job.at("protected"))protected_roots.emplace_back(WideText(item.get<std::string>()));
  std::vector<File> files;
  for(auto& entry:fs::recursive_directory_iterator(payload)){
    NoLinks(entry.path());if(entry.is_directory())continue;
    auto relative=entry.path().lexically_relative(payload);
    // Archives can carry documentation and an empty game directory, but only
    // files explicitly owned by the application are ever installed.
    if(!PackageOwned(relative))continue;
    auto destination=root/relative;NoLinks(destination);
    for(auto& preserved:protected_roots)if(Below(destination,preserved))throw std::runtime_error("Update overlaps a configured game or user data directory");
    if(fs::exists(destination)&&!fs::is_regular_file(destination))throw std::runtime_error("Update destination is not a regular file");
    const auto attributes=GetFileAttributesW(destination.c_str());
    if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_READONLY))throw std::runtime_error("An installed update file is read-only. Make the application folder writable before updating");
    const bool existed=fs::exists(destination);
    if(existed&&FileHash(destination)==FileHash(entry.path()))continue;
    files.push_back({relative,existed,false});
  }
  if(RunningGame(root))throw std::runtime_error("Close the game before installing the update");
  const auto backup=root/(L".motogp-update-"+token);
  if(!fs::create_directory(backup))throw std::runtime_error("Cannot create update backup. Check write permissions and free space");
  bool modified=false,success=false,rollback_complete=true;
  try{
    fs::create_directory(backup/L"original");fs::create_directory(backup/L"new");
    nlohmann::json journal=nlohmann::json::array();
    for(auto& file:files){
      if(file.existed){auto saved=backup/L"original"/file.relative;fs::create_directories(saved.parent_path());RetryCopy(root/file.relative,saved);if(FileHash(saved)!=FileHash(root/file.relative))throw std::runtime_error("Cannot verify update backup");}
      journal.push_back({{"path",Utf8Text(file.relative.generic_wstring())},{"existed",file.existed}});
    }
    {std::ofstream output(backup/L"files.json");output<<journal.dump();output.close();if(!output)throw std::runtime_error("Cannot save update backup index");}
    if(!SetEvent(ready.value))throw std::runtime_error("Cannot notify original launcher");
    HANDLE events[]{commit.value,cancel.value,parent.value};
    if(WaitForMultipleObjects(3,events,FALSE,60000)!=WAIT_OBJECT_0)throw std::runtime_error("Update installation was not authorized");
    installation_authorized=true;
    if(WaitForSingleObject(parent.value,30000)!=WAIT_OBJECT_0)throw std::runtime_error("Original launcher did not close; update cancelled");
    if(RunningGame(root))throw std::runtime_error("Game started before installation; update cancelled");
    Progress(turkish?L"Güncelleme kuruluyor…":L"Installing update…");
    for(size_t i=0;i<files.size();++i){
      auto& file=files[i];file.attempted=true;modified=true;
      AtomicCopy(payload/file.relative,root/file.relative,backup/L"new"/L"replace.tmp");
      if(FileHash(root/file.relative)!=FileHash(payload/file.relative))throw std::runtime_error("Installed update file verification failed");
      Progress(std::wstring(turkish?L"Güncelleme kuruluyor… ":L"Installing update… ")+std::to_wstring((i+1)*100/std::max<size_t>(1,files.size()))+L"%");
    }
    Progress(turkish?L"Launcher yeniden açılıyor…":L"Restarting launcher…");
    auto restart=Launch(root,token);
    if(!Healthy(restart)){StopFailedRestart(restart);throw std::runtime_error("The updated launcher did not start successfully");}
    success=true;
  }catch(...){
    // Fallback mechanism: restore the previous program files if installation
    // or restart fails. Game data, DLC, saves and settings are excluded.
    const auto failure=std::current_exception();
    if(modified){
      Progress(turkish?L"Önceki sürüm geri yükleniyor…":L"Restoring previous version…");
      for(auto it=files.rbegin();it!=files.rend();++it){
        if(!it->attempted)continue;
        try{
          if(it->existed)AtomicCopy(backup/L"original"/it->relative,root/it->relative,backup/L"new"/L"restore.tmp");
          else if(fs::exists(root/it->relative)&&!fs::remove(root/it->relative))throw std::runtime_error("Cannot remove new update file");
        }catch(...){rollback_complete=false;}
      }
      if(rollback_complete){
        try{auto restart=Launch(root,token);if(!Healthy(restart))StopFailedRestart(restart);}catch(...){}
      }
    }else if(WaitForSingleObject(parent.value,0)==WAIT_OBJECT_0){try{auto restart=Launch(root,token);Healthy(restart);}catch(...) {}}
    // If fallback itself fails, retain the backup for manual recovery.
    std::error_code error;if(rollback_complete)fs::remove_all(backup,error);
    if(!rollback_complete)throw std::runtime_error("Automatic rollback could not finish. Your previous files are preserved in the .motogp-update backup directory. Close the launcher and restore that backup");
    std::rethrow_exception(failure);
  }
  // Remove the fallback backup only after confirmed healthy startup.
  if(success){std::error_code error;fs::remove_all(backup,error);}
  // The running helper image itself can remain in TEMP until a later cleanup;
  // remove the potentially large archive, payload, script and job now.
  for(const auto* item:{L"files",L"update.zip",kExtractScript,L"install.json"}){std::error_code error;fs::remove_all(stage/item,error);}
}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
  int count{};auto arguments=CommandLineToArgvW(GetCommandLineW(),&count);fs::path job;
  if(arguments){if(count==2)job=arguments[1];LocalFree(arguments);}
  if(job.empty())return 1;
  try{Install(job);if(progress_window)DestroyWindow(progress_window);return 0;}
  catch(const std::exception& e){
    // Preparation errors are reported by the still-open launcher; installation
    // errors must be visible even after that launcher has exited.
    {std::ofstream output(job.parent_path()/L"error.txt",std::ios::binary);output<<e.what();}
    if(progress_window||installation_authorized){
      if(progress_window)DestroyWindow(progress_window);std::wstring details;
      try{details=WideText(e.what());}catch(...){details=L"See the updater error.txt for details.";}
      MessageBoxW(nullptr,(std::wstring(turkish?L"Güncelleme tamamlanamadı.\n":L"Update could not finish.\n")+details).c_str(),L"MotoGP 09/10",MB_OK|MB_ICONERROR);
    }
    return 1;
  }
}
