#include "client.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <bcrypt.h>
#endif
namespace motogp::online {
namespace {
std::string Hex(std::span<const uint8_t> bytes){static constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:bytes){out+=digits[b>>4];out+=digits[b&15];}return out;}
#ifdef _WIN32
std::wstring Wide(const std::string& value){
 const int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0);
 if(!n)throw std::runtime_error("Invalid online server address");std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),out.data(),n);return out;
}
struct Handle {HINTERNET value=nullptr;~Handle(){if(value)WinHttpCloseHandle(value);}operator HINTERNET()const{return value;}};
struct Url {
 std::wstring host;INTERNET_PORT port=0;bool secure=false;
 explicit Url(const std::string& value){
  if(value.empty() || value.size()>512)throw std::runtime_error("Set a community server HTTPS address in General");
  auto wide=Wide(value);URL_COMPONENTS c{};c.dwStructSize=sizeof(c);c.dwHostNameLength=c.dwUrlPathLength=c.dwUserNameLength=c.dwPasswordLength=c.dwExtraInfoLength=DWORD(-1);
  if(!WinHttpCrackUrl(wide.c_str(),static_cast<DWORD>(wide.size()),0,&c) || !c.dwHostNameLength || c.dwUserNameLength || c.dwPasswordLength || c.dwExtraInfoLength || (c.dwUrlPathLength && std::wstring_view(c.lpszUrlPath,c.dwUrlPathLength)!=L"/"))throw std::runtime_error("Use https://server.example[:port] without a path or login");
  host.assign(c.lpszHostName,c.dwHostNameLength);port=c.nPort;secure=c.nScheme==INTERNET_SCHEME_HTTPS;
  if(!secure && (c.nScheme!=INTERNET_SCHEME_HTTP || (host!=L"127.0.0.1" && host!=L"localhost")))throw std::runtime_error("Online server requires HTTPS; HTTP is allowed only for loopback development");
 }
};
std::vector<uint8_t> ReadProfile(const std::filesystem::path& path){
 std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Cannot read the online profile");
 // Bound the read itself: a file-size check alone races local file changes.
 std::array<uint8_t,4097> encrypted{};f.read(reinterpret_cast<char*>(encrypted.data()),static_cast<std::streamsize>(encrypted.size()));
 const auto size=f.gcount();
 if(size<=0 || size>4096 || f.bad())throw std::runtime_error("Invalid online profile; restore its backup rather than changing identity");
 DATA_BLOB input{static_cast<DWORD>(size),encrypted.data()},output{};
 if(!CryptUnprotectData(&input,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output))throw std::runtime_error("Cannot unlock online profile for this Windows user");
 std::vector<uint8_t> result(output.pbData,output.pbData+output.cbData);SecureZeroMemory(output.pbData,output.cbData);LocalFree(output.pbData);
 if(result.size()!=32)throw std::runtime_error("Invalid online profile credential");return result;
}
std::vector<uint8_t> Credential(const std::filesystem::path& requested,const std::string& url){
 auto path=requested;
 if(path.empty()){
  wchar_t local[32768]{};const auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);
  if(!n || n>=32768)throw std::runtime_error("Cannot locate the Windows online profile folder");
  std::array<uint8_t,32> hash{};BCRYPT_ALG_HANDLE alg=nullptr;
  if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Online profile hash initialization failed");
  const auto status=BCryptHash(alg,nullptr,0,reinterpret_cast<PUCHAR>(const_cast<char*>(url.data())),static_cast<ULONG>(url.size()),hash.data(),static_cast<ULONG>(hash.size()));BCryptCloseAlgorithmProvider(alg,0);
  if(status<0)throw std::runtime_error("Online profile hash failed");path=std::filesystem::path(local)/L"MotoGP0910"/L"Online"/(Hex(hash)+".profile");
 }
 if(std::filesystem::exists(path))return ReadProfile(path);
 if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());
 std::vector<uint8_t> secret(32);lan::RandomBytes(secret);DATA_BLOB input{32,secret.data()},output{};
 if(!CryptProtectData(&input,L"MotoGP 09/10 community identity",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output))throw std::runtime_error("Cannot protect online profile");
 HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_HIDDEN,nullptr);
 if(file==INVALID_HANDLE_VALUE){LocalFree(output.pbData);if(GetLastError()==ERROR_FILE_EXISTS)return ReadProfile(path);throw std::runtime_error("Cannot create online profile");}
 DWORD written=0;const bool ok=WriteFile(file,output.pbData,output.cbData,&written,nullptr) && written==output.cbData && FlushFileBuffers(file);CloseHandle(file);LocalFree(output.pbData);
 if(!ok){DeleteFileW(path.c_str());throw std::runtime_error("Cannot save online profile");}return secret;
}
#endif
std::vector<uint8_t> Post(const std::string& origin,const std::string& credential,const std::string& path,std::span<const uint8_t> body){
#ifdef _WIN32
 const Url url(origin);
 // Local development traffic stays on this machine; external HTTPS continues
 // to use the Windows system proxy and its normal certificate verification.
 const bool loopback=url.host==L"127.0.0.1" || url.host==L"localhost";
 Handle session{WinHttpOpen(L"MotoGP0910-Community/2",loopback?WINHTTP_ACCESS_TYPE_NO_PROXY:WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
 // A loading game keeps the CPU busy: allow a slow TLS handshake and DNS (12002 otherwise).
 if(!session.value || !WinHttpSetTimeouts(session,0,15000,20000,20000))throw std::runtime_error("Cannot initialize online HTTPS connection");
 Handle connection{WinHttpConnect(session,url.host.c_str(),url.port,0)};
 Handle request{connection.value?WinHttpOpenRequest(connection,L"POST",Wide(path).c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,url.secure?WINHTTP_FLAG_SECURE:0):nullptr};
 if(!request.value)throw std::runtime_error("Cannot open community server request");
 DWORD redirect=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;if(!WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&redirect,sizeof(redirect)))throw std::runtime_error("Cannot disable online credential redirects");
 const auto headers=L"Authorization: Bearer "+Wide(credential)+L"\r\nContent-Type: application/octet-stream\r\n";
 if(body.size()>65536 || !WinHttpSendRequest(request,headers.c_str(),static_cast<DWORD>(headers.size()),body.empty()?WINHTTP_NO_REQUEST_DATA:const_cast<uint8_t*>(body.data()),static_cast<DWORD>(body.size()),static_cast<DWORD>(body.size()),0) || !WinHttpReceiveResponse(request,nullptr))throw std::runtime_error("Community server is unreachable or HTTPS verification failed (Windows error "+std::to_string(GetLastError())+")");
 DWORD status=0,length=sizeof(status);if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&length,nullptr))throw std::runtime_error("Invalid community server HTTP response");
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(25);
 std::vector<uint8_t> result;std::array<uint8_t,4096> chunk{};DWORD count=0;
 while(WinHttpReadData(request,chunk.data(),static_cast<DWORD>(chunk.size()),&count)){
  if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("Community server response timed out");
  if(!count){if(status!=200){std::string detail(result.begin(),result.end());if(detail.size()>256)detail.resize(256);throw std::runtime_error("Community server rejected request ("+std::to_string(status)+"): "+detail);}return result;}
  if(result.size()+count>65536)throw std::runtime_error("Community server response too large");result.insert(result.end(),chunk.begin(),chunk.begin()+count);
 }
 throw std::runtime_error("Community server response was interrupted");
#else
 (void)origin;(void)credential;(void)path;(void)body;throw std::runtime_error("Online game client targets Windows");
#endif
}
}
std::vector<uint8_t> Client::Request(const std::string& path,std::span<const uint8_t> body)const{return Post(url_,credential_,path,body);}
std::string LinkAccount(std::string url,const std::string& token,const std::filesystem::path& profile){
#ifdef _WIN32
 // Same origin spelling and profile file as Client, so the game later connects
 // with exactly the credential that was linked here.
 while(url.ends_with('/'))url.pop_back();const Url parsed(url);
 auto secret=Credential(profile,url);const auto credential=Hex(secret);SecureZeroMemory(secret.data(),secret.size());
 const auto reply=Post(url,credential,"/v2/link",std::span(reinterpret_cast<const uint8_t*>(token.data()),token.size()));
 if(reply.empty() || reply.size()>15)throw std::runtime_error("Invalid account link answer");
 return std::string(reply.begin(),reply.end());
#else
 (void)url;(void)token;(void)profile;throw std::runtime_error("Online game client targets Windows");
#endif
}
Client::Client(std::string url,std::string name,const std::filesystem::path& profile):url_(std::move(url)){
#ifdef _WIN32
 while(url_.ends_with('/'))url_.pop_back();const Url parsed(url_);
 auto secret=Credential(profile,url_);credential_=Hex(secret);SecureZeroMemory(secret.data(),secret.size());
 lan::Writer w;w.U64(lan::kCompatibility);w.Text(name);const auto reply=Request("/v2/connect",w.bytes);lan::Reader r(reply);
 ticket_.peer=r.U64();auto id=r.Raw(16);std::copy(id.begin(),id.end(),ticket_.id.begin());auto key=r.Raw(32);std::copy(key.begin(),key.end(),ticket_.key.begin());ticket_.endpoint.port=r.U16();
 if(!r.End() || !ticket_.endpoint.port)throw std::runtime_error("Invalid community server handshake");VirtualEndpoint(ticket_.peer);
 WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))throw std::runtime_error("Online DNS initialization failed");
 ADDRINFOW hints{};hints.ai_family=AF_INET;hints.ai_socktype=SOCK_DGRAM;ADDRINFOW* address=nullptr;
 const auto status=GetAddrInfoW(parsed.host.c_str(),nullptr,&hints,&address);
 if(!status && address)ticket_.endpoint.ip=ntohl(reinterpret_cast<sockaddr_in*>(address->ai_addr)->sin_addr.s_addr);
 if(address)FreeAddrInfoW(address);WSACleanup();if(!ticket_.endpoint.ip)throw std::runtime_error("Community server needs a reachable IPv4 DNS record");
#else
 (void)name;(void)profile;throw std::runtime_error("Online game client targets Windows");
#endif
}
void Client::Write(const std::vector<View>& views,const std::string& nonce)const{
 lan::Writer w;w.Text(nonce);w.U32(static_cast<uint32_t>(views.size()));
 for(const auto& view:views){w.U32(view.id);w.U32(static_cast<uint32_t>(view.columns.size()));for(auto c:view.columns){w.U16(c.id);w.U8(c.type);w.U64(c.bits);}}
 const auto result=Request("/v2/stats/write",w.bytes);if(!result.empty())throw std::runtime_error("Invalid statistics write response");
}
Client::~Client(){
 if(ticket_.peer){try{Request("/v2/disconnect",ticket_.id);}catch(const std::exception&){} }
}
void Client::BeginWrite()const{std::lock_guard lock(writes_mutex_);++pending_writes_;}
void Client::EndWrite()const{std::lock_guard lock(writes_mutex_);if(pending_writes_)--pending_writes_;writes_changed_.notify_all();}
void Client::Flush()const{std::unique_lock lock(writes_mutex_);if(!writes_changed_.wait_for(lock,std::chrono::seconds(30),[&]{return !pending_writes_;}))throw std::runtime_error("Online statistics flush timed out");}
Page Client::Read(uint32_t view,uint32_t start,uint32_t count,std::span<const uint64_t> users,uint64_t around)const{
 if(!count || count>100 || users.size()>100)throw std::runtime_error("Invalid leaderboard query");lan::Writer w;w.U32(view);w.U32(start);w.U32(count);w.U64(around);w.U32(static_cast<uint32_t>(users.size()));for(auto id:users)w.U64(id);
 const auto response=Request("/v2/stats/read",w.bytes);lan::Reader r(response);Page page;page.total=r.U32();const auto amount=r.U32();if(amount>count || amount>page.total)throw std::runtime_error("Invalid leaderboard response");auto& rows=page.rows;
 for(unsigned i=0;i<amount;++i){Row row;row.peer=r.U64();row.rank=r.U32();row.name=r.Text(15);const auto columns=r.U32();if(!row.peer || !row.rank || columns>64)throw std::runtime_error("Invalid leaderboard row");for(unsigned j=0;j<columns;++j){Column c;c.id=r.U16();c.type=r.U8();c.bits=r.U64();if(c.type!=1 && c.type!=2 && c.type!=3 && c.type!=5)throw std::runtime_error("Invalid leaderboard column");row.columns.push_back(c);}rows.push_back(std::move(row));}
 if(!r.End())throw std::runtime_error("Invalid leaderboard response length");return page;
}
}
