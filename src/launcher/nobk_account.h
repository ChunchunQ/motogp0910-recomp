#pragma once
// NOBK Online site account for Online mode: sign-in through the site's launcher
// API (the same one the FIFA 09 launcher uses), a DPAPI-protected launcher token
// for the current Windows user, and the link of this PC's online profile to the
// account on the MotoGP community server. The password is never stored.
#include "core.h"
#include "../online/client.h"
#include "../../third_party/nlohmann/json.hpp"
#include <winhttp.h>
#include <wincrypt.h>
#include <chrono>

namespace launcher {
inline constexpr const wchar_t* kAccountHost = L"play.nobkonline.com";
inline constexpr const wchar_t* kAccountPath = L"/api/account/launcher";
inline constexpr const wchar_t* kAccountRegister = L"https://nobkonline.com/register/";
inline constexpr const wchar_t* kAccountForgotPassword = L"https://nobkonline.com/sifremi-unuttum/";
inline constexpr const wchar_t* kAccountMyAccount = L"https://nobkonline.com/hesabim/";

struct NobkAccount {std::string username, token, country;};
inline std::string AccountCountry(std::string value) {
  // FIFA's launcher receives an optional country code in the account response.
  // Do not infer it from Windows language, IP address or the player's name.
  if(value.size()!=2)return {};
  for(auto& ch:value){if(ch>='a'&&ch<='z')ch-=32;if(ch<'A'||ch>'Z')return {};}
  return value;
}
// The site answered and refused the sign-in (wrong password, expired token, banned).
struct SignInRejected : Error {using Error::Error;};

inline fs::path AccountTokenPath() {
  wchar_t local[32768]{};
  const auto n = GetEnvironmentVariableW(L"LOCALAPPDATA", local, 32768);
  if (!n || n >= 32768) throw Error(L"Cannot locate the Windows user data folder.", L"Windows kullanıcı veri klasörü bulunamadı.");
  return fs::path(local) / L"MotoGP0910" / L"Online" / L"nobk-account.bin";
}

// The optional third line adds country without invalidating old two-line files.
// All three fields stay DPAPI-protected for this Windows user.
inline void SaveAccount(const NobkAccount& account) {
  std::string plain = account.username + "\n" + account.token + "\n" + AccountCountry(account.country);
  DATA_BLOB input{static_cast<DWORD>(plain.size()), reinterpret_cast<BYTE*>(plain.data())}, output{};
  const bool ok = CryptProtectData(&input, L"NOBK Online launcher sign-in", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output);
  SecureZeroMemory(plain.data(), plain.size());
  if (!ok) throw Error(L"Could not protect the sign-in for this Windows user.", L"Oturum bilgisi bu Windows kullanıcısı için şifrelenemedi.");
  const auto path = AccountTokenPath();
  std::error_code error;
  fs::create_directories(path.parent_path(), error);
  auto temporary = path; temporary += L".tmp";
  {
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(output.pbData), output.cbData);
    LocalFree(output.pbData);
    if (!file) throw Error(L"Could not save the sign-in.", L"Oturum bilgisi kaydedilemedi.");
  }
  if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    fs::remove(temporary, error);
    throw Error(L"Could not save the sign-in.", L"Oturum bilgisi kaydedilemedi.");
  }
  SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
}

inline std::optional<NobkAccount> LoadAccount() {
  try {
    std::ifstream file(AccountTokenPath(), std::ios::binary);
    if (!file) return std::nullopt;
    std::array<char, 4097> data{};
    file.read(data.data(), static_cast<std::streamsize>(data.size()));
    const auto size = file.gcount();
    if (size <= 0 || size > 4096) return std::nullopt;
    DATA_BLOB input{static_cast<DWORD>(size), reinterpret_cast<BYTE*>(data.data())}, output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) return std::nullopt;
    std::string plain(reinterpret_cast<char*>(output.pbData), output.cbData);
    SecureZeroMemory(output.pbData, output.cbData); LocalFree(output.pbData);
    const auto split = plain.find('\n');
    if (split == std::string::npos || !split || split + 1 >= plain.size()) return std::nullopt;
    const auto country_split=plain.find('\n',split+1);
    NobkAccount result{plain.substr(0,split),plain.substr(split+1,country_split==std::string::npos?std::string::npos:country_split-split-1),
        country_split==std::string::npos?std::string{}:AccountCountry(plain.substr(country_split+1))};
    SecureZeroMemory(plain.data(),plain.size());
    if(result.token.empty())return std::nullopt;
    return result;
  } catch (const std::exception&) { return std::nullopt; }
}

inline void ForgetAccount() {
  std::error_code error;
  try { fs::remove(AccountTokenPath(), error); } catch (const std::exception&) {}
}

namespace detail {
struct WinHttpHandle {
  HINTERNET value = nullptr;
  ~WinHttpHandle() { if (value) WinHttpCloseHandle(value); }
  operator HINTERNET() const { return value; }
};
}

// POST JSON to the site's launcher sign-in API; returns account name + fresh token.
// Errors from the site (wrong password, expired token, banned) come back in the
// selected language and are shown as they are.
inline NobkAccount SiteSignIn(const nlohmann::json& request, bool turkish) {
  using detail::WinHttpHandle;
  const auto fail = [](const std::wstring& detail) {
    return Error(L"Could not reach NOBK Online. Check your internet connection. " + detail,
                 L"NOBK Online'a ulaşılamadı. İnternet bağlantını kontrol et. " + detail);
  };
  // Same proxy as the browser / Windows settings (Windows 8.1+), falling back to WinHTTP's own setting.
  // Generous timeouts: slow ISP DNS or a first TLS handshake must not fail the sign-in (12002).
  WinHttpHandle session{WinHttpOpen(L"MotoGP0910-Launcher/1", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
  if (!session.value) session.value = WinHttpOpen(L"MotoGP0910-Launcher/1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session.value || !WinHttpSetTimeouts(session, 0, 20000, 30000, 30000)) throw fail(L"(init)");
  WinHttpHandle connection{WinHttpConnect(session, kAccountHost, INTERNET_DEFAULT_HTTPS_PORT, 0)};
  WinHttpHandle http{connection.value ? WinHttpOpenRequest(connection, L"POST", kAccountPath, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr};
  if (!http.value) throw fail(L"(open)");
  DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
  WinHttpSetOption(http, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect));
  const std::wstring headers = std::wstring(L"Content-Type: application/json\r\nX-Requested-With: nobk\r\nAccept-Language: ") +
                               (turkish ? L"tr" : L"en") + L"\r\n";
  std::string body = request.dump();
  const bool sent = WinHttpSendRequest(http, headers.c_str(), static_cast<DWORD>(headers.size()), body.data(),
                                       static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0) &&
                    WinHttpReceiveResponse(http, nullptr);
  SecureZeroMemory(body.data(), body.size());
  if (!sent) {
    const auto code = GetLastError();
    if (code == ERROR_WINHTTP_TIMEOUT || code == ERROR_WINHTTP_CANNOT_CONNECT)
      throw Error(L"Could not reach NOBK Online in time (" + std::to_wstring(code) + L"). Check that a firewall, antivirus or VPN does not block the launcher, then try again.",
                  L"NOBK Online bağlantısı zaman aşımına uğradı (" + std::to_wstring(code) + L"). Güvenlik duvarı, antivirüs veya VPN'in Launcher'ı engellemediğini kontrol et ve yeniden dene.");
    throw fail(L"(" + std::to_wstring(code) + L")");
  }
  DWORD status = 0, length = sizeof(status);
  WinHttpQueryHeaders(http, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &length, nullptr);
  std::string reply;
  std::array<char, 4096> chunk{};
  DWORD count = 0;
  while (WinHttpReadData(http, chunk.data(), static_cast<DWORD>(chunk.size()), &count) && count) {
    reply.append(chunk.data(), count);
    if (reply.size() > 65536) throw fail(L"(size)");
  }
  nlohmann::json data = nlohmann::json::parse(reply, nullptr, false);
  if (status != 200) {
    std::string message = data.is_object() && data.contains("error") && data["error"].is_string() ? data["error"].get<std::string>() : std::string{};
    if (message.empty()) throw fail(L"(HTTP " + std::to_wstring(status) + L")");
    throw SignInRejected(Wide(message), Wide(message));
  }
  if (!data.is_object() || !data.contains("gamertag") || !data["gamertag"].is_string() || !data.contains("token") || !data["token"].is_string())
    throw fail(L"(answer)");
  auto username=data["gamertag"].get<std::string>(),token=data["token"].get<std::string>();
  if(username.empty()||token.empty()||username.find('\n')!=std::string::npos||token.find('\n')!=std::string::npos)throw fail(L"(answer)");
  const auto country=data.find("country");
  return NobkAccount{std::move(username),std::move(token),country!=data.end()&&country->is_string()?AccountCountry(country->get<std::string>()):std::string{}};
}

inline NobkAccount SignIn(const std::string& username, const std::string& password, bool turkish, bool remember) {
  if (username.empty() || password.empty())
    throw Error(L"Enter your NOBK Online username and password.", L"NOBK Online kullanıcı adını ve şifreni gir.");
  auto account = SiteSignIn({{"username", username}, {"password", password}}, turkish);
  // Same lifetime as FIFA: unchecked keeps the token only in this process.
  if(remember)SaveAccount(account);else ForgetAccount();
  return account;
}

// Before each Online launch: refresh the 90-day token (sliding), then link this
// PC's game profile to the account on the community server. Returns the name
// the game must use.
inline std::string PrepareOnline(std::optional<NobkAccount>& session, bool remember, const std::string& server, bool turkish) {
  if (!session || session->token.empty()) throw Error(L"Sign in with your NOBK Online account on the Online page first.",
                          L"Önce Online sekmesinden NOBK Online hesabınla giriş yap.");
  NobkAccount account;
  try { account = SiteSignIn({{"token", session->token}}, turkish); }
  catch (const SignInRejected&) { session.reset();ForgetAccount();throw; }
  if(remember)SaveAccount(account);else ForgetAccount();
  session=std::move(account);
  try {
    return motogp::online::LinkAccount(server, session->token);
  } catch (const std::exception& e) {
    throw Error(L"The MotoGP server could not link your account: " + Wide(e.what()),
                L"MotoGP sunucusu hesabını bağlayamadı: " + Wide(e.what()));
  }
}
}  // namespace launcher
