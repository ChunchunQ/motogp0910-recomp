#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <toml++/toml.hpp>

#include <algorithm>
#include <cmath>
#include <array>
#include "keyboard_bindings.h"
#include "render_presets.h"
#include "engine_options.h"
#include "../adaptive_trigger_config.h"
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace launcher {
namespace fs = std::filesystem;
inline constexpr auto kUpdate = "TU_11K61VG_0000004000000.0000000000081";
inline constexpr auto kXexHash = "54245417e996ed0aa561a5a803fd810e87684bd0b43082f316db711b3ee786ae";
inline constexpr auto kPatchHash = "0419205ba10dc65eaa7b0bb62abc64cada46119ece5ed749e4ba20c06a6bc079";
inline constexpr auto kUpdateHash = "441df6cc2e20ac8aeeed475138c1714d477f965423d8c5c9b0202e3a18d8141f";

inline std::string Utf8(std::wstring_view value) {
  if (value.empty()) return {};
  int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
      static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
  if (!size) throw std::runtime_error("Invalid UTF-16 text");
  std::string result(size, '\0');
  WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
      static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
  return result;
}
inline std::wstring Wide(std::string_view value) {
  if (value.empty()) return {};
  int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
      static_cast<int>(value.size()), nullptr, 0);
  if (!size) throw std::runtime_error("Invalid UTF-8 text");
  std::wstring result(size, L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
      static_cast<int>(value.size()), result.data(), size);
  return result;
}
struct Error : std::runtime_error {
  std::wstring english, turkish;
  Error(std::wstring en, std::wstring tr)
      : std::runtime_error(Utf8(en)), english(std::move(en)), turkish(std::move(tr)) {}
};
inline fs::path ExecutableFolder() {
  std::wstring buffer(32768, L'\0');
  auto n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  if (!n || n == buffer.size()) throw std::runtime_error("Cannot locate executable");
  buffer.resize(n);
  return fs::path(buffer).parent_path();
}
inline std::optional<std::string> ReadFile(const fs::path& path) {
  if (!fs::exists(path)) return std::nullopt;
  std::ifstream file(path, std::ios::binary);
  if (!file) throw Error(L"Cannot read file: " + path.wstring(), L"Dosya okunamıyor: " + path.wstring());
  std::string bytes((std::istreambuf_iterator<char>(file)), {});
  if (file.bad()) throw std::runtime_error("File read failed");
  return bytes;
}
// Parse the same flattened key syntax used by the SDK; retain unrelated tables.
inline const toml::node* Find(const toml::table& table, std::string_view name,
                             std::string prefix = {}) {
  for (const auto& [key, node] : table) {
    auto full = prefix.empty() ? std::string(key.str()) : prefix + "_" + std::string(key.str());
    if (full == name && !node.is_table()) return &node;
    if (node.is_table()) if (auto* found = Find(*node.as_table(), name, full)) return found;
  }
  return nullptr;
}
template <typename T>
inline T Get(const toml::table& table, std::string_view name, T fallback) {
  auto* node = Find(table, name);
  return node ? node->value<T>().value_or(fallback) : fallback;
}
inline void Erase(toml::table& table, std::string_view name, std::string prefix = {}) {
  for (auto it = table.begin(); it != table.end();) {
    auto full = prefix.empty() ? std::string(it->first.str()) : prefix + "_" + std::string(it->first.str());
    if (full == name && !it->second.is_table()) it = table.erase(it);
    else { if (it->second.is_table()) Erase(*it->second.as_table(), name, full); ++it; }
  }
}
template <typename T>
inline void Put(toml::table& table, std::string_view name, const T& value) {
  Erase(table, name);
  table.insert_or_assign(name, value);
}
class Config {
 public:
  fs::path path;
  toml::table table;
  std::optional<std::string> original;
  explicit Config(fs::path file) : path(std::move(file)), original(ReadFile(path)) {
    if (original) table = toml::parse(*original);
  }
  void Save() {
    if (ReadFile(path) != original) throw Error(
        L"Settings changed outside the launcher. Close and reopen it before saving.",
        L"Ayar dosyası launcher dışında değişti. Kaydetmeden önce launcher'ı yeniden aç.");
    std::ostringstream output;
    output << toml::toml_formatter{table} << '\n';
    auto bytes = output.str();
    // Keep the exact pre-launcher file once; never overwrite an existing backup.
    const fs::path backup(path.wstring() + L".bak");
    if (original && !fs::exists(backup)) fs::copy_file(path, backup);
    const fs::path temporary(path.wstring() + L".tmp");
    // CREATE_NEW prevents clobbering an unrelated or concurrent temporary file.
    HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw Error(
        L"Cannot save settings. Use a writable folder; close other launcher instances.",
        L"Ayarlar kaydedilemiyor. Yazılabilir bir klasör kullan; diğer launcher örneklerini kapat.");
    DWORD written = 0;
    bool ok = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr)
        && written == bytes.size() && FlushFileBuffers(file);
    CloseHandle(file);
    if (!ok || !MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
      DeleteFileW(temporary.c_str());
      throw Error(L"Failed to write settings; the original file was preserved.",
                  L"Ayarlar yazılamadı; önceki dosya korundu.");
    }
    original = std::move(bytes);
  }
};
struct Settings {
  motogp::adaptive::Profile adaptive;
  std::string aspect="auto";
  bool debug_overlay_enabled=false;
  bool lan_enabled=false;
  std::string network_mode="offline",online_server;
  std::string lan_player_name="Player",lan_room_name="MotoGP room",lan_bind_ip,lan_host_ip;
  int width = 0, height = 0, scale_x = 1, scale_y = 1, anisotropic = 3, volume = 100, adapter = -1;
  int guest_width=0,guest_height=0,game_language=1,refresh_rate=0,fps_limit=0;
  bool fullscreen = true, vsync = true, keyboard = true, mute = false;
  bool letterbox = true, dither = false, shader_cache = true, msaa = true;
  double fsr_reduction = 0.2, cas_sharpness = 0.0;
  std::string backend = "sdl", effect = "bilinear", post_effect = "none", prompt_style = "auto", display_mode = "auto", fsr_quality = "auto";
  std::array<std::string,kBindings.size()> bindings{};
  std::array<std::string,kEngineEffects.size()> engine_effects{"game","game","game"};
  int shadow_distance=0;
  Settings() { for(size_t i=0;i<bindings.size();++i) bindings[i]=kBindings[i].defaults; }
  static Settings Load(const toml::table& t) {
    Settings s;
    s.debug_overlay_enabled=Get(t,"debug_overlay_enabled",false);
    s.adaptive.enabled=Get(t,"dualsense_adaptive_triggers",false);
    for(int side=0;side<2;++side){auto& p=side?s.adaptive.left:s.adaptive.right;std::string key=side?"dualsense_lt_":"dualsense_rt_";
      p.mode=Get(t,key+"mode",p.mode);p.start=Get<int64_t>(t,key+"start",p.start);p.end=Get<int64_t>(t,key+"end",p.end);p.strength=Get<int64_t>(t,key+"strength",p.strength);
    }
    s.lan_enabled=Get(t,"lan_enabled",false);
    s.network_mode=Get(t,"network_mode",std::string("legacy"));
    if(s.network_mode=="legacy")s.network_mode=s.lan_enabled?"lan":"offline";
    s.lan_enabled=s.network_mode=="lan";
    s.online_server=Get(t,"online_server",std::string{});
    s.lan_player_name=Get(t,"lan_player_name",std::string("Player"));
    s.lan_room_name=Get(t,"lan_room_name",std::string("MotoGP room"));
    s.lan_bind_ip=Get(t,"lan_bind_ip",std::string{});s.lan_host_ip=Get(t,"lan_host_ip",std::string{});
    s.width = Get<int64_t>(t, "window_width", 0);
    s.height = Get<int64_t>(t, "window_height", 0);
    s.guest_width=Get<int64_t>(t,"video_mode_width",0);
    s.guest_height=Get<int64_t>(t,"video_mode_height",0);
    s.fullscreen = Get(t, "fullscreen", true);
    s.display_mode=Get(t,"display_mode",std::string("auto"));
    if(s.display_mode!="auto")s.fullscreen=s.display_mode!="windowed";
    s.refresh_rate=Get<int64_t>(t,"host_refresh_rate",0);s.fps_limit=Get<int64_t>(t,"fps_limit",0);
    s.vsync = Get(t, "vsync", true);
    s.keyboard = Get(t, "mnk_mode", true);
    s.mute = Get(t, "audio_mute", false);
    s.letterbox = Get(t, "present_letterbox", true);
    s.dither = Get(t, "present_dither", false);
    s.shader_cache = Get(t, "store_shaders", true);
    s.msaa = Get(t, "native_2x_msaa", true);
    s.post_effect = Get(t, "swap_post_effect", std::string("none"));
    s.backend = Get(t, "input_backend", std::string("sdl"));
    s.prompt_style = Get(t, "button_prompt_style", std::string("auto"));
    s.game_language = Get(t,"game_text_language",std::string("original"))=="turkish"?1000:Get<int64_t>(t, "user_language", 1);
    s.effect = Get(t, "present_effect", std::string("bilinear"));
    s.aspect=Get(t,"aspect_ratio",std::string("auto"));
    s.fsr_quality=Get(t,"present_fsr_quality_mode",std::string("auto"));
    s.scale_x = Get<int64_t>(t, "draw_resolution_scale_x", 1);
    s.scale_y = Get<int64_t>(t, "draw_resolution_scale_y", 1);
    // The SDK's single-value alias takes precedence only if actually changed.
    const auto uniform = Get<int64_t>(t, "resolution_scale", 1);
    if(uniform>1) s.scale_x=s.scale_y=static_cast<int>(uniform);
    s.anisotropic = Get<int64_t>(t, "anisotropic_override", 3);
    s.volume = Get<int64_t>(t, "audio_master_volume", 100);
    s.adapter = Get<int64_t>(t, "d3d12_adapter", -1);
    s.fsr_reduction = Get(t, "present_fsr_sharpness_reduction", 0.2);
    s.cas_sharpness = Get(t, "present_cas_additional_sharpness", 0.0);
    for(size_t i=0;i<s.bindings.size();++i)
      s.bindings[i]=Get(t,kBindings[i].key,std::string(kBindings[i].defaults));
    for(size_t i=0;i<s.engine_effects.size();++i)s.engine_effects[i]=Get(t,kEngineEffects[i].key,std::string("game"));
    s.shadow_distance=Get<int64_t>(t,"engine_shadow_distance",0);
    return s;
  }
  void Validate() const {
    if(network_mode!="offline" && network_mode!="lan" && network_mode!="online")throw Error(L"Choose Offline, LAN or Online.",L"Çevrimdışı, LAN veya Çevrimiçi seç.");
    if(network_mode=="online" && (online_server.empty() || online_server.size()>512 || online_server.find_first_of("\r\n\t ")!=std::string::npos || (!online_server.starts_with("https://") && !online_server.starts_with("http://127.0.0.1:") && !online_server.starts_with("http://localhost:"))))throw Error(L"Online requires a community server HTTPS address (for example https://play.example.com).",L"Çevrimiçi için topluluk sunucusunun HTTPS adresi gerekli (örneğin https://play.example.com).");
    if(!adaptive.Valid())throw Error(L"Invalid DualSense profile. Resistance-zone end must exceed start; positions 0-9, strength 0-8.",L"Geçersiz DualSense profili. Direnç aralığının bitişi başlangıçtan büyük olmalı; konumlar 0-9, güç 0-8.");
    const auto ipv4=[](const std::string& value){
      if(value.empty())return true;unsigned parts=0,number=0,digits=0;uint32_t address=0;
      for(char c:value){if(c=='.'){if(!digits || number>255 || ++parts>3)return false;address=(address<<8)|number;number=digits=0;}
        else if(c>='0' && c<='9'){if(++digits>3)return false;number=number*10+(c-'0');}else return false;}
      address=(address<<8)|number;return parts==3 && digits && number<=255 && address!=0 && address!=UINT32_MAX;
    };
    if((network_mode!="offline" && (lan_player_name.empty() || lan_player_name.size()>15 || lan_player_name.find('\0')!=std::string::npos ||
       lan_room_name.empty() || lan_room_name.size()>31 || lan_room_name.find('\0')!=std::string::npos ||
       (network_mode=="lan" && (!ipv4(lan_bind_ip) || !ipv4(lan_host_ip))))))throw Error(
          L"LAN: player name must be 1-15 UTF-8 bytes, room name 1-31 bytes; use a valid IPv4 or leave it empty.",
          L"LAN: oyuncu adı 1-15 UTF-8 bayt, oda adı 1-31 bayt olmalı; geçerli IPv4 yaz veya boş bırak.");
    Wide(lan_player_name);Wide(lan_room_name);
    if(aspect!="auto" && aspect!="16:9" && aspect!="21:9" && aspect!="32:9")throw Error(L"Unsupported aspect ratio.",L"Desteklenmeyen en-boy oranı.");
    for(const auto& effect:engine_effects)if(effect!="game" && effect!="off")throw Error(L"Unsupported engine effect setting.",L"Desteklenmeyen motor efekt ayarı.");
    if(shadow_distance<0 || shadow_distance>180)throw Error(L"Shadow distance must be 0 to 180.",L"Gölge mesafesi 0 ile 180 arasında olmalı.");
    if(width<0 || height<0 || width>8192 || height>8192 ||
       guest_width<0 || guest_height<0 || guest_width>8192 || guest_height>8192 ||
       scale_x<1 || scale_x>8 || scale_y<1 || scale_y>8 ||
       adapter < -2 || anisotropic < -1 || anisotropic>5 || volume<0 || volume>100 ||
       !std::isfinite(fsr_reduction) || fsr_reduction<0 || fsr_reduction>2 ||
       !std::isfinite(cas_sharpness) || cas_sharpness<0 || cas_sharpness>1 ||
       (backend!="sdl" && backend!="xinput") ||
       refresh_rate<0 || refresh_rate>1000 || fps_limit<0 || fps_limit>1000 ||
       (display_mode!="auto" && display_mode!="fullscreen" && display_mode!="borderless" && display_mode!="windowed") ||
       (game_language!=1 && game_language!=3 && game_language!=4 && game_language!=5 && game_language!=6 && game_language!=1000) ||
       (prompt_style!="auto" && prompt_style!="playstation" && prompt_style!="xbox" && prompt_style!="keyboard") ||
       (effect!="bilinear" && effect!="cas" && effect!="fsr" && effect!="fsr2" && effect!="fsr3") ||
       (fsr_quality!="auto" && fsr_quality!="nativeaa" && fsr_quality!="quality" && fsr_quality!="balanced" && fsr_quality!="performance" && fsr_quality!="ultra_performance") ||
       (post_effect!="none" && post_effect!="fxaa" && post_effect!="fxaa_extreme"))
      throw Error(L"Unsupported settings value. Choose a supported option or restore defaults.",
                  L"Desteklenmeyen ayar değeri. Desteklenen bir seçenek seç veya varsayılanlara dön.");
    for(size_t i=0;i<bindings.size();++i) if(!ValidBinding(bindings[i]))
      throw Error(L"Invalid binding for "+std::wstring(kBindings[i].english)+
          L". Use names such as Space, Return or Shift+Up; separate alternatives with commas.",
          L"Geçersiz tuş: "+std::wstring(kBindings[i].turkish)+
          L". Space, Return veya Shift+Up gibi adlar kullan; alternatifleri virgülle ayır.");
  }
  void Store(toml::table& t) const {
    Validate();
    Put(t,"dualsense_adaptive_triggers",adaptive.enabled);
    for(int side=0;side<2;++side){const auto& p=side?adaptive.left:adaptive.right;std::string key=side?"dualsense_lt_":"dualsense_rt_";
      Put(t,key+"mode",p.mode);Put(t,key+"start",p.start);Put(t,key+"end",p.end);Put(t,key+"strength",p.strength);
    }
    Put(t,"network_mode",network_mode);Put(t,"online_server",online_server);
    Put(t,"lan_enabled",network_mode=="lan");Put(t,"lan_player_name",lan_player_name);Put(t,"lan_room_name",lan_room_name);
    Put(t,"lan_bind_ip",lan_bind_ip);Put(t,"lan_host_ip",lan_host_ip);
    for(size_t i=0;i<engine_effects.size();++i)Put(t,kEngineEffects[i].key,engine_effects[i]);
    Put(t,"aspect_ratio",aspect);
    Put(t,"engine_shadow_distance",shadow_distance);
    Put(t,"debug_overlay_enabled",debug_overlay_enabled);
    // The removed launcher toggle must not leave older saved builds unable to
    // skip intros. Keep the runtime's internal diagnostic switch out of the UI.
    Put(t,"allow_intro_skip",true);
    Put(t, "window_width", width); Put(t, "window_height", height);
    if(guest_width)Put(t,"video_mode_width",guest_width);
    if(guest_height)Put(t,"video_mode_height",guest_height);
    Put(t, "fullscreen", fullscreen); Put(t, "vsync", vsync);
    Put(t,"display_mode",display_mode);Put(t,"host_refresh_rate",refresh_rate);Put(t,"fps_limit",fps_limit);
    Put(t, "mnk_mode", keyboard); Put(t, "audio_mute", mute);
    Put(t, "input_backend", backend); Put(t, "present_effect", effect);Put(t,"present_fsr_quality_mode",fsr_quality);
    Put(t, "button_prompt_style", prompt_style);
    Put(t, "user_language", game_language==1000?1:game_language);
    Put(t,"game_text_language",std::string(game_language==1000?"turkish":"original"));
    Put(t, "draw_resolution_scale_x", scale_x); Put(t, "draw_resolution_scale_y", scale_y);
    // Reset the alias so it cannot override the two edited axes.
    Put(t, "resolution_scale", 1);
    Put(t, "d3d12_adapter", adapter);
    Put(t, "anisotropic_override", anisotropic); Put(t, "audio_master_volume", volume);
    Put(t, "present_letterbox", letterbox); Put(t, "present_dither", dither);
    Put(t, "native_2x_msaa", msaa);
    Put(t, "store_shaders", shader_cache); Put(t, "swap_post_effect", post_effect);
    Put(t, "present_fsr_sharpness_reduction", fsr_reduction);
    Put(t, "present_cas_additional_sharpness", cas_sharpness);
    for(size_t i=0;i<bindings.size();++i) Put(t,kBindings[i].key,bindings[i]);
  }
};
inline fs::path ResolveGameRoot(const fs::path& exe, const toml::table& t) {
  auto root = Get(t, "game_data_root", std::string{});
  if (!root.empty()) {
    auto path = fs::path(Wide(root));
    return path.is_absolute() ? path : exe / path;
  }
  if (fs::is_regular_file(exe / "game/default.xex")) return exe / "game";
  if (fs::is_regular_file(exe / "default.xex")) return exe;
  return exe / "game";
}
inline std::string Sha256(const fs::path& path) {
  BCRYPT_ALG_HANDLE algorithm = nullptr;
  BCRYPT_HASH_HANDLE hash = nullptr;
  std::array<unsigned char, 32> digest{};
  if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
    throw std::runtime_error("SHA-256 initialization failed");
  if (BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) < 0) {
    BCryptCloseAlgorithmProvider(algorithm, 0); throw std::runtime_error("SHA-256 creation failed");
  }
  std::ifstream file(path, std::ios::binary);
  std::array<unsigned char, 65536> buffer;
  bool ok = static_cast<bool>(file);
  while (ok && file) {
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    if (file.gcount() && BCryptHashData(hash, buffer.data(), static_cast<ULONG>(file.gcount()), 0) < 0) ok = false;
  }
  ok = ok && !file.bad() && BCryptFinishHash(hash, digest.data(), digest.size(), 0) >= 0;
  BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(algorithm, 0);
  if (!ok) throw std::runtime_error("Could not hash game file");
  std::string result;
  for (auto byte : digest) { result += "0123456789abcdef"[byte >> 4]; result += "0123456789abcdef"[byte & 15]; }
  return result;
}
inline void Validate(const fs::path& exe, const fs::path& game) {
  for (const auto* name : {L"motogp0910.exe", L"rexruntime.dll", L"rexgpu-xenos.dll", L"amd_fidelityfx_dx12.dll",
       L"msvcp140.dll", L"msvcp140_atomic_wait.dll", L"vcruntime140.dll", L"vcruntime140_1.dll"}) {
    if (!fs::is_regular_file(exe / name)) throw Error(
        L"Program file missing: " + std::wstring(name) + L". Extract the complete Windows package.",
        L"Program dosyası eksik: " + std::wstring(name) + L". Windows paketini tamamen çıkar.");
  }
  if (!fs::is_regular_file(game / "default.xex")) throw Error(
      L"default.xex is missing from the selected game folder.",
      L"Seçilen oyun klasöründe default.xex yok.");
  if (Sha256(game / "default.xex") != kXexHash) throw Error(
      L"This is not the supported stock default.xex. Use your original, unmodified disk executable.",
      L"Bu desteklenen stok default.xex değil. Diskten çıkardığın değiştirilmemiş orijinal dosyayı kullan.");
  if (fs::is_regular_file(game / "default.xexp")) {
    if (Sha256(game / "default.xexp") != kPatchHash) throw Error(
        L"default.xexp does not match the supported title update. It was not overwritten.",
        L"default.xexp desteklenen title update ile eşleşmiyor. Dosya değiştirilmedi.");
  } else if (!fs::is_regular_file(game / kUpdate) || Sha256(game / kUpdate) != kUpdateHash) {
    throw Error(L"Copy your title update or its verified default.xexp patch into the game folder.",
                L"Title update dosyanı veya doğrulanmış default.xexp patch'ini oyun klasörüne koy.");
  }
  // Dashboard artwork (nxeart) may legitimately be empty in the disk dump.
  for (const auto* name : {L"xenondata"}) {
    if (!fs::is_directory(game / name) || fs::is_empty(game / name)) throw Error(
        L"Game data folder missing or empty: " + std::wstring(name) + L". Copy the complete disk data.",
        L"Oyun veri klasörü eksik veya boş: " + std::wstring(name) + L". Diskin bütün verilerini kopyala.");
  }
}
// Windows CRT command-line quoting, including embedded quotes/trailing slashes.
inline std::wstring Quote(std::wstring_view value) {
  std::wstring out = L"\"";
  size_t slashes = 0;
  for (auto c : value) {
    if (c == L'\\') { ++slashes; continue; }
    out.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
    slashes = 0; out += c;
  }
  out.append(slashes * 2, L'\\'); out += L'"'; return out;
}
inline std::vector<std::wstring> Arguments(const fs::path& game, const Settings& s,
                                         const toml::table& config) {
  std::vector<std::wstring> args = {L"--game_data_root=" + game.wstring(),
      L"--window_width=" + std::to_wstring(s.width), L"--window_height=" + std::to_wstring(s.height),
      std::wstring(L"--fullscreen=") + (s.fullscreen ? L"true" : L"false"),
      std::wstring(L"--vsync=") + (s.vsync ? L"true" : L"false"),
      std::wstring(L"--mnk_mode=") + (s.keyboard ? L"true" : L"false"),
      std::wstring(L"--audio_mute=") + (s.mute ? L"true" : L"false"),
      L"--button_prompt_style=" + Wide(s.prompt_style),
      L"--input_backend=" + Wide(s.backend)};
  // Forward every managed setting with command-line priority, including mappings.
  toml::table managed; s.Store(managed);
  for(const auto& [key,node]:managed) {
    const auto name=std::string(key.str());
    if(name=="window_width" || name=="window_height" || name=="fullscreen" || name=="vsync" ||
       name=="mnk_mode" || name=="audio_mute" || name=="input_backend" || name=="button_prompt_style") continue;
    std::string value;
    // toml++ value<bool>() converts integers too. Preserve each node's actual
    // type so values such as 3x, 720 and 37% never become the string "true".
    if(node.is_string()) value=*node.value<std::string>();
    else if(node.is_boolean()) value=*node.value<bool>()?"true":"false";
    else if(node.is_integer()) value=std::to_string(*node.value<int64_t>());
    else if(node.is_floating_point()) value=std::to_string(*node.value<double>());
    // CLI11 treats --name= as a missing value and consumes the next option.
    // SaveSettings writes intentional empty strings to TOML before Start; let
    // the SDK load those values there instead (Auto IPs and unbound keys).
    if(node.is_string() && value.empty())continue;
    args.push_back(L"--"+Wide(name)+L"="+Wide(value));
  }
  // FSR 1 is spatial: upscale the native 720p image to the selected host
  // output rather than silently raising the guest display mode to match it.
  // Keep an explicit custom guest mode from the user's TOML if present.
  if(s.effect=="fsr" || s.effect=="fsr2" || s.effect=="fsr3" || s.aspect!="16:9") {
    if(!s.guest_width)args.push_back(L"--video_mode_width="+std::to_wstring(Get<int64_t>(config,"video_mode_width",1280)));
    if(!s.guest_height)args.push_back(L"--video_mode_height="+std::to_wstring(Get<int64_t>(config,"video_mode_height",720)));
  }
  // These paths are resolved before SDK TOML loading. Forward existing custom
  // roots rather than accidentally changing a user's save/cache location.
  for (const auto* key : {"user_data_root", "cache_root", "update_data_root", "metadata_root"}) {
    const auto value = Get(config, key, std::string{});
    if (!value.empty()) args.push_back(L"--" + Wide(key) + L"=" + Wide(value));
  }
  return args;
}
inline HANDLE Start(const fs::path& exe, const std::vector<std::wstring>& arguments,bool monitored=false) {
  const auto program = exe / (monitored?"motogp0910-crash-monitor.exe":"motogp0910.exe");
  if(monitored && !fs::is_regular_file(program))throw Error(
      L"Crash monitor is missing. Extract the complete new Windows ZIP.",
      L"Çökme izleyicisi eksik. Yeni Windows ZIP’ini tamamen çıkar.");
  std::wstring command = Quote(program.wstring());
  for (const auto& arg : arguments) command += L" " + Quote(arg);
  STARTUPINFOW startup{sizeof(startup)};
  PROCESS_INFORMATION process{};
  if (!CreateProcessW(program.c_str(), command.data(), nullptr, nullptr, FALSE,
      0, nullptr, exe.c_str(), &startup, &process)) throw Error(
          L"Could not start motogp0910.exe. Check the package and folder permissions.",
          L"motogp0910.exe başlatılamadı. Paketi ve klasör izinlerini kontrol et.");
  CloseHandle(process.hThread);
  return process.hProcess;
}
}  // namespace launcher
