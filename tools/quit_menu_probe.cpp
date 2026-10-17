#include "motogp0910_init.h"
#include "../src/quit_menu.h"
#include "../src/localization.h"
#include <rex/runtime.h>
#include <rex/graphics/xenos.h>
#include <rex/kernel/init.h>
#include <rex/input/input_system.h>
#include <rex/input/mnk/mnk_input_driver.h>
#include <rex/ui/imgui_drawer.h>
#include <rex/ui/window.h>
#include <rex/ui/windowed_app_context.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <array>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace rex::input;
using rex::X_RESULT;using rex::X_STATUS;
REX_EXTERN(sub_8227F318);
REX_EXTERN(sub_821E4EE0);
REX_EXTERN(sub_826EC558);
REX_EXTERN(sub_821FCCA8);
REX_EXTERN(sub_822AE9F8);
REX_EXTERN(sub_8229C2C0);
namespace {
int checks=0;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);++checks;}
struct Driver final:InputDriver {
 X_INPUT_STATE state{};Driver():InputDriver(nullptr,0){}
 X_STATUS Setup()override{return X_STATUS_SUCCESS;}
 void EnumerateDevices(std::vector<DeviceInfo>& out)override{DeviceInfo info{};info.id=static_cast<DeviceId>(1);out.push_back(info);}
 X_RESULT GetDeviceState(DeviceId,X_INPUT_STATE* out)override{*out=state;return X_ERROR_SUCCESS;}
 X_RESULT GetDeviceCapabilities(DeviceId,uint32_t,X_INPUT_CAPABILITIES* out)override{*out={};return X_ERROR_SUCCESS;}
 X_RESULT SetDeviceVibration(DeviceId,X_INPUT_VIBRATION*)override{return X_ERROR_SUCCESS;}
 X_RESULT GetDeviceKeystroke(DeviceId,uint32_t,X_INPUT_KEYSTROKE*)override{return X_ERROR_EMPTY;}
};
class UIContext final:public rex::ui::WindowedAppContext {void NotifyUILoopOfPendingFunctions()override{}void PlatformQuitFromUIThread()override{}};
}
int main(int argc,char** argv){try{
 Check(argc==3,"Usage: quit_menu_probe GAME_ROOT FONT_TTF");
 Driver* pad=nullptr;rex::input::mnk::MnkInputDriver* keyboard=nullptr;
 rex::Runtime runtime(std::filesystem::canonical(argv[1]));rex::RuntimeConfig config{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true};
 config.input_factory=[&](bool){auto input=std::make_unique<InputSystem>(nullptr);auto driver=std::make_unique<Driver>();pad=driver.get();input->AddDriver(std::move(driver));auto mnk=std::make_unique<rex::input::mnk::MnkInputDriver>(nullptr,0);keyboard=mnk.get();Check(keyboard->Setup()==X_STATUS_SUCCESS,"Actual MnK driver initializes");input->AddDriver(std::move(mnk));input->SetDeviceAssignment(std::make_unique<SlotAssignment>());return input;};
 Check(!XFAILED(runtime.Setup(PPCImageConfig,std::move(config)))&&!XFAILED(runtime.LoadXexImage("game:\\default.xex")),"Load actual patched game and input system");
 auto* memory=runtime.memory();auto* base=runtime.virtual_membase();const auto allocation=memory->SystemHeapAlloc(0x4000);Check(allocation!=0,"Fixture allocation");
 std::memset(base+allocation,0,0x4000);
 const auto write=[&](uint32_t a,uint32_t v){rex::memory::store_and_swap<uint32_t>(base+a,v);};const auto read=[&](uint32_t a){return rex::memory::load_and_swap<uint32_t>(base+a);};
 motogp::quit::Initialize(&runtime);motogp::quit::SetFocused(true);motogp::quit::PollFrontendInput();const auto label=motogp::quit::LabelAddress();
 { // The original native sequence owns fade and video cleanup. Only its
   // request byte is changed; final Press Start and unrelated modes reject it.
   constexpr std::array<uint32_t,6> addresses={0x82942CC0,0x82942CC4,0x82942CB0,0x828DA060,0x82942CBC,0x8288B188};
   std::array<uint32_t,6> saved{};for(size_t i=0;i<saved.size();++i)saved[i]=read(addresses[i]);
   write(0x82942CC0,0);write(0x82942CC4,4);write(0x82942CB0,0);write(0x828DA060,11);write(0x82942CBC,30);write(0x8288B188,allocation);
   Check(motogp::quit::IntroSkipAllowed(base)&&motogp::quit::BeginIntroSkip(base),"Loaded non-final intro accepts a native skip request");
   Check(base[0x82942CB2]==1&&read(0x82942CC0)==0,"Skip sets only the native fade request, never jumps the index");
   Check(!motogp::quit::BeginIntroSkip(base),"A pending transition cannot be requested twice");
   motogp::quit::FinishIntroSkip(base);Check(base[0x82942CB2]==1,"Request persists while native fade is in progress");
   write(0x82942CC0,1);motogp::quit::FinishIntroSkip(base);Check(!base[0x82942CB2],"Advancing one entry clears the owned request so the next intro cannot be skipped automatically");
   for(uint32_t index:{3u,4u,0xffffffffu}){write(0x82942CC0,index);Check(!motogp::quit::BeginIntroSkip(base),"Final title and invalid sequence indices cannot be skipped");}
   write(0x82942CC0,0);write(0x82942CBC,10);Check(!motogp::quit::BeginIntroSkip(base),"Skip waits for the native initial frame guard");write(0x82942CBC,30);
   write(0x828DA060,15);Check(!motogp::quit::BeginIntroSkip(base),"Frontend/gameplay mode cannot use intro skipping");write(0x828DA060,11);
   rex::cvar::SetFlagByName("allow_intro_skip","false");Check(!motogp::quit::BeginIntroSkip(base),"Launcher option disables manual intro skipping");rex::cvar::SetFlagByName("allow_intro_skip","true");
   write(0x82942CC4,33);Check(!motogp::quit::BeginIntroSkip(base),"Corrupt sequence count is rejected");
   write(0x82942CC4,4);write(0x82942CC0,0);
   motogp::quit::RequestIntroSkip();motogp::quit::PollIntroInput(base);Check(base[0x82942CB2]==1,"Fresh keyboard request reaches the native intro fade path");
   write(0x82942CC0,1);motogp::quit::FinishIntroSkip(base);
   pad->state.gamepad.buttons=X_INPUT_GAMEPAD_A;motogp::quit::PollFrontendInput();motogp::quit::PollIntroInput(base);Check(base[0x82942CB2]==1,"Cross/A from the real input-system path requests intro skipping");
   X_INPUT_STATE masked{};static_cast<InputSystem*>(runtime.input_system())->GetState(0,&masked);Check(!(masked.gamepad.buttons&X_INPUT_GAMEPAD_A),"The consumed Cross/A is release-masked from the guest");
   write(0x82942CC0,2);motogp::quit::FinishIntroSkip(base);motogp::quit::PollFrontendInput();motogp::quit::PollIntroInput(base);Check(!base[0x82942CB2],"Holding Cross/A cannot skip the next entry");
   pad->state.gamepad.buttons=0;motogp::quit::PollFrontendInput();motogp::quit::PollIntroInput(base);
   motogp::quit::SetFocused(false);motogp::quit::RequestIntroSkip();motogp::quit::PollIntroInput(base);Check(!base[0x82942CB2],"Unfocused startup cannot consume a keyboard request");motogp::quit::SetFocused(true);motogp::quit::PollFrontendInput();
   for(size_t i=0;i<saved.size();++i)write(addresses[i],saved[i]);
 }
 const auto iterator=allocation+0x700;write(iterator,label);
 for(unsigned char expected:std::string("QUIT GAME\0",10)){
   PPCContext decode{};decode.r3.u32=iterator;sub_826EC558(decode,base);
   Check(decode.r3.u32==expected,"Actual native text decoder reads every QUIT GAME codepoint");
 }
 Check(read(iterator)==label+10,"Native decoder advances exactly through UTF8 label and terminator");
 const auto menu=allocation,screen=allocation+0x500,stack=allocation+0x3000;
 write(menu,motogp::quit::kMainMenuVtable);constexpr std::array<uint32_t,7> ids={2,0,1,3,4,5,0xffffffff};
 for(unsigned i=0;i<ids.size();++i)write(menu+0x20+4*i,ids[i]);
 for(unsigned i=0;i<7;++i){const auto row=menu+0x3c+i*40;write(row+4,1);write(row+16,i<6?1:0);write(row+28,i<6?allocation+0x800+32*i:0);}
 std::array<uint8_t,0x3c+6*40> prefix;std::memcpy(prefix.data(),base+menu,prefix.size());
 const auto quit_row=menu+0x3c+6*40;Check(motogp::quit::AppendItem(base,menu,label),"Append seventh option");
 Check(std::memcmp(prefix.data(),base+menu,prefix.size())==0,"Original six rows and IDs unchanged");
 Check(read(quit_row+28)==label&&read(quit_row+40+28)==0,"New native label and terminator");
 Check(motogp::quit::AppendItem(base,menu,label),"Append is idempotent");
 const auto list=allocation+0x900,descriptor=allocation+0xA00;write(list,menu+0x3c);
 for(unsigned i=0;i<7;++i)Check(motogp::quit::IsWhiteAccentDraw(base,menu,list,i,0x8228380C)==(i==6),"Only added tile selects white accent in actual list-render call");
 Check(motogp::quit::IsWhiteAccentDraw(base,menu,list,6,0x82283868),"Both native strip passes select white");
 Check(!motogp::quit::IsWhiteAccentDraw(base,menu,list,6,0x12345678),"Other mesh callers stay unchanged");
 write(list,allocation);Check(!motogp::quit::IsWhiteAccentDraw(base,menu,list,6,0x8228380C),"Other menu arrays stay unchanged");write(list,menu+0x3c);
 for(unsigned i=0;i<7;++i)Check(motogp::quit::IsQuitLabelDraw(base,menu,menu+0x3c,i,0x82283540)==(i==6),"Only added label scopes cap recoloring");
 Check(!motogp::quit::IsQuitLabelDraw(base,menu,allocation+0x800,6,0x82283540),"Unrelated label arrays stay original");
 Check(!motogp::quit::IsQuitLabelDraw(base,menu,menu+0x3c,6,0x12345678),"Unrelated label render callers stay original");
 Check(motogp::quit::IsWhiteCapPass(0x82280334)&&motogp::quit::IsWhiteCapPass(0x822803A8),"Both colored cap layers select white");
 Check(!motogp::quit::IsWhiteCapPass(0x822802FC)&&!motogp::quit::IsWhiteCapPass(0x82280370),"Gray background passes remain untouched");
 Check(read(0x821038A8+32)==0x82379D38,"Original type-7 strip material resolves to scoped native draw hook");
 rex::graphics::xenos::xe_gpu_texture_fetch_t fetch{};fetch.type=rex::graphics::xenos::FetchConstantType::kTexture;fetch.format=rex::graphics::xenos::TextureFormat::k_DXT4_5;fetch.base_address=0x12345;fetch.swizzle=0x688;fetch.tiled=1;fetch.size_2d.width=1023;fetch.size_2d.height=127;
 for(unsigned i=0;i<6;++i)write(descriptor+28+4*i,reinterpret_cast<uint32_t*>(&fetch)[i]);
 std::array<uint8_t,52> original_descriptor;std::memcpy(original_descriptor.data(),base+descriptor,52);
 const auto white=motogp::quit::WhiteAccentDescriptor(base,descriptor);Check(white&&white!=descriptor,"White view gets a stable separate native texture descriptor");
 rex::graphics::xenos::xe_gpu_texture_fetch_t white_fetch{};for(unsigned i=0;i<6;++i)reinterpret_cast<uint32_t*>(&white_fetch)[i]=read(white+28+4*i);
 Check((white_fetch.swizzle&0x1ff)==(2|(2<<3)|(2<<6))&&(white_fetch.swizzle&0xe00)==(fetch.swizzle&0xe00),"White view preserves original blue brightness layers and alpha channel");
 white_fetch.swizzle=fetch.swizzle;Check(std::memcmp(&white_fetch,&fetch,sizeof(fetch))==0,"Original pixels, pitch, tiling, size, endian and sampler descriptors remain exact");
 Check(std::memcmp(original_descriptor.data(),base+descriptor,52)==0&&std::memcmp(base+white,base+descriptor,28)==0,"Shared blue descriptor and native resource header remain unchanged");
 Check(motogp::quit::WhiteAccentDescriptor(base,descriptor)==white,"Repeated frames reuse the same white view");
 auto move=[&](int selection,int delta){write(screen+0x18,selection);PPCContext ctx{};ctx.r1.u64=stack;ctx.lr=0x12345678;ctx.r30.u64=0x1122334455667788;ctx.r31.u64=0x8877665544332211;ctx.r3.u32=menu+0x3c;ctx.r4.u32=screen+0x18;ctx.r5.s64=delta;ctx.r6.u64=0;sub_8227F318(ctx,base);Check(ctx.r1.u32==stack&&ctx.lr==0x12345678&&ctx.r30.u64==0x1122334455667788&&ctx.r31.u64==0x8877665544332211,"Original menu navigation preserves ABI");return read(screen+0x18);};
 Check(move(5,1)==6,"Original navigation reaches Quit game");Check(move(6,1)==0,"Original navigation wraps after seventh item");Check(move(0,-1)==6,"Original navigation wraps back to Quit game");
 write(screen+0x18,6);
 for(uint32_t event:{0u,0x40u,0x400u,0x2000u,0x800u})Check(motogp::quit::IsQuitSelection(base,menu,screen,event)==bool(event&motogp::quit::kAcceptEvents),"Only native accept events activate Quit game");
 base[menu+0x244]=1;Check(!motogp::quit::IsQuitSelection(base,menu,screen,0x400),"Profile/loading transition blocks quitting");base[menu+0x244]=0;
 base[menu+0x245]=1;Check(!motogp::quit::IsQuitSelection(base,menu,screen,0x400),"Second native transition blocks quitting");base[menu+0x245]=0;
 for(unsigned i=0;i<6;++i){write(screen+0x18,i);Check(!motogp::quit::IsQuitSelection(base,menu,screen,0x400),"Original choices stay original choices");}write(screen+0x18,6);
 write(menu,0);Check(!motogp::quit::AppendItem(base,menu,label),"Unknown menu layout untouched");write(menu,motogp::quit::kMainMenuVtable);
 motogp::quit::Confirmation model;model.Open(X_INPUT_GAMEPAD_B);Check(!model.exit_selected(),"Cancel is selected by default");
 Check(!model.Update(X_INPUT_GAMEPAD_B).cancel,"Opening B press cannot dismiss its own prompt");model.Update(0);auto decision=model.Update(X_INPUT_GAMEPAD_A);Check(decision.cancel&&!decision.accept,"Default A cancels");
 model.Open(0);model.Update(0);model.Update(X_INPUT_GAMEPAD_DPAD_RIGHT);Check(model.exit_selected(),"Controller navigation selects exit");model.Update(0);decision=model.Update(X_INPUT_GAMEPAD_A);Check(decision.accept&&!decision.cancel,"A confirms explicit exit choice");
 model.Open(0);model.Update(0);Check(model.Update(X_INPUT_GAMEPAD_B).cancel,"Fresh B cancels");
 pad->state.gamepad.buttons=X_INPUT_GAMEPAD_A;PPCContext ctx{};ctx.r1.u64=stack;ctx.lr=0x12345678;ctx.r3.u32=menu;ctx.r4.u32=screen;ctx.r5.u32=0x400;ctx.r23.u64=0xAABBCCDD;ctx.r31.u64=0xDEADBEEF;
 sub_821FCCA8(ctx,base);Check(motogp::quit::Pending()&&ctx.r3.s32==-1,"Actual MainMenu hook consumes new action");Check(ctx.r1.u32==stack&&ctx.lr==0x12345678&&ctx.r23.u64==0xAABBCCDD&&ctx.r31.u64==0xDEADBEEF,"Hook preserves nonvolatile state");
 X_INPUT_STATE state{};auto* input=static_cast<InputSystem*>(runtime.input_system());input->GetState(0,&state);Check(state.gamepad.buttons==0,"Guest input is neutral while confirmation owns it");input->GetStateForUI(0,&state);Check(state.gamepad.buttons&X_INPUT_GAMEPAD_A,"UI can still read actual pad");
 ctx.r3.u32=menu;sub_821FCCA8(ctx,base);Check(ctx.r3.s32==-1,"Pending modal blocks original menu actions");
 {
 UIContext context;auto window=rex::ui::Window::Create(context,"Quit menu probe");ImFont* font=nullptr;
 rex::ui::ImGuiDrawer drawer(window.get(),10,[&](ImFontAtlas* atlas){static const ImWchar range[]={0x20,0x1ff,0};font=atlas->AddFontFromFileTTF(argv[2],17,nullptr,range);});
 auto& io=drawer.GetIO();io.DisplaySize=ImVec2(1280,720);io.DeltaTime=1.f/60;unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);io.Fonts->SetTexID(1);
 int exits=0;auto dialog=motogp::quit::CreateConfirmationDialog(&drawer,font,false,[&]{++exits;});
 auto frame=[&]{drawer.GetIO();ImGui::NewFrame();dialog->Draw();ImGui::Render();};
 frame();frame();Check(exits==0,"Opening controller press cannot auto-exit");
 io.AddKeyEvent(ImGuiKey_Escape,true);frame();Check(!motogp::quit::Pending()&&exits==0,"Esc cancels actual dialog");io.AddKeyEvent(ImGuiKey_Escape,false);frame();
 // Check the title-only source with actual native game mode property layout.
 motogp::quit::PollFrontendInput();input->GetState(0,&state);Check(state.gamepad.buttons==0,"Dismissal A remains consumed until release");
 pad->state.gamepad.buttons=0;motogp::quit::PollFrontendInput();input->GetState(0,&state);pad->state.gamepad.buttons=X_INPUT_GAMEPAD_A;input->GetState(0,&state);Check(state.gamepad.buttons&X_INPUT_GAMEPAD_A,"Fresh press reaches game after release");
 const uint32_t mode_ptr=0x8292710C,mode_value=0x82927110;const auto saved_ptr=read(mode_ptr),saved_value=read(mode_value);
 write(mode_ptr,0);Check(!motogp::quit::IsTitleMode(base),"Uninitialized startup property cannot open exit");
 write(mode_ptr,mode_value);write(mode_value,15);Check(motogp::quit::IsTitleMode(base),"Original mode 15 identifies title renderer path");
 Check(!motogp::quit::ObserveTitleDraw(base,0,1.0),"In-game Press Start overlay never arms title exit");
 Check(!motogp::quit::ObserveTitleDraw(base,1,.5,0x822AE94C),"Startup fade cannot arm title exit");
 write(mode_value,3);Check(!motogp::quit::ObserveTitleDraw(base,1,1.0,0x822AE94C),"Other game modes never arm title exit");write(mode_value,15);
 Check(motogp::quit::ObserveTitleDraw(base,1,1.0,0x822AE94C),"Visible secondary title prompt arms native Back path");
 // Boot's application phase is separate from the frontend menu property.
 // Drive the ORIGINAL transition function instead of inventing a fixture mode.
 const auto saved_boot=read(0x828DA060);write(mode_value,11);
 Check(!motogp::quit::ObserveTitleDraw(base,0,0,0x8229CA24),"Frontend property 11 cannot impersonate real boot phase");write(mode_value,15);PPCContext phase{};phase.r1.u32=stack;phase.lr=0x12345678;phase.r3.u32=11;
 sub_821E4EE0(phase,base);
 Check(read(0x828DA060)==11&&read(mode_value)==15,"Original boot transition leaves frontend property distinct from application phase");
 Check(read(0x8229CA18)==0x38800000&&read(0x8229CA20)==0x4BFFF759,"Original boot Press Start draw passes flag 0");
 write(mode_ptr,0);Check(motogp::quit::ObserveTitleDraw(base,0,0,0x8229CA24),"Real boot title works before the frontend property pointer is initialized");write(mode_ptr,mode_value);
 Check(!motogp::quit::ObserveTitleDraw(base,0,0,0x12345678),"Unrelated flag-0 draw cannot arm exit");
 Check(motogp::quit::ObserveTitleDraw(base,0,0,0x8229CA24),"Actual boot caller in mode 11 arms exit without a false fade requirement");
 rex::cvar::SetFlagByName("mnk_mode","true");rex::cvar::SetFlagByName("keybind_b","Quote,Backspace");pad->state.gamepad.buttons=0;
 Check(!motogp::quit::PollTitleInput(base,11),"Neutral native poll between draw and Back does not request or consume title exit");
 rex::ui::KeyEvent backspace(nullptr,rex::ui::VirtualKey::kBack,1,false,false,false,false,false);keyboard->OnKeyDown(backspace);
 input->GetStateForUI(0,&state);Check(state.gamepad.buttons&X_INPUT_GAMEPAD_B,"Actual Backspace binding reaches the same title B input channel");

 Check(read(0x8229C630)==0x3D00822B && read(0x8229C640)==0x3868E9F8,"Original boot transition installs the live frontend update callback");
 pad->state.gamepad.buttons=X_INPUT_GAMEPAD_B;ctx.r3.u64=0xBEEF;sub_8229C2C0(ctx,base);Check(motogp::quit::Pending(),"Actual boot title-update hook opens confirmation on fresh B");
 frame();frame();Check(motogp::quit::Pending()&&exits==0,"Held title B cannot immediately dismiss confirmation");
 io.AddKeyEvent(ImGuiKey_Escape,true);frame();Check(!motogp::quit::Pending()&&exits==0,"Title cancellation returns without exit");io.AddKeyEvent(ImGuiKey_Escape,false);frame();
 motogp::quit::PollFrontendInput();motogp::quit::Request();Check(!motogp::quit::Pending(),"UI-first dismissal cannot reopen title prompt before B release");input->GetState(0,&state);Check(!(state.gamepad.buttons&X_INPUT_GAMEPAD_B),"Title dismissal B stays masked for guest");
 keyboard->OnKeyUp(backspace);
 write(mode_ptr,saved_ptr);write(mode_value,saved_value);write(0x828DA060,saved_boot);
 motogp::quit::SetFocused(false);motogp::quit::Request();Check(!motogp::quit::Pending(),"Unfocused window cannot request exit");
 motogp::quit::SetFocused(true);pad->state.gamepad.buttons=X_INPUT_GAMEPAD_A;motogp::quit::PollFrontendInput();motogp::quit::Request();Check(!motogp::quit::Pending(),"Held input on focus regain cannot open confirmation");
 pad->state.gamepad.buttons=0;motogp::quit::PollFrontendInput();motogp::quit::Request();motogp::quit::PollFrontendInput();frame();frame();Check(motogp::quit::Pending()&&exits==0,"Fresh neutral input rearms safe confirmation");
 io.AddKeyEvent(ImGuiKey_Escape,true);frame();io.AddKeyEvent(ImGuiKey_Escape,false);frame();
 // Drain a guest blocker on the guest thread before returning to title.
 pad->state.gamepad.buttons=X_INPUT_GAMEPAD_A;ctx.r3.u32=menu;ctx.r4.u32=screen;ctx.r5.u32=0;motogp::quit::Request();sub_821FCCA8(ctx,base);
 frame();frame();Check(exits==0,"Held A does not confirm a new dialog");
 pad->state.gamepad.buttons=0;motogp::quit::PollFrontendInput();frame();
 io.AddKeyEvent(ImGuiKey_RightArrow,true);frame();
 // A Shift+Arrow virtual D-pad sample can reach the next UI frame, rather than the key event's frame.
 pad->state.gamepad.buttons=X_INPUT_GAMEPAD_DPAD_RIGHT;motogp::quit::PollFrontendInput();frame();
 io.AddKeyEvent(ImGuiKey_RightArrow,false);pad->state.gamepad.buttons=0;motogp::quit::PollFrontendInput();frame();
 io.AddKeyEvent(ImGuiKey_Enter,true);frame();Check(exits==1,"Delayed virtual D-pad does not double-apply keyboard navigation");frame();Check(exits==1,"Exit callback never repeats");
 Check(ImGui::GetCurrentContext()->ColorStack.Size==0&&ImGui::GetCurrentContext()->StyleVarStack.Size==0,"Dialog style remains scoped");dialog.reset();Check(!drawer.HasDialogs(),"App-owned dialog detaches without self-deletion");
 }
 motogp::quit::Stop();memory->SystemHeapFree(allocation);
 rex::cvar::SetFlagByName("game_text_language","turkish");motogp::quit::Initialize(&runtime);
 Check(std::string(reinterpret_cast<char*>(base+motogp::quit::LabelAddress()))=="OYUNDAN ÇIK","Native added menu label follows Turkish written language");
 motogp::quit::Stop();
 std::cout<<checks<<" native quit menu, original navigation, ABI, confirmation and input checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
