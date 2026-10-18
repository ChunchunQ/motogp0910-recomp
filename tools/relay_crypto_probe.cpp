#include "online/relay.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <stdexcept>
using namespace motogp;
int main(){try{
  online::Ticket ticket;ticket.peer=0x0009000000000001;for(unsigned i=0;i<32;++i)ticket.key[i]=static_cast<uint8_t>(i);for(unsigned i=0;i<16;++i)ticket.id[i]=static_cast<uint8_t>(i+32);
  const std::vector<uint8_t> payload(1200,0x5a);unsigned checks=0;
  auto check=[&](bool ok,const char* message){if(!ok)throw std::runtime_error(message);++checks;};
  auto wire=online::Seal(ticket,1,ticket.peer,payload);
  auto result=online::Open(ticket,wire);check(result&&result->packet==payload&&result->sequence==1,"Authenticated relay payload round trips unchanged");
  std::string digest;for(auto it=wire.end()-32;it!=wire.end();++it){digest+="0123456789abcdef"[*it>>4];digest+="0123456789abcdef"[*it&15];}
  check(digest=="f2d8c07416bfe75ce88df5885394a9f4ed95621d67301ff11e92b85a25292b48","Wire MAC matches an independent Python HMAC-SHA256 reference vector");
  check(!online::Open(ticket,wire,true),"Client MAC cannot be replayed as a server response");
  wire.back()^=1;check(!online::Open(ticket,wire),"Modified relay MAC is rejected");
  for(unsigned size=0;size<online::kEnvelopeOverhead;++size)check(!online::Open(ticket,std::vector<uint8_t>(size)),"Truncated envelopes fail without reading out of bounds");
  std::atomic<bool> valid=true;std::vector<std::thread> workers;
  for(unsigned n=0;n<8;++n)workers.emplace_back([&,n]{auto personal=ticket;personal.key[0]^=static_cast<uint8_t>(n);for(unsigned i=1;i<=256;++i){auto bytes=online::Seal(personal,i+n*256,ticket.peer,payload);auto decoded=online::Open(personal,bytes);if(!decoded||decoded->packet!=payload||decoded->sequence!=i+n*256||(n&&online::Open(ticket,bytes)))valid=false;}});
  for(auto& worker:workers)worker.join();check(valid,"Concurrent relay authentication preserves independent per-call hash state");
  constexpr unsigned iterations=10000;const auto start=std::chrono::steady_clock::now();
  for(unsigned i=1;i<=iterations;++i){const auto bytes=online::Seal(ticket,i,ticket.peer,payload);if(!online::Open(ticket,bytes))throw std::runtime_error("Benchmark authentication failed");}
  const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  std::cout<<checks<<" relay crypto checks passed; "<<iterations<<" seal/open pairs: "<<ms<<" ms\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
