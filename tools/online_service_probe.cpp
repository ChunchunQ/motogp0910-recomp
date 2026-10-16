#include "online/client.h"
#include "lan/node.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
using namespace motogp;
using namespace std::chrono_literals;
namespace {
unsigned checks=0;
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);++checks;}
template<class F> bool Until(F test,std::chrono::milliseconds timeout=3000ms){const auto end=std::chrono::steady_clock::now()+timeout;do{if(test())return true;std::this_thread::sleep_for(10ms);}while(std::chrono::steady_clock::now()<end);return test();}
lan::Options Options(const online::Client& client){lan::Options o;o.relay=client.Connection();return o;}
std::string Nonce(){std::array<uint8_t,16> b{};lan::RandomBytes(b);constexpr char hex[]="0123456789abcdef";std::string out;for(auto c:b){out+=hex[c>>4];out+=hex[c&15];}return out;}
online::View Score(int64_t rating,uint32_t time){return {1,{{3,2,static_cast<uint64_t>(rating)},{2,1,time},{1,1,9}}};}
}
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("Usage: online_service_probe SERVER_URL PROFILE_DIRECTORY");
 const auto profile=std::filesystem::path(argv[2]);std::filesystem::create_directories(profile);
 const auto oversized=profile/"oversized.profile";
 {std::ofstream output(oversized,std::ios::binary);output<<std::string(8192,'X');}
 bool rejected=false;try{online::Client invalid(argv[1],"Invalid",oversized);}catch(const std::exception&){rejected=true;}
 Check(rejected,"Oversized credential file is rejected by a bounded read");
 Check(std::filesystem::file_size(oversized)==8192,"Invalid profile rejection preserves identity file rather than replacing it");
 uint64_t saved=0;
 {online::Client first(argv[1],"First",profile/"host.profile");saved=first.Connection().peer;}
 online::Client api_host(argv[1],"Host",profile/"host.profile"),api_client(argv[1],"Client",profile/"client.profile"),api_third(argv[1],"Third",profile/"third.profile");
 Check(api_host.Connection().peer==saved,"DPAPI-protected identity persists across connections");
 Check(api_host.Connection().peer!=api_client.Connection().peer,"Server allocates independent persistent player identities");
 {
 lan::Node host(Options(api_host)),client(Options(api_client)),third(Options(api_third));
 auto room=host.Host("Online Qatar",2,0x21,{1,2,3},{4});
 Check(host.Address()==online::VirtualEndpoint(saved),"Guest sees stable virtual endpoint, not the VDS endpoint");
 client.Search();Check(Until([&]{return client.Rooms().size()==1;}),"Internet directory discovers an actual authenticated host");
 const auto selected=client.Rooms().front();Check(selected.id==room.id && selected.host==host.Identity(),"Directory preserves original room ID and host");
 Check(client.Join(selected,3s),"Two Windows nodes join through VDS relay");
 Check(Until([&]{return host.Members()==2 && client.Connected(host.Identity());}),"Both peers retain admitted membership");
 std::vector<uint8_t> payload(lan::kMaxPacket-56);for(size_t i=0;i<payload.size();++i)payload[i]=static_cast<uint8_t>(i);
 Check(client.SendData(host.Identity(),1000,1001,payload),"Maximum-size original game datagram sent");
 std::optional<lan::Datagram> data;Check(Until([&]{data=host.ReceiveData();return bool(data);}),"Host receives relayed guest data");
 Check(data->bytes==payload && data->peer==client.Identity() && data->source_port==1000 && data->destination_port==1001,"Relay preserves opaque payload and guest ports");
 Check(host.SendData(client.Identity(),1001,1000,payload),"Host sends reply through relay");
 Check(Until([&]{data=client.ReceiveData();return bool(data);}),"Client receives reverse relay traffic");Check(data->bytes==payload,"Reverse payload unchanged");
 Check(!third.Join(selected,600ms),"Full lobby rejects a third player");
 auto alien=third.Host("Other room",2);third.RememberPeer(host.Identity(),host.Address());
 Check(third.SendData(host.Identity(),1000,1000,std::array<uint8_t,1>{99}),"Nonmember injection reaches transport for admission check");
 std::this_thread::sleep_for(100ms);Check(!host.ReceiveData(),"Server blocks data between different rooms");third.CloseRoom();
 std::this_thread::sleep_for(1700ms);Check(host.Members()==2 && client.CurrentRoom().has_value(),"Authenticated keepalive maintains online membership");
 client.Leave();Check(Until([&]{return host.Members()==1;}),"Leave releases online slot");
 client.Search();Check(Until([&]{return !client.Rooms().empty();}),"Room becomes discoverable again");Check(client.Join(client.Rooms().front(),3s),"Player can rejoin same lobby");
 host.CloseRoom();Check(Until([&]{return !client.CurrentRoom();}),"Host close tears down remote online session");
 }
 const auto nonce=Nonce();api_host.Write({Score(-12000000,120000)},nonce);api_host.Write({Score(-12000000,120000)},nonce);
 api_client.Write({Score(-10000000,100000)},Nonce());api_host.Write({Score(-15000000,150000)},Nonce());
 auto page=api_host.Read(1,1,10);Check(page.total>=2 && page.rows.size()>=2,"Native leaderboard has persisted player rows");
 auto host=std::find_if(page.rows.begin(),page.rows.end(),[&](const auto& row){return row.peer==api_host.Connection().peer;});
 auto client=std::find_if(page.rows.begin(),page.rows.end(),[&](const auto& row){return row.peer==api_client.Connection().peer;});
 Check(host!=page.rows.end() && client!=page.rows.end() && client->rank<host->rank,"Original signed rating ranks faster race time first");
 auto primary=std::find_if(host->columns.begin(),host->columns.end(),[](const auto& c){return c.id==3;});Check(primary!=host->columns.end() && static_cast<int64_t>(primary->bits)==-12000000,"Slower lap cannot overwrite a player's best result");
 const std::array<uint64_t,1> users{api_host.Connection().peer};const auto mine=api_host.Read(1,1,1,users);
 Check(mine.rows.size()==1 && mine.rows[0].peer==users[0] && mine.rows[0].rank==host->rank,"By-player query retains global leaderboard rank");
 Check(api_third.Read(1,1,1,{},api_third.Connection().peer).rows.empty(),"Unranked player's personal query never substitutes another player's best lap");
 online::ReplayWindow window;Check(window.Accept(100)&&window.Accept(98)&&!window.Accept(98)&&window.Accept(101)&&window.Accept(500)&&!window.Accept(1),"Replay window supports reordering and rejects duplicates/old packets");
 auto ticket=api_host.Connection();auto bytes=online::Seal(ticket,1,0,lan::Encode({lan::Kind::Discover,1,ticket.peer,0,{}}));
 Check(online::Open(ticket,bytes).has_value() && !online::Open(ticket,bytes,true),"Relay MAC separates client/server directions");bytes.back()^=1;Check(!online::Open(ticket,bytes),"Changed packet MAC rejected");
 std::cout<<checks<<" Windows HTTPS/directory/relay/persistence checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;}}
