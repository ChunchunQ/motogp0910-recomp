#include "metadata.h"
#include <stdexcept>
namespace motogp::lan {
bool ValidProperty(uint32_t id,std::span<const uint8_t> bytes){
  switch(id>>28){
    case 1:case 5:return bytes.size()==4;
    case 2:case 3:case 7:return bytes.size()==8;
    case 4:return bytes.size()>=2 && bytes.size()<=512 && bytes.size()%2==0 && bytes[bytes.size()-1]==0 && bytes[bytes.size()-2]==0;
    case 6:return bytes.size()<=512;
    default:return false;
  }
}
std::vector<uint8_t> EncodeMetadata(const Metadata& data){
  if(data.contexts.size()>32 || data.properties.size()>32)throw std::runtime_error("Too many LAN fields");
  Writer w;w.U8(1);w.U8(static_cast<uint8_t>(data.contexts.size()));
  for(auto [id,value]:data.contexts){w.U32(id);w.U32(value);}
  w.U8(static_cast<uint8_t>(data.properties.size()));
  for(const auto& [id,bytes]:data.properties){if(!ValidProperty(id,bytes))throw std::runtime_error("Invalid LAN property");w.U32(id);w.Blob(bytes);}
  if(w.bytes.size()>kMaxProperties)throw std::runtime_error("LAN metadata too large");return w.bytes;
}
std::optional<Metadata> DecodeMetadata(std::span<const uint8_t> bytes){try{
  if(bytes.empty())return Metadata{};if(bytes.size()>kMaxProperties)return {};
  Reader r(bytes);if(r.U8()!=1)return {};Metadata data;const auto contexts=r.U8();if(contexts>32)return {};
  for(unsigned i=0;i<contexts;++i){const auto id=r.U32(),value=r.U32();if(!data.contexts.emplace(id,value).second)return {};}
  const auto properties=r.U8();if(properties>32)return {};
  for(unsigned i=0;i<properties;++i){const auto id=r.U32();auto value=r.Blob(512);if(!ValidProperty(id,value) || !data.properties.emplace(id,std::move(value)).second)return {};}
  if(!r.End())return {};return data;
}catch(const std::exception&){return {};}}
}
