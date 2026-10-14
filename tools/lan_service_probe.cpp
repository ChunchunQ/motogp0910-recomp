#include "lan/node.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace motogp::lan;
using namespace std::chrono_literals;
namespace {
unsigned checks=0;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);++checks;}
template<class F> bool Until(F test,std::chrono::milliseconds timeout=1500ms){const auto deadline=std::chrono::steady_clock::now()+timeout;do{if(test())return true;std::this_thread::sleep_for(10ms);}while(std::chrono::steady_clock::now()<deadline);return test();}
Options Local(){Options options;options.bind_ip="127.0.0.1";options.discovery_targets={0x7F000001};return options;}
void Wire(){
 Check(ParseIPv4("26.7.8.9")==0x1A070809,"Parse Radmin IPv4");
 for(const char* text:{"","1.2.3","1.2.3.4.5","256.2.3.4","-1.2.3.4","1.2.3.4tail","1..3.4"," 1.2.3.4"})Check(!ParseIPv4(text),"Reject malformed IPv4");
 Check(IPv4(0x1A070809)=="26.7.8.9","Format IPv4 without byte reversal");
 Packet packet{Kind::Discover,0x12345678,0x1234567890ABCDEFull,0,{}};
 const auto bytes=Encode(packet);Check(bytes.size()==48 && bytes[0]=='M' && bytes[1]=='G' && bytes[2]=='P' && bytes[3]=='9',"Actual wire header size/magic");
 Check(bytes[24]==0x12 && bytes[25]==0x34 && bytes[26]==0x56 && bytes[27]==0x78,"Request ID network byte order");
 const auto decoded=Decode(bytes);Check(decoded && decoded->peer==packet.peer && decoded->request==packet.request,"Decode exact header fields");
 for(size_t n=0;n<bytes.size();++n)Check(!Decode(std::span(bytes).first(n)),"Truncated packet rejected");
 for(size_t i:{0u,4u,6u,8u,12u,16u,47u}){auto bad=bytes;bad[i]^=0x7F;Check(!Decode(bad),"Wrong magic/version/title/build/length rejected");}
 auto trailing=bytes;trailing.push_back(0);Check(!Decode(trailing),"Trailing bytes rejected");
 Room room;room.id=3;room.host=4;room.name="Qatar room";room.capacity=2;room.properties={1,2,0xFF,0};room.qos={5,0,7};RandomBytes(room.key);
 auto value=DecodeRoom(EncodeRoom(room));Check(value && value->properties==room.properties && value->qos==room.qos && value->key==room.key,"Opaque room fields retain all bits");
 auto bad=EncodeRoom(room);bad[23]=21;Check(!DecodeRoom(bad),"Oversized room capacity rejected");
}
void Service(){
 Node host(Local()), other(Local()), client(Local());
 auto first=host.Host("Qatar",2,0x21,{1,2,3},{0xAA,0xBB});auto second=other.Host("Jerez",2);
 Check(first.id!=second.id && host.Identity()!=client.Identity(),"Independent room and peer identities");
 client.Search();Check(Until([&]{return client.Rooms().size()==2;}),"Automatic discovery finds two actual hosts");
 client.Search();std::this_thread::sleep_for(60ms);Check(client.Rooms().size()==2,"Discovery replies deduplicated");
 auto rooms=client.Rooms();auto found=std::find_if(rooms.begin(),rooms.end(),[&](const Room& r){return r.id==first.id;});Check(found!=rooms.end() && found->endpoint==host.Address() && found->properties==first.properties,"Discovered room has actual endpoint/properties");
 auto wrong=*found;wrong.key[0]^=1;Check(!client.Join(wrong,300ms) && host.Members()==1,"Wrong session key cannot join");
 Check(client.Join(*found),"Join automatically discovered host");Check(host.Members()==2 && client.CurrentRoom()->id==first.id,"Both sides retain the real joined session");
 const std::array<uint8_t,5> payload{0,0xFF,0x80,4,0};
 Check(client.SendData(host.Identity(),1000,1000,payload),"Guest-port packet sent through host endpoint");
 std::optional<Datagram> received;Check(Until([&]{received=host.ReceiveData();return bool(received);}),"Host receives original opaque packet");Check(received->bytes==std::vector<uint8_t>(payload.begin(),payload.end()) && received->peer==client.Identity() && received->source_port==1000,"Source peer/port and packet bytes preserved");
 Check(host.SendData(client.Identity(),1000,1000,payload),"Host sends to the actual member");Check(Until([&]{received=client.ReceiveData();return bool(received);}),"Client receives host packet");
 Node full(Local());full.Search();Check(Until([&]{return full.Rooms().size()==1;}),"Full room hidden from normal search");Check(!full.Join(first,300ms),"Full host rejects another join");
 std::this_thread::sleep_for(650ms);Check(host.Members()==2 && bool(client.CurrentRoom()),"Member heartbeat keeps session alive");
 client.Leave();Check(Until([&]{return host.Members()==1;}),"Leave releases host slot");
 auto options=Local();options.discovery_targets={0xC0000201};Node direct(options);
 direct.Search();std::this_thread::sleep_for(100ms);Check(direct.Rooms().empty(),"Unavailable discovery does not fabricate rooms");
 direct.Search("127.0.0.1");Check(Until([&]{return direct.Rooms().size()==2;}),"Direct-IP fallback discovers actual rooms when automatic path cannot reach hosts");
 Check(direct.Join(first),"Direct-IP fallback joins using the same protocol");
 host.CloseRoom();Check(Until([&]{return !direct.CurrentRoom();}),"Host closing notifies joined client");
 direct.Search();std::this_thread::sleep_for(3700ms);const auto remaining=direct.Rooms();Check(std::none_of(remaining.begin(),remaining.end(),[&](const Room& r){return r.id==first.id;}),"Stale host removed");
}
int PairHost(const std::filesystem::path& ready,const std::filesystem::path& stop){
 Node host(Local());host.Host("Two-process Qatar",2,0x21,{8,9},{10});
 {std::ofstream file(ready);file<<host.Address().port<<'\n';}
 bool joined=false,echoed=false;const auto deadline=std::chrono::steady_clock::now()+20s;
 while(std::chrono::steady_clock::now()<deadline && !std::filesystem::exists(stop)){
  joined|=host.Members()==2;
  if(auto data=host.ReceiveData()){echoed=true;host.SendData(data->peer,data->destination_port,data->source_port,data->bytes);}
  std::this_thread::sleep_for(10ms);
 }
 Check(joined && echoed,"Two-process host received a real join and game datagram");return 0;
}
int PairClient(bool direct){
 auto options=Local();if(direct)options.discovery_targets={0xC0000201};Node client(options);
 client.Search(direct?"127.0.0.1":"");Check(Until([&]{return !client.Rooms().empty();},2500ms),"Separate process discovers room");
 const auto rooms=client.Rooms();auto found=std::find_if(rooms.begin(),rooms.end(),[](const Room& r){return r.name=="Two-process Qatar";});Check(found!=rooms.end(),"Found the actual host process");
 Check(client.Join(*found),"Separate process joins host");
 std::array<uint8_t,7> payload{0,1,2,0xFF,0x80,6,7};Check(client.SendData(found->host,1000,1000,payload),"Separate process sends original game bytes");
 std::optional<Datagram> packet;Check(Until([&]{packet=client.ReceiveData();return bool(packet);}),"Separate process receives echoed game bytes");Check(packet->bytes==std::vector<uint8_t>(payload.begin(),payload.end()),"Two-way packet content unchanged");client.Leave();return 0;
}
void Broadcast(){
 const auto adapters=Adapters();
 const auto adapter=adapters.empty()?Adapter{"Loopback",0x7F000001,0xFF000000,false}:adapters.front();
 if(adapters.empty())std::cout<<"IP Helper reports no physical adapter in this environment; validating loopback subnet broadcast\n";
 Check((adapter.ip|~adapter.mask)!=adapter.ip,"Discovery uses a broadcast destination, not the host's unicast address");
 Options options;options.bind_ip=IPv4(adapter.ip);
 options.discovery_targets={adapter.ip|~adapter.mask};
 Node host(options),client(options);host.Host("Broadcast Qatar",2);
 client.Search();Check(Until([&]{return client.Rooms().size()==1;}),"UDP subnet broadcast finds host without local-IP unicast fanout");
 const auto room=client.Rooms().front();Check(room.endpoint.ip==adapter.ip,"Broadcast advert contains actual source interface");
 Check(client.Join(room) && host.Members()==2,"Actually broadcast-discovered peer joins");
 client.Leave();
}
}
int main(int argc,char** argv){try{
 if(argc==4 && std::string(argv[1])=="host")PairHost(argv[2],argv[3]);
 else if(argc==3 && std::string(argv[1])=="client")PairClient(std::string(argv[2])=="direct");
 else if(argc==1){Wire();Service();Broadcast();}
 else throw std::runtime_error("Usage: lan_service_probe [host READY STOP | client automatic|direct]");
 std::cout<<checks<<" LAN protocol/discovery/session checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;}}
