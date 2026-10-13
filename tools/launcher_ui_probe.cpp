// Run while the launcher is open, on a Windows desktop (Wine/Xvfb supported).
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "../src/launcher/ui_ids.h"
using namespace launcher;
#include <commctrl.h>
#include <string>
#include <iostream>
std::wstring Text(HWND h) {
 std::wstring s(SendMessageW(h,WM_GETTEXTLENGTH,0,0)+1,L'\0');
 s.resize(SendMessageW(h,WM_GETTEXT,s.size(),reinterpret_cast<LPARAM>(s.data())));return s;
}
int main(int argc,char** argv) {
 auto w=FindWindowW(L"MotoGP0910Launcher",nullptr);
 if(!w || argc!=2) return 2;
 std::string mode=argv[1];
 auto child=[&](int id){return GetDlgItem(w,id);};
 if(mode=="check-saved") {
  if(Text(child(kLanguage))!=L"Türkçe" || Text(child(kPlay))!=L"OYUNU BAŞLAT" ||
     Text(child(kSize))!=L"1920 × 1080" || SendMessageW(child(kBackend),CB_GETCURSEL,0,0)!=1 ||
     SendMessageW(child(kFullscreen),BM_GETCHECK,0,0)!=BST_UNCHECKED ||
     SendMessageW(child(kKeyboard),BM_GETCHECK,0,0)!=BST_UNCHECKED ||
     SendMessageW(child(kMute),BM_GETCHECK,0,0)!=BST_CHECKED) return 1;
 } else if(mode=="language-en" || mode=="language-tr") {
  SendMessageW(child(kLanguage),CB_SETCURSEL,mode=="language-tr"?1:0,0);
  SendMessageW(w,WM_COMMAND,MAKEWPARAM(kLanguage,CBN_SELCHANGE),reinterpret_cast<LPARAM>(child(kLanguage)));
  if(Text(child(kPlay))!=(mode=="language-tr"?L"OYUNU BAŞLAT":L"PLAY")) return 1;
 } else if(mode=="defaults") {
  SendMessageW(w,WM_COMMAND,MAKEWPARAM(110,BN_CLICKED),0);
  if(Text(child(kSize))!=(Text(child(kLanguage))==L"Türkçe"?L"Otomatik / masaüstü":L"Automatic / desktop") ||
     SendMessageW(child(kKeyboard),BM_GETCHECK,0,0)!=BST_CHECKED) return 1;
 } else if(mode=="graphics" || mode=="audio" || mode=="controls") {
  int page=mode=="graphics"?0:mode=="audio"?1:2;
  SendMessageW(w,WM_COMMAND,MAKEWPARAM(kNavGraphics+page,BN_CLICKED),0);
  if(!IsWindowVisible(child(page==0?kSize:page==1?kVolume:kBindingBase))) return 1;
 } else if(mode=="configure-advanced") {
  SendMessageW(child(kEffect),CB_SETCURSEL,1,0); // FSR 1
  SendMessageW(w,WM_COMMAND,MAKEWPARAM(kEffect,CBN_SELCHANGE),0);
  SendMessageW(child(kSharpness),TBM_SETPOS,TRUE,73);
  SendMessageW(w,WM_HSCROLL,TB_THUMBPOSITION,reinterpret_cast<LPARAM>(child(kSharpness)));
  SendMessageW(child(kRenderResolution),CB_SETCURSEL,1,0);

  SendMessageW(child(kAnisotropic),CB_SETCURSEL,6,0);
  SendMessageW(child(kVolume),TBM_SETPOS,TRUE,37);
  SendMessageW(w,WM_HSCROLL,TB_THUMBPOSITION,reinterpret_cast<LPARAM>(child(kVolume)));
  SendMessageW(child(kMsaa),BM_SETCHECK,BST_UNCHECKED,0);
  SetWindowTextW(child(kBindingBase),L"Ctrl+K,Space");
  SendMessageW(w,WM_COMMAND,MAKEWPARAM(kBindingBase,EN_CHANGE),reinterpret_cast<LPARAM>(child(kBindingBase)));
 } else if(mode=="standard-root") {
  SetWindowTextW(child(kPath),L".\\game");
 } else if(mode=="demo-graphics") {
  SendMessageW(child(kSize),CB_SETCURSEL,3,0);
  SendMessageW(child(kRenderResolution),CB_SETCURSEL,0,0);

  SendMessageW(child(kEffect),CB_SETCURSEL,1,0);
  SendMessageW(w,WM_COMMAND,MAKEWPARAM(kEffect,CBN_SELCHANGE),0);
  SendMessageW(child(kSharpness),TBM_SETPOS,TRUE,85);
  SendMessageW(w,WM_HSCROLL,TB_THUMBPOSITION,reinterpret_cast<LPARAM>(child(kSharpness)));
 } else if(mode=="capture") {
  SendMessageW(w,WM_COMMAND,MAKEWPARAM(kCaptureBase,BN_CLICKED),0);
 } else if(mode=="check-advanced") {
  if(SendMessageW(child(kEffect),CB_GETCURSEL,0,0)!=1 ||
     SendMessageW(child(kSharpness),TBM_GETPOS,0,0)!=73 ||
     SendMessageW(child(kRenderResolution),CB_GETCURSEL,0,0)!=1 ||
     SendMessageW(child(kAnisotropic),CB_GETCURSEL,0,0)!=6 ||
     SendMessageW(child(kVolume),TBM_GETPOS,0,0)!=37 ||
     SendMessageW(child(kMsaa),BM_GETCHECK,0,0)!=BST_UNCHECKED || Text(child(kBindingBase))!=L"Ctrl+K,Space") return 1;
 } else if(mode=="save" || mode=="play" || mode=="report" || mode=="browse" || mode=="logs") {
  int id=mode=="save"?109:mode=="play"?113:mode=="report"?112:mode=="browse"?102:111;
  PostMessageW(w,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),0);
 } else if(mode=="missing-root") {
  SetWindowTextW(child(kPath),L"Z:\\missing-game-folder");
 } else if(mode=="close") PostMessageW(w,WM_CLOSE,0,0);
 else return 2;
 std::cout<<"PASS: "<<mode<<'\n';return 0;
}
