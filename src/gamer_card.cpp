#include "gamer_card.h"
#include "in_game_settings.h"
#include "quit_menu.h"
#include "overlay_theme.h"
#include "gamer_card_flags.h"
#include "../third_party/nlohmann/json.hpp"

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/memory.h>
#include <rex/runtime.h>
#include <rex/input/input_system.h>
#include <rex/ui/immediate_drawer.h>
#include <rex/ui/image_decode.h>
#include <rex/ui/window.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <cstdint>
#include <cfloat>
#include <ctime>
#include <cstring>
#include <mutex>
#include <optional>
#include <set>
#include <span>
#include <stop_token>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <unordered_map>
#include <vector>

REX_EXTERN(__imp__sub_82303260);
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>
#include <wincodec.h>
#endif

namespace motogp::gamer_card {
namespace {
constexpr uint64_t kPeerBase = 0x0009000000000000ull;
constexpr uint64_t kPeerMask = 0xFFFFFFFFFF000000ull;
const auto& kAccent=overlay::orange;
const auto& kWhite=overlay::white;
const auto& kMuted=overlay::muted;
struct FontFace {ImFont* font;float size;};
constexpr int kFontSizes[]{5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,36,40,44,48,52,56};
std::array<ImFont*,std::size(kFontSizes)> profile_fonts{};
struct BioHash {
  using is_transparent=void;
  size_t operator()(std::string_view value)const noexcept{return std::hash<std::string_view>{}(value);}
};
std::unordered_map<std::string,ImWchar,BioHash,std::equal_to<>> emoji_aliases;
size_t emoji_max_bytes=0;
std::string EmojiText(std::string_view source) {
  if(emoji_aliases.empty())return std::string(source);
  std::string text;
  for(size_t i=0;i<source.size();) {
    if(source.substr(i,3)=="\xEF\xB8\x8F"){i+=3;continue;} // Emoji presentation selector.
    text+=source[i++];
  }
  std::string result;
  for(size_t at=0;at<text.size();) {
    const auto byte=static_cast<unsigned char>(text[at]);
    if(byte<0x80&&text[at]!='#'&&text[at]!='*'&&(text[at]<'0'||text[at]>'9')) {
      result+=text[at++];continue;
    }
    bool matched=false;
    for(size_t size=std::min(emoji_max_bytes,text.size()-at);size>0;--size) {
      const auto item=emoji_aliases.find(std::string_view(text).substr(at,size));
      if(item==emoji_aliases.end())continue;
      const auto code=uint32_t(item->second);
      result+=char(0xE0|(code>>12));result+=char(0x80|((code>>6)&63));result+=char(0x80|(code&63));
      at+=size;matched=true;break;
    }
    if(!matched) {
      // A joiner without a supported combined sequence remains invisible.
      if(std::string_view(text).substr(at,3)=="\xE2\x80\x8D")at+=3;
      else result+=text[at++];
    }
  }
  return result;
}
FontFace Face(ImFont* fallback,float pixels) {
  size_t best=profile_fonts.size();
  float distance=FLT_MAX;
  for(size_t i=0;i<profile_fonts.size();++i) {
    const float difference=std::abs(float(kFontSizes[i])-pixels);
    if(profile_fonts[i]&&difference<distance){distance=difference;best=i;}
  }
  if(best<profile_fonts.size())return {profile_fonts[best],float(kFontSizes[best])};
  return {fallback,std::max(1.f,std::round(pixels))};
}
const char* Tr(bool tr, const char* en, const char* translated) {
  return tr ? translated : en;
}

std::string Country(std::string value) {
  if (value.size() != 2) return {};
  for (char& ch : value) {
    if (ch >= 'a' && ch <= 'z') ch -= 32;
    if (ch < 'A' || ch > 'Z') return {};
  }
  return value;
}

std::string EncodeName(std::string_view name) {
  constexpr char hex[] = "0123456789ABCDEF";
  std::string url;
  for (unsigned char ch : name) {
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
      url += static_cast<char>(ch);
    } else {
      url += '%'; url += hex[ch >> 4]; url += hex[ch & 15];
    }
  }
  return url;
}

bool OpenProfile(std::string_view name) {
#ifdef _WIN32
  const auto url="https://nobkonline.com/profile/?u="+EncodeName(name);
  const std::wstring wide(url.begin(),url.end());
  return reinterpret_cast<intptr_t>(ShellExecuteW(nullptr,L"open",wide.c_str(),
      nullptr,nullptr,SW_SHOWNORMAL))>32;
#else
  (void)name;
  return false;
#endif
}

struct Player {
  std::string name, country;
  // Absent: older API, use avatar-by-name. Empty: explicitly no photo.
  std::optional<std::string> avatar_path;
  int64_t member_since=0;
  std::string bio;
};
struct Avatar { uint32_t width=0, height=0; std::vector<uint8_t> rgba; };
struct PublicReply { uint32_t status=0; std::string body; };
struct Record {
  uint32_t view=0, rank=0, total=0;
  int track=-1, season=-1;
  int64_t mode=-1, updated=0;
  std::string track_name, category, bike, result;
};
struct Results {
  std::vector<Record> rows;
};
struct RecordSummary {size_t tracks_held=0,records_held=0,boards_entered=0;};
RecordSummary SummarizeRecords(const std::vector<const Record*>& rows) {
  std::set<int> tracks;
  size_t firsts=0;
  for(const auto* row:rows)if(row->rank==1) {
    if(row->track!=17)tracks.insert(row->track);
    ++firsts;
  }
  return {tracks.size(),firsts,rows.size()};
}
struct RankingEntry {
  uint32_t id=0;
  int64_t firsts=0,points=0,position=0;
  std::set<int> tracks;
};
struct Ranking {std::vector<RankingEntry> players;};

std::string Text(const nlohmann::json& object,const char* key,size_t limit=96) {
  const auto value=object.find(key);
  if(value==object.end()||!value->is_string())return {};
  auto text=value->get<std::string>();
  if(text.size()>limit||std::any_of(text.begin(),text.end(),[](unsigned char c){return c<32||c==127;}))return {};
  return text;
}
std::string ReadBio(const nlohmann::json& object) {
  const auto field=object.find("bio");
  if(field==object.end()||!field->is_string())return {};
  const auto& value=field->get_ref<const std::string&>();
  // JSON parsing has already validated UTF-8. Bound Unicode characters, not
  // bytes, and preserve line breaks while excluding embedded UI controls.
  if(value.size()>1200)return {};
  std::string bio;
  unsigned characters=0;
  for(const unsigned char ch:value) {
    if((ch&0xC0)!=0x80&&++characters>300)return {};
    if(ch=='\r')continue;
    if(ch=='\t')bio+=' ';
    else if(ch=='\n'||(ch>=32&&ch!=127))bio+=static_cast<char>(ch);
  }
  const auto first=bio.find_first_not_of(" \n");
  if(first==std::string::npos)return {};
  return bio.substr(first,bio.find_last_not_of(" \n")-first+1);
}
int64_t Number(const nlohmann::json& object,const char* key,int64_t fallback=-1) {
  const auto value=object.find(key);
  if(value==object.end()||!value->is_number_integer())return fallback;
  if(value->is_number_unsigned()&&value->get<uint64_t>()>uint64_t(INT64_MAX))return fallback;
  return value->get<int64_t>();
}
constexpr std::string_view kTracks[]{"Losail International Circuit","Twin Ring Motegi","Jerez",
  "Le Mans","Mugello","Catalunya","TT Circuit Assen","Laguna Seca","Sachsenring",
  "Donington Park","Brno","Indianapolis","Misano","Estoril","Phillip Island",
  "Sepang","Valencia","","Silverstone","Shanghai","Istanbul Park"};
// Verified NOBK outline filenames in native track order. The reserved slot and
// DLC tracks have no published outline in the site's current track catalogue.
constexpr std::string_view kTrackMaps[]{"omap-qatar.png","omap-motegi.png","omap-jerez.png",
  "omap-lemans.png","omap-mugello.png","omap-catalunya.png","omap-assen.png",
  "omap-lagunaseca.png","omap-sachsenring.png","omap-donington.png","omap-brno.png",
  "omap-indianapolis.png","omap-misano.png","omap-estoril.png","omap-phillipisland.png",
  "omap-sepang.png","omap-valencia.png","","","",""};
static_assert(std::size(kTrackMaps)==std::size(kTracks));

std::shared_ptr<const Results> ReadResults(const nlohmann::json& json,uint64_t serial) {
  const auto records=json.find("records");
  if(records==json.end()||!records->is_array()||records->size()>378) {
    REXLOG_WARN("PLAYER_CARD request={} stage=records event=unavailable reason=invalid_records_array",serial);
    return {};
  }
  auto results=std::make_shared<Results>();
  std::set<uint32_t> views;
  for(const auto& item:*records) {
    if(!item.is_object())continue;
    const auto view=Number(item,"view"),rank=Number(item,"rank"),total=Number(item,"total",0);
    if(view<1||view>378||rank<1||rank>UINT32_MAX||total<0||total>UINT32_MAX||
        !views.insert(static_cast<uint32_t>(view)).second)continue;
    Record row;
    row.view=static_cast<uint32_t>(view);row.rank=static_cast<uint32_t>(rank);row.total=static_cast<uint32_t>(total);
    row.track=int((view-1)%21);
    row.track_name=Text(item,"track");
    // Native track indices include an unused slot 17; never compact DLC slots.
    if(!kTracks[row.track].empty())row.track_name=kTracks[row.track];
    if(row.track_name.empty())row.track_name="Board #"+std::to_string(view);
    row.category=Text(item,"class",40);
    row.bike=Text(item,"bike",64);
    if(row.bike.empty())row.bike=Text(item,"manufacturer",64);
    row.updated=Number(item,"updated",0);
    if(const auto columns=item.find("columns");columns!=item.end()&&columns->is_object()) {
      const auto season=Number(*columns,"13");
      if(season==0||season==1)row.season=int(season);
      row.mode=Number(*columns,"6");
    }
    if(const auto display=item.find("display");display!=item.end()&&display->is_object())
      row.result=Text(*display,"text",32);
    if(row.result.empty())row.result="-";
    results->rows.push_back(std::move(row));
  }
  std::stable_sort(results->rows.begin(),results->rows.end(),[](const auto& a,const auto& b){
    return a.rank!=b.rank?a.rank<b.rank:a.updated>b.updated;
  });
  return results;
}

struct RequestCancelled : std::runtime_error {
  RequestCancelled() : std::runtime_error("Player card request cancelled") {}
};
struct PublicHttpFailure : std::runtime_error {
  uint32_t code;
  PublicHttpFailure(const char* operation,uint32_t error)
      : std::runtime_error(std::string("Public profile request failed at ")+operation),code(error) {}
};

// Owned by the profile worker, so all requests can reuse the session and its
// keep-alive connection. Only request cancellation crosses threads. These are
// public read-only endpoints; no launcher token or account password is sent.
class PublicClient {
#ifdef _WIN32
  struct Handle {
    HINTERNET value=nullptr;
    Handle()=default;
    Handle(const Handle&)=delete;
    Handle& operator=(const Handle&)=delete;
    void Close() { if(value)WinHttpCloseHandle(std::exchange(value,nullptr)); }
    ~Handle() { Close(); }
  };
  struct Exchange {
    std::mutex mutex;
    std::condition_variable_any changed;
    DWORD status=0,error=0,bytes=0;
    bool closed=false,callbacks=false;
    Handle request;

    static void CALLBACK Notify(HINTERNET,DWORD_PTR context,DWORD status,
        void* information,DWORD length) {
      if(!context)return;
      auto& self=*reinterpret_cast<Exchange*>(context);
      std::lock_guard lock(self.mutex);
      if(status==WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING)self.closed=true;
      else if(status==WINHTTP_CALLBACK_STATUS_REQUEST_ERROR) {
        self.error=static_cast<WINHTTP_ASYNC_RESULT*>(information)->dwError;
      } else {
        self.status=status;
        if(status==WINHTTP_CALLBACK_STATUS_READ_COMPLETE)self.bytes=length;
      }
      self.changed.notify_all();
    }
    bool Wait(DWORD expected,std::stop_token cancel,
        std::chrono::steady_clock::time_point deadline) {
      std::unique_lock lock(mutex);
      changed.wait_until(lock,cancel,deadline,[&]{return status==expected||error;});
      if(cancel.stop_requested())throw RequestCancelled();
      if(error) { SetLastError(error);return false; }
      if(status!=expected) { SetLastError(ERROR_WINHTTP_TIMEOUT);return false; }
      status=0;
      return true;
    }
    ~Exchange() {
      request.Close();
      // HANDLE_CLOSING is the last callback. Its context and read buffer must
      // outlive it, including when a card closes during an unfinished read.
      if(callbacks) {
        std::unique_lock lock(mutex);
        changed.wait(lock,[&]{return closed;});
      }
    }
  };
  Handle session_,connection_;
#endif
  bool first_request_=true;
 public:
  PublicReply Get(std::string_view relative_path,size_t max_body,uint64_t serial,
      std::string_view stage,std::stop_token cancel) {
    const auto started=std::chrono::steady_clock::now();
    const auto deadline=started+std::chrono::seconds(5);
    // The observed first-connection timeout recovered immediately when the
    // same profile was reopened. Retry that read-only identity request once,
    // within the original budget, instead of requiring a manual reopen.
    const bool cold=std::exchange(first_request_,false)&&stage=="identity";
    try {
      return GetOnce(relative_path,max_body,serial,stage,cancel,
          cold?started+std::chrono::seconds(2):deadline);
    } catch(const PublicHttpFailure& error) {
#ifdef _WIN32
      const bool transient=error.code==ERROR_WINHTTP_TIMEOUT||error.code==ERROR_WINHTTP_CANNOT_CONNECT||
          error.code==ERROR_WINHTTP_CONNECTION_ERROR||error.code==ERROR_WINHTTP_NAME_NOT_RESOLVED||
          error.code==ERROR_WINHTTP_RESEND_REQUEST;
      if(cancel.stop_requested())throw RequestCancelled();
      const auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(
          deadline-std::chrono::steady_clock::now()).count();
      if(!cold||!transient||remaining<250)throw;
      return GetOnce(relative_path,max_body,serial,stage,cancel,deadline);
#else
      throw;
#endif
    }
  }
 private:
  PublicReply GetOnce(std::string_view relative_path,size_t max_body,uint64_t serial,
      std::string_view stage,std::stop_token cancel,
      std::chrono::steady_clock::time_point deadline) {
#ifdef _WIN32
  if(cancel.stop_requested())throw RequestCancelled();
  // Unlike separate resolve/connect/read timeouts, this bounds the entire
  // HTTP exchange. Optional metadata cannot occupy the worker indefinitely.
  const auto failure=[&](const char* operation) {
    const auto error=GetLastError();
    REXLOG_WARN("PLAYER_CARD request={} stage={} event=http_failed operation={} win_error={}",
        serial,stage,operation,error);
    return PublicHttpFailure(operation,error);
  };
  if(!session_.value) {
    session_.value=WinHttpOpen(L"MotoGP0910-PlayerCard/1",
      WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
      WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    if(!session_.value)session_.value=WinHttpOpen(L"MotoGP0910-PlayerCard/1",
      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
      WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    if(!session_.value)throw failure("session");
    if(!WinHttpSetTimeouts(session_.value,3000,4000,4000,4000)) {
      const auto error=failure("session_timeouts");session_.Close();throw error;
    }
  }
  if(!connection_.value)connection_.value=WinHttpConnect(session_.value,
      L"www.nobkonline.com",INTERNET_DEFAULT_HTTPS_PORT,0);
  if(!connection_.value)throw failure("connect");
  // Paths are built locally or validated against the site's avatar prefix.
  // Server-returned hosts and redirects are never followed.
  const std::wstring path(relative_path.begin(), relative_path.end());
  std::string body;
  char chunk[4096];
  Exchange exchange;
  auto& request=exchange.request;
  request.value=WinHttpOpenRequest(connection_.value,
      L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
  if (!request.value) throw failure("open");
  DWORD_PTR context=reinterpret_cast<DWORD_PTR>(&exchange);
  if(!WinHttpSetOption(request.value,WINHTTP_OPTION_CONTEXT_VALUE,&context,sizeof(context)))throw failure("context");
  if(WinHttpSetStatusCallback(request.value,&Exchange::Notify,
      WINHTTP_CALLBACK_FLAG_SENDREQUEST_COMPLETE|WINHTTP_CALLBACK_FLAG_HEADERS_AVAILABLE|
      WINHTTP_CALLBACK_FLAG_READ_COMPLETE|WINHTTP_CALLBACK_FLAG_REQUEST_ERROR|
      WINHTTP_CALLBACK_FLAG_HANDLES,0)==WINHTTP_INVALID_STATUS_CALLBACK)throw failure("callback");
  exchange.callbacks=true;
  const auto complete=[&](BOOL initiated,DWORD expected,const char* operation) {
    if(!initiated&&GetLastError()!=ERROR_IO_PENDING)throw failure(operation);
    if(!exchange.Wait(expected,cancel,deadline))throw failure(operation);
  };
  DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
  if (!WinHttpSetOption(request.value, WINHTTP_OPTION_REDIRECT_POLICY,
      &redirects, sizeof(redirects)))throw failure("redirect_policy");
  complete(WinHttpSendRequest(request.value,WINHTTP_NO_ADDITIONAL_HEADERS,
      0,WINHTTP_NO_REQUEST_DATA,0,0,context),WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE,"send");
  complete(WinHttpReceiveResponse(request.value,nullptr),WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE,"headers");
  DWORD status = 0, size = sizeof(status);
  if (!WinHttpQueryHeaders(request.value,
      WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
      WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX)) throw failure("status");
  if (status!=200) {
    if(status!=404)REXLOG_WARN("PLAYER_CARD request={} stage={} event=http_failed status={}",serial,stage,status);
    return {status,{}};
  }

  for (;;) {
    if(cancel.stop_requested())throw RequestCancelled();
    if(std::chrono::steady_clock::now()>=deadline) {
      SetLastError(ERROR_WINHTTP_TIMEOUT);throw failure("read");
    }
    if(!WinHttpReadData(request.value,chunk,sizeof(chunk),nullptr)&&GetLastError()!=ERROR_IO_PENDING)
      throw failure("read");
    if(!exchange.Wait(WINHTTP_CALLBACK_STATUS_READ_COMPLETE,cancel,deadline))throw failure("read");
    const DWORD received=exchange.bytes;
    if (!received) break;
    if (body.size() + received > max_body) {
      REXLOG_WARN("PLAYER_CARD request={} stage={} event=http_failed reason=body_limit limit={}",serial,stage,max_body);
      throw std::runtime_error("Player profile response is too large");
    }
    body.append(chunk, received);
  }
  return {status,std::move(body)};
#else
  (void)relative_path; (void)max_body; (void)serial; (void)stage; (void)cancel; (void)deadline;
  throw std::runtime_error("Player cards currently target Windows");
#endif
  }
};

Player ReadPlayer(PublicClient& client,uint32_t id,uint64_t serial,
    std::shared_ptr<const Results>& results,std::stop_token cancel) {
  const auto response=client.Get("/api/motogp/player?id="+std::to_string(id),256*1024,serial,"identity",cancel);
  if(response.status!=200) throw std::runtime_error("Player identity endpoint did not return HTTP 200");
  const auto json = nlohmann::json::parse(response.body,nullptr,false);
  if(json.is_discarded())throw std::runtime_error("Player identity response is not valid JSON");
  // Numeric IDs belong to this game's service, not to the site's account ID.
  // Resolve the name from the matching record instead of guessing from XUID.
  if (!json.is_object() || json.value("missing", false) ||
      !json.contains("id") || !json["id"].is_number_unsigned() ||
      json["id"].get<uint64_t>() != id ||
      !json.contains("name") || !json["name"].is_string())
    throw std::runtime_error("Player profile identity did not match");
  Player player{json["name"].get<std::string>(), {}, {}};
  if (player.name.empty() || player.name.size() > 64 ||
      std::any_of(player.name.begin(), player.name.end(),
          [](unsigned char ch) { return ch < 0x20 || ch == 0x7F; }))
    throw std::runtime_error("Player profile name is invalid");
  if (const auto country = json.find("country"); country != json.end() && country->is_string())
    player.country = Country(country->get<std::string>());
  // Records arrive with the identity; switching tabs needs no new HTTP request.
  try { results=ReadResults(json,serial); }
  catch(const std::exception&) { REXLOG_WARN("PLAYER_CARD request={} stage=records event=unavailable reason=invalid_record",serial); }
  return player;
}

std::shared_ptr<const Ranking> ParseRanking(const nlohmann::json& json,const nlohmann::json& views) {
  // /motogp/ranking/ reorders this API list using /views. The raw "pos"
  // alone is not the rider position displayed on the website.
  if(!json.is_object()||!json.contains("players")||!json["players"].is_array()||json["players"].size()>100)
    throw std::runtime_error("Rider ranking response is invalid");
  auto ranking=std::make_shared<Ranking>();
  std::set<uint32_t> ids;
  std::set<int64_t> positions;
  for(const auto& player:json["players"]) {
    if(!player.is_object())throw std::runtime_error("Rider ranking entry is invalid");
    const auto id=Number(player,"id"),position=Number(player,"pos"),
        firsts=Number(player,"firsts"),points=Number(player,"points");
    if(id<1||id>0xFFFFFF||position<1||position>100||firsts<0||firsts>378||points<0||
        !ids.insert(uint32_t(id)).second||!positions.insert(position).second)
      throw std::runtime_error("Rider ranking fields are invalid");
    ranking->players.push_back({uint32_t(id),firsts,points,position,{}});
  }
  if(!views.is_object()||!views.contains("views")||!views["views"].is_array()||views["views"].size()>378)
    throw std::runtime_error("Rider ranking boards are invalid");
  std::set<int64_t> seen;
  for(const auto& board:views["views"]) {
    if(!board.is_object())throw std::runtime_error("Rider ranking board is invalid");
    const auto view=Number(board,"view");
    if(view<1||view>378||!seen.insert(view).second)
      throw std::runtime_error("Rider ranking board index is invalid");
    const int track=int((view-1)%21);
    // Match the site's current 17-entry TRACKS catalogue and complete ranking
    // scope, separately from the card's DLC /20 and temporary 2009 summaries.
    if(track>=17||!board.contains("best")||board["best"].is_null())continue;
    const auto& best=board["best"];
    if(!best.is_object())throw std::runtime_error("Rider ranking leader is invalid");
    const auto id=Number(best,"id");
    if(id<1||id>0xFFFFFF)throw std::runtime_error("Rider ranking leader identity is invalid");
    const auto player=std::find_if(ranking->players.begin(),ranking->players.end(),
        [&](const auto& item){return item.id==uint32_t(id);});
    if(player!=ranking->players.end())player->tracks.insert(track);
  }
  std::stable_sort(ranking->players.begin(),ranking->players.end(),[](const auto& a,const auto& b) {
    if(a.tracks.size()!=b.tracks.size())return a.tracks.size()>b.tracks.size();
    if(a.firsts!=b.firsts)return a.firsts>b.firsts;
    if(a.points!=b.points)return a.points>b.points;
    return a.position<b.position;
  });
  return ranking;
}

std::shared_ptr<const Ranking> ReadRanking(PublicClient& client,uint64_t serial,std::stop_token cancel) {
  const auto response=client.Get("/api/motogp/ranking",256*1024,serial,"ranking",cancel);
  if(response.status!=200)throw std::runtime_error("Rider ranking endpoint did not return HTTP 200");
  const auto boards=client.Get("/api/motogp/views",512*1024,serial,"ranking_views",cancel);
  if(boards.status!=200)throw std::runtime_error("Rider ranking boards did not return HTTP 200");
  return ParseRanking(nlohmann::json::parse(response.body,nullptr,false),
      nlohmann::json::parse(boards.body,nullptr,false));
}

bool SameName(std::string_view first,std::string_view second) {
  if(first.size()!=second.size())return false;
  const auto fold=[](unsigned char ch){return ch>='A'&&ch<='Z'?ch+32:ch;};
  for(size_t i=0;i<first.size();++i)if(fold(first[i])!=fold(second[i]))return false;
  return true;
}

std::optional<std::string> AvatarPath(std::string value) {
  for (std::string_view origin:{"https://www.nobkonline.com","https://nobkonline.com"})
    if(value.starts_with(origin)) { value.erase(0,origin.size()); break; }
  if(!value.starts_with("/api/avatar/")||value.size()>200||value.find("..")!=std::string::npos||
      value.find('#')!=std::string::npos||value.find('\\')!=std::string::npos||
      std::any_of(value.begin(),value.end(),[](unsigned char ch){return ch<=0x20||ch>=0x7F;}))return {};
  return value;
}

Player ReadAccount(PublicClient& client,Player player,uint64_t serial,std::stop_token cancel) {
  // The general /profile/?u= page itself uses this shared public endpoint,
  // even for accounts whose only game is MotoGP. It is not a FIFA stats lookup
  // by MotoGP numeric ID: the already resolved account name is the query key.
  const auto response=client.Get("/api/fifa09/player?name="+EncodeName(player.name),256*1024,serial,"account",cancel);
  if(response.status!=200)throw std::runtime_error("Shared account endpoint did not return HTTP 200");
  const auto json=nlohmann::json::parse(response.body,nullptr,false);
  if(json.is_discarded())throw std::runtime_error("Shared account response is not valid JSON");
  if(!json.is_object()||!json.contains("player")||!json["player"].is_string()||
      !SameName(json["player"].get<std::string>(),player.name))
    throw std::runtime_error("Shared account name did not match the selected player");
  if(const auto country=json.find("country");country!=json.end()&&country->is_string()) {
    const auto parsed=Country(country->get<std::string>());
    if(!parsed.empty())player.country=parsed;
  }
  const auto joined=Number(json,"member_since",0);
  if(joined>=946684800&&joined<4102444800LL)player.member_since=joined;
  player.bio=ReadBio(json);
  if(const auto avatar=json.find("avatar");avatar!=json.end()&&avatar->is_string()) {
    const auto value=avatar->get<std::string>();
    if(value.empty())player.avatar_path=std::string{};
    else if(auto path=AvatarPath(value))player.avatar_path=std::move(*path);
    else REXLOG_WARN("PLAYER_CARD request={} stage=account event=avatar_path_rejected",serial);
  }
  return player;
}

#ifdef _WIN32
template<class T> struct ComObject {
  T* value=nullptr;
  ~ComObject(){if(value)value->Release();}
  T** Address(){return &value;}
  T* operator->()const{return value;}
};
#endif

std::shared_ptr<const Avatar> ReadAvatar(PublicClient& client,const Player& player,
    uint64_t serial,std::stop_token cancel,bool& complete) {
  complete=false;
  if(player.avatar_path&&player.avatar_path->empty()) {
    complete=true;
    return {};
  }
  const auto path=player.avatar_path.value_or("/api/avatar/u/"+EncodeName(player.name));
  auto response=client.Get(path,512*1024,serial,"avatar",cancel);
  if(response.status!=200) {
    complete=response.status==404;
    return {};
  }
#ifdef _WIN32
  // NOBK serves JPEG photos as well as PNG. The SDK helper supports PNG only;
  // use Windows' image codecs without changing the pinned runtime binaries.
  const auto initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  if(FAILED(initialized)) {
    REXLOG_WARN("PLAYER_CARD request={} stage=avatar event=fallback reason=com_init hr={:08X}",serial,uint32_t(initialized));
    return {};
  }
  struct Apartment {~Apartment(){CoUninitialize();}} apartment;
  const auto failed=[&](HRESULT hr,const char* operation) {
    if(SUCCEEDED(hr))return false;
    REXLOG_WARN("PLAYER_CARD request={} stage=avatar event=fallback reason=decode_failed operation={} hr={:08X}",serial,operation,uint32_t(hr));
    return true;
  };
  ComObject<IWICImagingFactory> factory;
  if(failed(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,
      IID_PPV_ARGS(factory.Address())),"factory"))return {};
  ComObject<IWICStream> stream;
  if(failed(factory->CreateStream(stream.Address()),"stream")||
      failed(stream->InitializeFromMemory(reinterpret_cast<BYTE*>(response.body.data()),
          static_cast<DWORD>(response.body.size())),"memory"))return {};
  ComObject<IWICBitmapDecoder> decoder;
  if(failed(factory->CreateDecoderFromStream(stream.value,nullptr,WICDecodeMetadataCacheOnDemand,
      decoder.Address()),"decoder"))return {};
  GUID format{};
  if(failed(decoder->GetContainerFormat(&format),"format"))return {};
  const bool jpeg=IsEqualGUID(format,GUID_ContainerFormatJpeg),png=IsEqualGUID(format,GUID_ContainerFormatPng);
  if(!jpeg&&!png) {
    REXLOG_WARN("PLAYER_CARD request={} stage=avatar event=fallback reason=unsupported_format",serial);
    return {};
  }
  ComObject<IWICBitmapFrameDecode> frame;
  if(failed(decoder->GetFrame(0,frame.Address()),"frame"))return {};
  UINT width=0,height=0;
  if(failed(frame->GetSize(&width,&height),"size"))return {};
  // Validate dimensions before allocating or decoding pixel data.
  if(!width||!height||width>1024||height>1024) {
    REXLOG_WARN("PLAYER_CARD request={} stage=avatar event=fallback reason=dimensions width={} height={}",serial,width,height);
    return {};
  }
  ComObject<IWICFormatConverter> converter;
  if(failed(factory->CreateFormatConverter(converter.Address()),"converter")||
      failed(converter->Initialize(frame.value,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,
          nullptr,0.,WICBitmapPaletteTypeCustom),"convert"))return {};
  std::vector<uint8_t> pixels(size_t(width)*height*4);
  if(failed(converter->CopyPixels(nullptr,width*4,static_cast<UINT>(pixels.size()),pixels.data()),"pixels"))return {};
  complete=true;
  return std::make_shared<Avatar>(Avatar{width,height,std::move(pixels)});
#else
  return {};
#endif
}

enum class Status { Loading, Ready, Unavailable };
struct Snapshot {
  uint64_t serial = 0;
  uint32_t player_id=0;
  Status status = Status::Loading;
  Status ranking_status=Status::Unavailable;
  std::shared_ptr<const Ranking> ranking;
  Player player;
  std::shared_ptr<const Avatar> avatar;
  std::shared_ptr<const Results> results;
  std::shared_ptr<const Avatar> flags;
  std::shared_ptr<const Avatar> helmet;
  std::shared_ptr<const Avatar> track_map;
  int map_track=-1;
  bool map_loading=false;
  uint32_t preferred_view=0;
};

std::shared_ptr<const Avatar> ReadFlags() {
  int width=0,height=0;
  auto pixels=rex::ui::DecodeImageRGBA(flags::png,sizeof(flags::png),width,height);
  if(width!=768||height!=544||pixels.size()!=size_t(width)*height*4) {
    REXLOG_WARN("PLAYER_CARD stage=flags event=decode_failed");return {};
  }
  return std::make_shared<Avatar>(Avatar{uint32_t(width),uint32_t(height),std::move(pixels)});
}

std::span<const uint8_t> Embedded(uint16_t id) {
#ifdef _WIN32
  const auto module=GetModuleHandleW(nullptr);
  const auto resource=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));
  if(!resource)return {};
  const auto size=SizeofResource(module,resource);
  const auto loaded=LoadResource(module,resource);
  const auto* png=loaded?static_cast<const uint8_t*>(LockResource(loaded)):nullptr;
  if(!png||!size)return {};
  return {png,size};
#else
  (void)id;
  return {};
#endif
}
std::shared_ptr<const Avatar> ReadHelmet() {
  const auto png=Embedded(201);
  if(png.empty())return {};
  int width=0,height=0;
  auto pixels=rex::ui::DecodeImageRGBA(png.data(),png.size(),width,height);
  if(width!=1254||height!=1254||pixels.size()!=size_t(width)*height*4)return {};
  return std::make_shared<Avatar>(Avatar{uint32_t(width),uint32_t(height),std::move(pixels)});
}

void PrepareEmoji(ImFontAtlas* atlas) {
  emoji_aliases.clear();emoji_max_bytes=0;
#ifdef _WIN32
  const auto png=Embedded(202),metadata=Embedded(203);
  if(png.empty()||metadata.empty())throw std::runtime_error("Embedded bio emoji assets are missing");
  const auto table=nlohmann::json::parse(metadata.begin(),metadata.end());
  const auto count=Number(table,"glyphs"),cell=Number(table,"cell"),columns=Number(table,"columns");
  if(count<1||count>0x1900||cell!=32||columns!=64||!table.contains("aliases")||!table["aliases"].is_object())
    throw std::runtime_error("Embedded bio emoji metadata is invalid");
  int width=0,height=0;
  const auto rgba=rex::ui::DecodeImageRGBA(png.data(),png.size(),width,height);
  if(width!=columns*cell||height!=((count+columns-1)/columns)*cell||rgba.size()!=size_t(width)*height*4)
    throw std::runtime_error("Embedded bio emoji atlas is invalid");
  atlas->TexMinWidth=std::max(atlas->TexMinWidth,4096);
  const auto id=atlas->AddCustomRect(width,height);
  if(id==ImFontAtlasRectId_Invalid)throw std::runtime_error("Could not reserve bio emoji atlas");
  unsigned char* pixels;int tw,th;atlas->GetTexDataAsRGBA32(&pixels,&tw,&th);
  ImFontAtlasRect rect;
  if(!atlas->GetCustomRect(id,&rect))throw std::runtime_error("Could not locate bio emoji atlas");
  for(int y=0;y<height;++y)
    std::memcpy(pixels+(size_t(rect.y+y)*tw+rect.x)*4,rgba.data()+size_t(y)*width*4,size_t(width)*4);
  // Share the same packed color sprites at every native text size. Pixels are
  // stored once; only their glyph metrics differ between prepared fonts.
  for(size_t f=0;f<profile_fonts.size();++f)if(auto* font=profile_fonts[f]) {
    auto* baked=font->GetFontBaked(float(kFontSizes[f]));
    for(int index=0;index<count;++index) {
      ImFontGlyph glyph;glyph.Codepoint=0xE000+index;glyph.Visible=true;glyph.Colored=true;
      glyph.AdvanceX=float(kFontSizes[f])+1;glyph.X1=glyph.Y1=float(kFontSizes[f]);
      const auto x=rect.x+(index%columns)*cell,y=rect.y+(index/columns)*cell;
      glyph.U0=float(x)/tw;glyph.V0=float(y)/th;glyph.U1=float(x+cell)/tw;glyph.V1=float(y+cell)/th;
      ImFontAtlasBakedAddFontGlyph(atlas,baked,nullptr,&glyph);
    }
    font->Flags|=ImFontFlags_LockBakedSizes;
  }
  for(const auto& [sequence,index]:table["aliases"].items()) {
    if(sequence.empty()||sequence.size()>128||!index.is_number_unsigned()||index.get<uint64_t>()>=uint64_t(count))
      throw std::runtime_error("Embedded bio emoji alias is invalid");
    emoji_aliases.emplace(sequence,ImWchar(0xE000+index.get<unsigned>()));
    emoji_max_bytes=std::max(emoji_max_bytes,sequence.size());
  }
#else
  (void)atlas;
#endif
}

std::shared_ptr<const Avatar> ReadTrackMap(PublicClient& client,int track,uint64_t serial,
    std::stop_token cancel) {
  if(track<0||track>=int(std::size(kTrackMaps))||kTrackMaps[track].empty())return {};
  const auto response=client.Get("/assets/motogp/tracks/"+std::string(kTrackMaps[track]),
      256*1024,serial,"track_map",cancel);
  if(response.status!=200)return {};
  const auto& body=response.body;
  if(body.size()<33||body.compare(0,8,"\x89PNG\r\n\x1a\n",8)||body.compare(12,4,"IHDR"))
    throw std::runtime_error("Track outline response is not PNG");
  const auto dimension=[&](size_t at) {
    uint32_t value=0;
    for(size_t i=0;i<4;++i)value=(value<<8)|static_cast<uint8_t>(body[at+i]);
    return value;
  };
  const auto expected_width=dimension(16),expected_height=dimension(20);
  if(!expected_width||!expected_height||expected_width>1024||expected_height>1024)
    throw std::runtime_error("Track outline dimensions are invalid");
  int width=0,height=0;
  auto pixels=rex::ui::DecodeImageRGBA(reinterpret_cast<const uint8_t*>(body.data()),body.size(),width,height);
  if(width!=int(expected_width)||height!=int(expected_height)||pixels.size()!=size_t(width)*height*4)
    throw std::runtime_error("Track outline decode failed");
  return std::make_shared<Avatar>(Avatar{uint32_t(width),uint32_t(height),std::move(pixels)});
}

class State {
 public:
  State(rex::Runtime* runtime, std::shared_ptr<settings::InputGate> input_gate)
      : gate(std::move(input_gate)), kernel_(runtime->kernel_state()) {
    // Independent servers may reuse the same numeric IDs. Never resolve one
    // of their players against the production NOBK account directory.
    const auto origin = rex::cvar::Query<std::string>("online_server");
    available_ = rex::cvar::Query<std::string>("network_mode") == "online" &&
        (origin == "https://motogp.nobkonline.com" || origin == "https://motogp.nobkonline.com/");
  }
  ~State() { Stop(); }

  uint32_t Show(uint32_t user, uint64_t xuid,uint32_t preferred_view) {
    if (user >= 4 || (xuid & kPeerMask) != kPeerBase || !(xuid & 0xFFFFFF)) {
      REXLOG_WARN("PLAYER_CARD event=open_rejected reason=identity user={} xuid={:016X}",user,xuid);
      return 87;
    }
    if (!available_) return 1168;
    if (quit::Pending() || settings::GuestInputBlocked()) {
      return 170;
    }
    std::lock_guard lock(mutex_);
    if (stopping_) return 1168;
    if (gate->Blocking()) return 170;
    // Only copy the identity. Lookup is on demand on a worker; neither the
    // UI thread nor the guest's lobbies/leaderboards wait for HTTP.
    if (!worker_.joinable()) worker_ = std::thread([this] { Run(); });
    gate->Open();
    if (!gate->IsOpen()) return 170;
    cancellation_.request_stop();
    cancellation_=std::stop_source{};
    requested_id_.reset();
    requested_map_.reset();
    requested_ranking_=false;
    const auto id=static_cast<uint32_t>(xuid&0xFFFFFF);
    const auto serial=snapshot_.serial+1;
    snapshot_={};
    snapshot_.serial=serial;
    snapshot_.preferred_view=preferred_view;
    snapshot_.helmet=helmet_;
    bool fresh=false;
    const auto now=std::chrono::steady_clock::now();
    for(const auto& entry:cache_) {
      if(entry.id!=id||now-entry.stored>std::chrono::minutes(5))continue;
      snapshot_=entry.data;
      snapshot_.serial=serial;
      snapshot_.preferred_view=preferred_view;
      fresh=entry.complete&&now-entry.stored<std::chrono::seconds(60);
      break;
    }
    request_serial_=serial;
    if(!fresh)requested_id_=id;
    snapshot_.player_id=id;
    snapshot_.ranking.reset();snapshot_.ranking_status=Status::Unavailable;
    if(ranking_&&now-ranking_stored_<std::chrono::seconds(60)) {
      snapshot_.ranking=ranking_;snapshot_.ranking_status=Status::Ready;
    } else if(now>=ranking_retry_at_) {
      requested_ranking_=true;snapshot_.ranking_status=Status::Loading;
    }
    // The selected board is opening-specific, even on a cached player card.
    // Keep artwork separately keyed by circuit so another board cannot inherit
    // a previous card's map. A map-only task does not re-fetch the profile.
    snapshot_.map_track=preferred_view?int((preferred_view-1)%21):-1;
    snapshot_.track_map.reset();snapshot_.map_loading=false;
    if(snapshot_.map_track>=0) {
      const auto track=snapshot_.map_track;
      snapshot_.track_map=track_maps_[track];
      if(!snapshot_.track_map&&!kTrackMaps[track].empty()&&now>=map_retry_at_[track]) {
        requested_map_=track;snapshot_.map_loading=true;
      }
    }
    kernel_->BroadcastNotification(0x9, true); // XN_SYS_UI
    notified_ = true;
    changed_.notify_one();
    return 0;
  }
  Snapshot Read() {
    std::lock_guard lock(mutex_);
    return snapshot_;
  }
  void Dismiss() {
    std::lock_guard lock(mutex_);
    gate->Close();
    cancellation_.request_stop();
    requested_id_.reset();
    requested_map_.reset();
    requested_ranking_=false;
    if (notified_) { kernel_->BroadcastNotification(0x9, false); notified_ = false; }
  }
  void Stop() {
    {
      std::lock_guard lock(mutex_);
      stopping_ = true;
      cancellation_.request_stop();
      requested_id_.reset();
      requested_map_.reset();
      requested_ranking_=false;
      gate->Close();
      if (notified_) { kernel_->BroadcastNotification(0x9, false); notified_ = false; }
    }
    changed_.notify_one();
    if (worker_.joinable()) worker_.join();
  }
  const std::shared_ptr<settings::InputGate> gate;

 private:
  struct CachedProfile {
    uint32_t id;
    Snapshot data;
    std::chrono::steady_clock::time_point stored;
    bool complete;
  };
  // mutex_ protects the cache and visible snapshot together. Shared immutable
  // image/record data is retained for at most eight players; nothing hits disk.
  void Remember(uint32_t id,bool complete) {
    std::erase_if(cache_,[&](const auto& entry){return entry.id==id;});
    if(cache_.size()>=8)cache_.erase(cache_.begin());
    cache_.push_back({id,snapshot_,std::chrono::steady_clock::now(),complete});
  }
  void Run() {
    PublicClient client;
    std::shared_ptr<const Avatar> flag_atlas;
    try { flag_atlas=ReadFlags(); }
    catch(const std::exception&) { REXLOG_WARN("PLAYER_CARD stage=flags event=decode_failed"); }
    std::shared_ptr<const Avatar> helmet;
    try {
      helmet=ReadHelmet();
      if(!helmet)REXLOG_WARN("PLAYER_CARD stage=default_avatar event=unavailable");
    }
    catch(const std::exception&) { REXLOG_WARN("PLAYER_CARD stage=default_avatar event=decode_failed"); }
    std::unique_lock lock(mutex_);
    helmet_=std::move(helmet);
    snapshot_.helmet=helmet_;
    snapshot_.flags=flag_atlas;
    while (!stopping_) {
      changed_.wait(lock, [this] { return stopping_ || requested_id_.has_value() || requested_ranking_ || requested_map_.has_value(); });
      if (stopping_) break;
      // Publish identity, records, metadata and photo before optional ranking
      // or artwork requests. Neither can delay the existing profile fields.
      if(!requested_id_&&requested_ranking_) {
        const auto serial=request_serial_;
        const auto cancel=cancellation_.get_token();
        requested_ranking_=false;
        if(snapshot_.status!=Status::Ready) {
          snapshot_.ranking_status=Status::Unavailable;
          continue;
        }
        lock.unlock();
        std::shared_ptr<const Ranking> ranking;
        try {ranking=ReadRanking(client,serial,cancel);}
        catch(const RequestCancelled&) {} // Closing or switching cards is expected.
        catch(const std::exception& error) {
          REXLOG_WARN("PLAYER_CARD request={} stage=ranking event=unavailable reason={}",serial,error.what());
        }
        lock.lock();
        if(!stopping_&&!cancel.stop_requested()&&serial==snapshot_.serial&&gate->IsOpen()) {
          const auto now=std::chrono::steady_clock::now();
          if(ranking) {ranking_=ranking;ranking_stored_=now;}
          ranking_retry_at_=now+std::chrono::seconds(60);
          snapshot_.ranking=std::move(ranking);
          snapshot_.ranking_status=snapshot_.ranking?Status::Ready:Status::Unavailable;
        }
        continue;
      }
      if(!requested_id_&&requested_map_) {
        const auto track=*requested_map_;
        const auto serial=request_serial_;
        const auto cancel=cancellation_.get_token();
        requested_map_.reset();
        lock.unlock();
        std::shared_ptr<const Avatar> map;
        try {map=ReadTrackMap(client,track,serial,cancel);}
        catch(const RequestCancelled&) {} // Closing or switching cards is expected.
        catch(const std::exception& error) {
          REXLOG_WARN("PLAYER_CARD request={} stage=track_map event=unavailable reason={}",serial,error.what());
        }
        lock.lock();
        if(!stopping_&&!cancel.stop_requested()&&serial==snapshot_.serial&&gate->IsOpen()&&snapshot_.map_track==track) {
          track_maps_[track]=std::move(map);
          map_retry_at_[track]=std::chrono::steady_clock::now()+std::chrono::seconds(60);
          snapshot_.track_map=track_maps_[track];snapshot_.map_loading=false;
        }
        continue;
      }
      const auto id = *requested_id_;
      const auto serial = request_serial_;
      const auto cancel=cancellation_.get_token();
      requested_id_.reset();
      lock.unlock();
      Player player; bool loaded = false;
      std::shared_ptr<const Results> results;
      try { player = ReadPlayer(client,id,serial,results,cancel); loaded = true; }
      catch(const RequestCancelled&) {} // Closing or switching cards is expected.
      catch (const std::exception& error) {
        REXLOG_WARN("PLAYER_CARD request={} stage=identity event=failed reason={}",serial,error.what());
      }
      lock.lock();
      // Closing/reopening or selecting another player must not publish an
      // older response into the new card.
      if (!stopping_ && !cancel.stop_requested() && serial == snapshot_.serial && gate->IsOpen()) {
        snapshot_.flags=flag_atlas;
        if(loaded) {
          // A cached photo can remain visible during a same-player refresh.
          // A renamed account must never inherit its old account artwork.
          if(!SameName(snapshot_.player.name,player.name))snapshot_.avatar.reset();
          else {
            if(player.country.empty())player.country=snapshot_.player.country;
            player.member_since=snapshot_.player.member_since;
            player.avatar_path=snapshot_.player.avatar_path;
            player.bio=snapshot_.player.bio;
          }
          snapshot_.player=player;
          snapshot_.results=std::move(results);
          snapshot_.status=Status::Ready;
          Remember(id,false);
        } else if(snapshot_.status!=Status::Ready)snapshot_.status=Status::Unavailable;
        if (loaded) {
          // Publish the name first. An absent or slow picture must not delay
          // the profile action or return stale artwork for another player.
          lock.unlock();
          // Metadata is optional. A missing shared profile must not disable a
          // valid MotoGP identity or prevent the legacy avatar-by-name route.
          bool account_loaded=false;
          try { player=ReadAccount(client,player,serial,cancel);account_loaded=true; }
          catch(const RequestCancelled&) {} // Closing or switching cards is expected.
          catch(const std::exception& error) {
            REXLOG_WARN("PLAYER_CARD request={} stage=account event=failed reason={}",serial,error.what());
          }
          lock.lock();
          if(stopping_||cancel.stop_requested()||serial!=snapshot_.serial||!gate->IsOpen()) {
            continue;
          }
          snapshot_.player=player;
          Remember(id,false);
          lock.unlock();
          std::shared_ptr<const Avatar> avatar;
          bool avatar_loaded=false;
          try { avatar=ReadAvatar(client,player,serial,cancel,avatar_loaded); }
          catch(const RequestCancelled&) {} // Closing or switching cards is expected.
          catch (const std::exception& error) {
            REXLOG_WARN("PLAYER_CARD request={} stage=avatar event=fallback reason={}",serial,error.what());
          }
          lock.lock();
          if (!stopping_ && !cancel.stop_requested() && serial==snapshot_.serial && gate->IsOpen()) {
            if(avatar_loaded)snapshot_.avatar=std::move(avatar);
            Remember(id,account_loaded&&avatar_loaded);
          }
        }
      }
    }
  }
  rex::system::KernelState* kernel_;
  bool available_ = false, stopping_ = false, notified_ = false;
  std::mutex mutex_;
  std::condition_variable changed_;
  std::thread worker_;
  std::stop_source cancellation_;
  std::vector<CachedProfile> cache_;
  std::shared_ptr<const Ranking> ranking_;
  std::chrono::steady_clock::time_point ranking_stored_{},ranking_retry_at_{};
  bool requested_ranking_=false;
  std::shared_ptr<const Avatar> helmet_;
  std::array<std::shared_ptr<const Avatar>,std::size(kTracks)> track_maps_{};
  std::array<std::chrono::steady_clock::time_point,std::size(kTracks)> map_retry_at_{};
  Snapshot snapshot_;
  std::optional<uint32_t> requested_id_;
  std::optional<int> requested_map_;
  uint64_t request_serial_ = 0;
};

std::atomic<std::shared_ptr<State>> current;
thread_local uint32_t selected_view=0;

ImU32 Color(int red,int green,int blue,int alpha=255) {
  // GetColorU32 applies the current style alpha to custom geometry too.
  return ImGui::GetColorU32(ImVec4(red/255.f,green/255.f,blue/255.f,alpha/255.f));
}

bool AnimationsEnabled() {
#ifdef _WIN32
  BOOL enabled=TRUE;
  SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&enabled,0);
  return enabled!=FALSE;
#else
  return true;
#endif
}

std::string Membership(int64_t timestamp,bool tr,bool compact=false) {
  if(!timestamp)return Tr(tr,"Membership date unavailable","Üyelik tarihi alınamadı");
  constexpr const char* months[]{"January","February","March","April","May","June","July",
    "August","September","October","November","December"};
  constexpr const char* months_tr[]{"Ocak","Şubat","Mart","Nisan","Mayıs","Haziran","Temmuz",
    "Ağustos","Eylül","Ekim","Kasım","Aralık"};
  constexpr const char* short_months[]{"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  const std::chrono::year_month_day date{std::chrono::floor<std::chrono::days>(std::chrono::sys_seconds{std::chrono::seconds{timestamp}})};
  const auto month=unsigned(date.month())-1;
  return tr?std::string("Üyelik: ")+months_tr[month]+" "+std::to_string(int(date.year())):
      std::string("Member since ")+(compact?short_months[month]:months[month])+" "+std::to_string(int(date.year()));
}

std::string SetTime(int64_t timestamp,bool tr,bool full=false) {
  if(timestamp<=0||timestamp>=4102444800LL)return "-";
  if(!full) {
    const auto now=std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const auto age=std::max<int64_t>(0,now-timestamp);
    if(age<60)return Tr(tr,"just now","az önce");
    if(age<3600)return std::to_string(age/60)+Tr(tr," min ago"," dk önce");
    if(age<86400)return std::to_string(age/3600)+Tr(tr," h ago"," sa önce");
    if(age<86400*7)return std::to_string(age/86400)+
        (tr?" gün önce":(age/86400==1?" day ago":" days ago"));
  }
  const auto raw=static_cast<std::time_t>(timestamp);
  std::tm local{};
#ifdef _WIN32
  if(localtime_s(&local,&raw))return "-";
#else
  if(!localtime_r(&raw,&local))return "-";
#endif
  constexpr const char* months[]{"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  constexpr const char* months_tr[]{"Oca","Şub","Mar","Nis","May","Haz","Tem","Ağu","Eyl","Eki","Kas","Ara"};
  auto result=std::to_string(local.tm_mday)+" "+(tr?months_tr[local.tm_mon]:months[local.tm_mon])+" "+std::to_string(local.tm_year+1900);
  if(full) {
    const auto two=[](int number){return (number<10?"0":"")+std::to_string(number);};
    result+=" "+two(local.tm_hour)+":"+two(local.tm_min);
  }
  return result;
}

std::string Mode(int64_t mode,bool tr) {
  if(mode<0||mode>UINT32_MAX)return "-";
  const auto value=uint32_t(mode);
  if((value&~0x10000000u)>16)return "-";
  if(value&0x10000000u)return Tr(tr,"Championship","Şampiyona");
  // The native 8223F340 getter groups 3..6 as Career, 10 as Time Trial,
  // and 2 as Arcade. Preserve those stored mode bits instead of guessing.
  switch(value) {
    case 1:return Tr(tr,"Quick Race","Hızlı yarış");
    case 2:return Tr(tr,"Arcade","Arcade");
    case 3:case 4:case 5:case 6:return Tr(tr,"Career","Kariyer");
    case 7:return Tr(tr,"Preview","Önizleme");
    case 8:return Tr(tr,"Training","Antrenman");
    case 9:return Tr(tr,"Tutorial","Eğitim");
    case 10:return Tr(tr,"Time Trial","Zamana karşı");
    case 11:return "Splitscreen";
    case 12:return Tr(tr,"Online Splitscreen","Çevrimiçi splitscreen");
    case 13:return "LAN";
    case 14:return Tr(tr,"Online","Çevrimiçi");
    case 15:return "Demo";
    case 16:return Tr(tr,"Replay","Tekrar");
    default:return "-";
  }
}

void Flag(rex::ui::ImmediateTexture* atlas,std::string_view country,float scale) {
  if(!atlas||country.size()!=2)return;
  for(size_t i=0;i<(sizeof(flags::codes)-1)/2;++i) {
    if(country!=std::string_view(flags::codes+i*2,2))continue;
    const float x=float(i%16)*48.f+4.f,y=float(i/16)*32.f+1.f;
    const auto position=ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<ImTextureID>(atlas),position,
        {position.x+28.f*scale,position.y+21.f*scale},
        {x/atlas->width,y/atlas->height},{(x+40.f)/atlas->width,(y+30.f)/atlas->height},
        ImGui::GetColorU32(ImVec4(1,1,1,1)));
    ImGui::Dummy({28.f*scale,21.f*scale});
    if(ImGui::IsItemHovered())ImGui::SetTooltip("%.*s",int(country.size()),country.data());
    return;
  }
  // End SameLine cleanly for unrecognized country codes without a fake flag.
  ImGui::Dummy({0,21.f*scale});
}

struct View {
  int tab=0, category=0, first_row=0;
  float scroll=0.f, drag_offset=-1.f;
  bool reset_scroll=true;
  bool bio_open=false,bio_close=false;
  float bio_scroll=0.f;
};
std::vector<std::string> Categories(const Results* results,bool tr) {
  std::vector<std::string> categories{Tr(tr,"All classes","Tüm sınıflar")};
  if(results)for(const auto& row:results->rows)
    if(row.season!=1&&!row.category.empty()&&std::find(categories.begin()+1,categories.end(),row.category)==categories.end())
      categories.push_back(row.category);
  return categories;
}
std::vector<const Record*> OrderedRecords(const Results& results,uint32_t preferred_view,
    const std::vector<std::string>& categories,int category) {
  std::vector<const Record*> rows;
  // 2009 is the agreed initial scope. Do not relabel known 2010 records as 2009.
  for(const auto& row:results.rows)
    if(row.season!=1&&(category==0||row.category==categories[category]))rows.push_back(&row);
  if(preferred_view) {
    const auto weight=[&](const Record* row) {
      if(row->view==preferred_view)return 0;
      return row->track==int((preferred_view-1)%21)?1:2;
    };
    std::stable_sort(rows.begin(),rows.end(),[&](auto a,auto b){return weight(a)<weight(b);});
  }
  return rows;
}

// Coordinates below share one design canvas. Text and hit targets use the same
// scale, so saved child-window scroll and ImGui line spacing cannot clip stats.
struct Canvas {
  ImDrawList* draw;
  ImVec2 origin;
  float scale;
  ImFont* font;
  ImVec2 Point(float x,float y)const{return {std::round(origin.x+x*scale),std::round(origin.y+y*scale)};}
  FontFace Font(float pixels)const{return Face(font,pixels*scale);}
  float Measure(std::string_view text,float pixels)const {
    const auto face=Font(pixels);
    return face.font->CalcTextSizeA(face.size,FLT_MAX,0,text.data(),text.data()+text.size()).x/scale;
  }
  void Line(float x,float y,float end_x,float end_y)const {
    draw->AddLine(Point(x,y),Point(end_x,end_y),Color(48,54,64),scale);
  }
  float Text(float x,float y,float pixels,ImVec4 color,std::string text,float width=10000.f)const {
    const auto face=Font(pixels);
    if(Measure(text,pixels)>width) {
      while(!text.empty()&&Measure(text+"...",pixels)>width) {
        size_t end=text.size()-1;
        while(end&&(static_cast<unsigned char>(text[end])&0xC0)==0x80)--end;
        text.resize(end);
      }
      text+="...";
    }
    draw->AddText(face.font,face.size,Point(x,y),ImGui::GetColorU32(color),text.c_str());
    return Measure(text,pixels);
  }
  void Centered(float x,float y,float pixels,ImVec4 color,std::string text,float width)const {
    const bool truncated=Measure(text,pixels)>width;
    while(!text.empty()&&Measure(text+(truncated?"...":""),pixels)>width) {
      size_t end=text.size()-1;
      while(end&&(static_cast<unsigned char>(text[end])&0xC0)==0x80)--end;
      text.resize(end);
    }
    if(truncated)text+="...";
    const float at=x+(width-Measure(text,pixels))*.5f;
    Text(at,y,pixels,color,std::move(text),width);
  }
  void Rank(float x,float y,float width,float pixels,uint32_t rank,uint32_t total)const {
    const auto first=std::to_string(rank),rest=total?" / "+std::to_string(total):std::string{};
    // Step down through actual baked sizes rather than scaling one texture.
    while(pixels>1&&Measure(first+rest,pixels)>width)pixels-=1;
    if(Measure(first+rest,pixels)>width){Centered(x,y,pixels,rank==1?kAccent:kWhite,first+rest,width);return;}
    const float at=x+(width-Measure(first+rest,pixels))*.5f;
    Text(at,y,pixels,rank==1?kAccent:kWhite,first);
    Text(at+Measure(first,pixels),y,pixels,kMuted,rest);
  }
};

struct BioExcerpt {std::vector<std::string> lines;bool more=false;};
BioExcerpt Excerpt(const Canvas& c,const std::string& bio,float last_width) {
  BioExcerpt result;
  const auto face=c.Font(15.5f);
  size_t position=0;
  for(int line=0;line<3&&position<bio.size();++line) {
    const auto newline=bio.find('\n',position);
    const char* begin=bio.data()+position;
    const char* paragraph=bio.data()+(newline==std::string::npos?bio.size():newline);
    const char* end=face.font->CalcWordWrapPosition(face.size,begin,paragraph,
        (line==2?last_width:196.f)*c.scale);
    if(end<=begin&&begin<paragraph) {
      end=begin+1;
      while(end<paragraph&&(static_cast<unsigned char>(*end)&0xC0)==0x80)++end;
    }
    position=size_t(end-bio.data());
    if(end==paragraph)position+=newline!=std::string::npos?1:0;
    else while(position<bio.size()&&bio[position]==' ')++position;
    while(end>begin&&end[-1]==' ')--end;
    result.lines.emplace_back(begin,end);
  }
  result.more=position<bio.size();
  return result;
}
void BioPreview(const Snapshot& state,View& view,const Canvas& c,bool tr,bool ready) {
  if(state.player.bio.empty())return;
  c.Text(32,397,13.5f,kMuted,Tr(tr,"ABOUT ME","HAKKINDA"),196);
  const auto bio=EmojiText(state.player.bio);
  auto excerpt=Excerpt(c,bio,196);
  if(excerpt.more)excerpt=Excerpt(c,bio,152);
  for(size_t i=0;i<excerpt.lines.size();++i)c.Text(32,420+float(i)*19,15.5f,kWhite,excerpt.lines[i],196);
  if(excerpt.more) {
    ImGui::SetCursorScreenPos(c.Point(194,455));
    ImGui::BeginDisabled(!ready);
    if(ImGui::Button("...##license_bio_more",{34*c.scale,24*c.scale}))view.bio_open=true;
    if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",Tr(tr,"Read full bio","Biyografinin tamamını oku"));
    ImGui::EndDisabled();
  }
}
void Stripes(const Canvas& c) {
  // Distinct native-style ramps: most of the left stripe fades, only the lower
  // tip of the middle fades, and the entire right stripe stays solid orange.
  const auto uv=ImGui::GetFontTexUvWhitePixel();
  for(int stripe=0;stripe<3;++stripe)for(int band=0;band<24;++band) {
    const float a=band/24.f,b=(band+1)/24.f;
    const auto point=[&](float t,float edge){return c.Point(18.f+stripe*15.f+24.f*t+edge,49.f-36.f*t);};
    const auto tint=[&](float t) {
      auto color=kAccent;
      const float fade_length=stripe==0?.8f:.18f;
      const float ramp=std::clamp(t/fade_length,0.f,1.f);
      color.w=stripe==2?1.f:ramp*ramp*(3.f-2.f*ramp);
      return ImGui::GetColorU32(color);
    };
    c.draw->PrimReserve(6,4);
    const auto index=c.draw->_VtxCurrentIdx;
    c.draw->PrimWriteIdx(static_cast<ImDrawIdx>(index));
    c.draw->PrimWriteIdx(static_cast<ImDrawIdx>(index+1));
    c.draw->PrimWriteIdx(static_cast<ImDrawIdx>(index+2));
    c.draw->PrimWriteIdx(static_cast<ImDrawIdx>(index));
    c.draw->PrimWriteIdx(static_cast<ImDrawIdx>(index+2));
    c.draw->PrimWriteIdx(static_cast<ImDrawIdx>(index+3));
    c.draw->PrimWriteVtx(point(a,0),uv,tint(a));
    c.draw->PrimWriteVtx(point(a,12),uv,tint(a));
    c.draw->PrimWriteVtx(point(b,12),uv,tint(b));
    c.draw->PrimWriteVtx(point(b,0),uv,tint(b));
  }
}
void Portrait(const Canvas& c,rex::ui::ImmediateTexture* texture,float side,bool placeholder) {
  const auto position=c.Point(32,96);
  const float size=side*c.scale;
  if(!texture)return;
  if(!placeholder)c.draw->AddRectFilled(position,{position.x+size,position.y+size},Color(12,15,20));
  const float fit=(side-(placeholder?16.f:0.f))*c.scale/std::max(texture->width,texture->height);
  const ImVec2 dimensions(texture->width*fit,texture->height*fit);
  const ImVec2 first(position.x+(size-dimensions.x)*.5f,position.y+(size-dimensions.y)*.5f);
  c.draw->AddImage(reinterpret_cast<ImTextureID>(texture),first,{first.x+dimensions.x,first.y+dimensions.y},
      {0,0},{1,1},ImGui::GetColorU32(ImVec4(1,1,1,1)));
  if(!placeholder)c.draw->AddRect(position,{position.x+size,position.y+size},Color(49,54,66));
}
void TrackOutline(const Snapshot& state,const Canvas& c,rex::ui::ImmediateTexture* texture,
    int track,float x,float y,float width,float height,bool tr) {
  if(texture&&state.map_track==track) {
    const float fit=std::min(width/texture->width,height/texture->height)*c.scale;
    const ImVec2 size(texture->width*fit,texture->height*fit);
    const auto at=c.Point(x,y);
    const ImVec2 first(std::round(at.x+(width*c.scale-size.x)*.5f),std::round(at.y+(height*c.scale-size.y)*.5f));
    c.draw->AddImage(reinterpret_cast<ImTextureID>(texture),first,{first.x+size.x,first.y+size.y},
        {0,0},{1,1},ImGui::GetColorU32(ImVec4(1,1,1,1)));
  } else c.Centered(x,y+height*.5f-7,13,kMuted,state.map_loading?
      Tr(tr,"Loading map...","Şema yükleniyor..."):Tr(tr,"Map unavailable","Şema bulunamadı"),width);
}

void Overview(const Snapshot& state,const Canvas& c,float x,float width,bool tr,rex::ui::ImmediateTexture* map) {
  if(!state.results){c.Text(x,160,16,kMuted,Tr(tr,"Statistics unavailable","İstatistikler alınamadı"),width);return;}
  const auto categories=Categories(state.results.get(),tr);
  const auto rows=OrderedRecords(*state.results,0,categories,0);
  const auto summary=SummarizeRecords(rows);
  const std::string values[]{std::to_string(summary.tracks_held)+" / 20",std::to_string(summary.records_held),std::to_string(summary.boards_entered)};
  const char* labels[]{Tr(tr,"TRACKS HELD","LİDER OLDUĞU PİSTLER"),Tr(tr,"RECORDS HELD","LİDER OLDUĞU TABLOLAR"),Tr(tr,"BOARDS ENTERED","KATILDIĞI TABLOLAR")};
  const float column=width/3.f;
  for(int i=0;i<3;++i) {
    const float at=x+i*column;
    c.Centered(at+10,164,40,kAccent,values[i],column-20);
    c.Centered(at+10,217,15.5f,kMuted,labels[i],column-20);
    if(i)c.Line(at,162,at,244);
  }
  const float fraction=std::clamp(float(summary.tracks_held)/20.f,0.f,1.f),bar=width-70.f;
  c.Text(x,273,16,kMuted,Tr(tr,"TOTAL TRACK DOMINATION","TOPLAM PİST HÂKİMİYETİ"),width);
  c.draw->AddRectFilled(c.Point(x,304),c.Point(x+bar,314),ImGui::GetColorU32(overlay::card),2*c.scale);
  if(fraction)c.draw->AddRectFilled(c.Point(x,304),c.Point(x+bar*fraction,314),ImGui::GetColorU32(kAccent),2*c.scale);
  c.Centered(x+bar+12,299,17,kWhite,std::to_string(int(fraction*100.f+.5f))+"%",58);
  c.Line(x,346,x+width,346);
  c.Text(x,368,15,kMuted,Tr(tr,"PERSONAL BEST","KİŞİSEL EN İYİ"),width);
  if(!state.preferred_view) {
    c.Centered(x,434,19,kMuted,Tr(tr,"No track selected.","Seçili pist yok."),width);return;
  }
  const auto record=std::find_if(rows.begin(),rows.end(),[&](auto row){return row->view==state.preferred_view;});
  const int track=int((state.preferred_view-1)%21);
  constexpr float map_width=166.f,rank_width=142.f;
  const float rank=x+width-rank_width;
  const float details=x+map_width+28.f,details_width=rank-details-18.f;
  TrackOutline(state,c,map,track,x+6,403,map_width-12,94,tr);
  c.Line(x+map_width+10,400,x+map_width+10,496);
  c.Line(rank,400,rank,496);
  c.Centered(details,399,18,kWhite,record!=rows.end()?(*record)->track_name:
      (kTracks[track].empty()?Tr(tr,"Unknown circuit","Bilinmeyen pist"):std::string(kTracks[track])),details_width);
  c.Centered(rank+10,399,13,kMuted,Tr(tr,"Rank","Sıra"),rank_width-10);
  if(record==rows.end()) {
    c.Centered(details,437,16,kMuted,Tr(tr,"No published lap for this board.","Bu tabloda yayımlanmış turu yok."),details_width);
    c.Centered(rank+10,429,26,kMuted,"-",rank_width-10);
    return;
  }
  const auto& row=**record;
  c.Centered(details,430,36,kAccent,row.result,details_width);
  c.Centered(details,475,14,kMuted,row.category+" · 2009 · "+Mode(row.mode,tr),details_width);
  c.Rank(rank+10,423,rank_width-10,26,row.rank,row.total);
  c.Centered(rank+10,465,12.5f,kMuted,std::string(Tr(tr,"Set · ","Tarih · "))+SetTime(row.updated,tr),rank_width-10);
  if(row.updated>0&&ImGui::IsWindowHovered()&&
      ImGui::IsMouseHoveringRect(c.Point(rank,460),c.Point(x+width,489)))
    ImGui::SetTooltip("%s",SetTime(row.updated,tr,true).c_str());
}
void Records(const Snapshot& state,View& view,const Canvas& c,float x,float width,float bottom,bool tr,bool ready) {
  const auto categories=Categories(state.results.get(),tr);
  view.category=std::clamp(view.category,0,int(categories.size())-1);
  ImGui::SetCursorScreenPos(c.Point(x,143));
  ImGui::SetNextItemWidth(174*c.scale);
  ImGui::BeginDisabled(!ready);
  // The card uses zero window padding. A combo inherits its vertical padding,
  // so give this popup its own inset and complete selectable row heights.
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{12*c.scale,8*c.scale});
  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,4*c.scale);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{8*c.scale,6*c.scale});
  const float popup_height=(categories.size()*30.f+24.f)*c.scale;
  ImGui::SetNextWindowSizeConstraints({174*c.scale,popup_height},{300*c.scale,popup_height});
  ImGui::SetNextWindowScroll({0,0});
  if(ImGui::BeginCombo("##license_class",categories[view.category].c_str())) {
    for(int i=0;i<int(categories.size());++i) {
      if(ImGui::Selectable(categories[i].c_str(),view.category==i,0,{0,24*c.scale})){view.category=i;view.reset_scroll=true;}
      if(view.category==i)ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  ImGui::PopStyleVar(3);
  ImGui::EndDisabled();
  if(!state.results){c.Text(x,211,16,kMuted,Tr(tr,"Records unavailable","Tur kayıtları alınamadı"),width);return;}
  const auto rows=OrderedRecords(*state.results,state.preferred_view,categories,view.category);
  c.Text(x+width-90,149,13,kMuted,std::to_string(rows.size())+Tr(tr," records"," kayıt"),90);
  const float track=x+100.f,time=x+width-342.f,mode=x+width-222.f,set=x+width-122.f;
  c.Centered(x+6,194,13,kMuted,Tr(tr,"Rank","Sıra"),82);
  c.Text(track,194,13,kMuted,Tr(tr,"Track / class","Pist / sınıf"),time-track-16);
  c.Centered(time,194,13,kMuted,Tr(tr,"Best lap","En iyi tur"),112);
  c.Centered(mode,194,13,kMuted,Tr(tr,"Mode","Mod"),84);
  c.Centered(set,194,13,kMuted,Tr(tr,"Set","Tarih"),104);
  const float top=220.f,available=bottom-top;
  const int visible=std::max(1,int(available/46.f));
  const float row_height=available/visible;
  const int maximum=std::max(0,int(rows.size())-visible);
  if(view.reset_scroll)view.first_row=0;
  else if(view.scroll)view.first_row+=view.scroll>0?1:-1;
  const auto first=c.Point(x,top),last=c.Point(x+width,bottom);
  if(ready&&ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)&&ImGui::IsMouseHoveringRect(first,last))
    view.first_row-=int(ImGui::GetIO().MouseWheel*3);
  view.first_row=std::clamp(view.first_row,0,maximum);
  // Only complete rows are drawn. Scrolling changes the first row, never the
  // whole-card position, so header, filters and footer cannot be scrolled away.
  if(rows.empty())c.Text(x,top+20,16,kMuted,Tr(tr,"No published records in this category.","Bu kategoride yayımlanmış tur kaydı yok."),width);
  for(int i=0;i<visible&&view.first_row+i<int(rows.size());++i) {
    const auto& row=*rows[view.first_row+i];
    const float y=top+i*row_height;
    if(row.view==state.preferred_view)c.draw->AddRectFilled(c.Point(x,y),c.Point(x+width-18,y+row_height),ImGui::GetColorU32(overlay::AccentTint(.10f)));
    c.Rank(x+6,y+(row_height-18)*.5f,82,18,row.rank,row.total);
    c.Text(track,y+6,15,kWhite,row.track_name,time-track-18);
    c.Text(track,y+29,12.5f,kMuted,row.category+" · 2009",time-track-18);
    c.Centered(time,y+13,17,kAccent,row.result,112);
    c.Centered(mode,y+15,14,kWhite,Mode(row.mode,tr),84);
    c.Centered(set,y+15,14,kMuted,SetTime(row.updated,tr),104);
    if(ready&&row.updated>0&&ImGui::IsWindowHovered()&&
        ImGui::IsMouseHoveringRect(c.Point(set,y),c.Point(set+104,y+row_height)))
      ImGui::SetTooltip("%s",SetTime(row.updated,tr,true).c_str());
    c.Line(x,y+row_height,x+width-18,y+row_height);
  }
  if(maximum) {
    const float height=available,thumb=std::max(22.f,height*visible/rows.size());
    const float thumb_y=top+(height-thumb)*view.first_row/maximum;
    ImGui::SetCursorScreenPos(c.Point(x+width-10,top));
    ImGui::BeginDisabled(!ready);
    ImGui::InvisibleButton("##license_scrollbar",{10*c.scale,height*c.scale});
    if(ImGui::IsItemActive()) {
      const float mouse=(ImGui::GetIO().MousePos.y-c.origin.y)/c.scale;
      if(view.drag_offset<0)view.drag_offset=mouse>=thumb_y&&mouse<=thumb_y+thumb?mouse-thumb_y:thumb*.5f;
      const float fraction=std::clamp((mouse-top-view.drag_offset)/(height-thumb),0.f,1.f);
      view.first_row=int(fraction*maximum+.5f);
    } else view.drag_offset=-1;
    ImGui::EndDisabled();
    c.draw->AddRectFilled(c.Point(x+width-7,top),c.Point(x+width-3,bottom),Color(35,41,50),2*c.scale);
    c.draw->AddRectFilled(c.Point(x+width-7,thumb_y),c.Point(x+width-3,thumb_y+thumb),Color(119,134,154),2*c.scale);
  }
  view.scroll=0;view.reset_scroll=false;
}

void BioPopup(const Snapshot& state,View& view,const Canvas& parent,bool tr,bool ready) {
  constexpr const char* id="##license_full_bio";
  if(view.bio_close||state.player.bio.empty())view.bio_open=false;
  const bool popup_open=ImGui::IsPopupOpen(id);
  if(!view.bio_open&&!popup_open) {
    view.bio_close=false;view.bio_scroll=0;return;
  }
  const bool opening=view.bio_open&&!popup_open;
  if(opening)ImGui::OpenPopup(id);
  const float scale=parent.scale;
  const auto bio=EmojiText(state.player.bio);
  const auto body_font=parent.Font(18);
  const float text_height=body_font.font->CalcTextSizeA(body_font.size,FLT_MAX,572*scale,bio.c_str()).y/scale;
  const float height=std::clamp(text_height+176.f,240.f,440.f);
  const auto& io=ImGui::GetIO();
  ImGui::SetNextWindowPos({io.DisplaySize.x*.5f,io.DisplaySize.y*.5f},ImGuiCond_Always,{.5f,.5f});
  ImGui::SetNextWindowSize({620*scale,height*scale},ImGuiCond_Always);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{24*scale,20*scale});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,4*scale);
  if(ImGui::BeginPopupModal(id,nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings|
      ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoNavInputs|ImGuiWindowFlags_NoScrollWithMouse)) {
    Canvas c{ImGui::GetWindowDrawList(),ImGui::GetWindowPos(),scale,parent.font};
    if(!view.bio_open||view.bio_close||state.player.bio.empty()) {
      view.bio_open=false;
      ImGui::CloseCurrentPopup();
    } else {
      c.Text(24,20,22,kAccent,Tr(tr,"ABOUT ME","HAKKINDA"),572);
      c.Text(24,52,16,kMuted,state.player.name,572);
      c.Line(24,80,596,80);
      ImGui::SetCursorPos({24*scale,98*scale});
      ImGui::PushFont(body_font.font,body_font.size);
      if(opening)ImGui::SetNextWindowScroll({0,0});
      if(ImGui::BeginChild("##license_bio_text",{572*scale,(height-164)*scale},0,ImGuiWindowFlags_NoNavInputs)) {
        if(!opening&&view.bio_scroll)ImGui::SetScrollY(std::clamp(ImGui::GetScrollY()+view.bio_scroll,0.f,ImGui::GetScrollMaxY()));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+ImGui::GetContentRegionAvail().x);
        ImGui::TextUnformatted(bio.c_str());
        ImGui::PopTextWrapPos();
      }
      ImGui::EndChild();
      ImGui::PopFont();
      c.Line(24,height-52,596,height-52);
      ImGui::SetCursorPos({265*scale,(height-38)*scale});
      ImGui::BeginDisabled(!ready);
      if(ImGui::Button(Tr(tr,"Close","Kapat"),{90*scale,30*scale})) {
        view.bio_open=false;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndDisabled();
    }
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar(2);
  view.bio_close=false;
  view.bio_scroll=0;
}

struct Action { bool profile = false, close = false; };
void RiderRank(const Snapshot& state,const Canvas& c,bool tr,bool tooltip) {
  size_t position=0;
  if(state.ranking) {
    const auto& players=state.ranking->players;
    const auto player=std::find_if(players.begin(),players.end(),
        [&](const auto& item){return item.id==state.player_id;});
    if(player!=players.end())position=size_t(player-players.begin())+1;
  }
  const auto value=position?"#"+std::to_string(position):
      std::string(state.ranking_status==Status::Loading?"...":"-");
  c.Text(858,26,13,kMuted,Tr(tr,"RIDER RANK","PİLOT SIRASI"),128);
  c.Centered(996,17,30,position?kAccent:kMuted,value,72);
  if(!tooltip||!ImGui::IsWindowHovered()||
      !ImGui::IsMouseHoveringRect(c.Point(850,12),c.Point(1074,54)))return;
  if(state.ranking_status==Status::Loading)
    ImGui::SetTooltip("%s",Tr(tr,"Loading NOBK Online rider ranking...","NOBK Online pilot sıralaması yükleniyor..."));
  else if(state.ranking_status!=Status::Ready)
    ImGui::SetTooltip("%s",Tr(tr,"Rider rank is unavailable.","Pilot sırası alınamadı."));
  else if(!position)
    ImGui::SetTooltip("%s",Tr(tr,"Not listed in NOBK Online's ranking (up to 100 riders).",
        "NOBK Online sıralamasında listelenmiyor (en fazla 100 pilot)."));
  else ImGui::SetTooltip("%s",Tr(tr,
      "NOBK Online rider ranking: tracks held, then records held, then points.\nUses the website's track catalogue and ranking scope.",
      "NOBK Online pilot sıralaması: lider olduğu pist, ardından rekor ve puan sayısı.\nSitenin pist kataloğu ve sıralama kapsamı kullanılır."));
}
Action DrawCard(const Snapshot& state, rex::ui::ImmediateTexture* texture,
    rex::ui::ImmediateTexture* flag_atlas,rex::ui::ImmediateTexture* helmet,
    rex::ui::ImmediateTexture* map,View& view,bool tr, ImFont* font, ImFont* heading, bool ready,float alpha) {
  (void)heading;
  const auto& io=ImGui::GetIO();
  const float scale=std::max(.1f,std::min({io.DisplaySize.x/1440.f,io.DisplaySize.y/900.f,1.35f}));
  const ImVec2 size(1100.f*scale,580.f*scale);
  const bool main_ready=ready&&!view.bio_open;
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ImGui::GetStyle().Alpha*alpha);
  ImGui::GetBackgroundDrawList()->AddRectFilled({0,0},io.DisplaySize,Color(0,0,0,155));
  const auto body_font=Face(font?font:ImGui::GetFont(),16.f*scale);
  ImGui::PushFont(body_font.font,body_font.size);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,2.f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,2.f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{12.f*scale,7.f*scale});
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{10.f*scale,8.f*scale});
  ImGui::PushStyleColor(ImGuiCol_WindowBg,overlay::ink);
  ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(.19f,.21f,.26f,1));
  ImGui::PushStyleColor(ImGuiCol_Text,kWhite);
  ImGui::PushStyleColor(ImGuiCol_TextDisabled,kMuted);
  ImGui::PushStyleColor(ImGuiCol_Button,overlay::card);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,overlay::AccentTint(.24f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,overlay::AccentTint(.36f));
  ImGui::PushStyleColor(ImGuiCol_NavCursor,kAccent);
  ImGui::PushStyleColor(ImGuiCol_FrameBg,overlay::card);
  ImGui::PushStyleColor(ImGuiCol_PopupBg,overlay::ink);
  ImGui::SetNextWindowPos({io.DisplaySize.x*.5f,io.DisplaySize.y*.5f},ImGuiCond_Always,{.5f,.5f});
  ImGui::SetNextWindowSize(size,ImGuiCond_Always);
  Action action;
  if(ImGui::Begin("##motogp_player_card",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings|
      ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoNavInputs|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)) {
    ImGui::SetScrollY(0);
    Canvas c{ImGui::GetWindowDrawList(),ImGui::GetWindowPos(),scale,font?font:ImGui::GetFont()};
    c.draw->AddRectFilledMultiColor(c.Point(0,0),c.Point(1100,64),
        ImGui::GetColorU32(overlay::AccentTint(.10f)),ImGui::GetColorU32(overlay::ink),
        ImGui::GetColorU32(overlay::ink),ImGui::GetColorU32(overlay::ink));
    Stripes(c);
    c.Text(96,21,24,kWhite,Tr(tr,"RIDER LICENSE","PİLOT LİSANSI"),740);
    RiderRank(state,c,tr,!view.bio_open);
    c.draw->AddLine(c.Point(0,64),c.Point(1100,64),ImGui::GetColorU32(kAccent),scale);
    c.Line(260,84,260,496);
    Portrait(c,texture?texture:helmet,196,!texture);
    const auto name=state.status==Status::Ready?state.player.name:
        Tr(tr,state.status==Status::Loading?"Loading...":"Player unavailable",state.status==Status::Loading?"Yükleniyor...":"Oyuncu bilgisi alınamadı");
    const float name_width=c.Text(32,310,26,kAccent,name,154);
    if(flag_atlas&&!state.player.country.empty()){
      ImGui::SetCursorScreenPos(c.Point(32+name_width+12,315));Flag(flag_atlas,state.player.country,scale);
    }
    if(state.player.member_since) {
      auto membership=Membership(state.player.member_since,tr);
      if(c.Measure(membership,14.5f)>196)membership=Membership(state.player.member_since,tr,true);
      c.Text(32,365,14.5f,kMuted,std::move(membership),196);
    }
    BioPreview(state,view,c,tr,main_ready);
    constexpr float content_x=292.f,content_width=776.f;
    constexpr float tabs_x=content_x+(content_width-334.f)*.5f;
    for(int tab=0;tab<2;++tab) {
      ImGui::SetCursorScreenPos(c.Point(tabs_x+tab*170,84));
      ImGui::BeginDisabled(!main_ready);
      ImGui::PushID(tab);
      if(ImGui::InvisibleButton("license_tab",{164*scale,38*scale})){view.tab=tab;view.reset_scroll=true;}
      ImGui::PopID();
      ImGui::EndDisabled();
      c.Centered(tabs_x+tab*170,92,18,view.tab==tab?kAccent:kMuted,tab?Tr(tr,"Records","Tur kayıtları"):Tr(tr,"Overview","Genel bilgiler"),164);
    }
    c.Line(content_x,124,content_x+content_width,124);
    c.draw->AddLine(c.Point(tabs_x+view.tab*170,124),c.Point(tabs_x+view.tab*170+164,124),ImGui::GetColorU32(kAccent),3*scale);
    if(state.status!=Status::Ready)c.Text(content_x,170,16,kMuted,state.status==Status::Loading?
        Tr(tr,"Loading player...","Oyuncu yükleniyor..."):Tr(tr,"Could not load this profile. Close and try again.","Profil yüklenemedi. Kapatıp tekrar dene."),content_width);
    else if(view.tab==0)Overview(state,c,content_x,content_width,tr,map);
    else Records(state,view,c,content_x,content_width,505,tr,main_ready);
    c.Line(32,516,1068,516);
    ImGui::SetCursorScreenPos(c.Point(32,534));
    ImGui::BeginDisabled(!main_ready||state.status!=Status::Ready);
    action.profile=ImGui::Button(Tr(tr,"Open NOBK Online profile","NOBK Online profilini aç"));
    ImGui::EndDisabled();
    std::string controls=Tr(tr,"LB / RB: Tabs   A: Profile   B: Back",
        "LB / RB: Sekmeler   A: Profil   B: Geri");
    if(view.tab==1)controls+=Tr(tr,"   D-pad: Scroll","   Yön: Kaydır");
    if(!state.player.bio.empty())controls+="   Y: Bio";
    // The hint string changes with the active tab and bio availability. Keep
    // the entire group centered on the same content area in every state.
    c.Centered(content_x,543,12,kMuted,std::move(controls),content_width);
    BioPopup(state,view,c,tr,ready);
  }
  ImGui::End();
  ImGui::PopStyleColor(10);ImGui::PopStyleVar(6);ImGui::PopFont();
  return action;
}


class Dialog final : public rex::ui::ImGuiDialog, public rex::ui::WindowInputListener {
 public:
  Dialog(rex::ui::ImGuiDrawer* drawer, rex::ui::Window* window,
      rex::ui::ImmediateDrawer* immediate, std::shared_ptr<State> state,
      ImFont* font, ImFont* heading, bool turkish)
      : ImGuiDialog(drawer), window_(window), state_(std::move(state)),
        immediate_(immediate), font_(font), heading_(heading), tr_(turkish) {
    if (window_) window_->AddInputListener(this, 62);
  }
  ~Dialog() override {
    if (window_) window_->RemoveInputListener(this);
    state_->Dismiss();
  }
  void OnKeyDown(rex::ui::KeyEvent& event) override {
    // ImGui has already received the key at priority 64. Let key-UP events
    // reach MnK, but do not forward card navigation to the native menu/F7.
    if (state_->gate->IsOpen() && !(event.is_alt_pressed() &&
        event.virtual_key() == rex::ui::VirtualKey::kF4)) event.set_handled(true);
  }
  void OnKeyChar(rex::ui::KeyEvent& event) override {
    if (state_->gate->IsOpen()) event.set_handled(true);
  }
  void OnMouseDown(rex::ui::MouseEvent& event) override {
    if (state_->gate->IsOpen()) event.set_handled(true);
  }

 protected:
  void OnDraw(ImGuiIO&) override {
    if (!state_->gate->IsOpen()) { armed_ = false; previous_ = 0; return; }
    const auto snapshot = state_->Read();
    const auto epoch = state_->gate->Epoch();
    if (serial_ != snapshot.serial || epoch_ != epoch) {
      if (serial_ != snapshot.serial) {
        texture_.reset(); avatar_.reset();
        view_={};scroll_at_=0.;
        shown_at_=ImGui::GetTime();animate_=AnimationsEnabled();
      }
      serial_ = snapshot.serial; epoch_ = epoch; armed_ = false; previous_ = 0;
      ImGui::SetNextWindowFocus();
    }
    const auto buttons = state_->gate->Buttons();
    if (!armed_ && state_->gate->NeutralReady() && !buttons) armed_ = true;
    const float progress=animate_?std::clamp(static_cast<float>((ImGui::GetTime()-shown_at_)/.18),0.f,1.f):1.f;
    const float alpha=progress*progress*(3.f-2.f*progress);
    const bool ready = armed_ && state_->gate->NeutralReady() && progress>=1.f;
    const uint16_t fresh = ready ? buttons & ~previous_ : 0;
    previous_ = buttons;
    if(snapshot.flags&&snapshot.flags!=flags_) {
      flags_=snapshot.flags;flag_texture_.reset();
      if(flags_&&immediate_)flag_texture_=immediate_->CreateTexture(flags_->width,flags_->height,
          rex::ui::ImmediateTextureFilter::kLinear,false,flags_->rgba.data());
      if(flags_&&!flag_texture_)REXLOG_WARN("PLAYER_CARD request={} stage=flags event=texture_upload_failed",snapshot.serial);
    }
    if(snapshot.helmet!=helmet_) {
      helmet_=snapshot.helmet;helmet_texture_.reset();
      if(helmet_&&immediate_)helmet_texture_=immediate_->CreateTexture(helmet_->width,helmet_->height,
          rex::ui::ImmediateTextureFilter::kLinear,false,helmet_->rgba.data());
      if(helmet_&&!helmet_texture_)REXLOG_WARN("PLAYER_CARD request={} stage=default_avatar event=texture_upload_failed",snapshot.serial);
    }
    if(snapshot.track_map!=track_map_) {
      track_map_=snapshot.track_map;map_texture_.reset();
      if(track_map_&&immediate_)map_texture_=immediate_->CreateTexture(track_map_->width,track_map_->height,
          rex::ui::ImmediateTextureFilter::kLinear,false,track_map_->rgba.data());
      if(track_map_&&!map_texture_)REXLOG_WARN("PLAYER_CARD request={} stage=track_map event=texture_upload_failed track={}",
          snapshot.serial,snapshot.map_track);
    }
    if (snapshot.avatar!=avatar_) {
      avatar_=snapshot.avatar;
      texture_.reset();
      if (avatar_ && immediate_) texture_=immediate_->CreateTexture(avatar_->width,avatar_->height,
          rex::ui::ImmediateTextureFilter::kLinear,false,avatar_->rgba.data());
      if(avatar_&&!texture_)REXLOG_WARN("PLAYER_CARD request={} stage=avatar event=texture_upload_failed",snapshot.serial);
    }
    Action shortcuts;
    const bool bio_was_open=view_.bio_open;
    if(ready) {
      const bool back=(fresh&rex::input::X_INPUT_GAMEPAD_B)||ImGui::IsKeyPressed(ImGuiKey_Escape,false);
      const bool confirm=(fresh&rex::input::X_INPUT_GAMEPAD_A)||ImGui::IsKeyPressed(ImGuiKey_Enter,false);
      if(bio_was_open)view_.bio_close=back||confirm;
      else {
        shortcuts.close=back;
        if(!back&&!snapshot.player.bio.empty()&&
            ((fresh&rex::input::X_INPUT_GAMEPAD_Y)||ImGui::IsKeyPressed(ImGuiKey_R,false)))view_.bio_open=true;
        else shortcuts.profile=confirm;
      }
      if(!view_.bio_open) {
        const bool previous=(fresh&rex::input::X_INPUT_GAMEPAD_LEFT_SHOULDER)||ImGui::IsKeyPressed(ImGuiKey_Q,false);
        const bool next=(fresh&rex::input::X_INPUT_GAMEPAD_RIGHT_SHOULDER)||ImGui::IsKeyPressed(ImGuiKey_E,false);
        if(previous!=next) { view_.tab=1-view_.tab;view_.reset_scroll=true; }
      }
      if(!view_.bio_open&&view_.tab==1) {
        if((fresh&rex::input::X_INPUT_GAMEPAD_X)||ImGui::IsKeyPressed(ImGuiKey_F,false)) {
          view_.category=(view_.category+1)%int(Categories(snapshot.results.get(),tr_).size());view_.reset_scroll=true;
        }
      }
      if(view_.bio_open||view_.tab==1) {
        const auto now=ImGui::GetTime();
        const uint16_t directions=buttons&(rex::input::X_INPUT_GAMEPAD_DPAD_UP|rex::input::X_INPUT_GAMEPAD_DPAD_DOWN);
        const bool repeat=directions&&(now>=scroll_at_||(fresh&directions));
        const bool down=(repeat&&(directions&rex::input::X_INPUT_GAMEPAD_DPAD_DOWN))||ImGui::IsKeyPressed(ImGuiKey_DownArrow);
        const bool up=(repeat&&(directions&rex::input::X_INPUT_GAMEPAD_DPAD_UP))||ImGui::IsKeyPressed(ImGuiKey_UpArrow);
        if(down!=up) {
          const float movement=(down?1.f:-1.f)*60.f*std::clamp(ImGui::GetIO().DisplaySize.y/640.f,.65f,1.5f);
          if(view_.bio_open)view_.bio_scroll=movement;
          else view_.scroll=movement;
        }
        if(repeat)scroll_at_=now+.12;
      }
    }
    auto action = DrawCard(snapshot, texture_.get(),flag_texture_.get(),helmet_texture_.get(),map_texture_.get(),view_, tr_, font_, heading_, ready,alpha);
    action.close|=shortcuts.close;
    // Closing the bio consumes A/Enter and B/Esc. Neither action may also
    // open the website or dismiss the license behind the modal in that frame.
    if(!bio_was_open&&!view_.bio_open)action.profile|=shortcuts.profile;
    if (action.close) { state_->Dismiss(); return; }
    if (action.profile && snapshot.status == Status::Ready) {
      if(!OpenProfile(snapshot.player.name))
        REXLOG_WARN("PLAYER_CARD request={} event=open_profile_failed",snapshot.serial);
    }
  }

 private:
  rex::ui::Window* window_;
  std::shared_ptr<State> state_;
  rex::ui::ImmediateDrawer* immediate_;
  std::shared_ptr<const Avatar> avatar_;
  std::shared_ptr<const Avatar> flags_;
  std::shared_ptr<const Avatar> helmet_;
  std::shared_ptr<const Avatar> track_map_;
  std::unique_ptr<rex::ui::ImmediateTexture> texture_;
  std::unique_ptr<rex::ui::ImmediateTexture> flag_texture_;
  std::unique_ptr<rex::ui::ImmediateTexture> helmet_texture_;
  std::unique_ptr<rex::ui::ImmediateTexture> map_texture_;
  View view_;
  double scroll_at_=0.;
  ImFont* font_;
  ImFont* heading_;
  bool tr_, armed_ = false;
  bool animate_=true;
  double shown_at_=0.;
  uint64_t serial_ = 0, epoch_ = 0;
  uint16_t previous_ = 0;
};
}

void ConfigureFonts(ImFontAtlas* atlas,const char* path) {
  // The pinned renderer uploads a static atlas. All profile sizes must exist
  // before that upload; scaling the 17px settings font blurs larger headings.
  static const ImWchar ranges[]{0x20,0x1ff,0};
  profile_fonts.fill(nullptr);
  ImFontConfig config;
  config.OversampleH=2;config.OversampleV=1;config.PixelSnapH=true;
  for(size_t i=0;i<profile_fonts.size();++i)
    profile_fonts[i]=atlas->AddFontFromFileTTF(path,float(kFontSizes[i]),&config,ranges);
  const auto loaded=std::count_if(profile_fonts.begin(),profile_fonts.end(),[](auto* font){return font!=nullptr;});
  if(size_t(loaded)!=profile_fonts.size())
    REXLOG_WARN("PLAYER_CARD event=configure_fonts_failed loaded={} expected={}",loaded,profile_fonts.size());
  try {PrepareEmoji(atlas);}
  catch(const std::exception& error) {
    emoji_aliases.clear();emoji_max_bytes=0;
    REXLOG_WARN("PLAYER_CARD event=emoji_font_failed reason={}",error.what());
  }
}

void Initialize(rex::Runtime* runtime, std::shared_ptr<settings::InputGate> gate) {
  current.store(std::make_shared<State>(runtime, std::move(gate)), std::memory_order_release);
}
void Shutdown() {
  if (auto state = current.exchange({}, std::memory_order_acq_rel)) state->Stop();
}
bool Active() {
  const auto state = current.load(std::memory_order_acquire);
  return state && state->gate->Blocking();
}
std::unique_ptr<rex::ui::ImGuiDialog> CreateMenu(rex::ui::ImGuiDrawer* drawer,
    rex::ui::Window* window, rex::ui::ImmediateDrawer* immediate,
    ImFont* font, ImFont* heading, bool turkish) {
  auto state = current.load(std::memory_order_acquire);
  return state ? std::make_unique<Dialog>(drawer, window, immediate, std::move(state), font, heading, turkish) : nullptr;
}
}

REX_HOOK_RAW(__imp__XamShowGamerCardUIForXUID) {
  (void)base;
  // Native 8239C430 sets r5=0 and forwards r3=user / r4=selected XUID.
  // Both the lobby and leaderboard reach it through vtable slot +56 of
  // 82303260. Do not substitute the launcher's signed-in account here.
  const auto state = motogp::gamer_card::current.load(std::memory_order_acquire);
  try { ctx.r3.u64 = state ? state->Show(ctx.r3.u32, ctx.r4.u64,motogp::gamer_card::selected_view) : 1168; }
  catch (const std::exception& error) {
    if (state) state->Dismiss();
    REXLOG_WARN("PLAYER_CARD: could not open: {}", error.what());
    ctx.r3.u64 = 8;
  }
}

REX_HOOK_RAW(sub_82303260) {
  // Only the verified leaderboard call site carries a selected board. The
  // native selector 82300088 stores its current view at 833362D8. Copy the
  // integer synchronously; a lobby call must never inherit the last board.
  auto& selection=motogp::gamer_card::selected_view;
  struct Restore {uint32_t& target;uint32_t previous;~Restore(){target=previous;}} restore{selection,selection};
  selection=0;
  if(ctx.lr==0x8223EDD4) {
    const auto view=rex::memory::load_and_swap<uint32_t>(base+0x833362D8);
    if(view>=1&&view<=378)selection=view;
  }
  __imp__sub_82303260(ctx,base);
}
