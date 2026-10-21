// Binary KeyValues handling for Steam non-Steam shortcuts.
// Behavior reference: ChunchunQ/fifa09-recomp launcher/SteamShortcut.cs (MIT).
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace launcher::steam {
struct Value {
 uint8_t type=0;
 std::string key;
 std::vector<uint8_t> data;
 std::vector<Value> children;
};
inline bool Equal(std::string_view a,std::string_view b){
 if(a.size()!=b.size())return false;
 for(size_t i=0;i<a.size();++i){auto lower=[](unsigned char c){return c>='A'&&c<='Z'?c+32:c;};if(lower(a[i])!=lower(b[i]))return false;}
 return true;
}
inline Value* Find(std::vector<Value>& values,std::string_view key){
 for(auto& v:values)if(Equal(v.key,key))return &v;
 return nullptr;
}
inline std::string Text(const Value& value){
 return value.type==1&&!value.data.empty()?std::string(value.data.begin(),value.data.end()-1):std::string{};
}
class Reader {
 std::span<const uint8_t> bytes;size_t at=0,nodes=0;
 void Require(size_t n){if(n>bytes.size()-at)throw std::runtime_error("Truncated Steam shortcuts file");}
 std::string CString(){
  const auto start=at;
  while(at<bytes.size()&&bytes[at])++at;
  if(at==bytes.size())throw std::runtime_error("Unterminated Steam shortcuts string");
  const auto length=at-start;++at;
  return std::string(reinterpret_cast<const char*>(bytes.data()+start),length);
 }
 std::vector<Value> Object(unsigned depth){
  if(depth>32)throw std::runtime_error("Steam shortcuts nesting is too deep");
  std::vector<Value> result;
  for(;;){
   Require(1);const auto type=bytes[at++];if(type==8)return result;
   if(++nodes>100000)throw std::runtime_error("Steam shortcuts file has too many fields");
   Value v;v.type=type;v.key=CString();const auto start=at;
   switch(type){
    case 0:v.children=Object(depth+1);break;
    case 1:CString();break;
    case 2:case 3:case 4:case 6:Require(4);at+=4;break;
    case 7:Require(8);at+=8;break;
    case 5:for(;;){Require(2);const bool end=bytes[at]==0&&bytes[at+1]==0;at+=2;if(end)break;}break;
    default:throw std::runtime_error("Unsupported Steam shortcuts field type; the file was not changed");
   }
   if(type!=0)v.data.assign(bytes.begin()+start,bytes.begin()+at);
   result.push_back(std::move(v));
  }
 }
 public:
 explicit Reader(std::span<const uint8_t> value):bytes(value){if(bytes.size()>4*1024*1024)throw std::runtime_error("Steam shortcuts file is too large");}
 std::vector<Value> Read(){auto result=Object(0);if(at!=bytes.size())throw std::runtime_error("Unexpected trailing Steam shortcuts data");return result;}
};
inline void WriteObject(std::vector<uint8_t>& out,const std::vector<Value>& values){
 for(const auto& v:values){
  out.push_back(v.type);out.insert(out.end(),v.key.begin(),v.key.end());out.push_back(0);
  if(v.type==0)WriteObject(out,v.children);else out.insert(out.end(),v.data.begin(),v.data.end());
 }
 out.push_back(8);
}
inline std::vector<uint8_t> Write(const std::vector<Value>& values){std::vector<uint8_t> out;WriteObject(out,values);return out;}
inline void SetText(std::vector<Value>& values,std::string key,std::string text){
 Value value{1,std::move(key)};value.data.assign(text.begin(),text.end());value.data.push_back(0);
 if(auto* old=Find(values,value.key))*old=std::move(value);else values.push_back(std::move(value));
}
inline void SetInt(std::vector<Value>& values,std::string key,uint32_t number){
 Value value{2,std::move(key)};for(unsigned i=0;i<4;++i)value.data.push_back(static_cast<uint8_t>(number>>(8*i)));
 if(auto* old=Find(values,value.key))*old=std::move(value);else values.push_back(std::move(value));
}
inline uint32_t ShortcutId(std::string_view quoted_exe,std::string_view name){
 uint32_t crc=0xFFFFFFFF;
 for(auto input:{quoted_exe,name})for(unsigned char c:input){crc^=c;for(unsigned i=0;i<8;++i)crc=(crc>>1)^((crc&1)?0xEDB88320:0);}
 return ~crc|0x80000000u;
}
inline constexpr std::string_view kName="MotoGP 09/10";
inline std::vector<uint8_t> AddShortcut(std::span<const uint8_t> before,const std::string& quoted_exe,
 const std::string& start_dir,const std::string& icon){
 auto root=before.empty()?std::vector<Value>{}:Reader(before).Read();
 if(std::count_if(root.begin(),root.end(),[](const auto& value){return Equal(value.key,"shortcuts");})>1)
  throw std::runtime_error("Duplicate Steam shortcuts roots; the file was not changed");
 auto* list=Find(root,"shortcuts");
 if(!list){root.push_back(Value{0,"shortcuts"});list=&root.back();}
 if(list->type!=0)throw std::runtime_error("Invalid Steam shortcuts root; the file was not changed");
 Value* ours=nullptr;
 for(auto& entry:list->children){
  if(entry.type!=0)throw std::runtime_error("Invalid Steam shortcut entry; the file was not changed");
  auto* exe=Find(entry.children,"Exe");
  if(exe && Equal(Text(*exe),quoted_exe)){
   if(ours)throw std::runtime_error("Duplicate launcher shortcuts found; remove the duplicate in Steam first");
   ours=&entry;
  }
 }
 if(!ours){
  list->children.push_back(Value{0,""});ours=&list->children.back();
  for(auto key:{"IsHidden","OpenVR","Devkit","DevkitOverrideAppID","LastPlayTime"})SetInt(ours->children,key,0);
  for(auto key:{"AllowDesktopConfig","AllowOverlay"})SetInt(ours->children,key,1);
  for(auto key:{"ShortcutPath","LaunchOptions","DevkitGameID","FlatpakAppID"})SetText(ours->children,key,"");
  ours->children.push_back(Value{0,"tags"});
 }
 SetInt(ours->children,"appid",ShortcutId(quoted_exe,kName));
 SetText(ours->children,"AppName",std::string(kName));SetText(ours->children,"Exe",quoted_exe);
 SetText(ours->children,"StartDir",start_dir);SetText(ours->children,"icon",icon);
 // Retain user tags, launch options, play history and all unrelated/raw fields.
 for(size_t i=0;i<list->children.size();++i)list->children[i].key=std::to_string(i);
 return Write(root);
}
}
