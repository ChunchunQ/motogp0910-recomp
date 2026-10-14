#include <rex/cvar.h>
#include <rex/input/input_system.h>
#include <iostream>
#include <stdexcept>
using namespace rex::input;
using rex::X_RESULT;
using rex::X_STATUS;
struct Driver final:InputDriver {
  X_INPUT_STATE state{};
  Driver():InputDriver(nullptr,0){}
  rex::X_STATUS Setup()override{return X_STATUS_SUCCESS;}
  void EnumerateDevices(std::vector<DeviceInfo>& out)override{DeviceInfo device{};device.id=static_cast<DeviceId>(1);out.push_back(device);}
  rex::X_RESULT GetDeviceState(DeviceId,X_INPUT_STATE* out)override{*out=state;return X_ERROR_SUCCESS;}
  rex::X_RESULT GetDeviceCapabilities(DeviceId,uint32_t,X_INPUT_CAPABILITIES* out)override{
    *out={};out->gamepad.thumb_lx=out->gamepad.thumb_ly=out->gamepad.thumb_rx=out->gamepad.thumb_ry=32767;return X_ERROR_SUCCESS;}
  rex::X_RESULT SetDeviceVibration(DeviceId,X_INPUT_VIBRATION*)override{return X_ERROR_SUCCESS;}
  rex::X_RESULT GetDeviceKeystroke(DeviceId,uint32_t,X_INPUT_KEYSTROKE*)override{return X_ERROR_EMPTY;}
};
int main(){try{
  int checks=0;auto check=[&](bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);++checks;};
  InputSystem system(nullptr);auto driver=std::make_unique<Driver>();auto* input=driver.get();system.AddDriver(std::move(driver));system.SetDeviceAssignment(std::make_unique<SlotAssignment>());check(system.Setup()==X_STATUS_SUCCESS,"Actual input system setup");
  rex::cvar::SetFlagByName("left_stick_deadzone_percentage","0.2");rex::cvar::SetFlagByName("right_stick_deadzone_percentage","0.2");
  for(int x:{-10000,-1000,0,1000,10000})for(int y:{-10000,-1000,0,1000,10000}){
    input->state.gamepad.thumb_lx=x;input->state.gamepad.thumb_ly=y;input->state.gamepad.thumb_rx=-x;input->state.gamepad.thumb_ry=-y;
    X_INPUT_STATE result{};check(system.GetState(0,&result)==X_ERROR_SUCCESS,"Actual guest input polling");
    if(x*x+y*y<4000000)check(result.gamepad.thumb_lx==0 && result.gamepad.thumb_ly==0 && result.gamepad.thumb_rx==0 && result.gamepad.thumb_ry==0,"Drift removed equally in all four quadrants");
    else check(result.gamepad.thumb_lx==x && result.gamepad.thumb_ly==y && result.gamepad.thumb_rx==-x && result.gamepad.thumb_ry==-y,"Large stick input preserved symmetrically");
  }
  input->state.gamepad.buttons=X_INPUT_GAMEPAD_A;system.AddUIInputBlocker();X_INPUT_STATE result{};
  check(system.GetState(0,&result)==X_ERROR_SUCCESS && result.gamepad.buttons==0,"Dialog blocks guest controls");
  system.RemoveUIInputBlocker();system.GetState(0,&result);check(result.gamepad.buttons==0,"Dialog dismissal press consumed");
  input->state.gamepad.buttons=0;system.GetState(0,&result);input->state.gamepad.buttons=X_INPUT_GAMEPAD_A;system.GetState(0,&result);check(result.gamepad.buttons==X_INPUT_GAMEPAD_A,"Fresh press accepted after release");
  system.Shutdown();std::cout<<checks<<" actual nightly deadzone and input-blocking checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
