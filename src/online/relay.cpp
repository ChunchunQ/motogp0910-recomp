#include "relay.h"
#include <algorithm>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#else
#include <openssl/hmac.h>
#endif
namespace motogp::online {
namespace {
std::array<uint8_t,32> Mac(const Ticket& ticket,std::span<const uint8_t> bytes,bool server){
  std::vector<uint8_t> data{static_cast<uint8_t>(server?'S':'C')};data.insert(data.end(),bytes.begin(),bytes.end());
  std::array<uint8_t,32> output{};
#ifdef _WIN32
  BCRYPT_ALG_HANDLE algorithm=nullptr;
  if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0)
    throw std::runtime_error("Online HMAC initialization failed");
  const auto status=BCryptHash(algorithm,const_cast<PUCHAR>(ticket.key.data()),static_cast<ULONG>(ticket.key.size()),data.data(),static_cast<ULONG>(data.size()),output.data(),static_cast<ULONG>(output.size()));
  BCryptCloseAlgorithmProvider(algorithm,0);
  if(status<0)throw std::runtime_error("Online HMAC failed");
#else
  unsigned length=0;if(!HMAC(EVP_sha256(),ticket.key.data(),static_cast<int>(ticket.key.size()),data.data(),data.size(),output.data(),&length) || length!=output.size())throw std::runtime_error("Online HMAC failed");
#endif
  return output;
}
}
lan::Endpoint VirtualEndpoint(uint64_t peer){
  if((peer&0xFFFFFFFFFF000000ull)!=0x0009000000000000ull || !(peer&0xFFFFFF))throw std::runtime_error("Invalid online peer identity");
  return {0x0A000000u|static_cast<uint32_t>(peer&0xFFFFFF),lan::kFirstPort};
}
uint64_t VirtualPeer(lan::Endpoint ep){return (ep.ip&0xFF000000)==0x0A000000 && (ep.ip&0xFFFFFF) && ep.port==lan::kFirstPort?0x0009000000000000ull|(ep.ip&0xFFFFFF):0;}
std::vector<uint8_t> Seal(const Ticket& ticket,uint64_t sequence,uint64_t destination,std::span<const uint8_t> packet,bool server){
  if(!sequence || packet.size()>lan::kMaxPacket)throw std::runtime_error("Invalid relay envelope");
  lan::Writer w;w.U32(0x4D474F31);w.Raw(ticket.id);w.U64(sequence);w.U64(destination);w.Raw(packet);
  const auto mac=Mac(ticket,w.bytes,server);w.Raw(mac);return w.bytes;
}
std::optional<Envelope> Open(const Ticket& ticket,std::span<const uint8_t> bytes,bool server){
  if(bytes.size()<kEnvelopeOverhead || bytes.size()>lan::kMaxPacket+kEnvelopeOverhead)return {};
  const auto body=bytes.first(bytes.size()-32);const auto expected=Mac(ticket,body,server);
  uint8_t difference=0;for(size_t i=0;i<32;++i)difference|=expected[i]^bytes[body.size()+i];
  if(difference)return {};
  try{lan::Reader r(body);if(r.U32()!=0x4D474F31)return {};const auto id=r.Raw(16);if(!std::equal(id.begin(),id.end(),ticket.id.begin()))return {};
    Envelope result;result.sequence=r.U64();result.destination=r.U64();if(!result.sequence)return {};const auto packet=r.Raw(body.size()-36);result.packet.assign(packet.begin(),packet.end());return result;
  }catch(const std::exception&){return {};}
}
bool ReplayWindow::Accept(uint64_t sequence){
  if(!sequence)return false;
  if(sequence>top_){const auto shift=sequence-top_;if(shift>=128){low_=0;high_=0;}else if(shift>=64){high_=low_<<(shift-64);low_=0;}else if(shift){high_=(high_<<shift)|(low_>>(64-shift));low_<<=shift;}top_=sequence;low_|=1;return true;}
  const auto offset=top_-sequence;if(offset>=128)return false;auto& word=offset<64?low_:high_;const auto bit=uint64_t(1)<<(offset%64);if(word&bit)return false;word|=bit;return true;
}
}
