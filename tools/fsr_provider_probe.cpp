#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <iostream>
#include <cstdint>
int main(){
  auto runtime=LoadLibraryW(L"rexruntime.dll");
  if(!runtime){std::cerr<<"Runtime load failed: "<<GetLastError()<<'\n';return 1;}
  using Select=uint64_t(*)(uint32_t,void*);
  auto select=reinterpret_cast<Select>(GetProcAddress(runtime,"Motogp_SelectFsrProvider"));
  if(!select)return 2;
  auto two=select(2,nullptr),three=select(3,nullptr);
  if(!two || !three || two==three || select(1,nullptr)){std::cerr<<"Invalid/missing distinct providers\n";return 3;}
  std::cout<<"FidelityFX providers: FSR2="<<std::hex<<two<<" FSR3="<<three<<"; distinct real provider selection passed\n";
  return 0;
}
