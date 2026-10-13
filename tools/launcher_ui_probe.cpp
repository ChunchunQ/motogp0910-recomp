// Run while the launcher is open, on a Windows desktop (Wine/Xvfb supported).
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
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
  if(Text(child(114))!=L"Türkçe" || Text(child(113))!=L"OYUNU BAŞLAT" ||
     Text(child(103))!=L"1920 × 1080" || SendMessageW(child(104),CB_GETCURSEL,0,0)!=1 ||
     SendMessageW(child(105),BM_GETCHECK,0,0)!=BST_UNCHECKED ||
     SendMessageW(child(107),BM_GETCHECK,0,0)!=BST_UNCHECKED ||
     SendMessageW(child(108),BM_GETCHECK,0,0)!=BST_CHECKED) return 1;
 } else if(mode=="language-en" || mode=="language-tr") {
  SendMessageW(child(114),CB_SETCURSEL,mode=="language-tr"?1:0,0);
  SendMessageW(w,WM_COMMAND,MAKEWPARAM(114,CBN_SELCHANGE),reinterpret_cast<LPARAM>(child(114)));
  if(Text(child(113))!=(mode=="language-tr"?L"OYUNU BAŞLAT":L"PLAY")) return 1;
 } else if(mode=="defaults") {
  SendMessageW(w,WM_COMMAND,MAKEWPARAM(110,BN_CLICKED),0);
  if(Text(child(103))!=(Text(child(114))==L"Türkçe"?L"Otomatik":L"Automatic") ||
     SendMessageW(child(107),BM_GETCHECK,0,0)!=BST_CHECKED) return 1;
 } else if(mode=="save" || mode=="play" || mode=="report" || mode=="browse" || mode=="logs") {
  int id=mode=="save"?109:mode=="play"?113:mode=="report"?112:mode=="browse"?102:111;
  PostMessageW(w,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),0);
 } else if(mode=="missing-root") {
  SetWindowTextW(child(101),L"Z:\\missing-game-folder");
 } else if(mode=="close") PostMessageW(w,WM_CLOSE,0,0);
 else return 2;
 std::cout<<"PASS: "<<mode<<'\n';return 0;
}
