#pragma once
#include "relay.h"
#include <filesystem>
#include <map>
#include <mutex>
#include <condition_variable>

namespace motogp::online {
struct Column {uint16_t id=0;uint8_t type=0;uint64_t bits=0;};
struct View {uint32_t id=0;std::vector<Column> columns;};
struct Row {uint64_t peer=0;uint32_t rank=0;std::string name;std::vector<Column> columns;};
struct Page {uint32_t total=0;std::vector<Row> rows;};
class Client {
  std::string url_,credential_;
  Ticket ticket_;
  mutable std::mutex writes_mutex_;mutable std::condition_variable writes_changed_;mutable unsigned pending_writes_=0;
  std::vector<uint8_t> Request(const std::string&,std::span<const uint8_t>)const;
 public:
  // The profile is protected for the current Windows user with DPAPI. It never
  // lives in the TOML settings, crash report, build archive or log output.
  Client(std::string url,std::string name,const std::filesystem::path& profile={});
  ~Client();
  Client(const Client&)=delete;Client& operator=(const Client&)=delete;
  const Ticket& Connection()const{return ticket_;}
  void Write(const std::vector<View>&,const std::string& nonce)const;
  void BeginWrite()const;void EndWrite()const;void Flush()const;
  Page Read(uint32_t view,uint32_t start,uint32_t count,
                        std::span<const uint64_t> users={},uint64_t around=0)const;
};
// Binds this PC's online profile for `url` to a NOBK Online site account, using
// the launcher token from the site's sign-in API. Returns the account name the
// server will use for this player. The launcher calls it before starting the game.
std::string LinkAccount(std::string url,const std::string& launcher_token,const std::filesystem::path& profile={});
}
