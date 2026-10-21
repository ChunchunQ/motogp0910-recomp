// NOBK version/download contract adapted from ChunchunQ/fifa09-recomp
// launcher/Updater.cs (MIT). See third_party/fifa09-launcher-LICENSE.txt.
#include "updates.h"
#include "update_protocol.h"
#include "../../third_party/nlohmann/json.hpp"
#include <winhttp.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace launcher::updates {
namespace {
constexpr size_t kManifestLimit = 128 * 1024;
constexpr uint64_t kDownloadLimit = 512ull * 1024 * 1024;
constexpr wchar_t kStagingPrefix[] = L"MotoGP0910-update-";
using Clock = std::chrono::steady_clock;

struct Cancelled : std::runtime_error { Cancelled() : std::runtime_error("Update cancelled") {} };
struct InternetHandle {
  HINTERNET value{};
  ~InternetHandle() { if (value) WinHttpCloseHandle(value); }
  InternetHandle() = default;
  InternetHandle(const InternetHandle&) = delete;
  InternetHandle& operator=(const InternetHandle&) = delete;
};

std::array<uint32_t, 3> Version(std::string_view text) {
  // Match System.Version + Normalize: 2-4 numeric components; an absent build
  // becomes zero and revision does not participate in update comparison.
  std::array<uint32_t, 4> parts{};
  size_t count = 0;
  for (;;) {
    if (count == parts.size()) throw std::runtime_error("Update version has too many components");
    const auto dot = text.find('.');
    auto part = text.substr(0, dot);
    const auto first = part.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) throw std::runtime_error("Invalid numeric update version");
    part = part.substr(first, part.find_last_not_of(" \t\r\n") - first + 1);
    if (part.front() == '+') part.remove_prefix(1);
    if (part.empty()) throw std::runtime_error("Invalid numeric update version");
    auto [end, error] = std::from_chars(part.data(), part.data() + part.size(), parts[count]);
    if (error != std::errc{} || end != part.data() + part.size() || parts[count] > 0x7fffffff)
      throw std::runtime_error("Invalid numeric update version");
    ++count;
    if (dot == std::string_view::npos) break;
    text.remove_prefix(dot + 1);
  }
  if (count < 2) throw std::runtime_error("Update version has too few components");
  return {parts[0], parts[1], parts[2]};
}

std::wstring CurrentText(std::string_view text) {
  const auto version = Version(text);
  return std::to_wstring(version[0]) + L"." + std::to_wstring(version[1]) + L"." + std::to_wstring(version[2]);
}

struct Url { std::wstring host, target, path; INTERNET_PORT port{}; };
Url HttpsUrl(std::wstring_view text) {
  if (text.empty() || text.size() > 8192 || text.find(L'\0') != std::wstring_view::npos ||
      text.find(L'\r') != std::wstring_view::npos || text.find(L'\n') != std::wstring_view::npos)
    throw std::runtime_error("Invalid update URL");
  URL_COMPONENTS parts{}; parts.dwStructSize = sizeof(parts);
  parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwExtraInfoLength =
      parts.dwUserNameLength = parts.dwPasswordLength = static_cast<DWORD>(-1);
  if (!WinHttpCrackUrl(text.data(), static_cast<DWORD>(text.size()), 0, &parts) ||
      parts.nScheme != INTERNET_SCHEME_HTTPS || !parts.dwHostNameLength ||
      parts.dwUserNameLength || parts.dwPasswordLength)
    throw std::runtime_error("Updates require HTTPS without embedded credentials");
  Url url;
  url.host.assign(parts.lpszHostName, parts.dwHostNameLength);
  if (parts.dwUrlPathLength) url.path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
  url.target = url.path.empty() ? L"/" : url.path;
  if (parts.dwExtraInfoLength) {
    std::wstring_view extra(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    if (extra.find(L'#') != std::wstring_view::npos) throw std::runtime_error("Update URL must not contain a fragment");
    url.target += extra;
  }
  url.port = parts.nPort;
  return url;
}

std::string Digest(std::string_view value) {
  if (value.size() != 64) throw std::runtime_error("Update SHA-256 must contain 64 hexadecimal characters");
  std::string result(value);
  for (auto& c : result) {
    if (c >= 'A' && c <= 'F') c = static_cast<char>(c - 'A' + 'a');
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
      throw std::runtime_error("Invalid update SHA-256");
  }
  return result;
}

void ValidatePackage(const Release& release) {
  if (!release.CanInstall()) throw std::runtime_error("Update does not provide an HTTPS ZIP and checksum");
  Version(release.version);
  Digest(release.sha256);
  HttpsUrl(detail::WideText(release.url));
}

class Request {
  InternetHandle session_, connection_, request_;
  const std::atomic_bool* cancelled_;
  Clock::time_point deadline_;
 public:
  uint64_t length = 0;
  void Check() const {
    if (cancelled_ && cancelled_->load()) throw Cancelled();
    if (Clock::now() > deadline_) throw std::runtime_error("Update request timed out");
  }
  Request(std::wstring_view address, const Configuration& configuration, bool package, const std::atomic_bool* cancelled)
      : cancelled_(cancelled), deadline_(Clock::now() + (package ? std::chrono::seconds(600) : std::chrono::seconds(8))) {
    Check();
    const auto url = HttpsUrl(address);
    const auto agent = L"NOBK-Launcher/" + CurrentText(configuration.current_version);
    session_.value = WinHttpOpen(agent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session_.value) session_.value = WinHttpOpen(agent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session_.value || !WinHttpSetTimeouts(session_.value, 2000, 3000, 3000, 3000))
      throw std::runtime_error("Could not initialize update connection");
    connection_.value = WinHttpConnect(session_.value, url.host.c_str(), url.port, 0);
    if (!connection_.value) throw std::runtime_error("Could not connect to update service");
    request_.value = WinHttpOpenRequest(connection_.value, L"GET", url.target.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    if (!request_.value || !WinHttpSetOption(request_.value, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect)))
      throw std::runtime_error("Could not create update request");
    const auto* headers = package ? L"Accept: application/zip\r\n" : L"Accept: application/json\r\n";
    if (!WinHttpSendRequest(request_.value, headers, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request_.value, nullptr)) {
      Check();
      throw std::runtime_error("Could not reach update service");
    }
    Check();
    DWORD status{}, size = sizeof(status);
    if (!WinHttpQueryHeaders(request_.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
          nullptr, &status, &size, nullptr) || status != 200)
      throw std::runtime_error("Update service did not return HTTP 200");
    DWORD content_length{}; size = sizeof(content_length);
    if (WinHttpQueryHeaders(request_.value, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
          nullptr, &content_length, &size, nullptr)) length = content_length;
  }
  DWORD Read(std::array<char, 32768>& buffer) {
    Check(); DWORD count{};
    if (!WinHttpReadData(request_.value, buffer.data(), static_cast<DWORD>(buffer.size()), &count)) {
      Check(); throw std::runtime_error("Update download was interrupted");
    }
    Check(); return count;
  }
};

std::filesystem::path TemporaryDirectory() {
  std::array<wchar_t, 32768> buffer{};
  const auto count = GetTempPathW(static_cast<DWORD>(buffer.size()), buffer.data());
  if (!count || count >= buffer.size()) throw std::runtime_error("Cannot locate update staging directory");
  for (int attempt = 0; attempt < 8; ++attempt) {
    std::array<unsigned char, 16> bytes{};
    if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
      throw std::runtime_error("Could not create unique update staging name");
    std::wstring name(kStagingPrefix);
    for (auto byte : bytes) { name += L"0123456789abcdef"[byte >> 4]; name += L"0123456789abcdef"[byte & 15]; }
    auto directory = std::filesystem::path(buffer.data()) / name;
    if (std::filesystem::create_directory(directory)) return directory;
  }
  throw std::runtime_error("Could not create update staging directory");
}
}  // namespace

std::string Release::NotesFor(std::string_view language) const {
  auto found = notes.find(std::string(language));
  if (found == notes.end()) found = notes.find("en");
  return found == notes.end() ? std::string{} : found->second;
}

bool Release::CanInstall() const {
  auto equal_ascii = [](std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
      auto c = a[i]; if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
      if (c != b[i]) return false;
    }
    return true;
  };
  return url.size() >= 12 && equal_ascii(std::string_view(url).substr(0, 8), "https://") &&
      equal_ascii(std::string_view(url).substr(url.size() - 4), ".zip") && sha256.size() == 64;
}

bool Configured(const Configuration& configuration) {
  return !configuration.manifest_url.empty() && !configuration.current_version.empty() &&
      configuration.current_version != "0.0.0";
}

bool IsNewer(std::string_view candidate, std::string_view installed) { return Version(candidate) > Version(installed); }

std::wstring VersionRequestUrl(const Configuration& configuration) {
  if (!Configured(configuration)) return {};
  HttpsUrl(configuration.manifest_url);
  auto address = configuration.manifest_url;
  address += address.find(L'?') == std::wstring::npos ? L"?v=" :
      address.ends_with(L'?') || address.ends_with(L'&') ? L"v=" : L"&v=";
  return address + CurrentText(configuration.current_version);
}

Release ParseManifest(std::string_view json) {
  if (json.size() > kManifestLimit) throw std::runtime_error("Update manifest exceeds the size limit");
  const auto document = nlohmann::json::parse(json, nullptr, false);
  if (!document.is_object()) throw std::runtime_error("Invalid update manifest JSON");
  Release result;
  auto text = [&](const char* key, size_t limit) {
    const auto found = document.find(key);
    if (found == document.end() || found->is_null()) return std::string{};
    if (!found->is_string()) throw std::runtime_error("Invalid update manifest field type");
    auto value = found->get<std::string>();
    if (value.size() > limit) throw std::runtime_error("Invalid update manifest field length");
    return value;
  };
  result.version = text("version", 64); result.url = text("url", 8192); result.sha256 = text("sha256", 128);
  if (const auto found = document.find("notes"); found != document.end()) {
    if (!found->is_null()) {
      if (!found->is_object()) throw std::runtime_error("Update notes must be a language dictionary");
      for (auto note = found->begin(); note != found->end(); ++note) {
        if (!note->is_string() || note->get_ref<const std::string&>().size() > 4096 || note.key().size() > 32)
          throw std::runtime_error("Invalid update release notes");
        result.notes.emplace(note.key(), note->get<std::string>());
      }
    }
  }
  Version(result.version);
  return result;
}

CheckResult CheckForUpdate(const Configuration& configuration, const std::atomic_bool* cancelled) {
  if (!Configured(configuration)) return {};
  try {
    const auto address = VersionRequestUrl(configuration);
    Request request(address, configuration, false, cancelled);
    if (request.length > kManifestLimit) throw std::runtime_error("Update manifest exceeds the size limit");
    std::string body; std::array<char, 32768> buffer{};
    while (const auto count = request.Read(buffer)) {
      if (body.size() + count > kManifestLimit) throw std::runtime_error("Update manifest exceeds the size limit");
      body.append(buffer.data(), count);
    }
    auto release = ParseManifest(body);
    if (!IsNewer(release.version, configuration.current_version)) return {CheckStatus::UpToDate, {}, {}};
    return {CheckStatus::Available, std::move(release), {}};
  } catch (const Cancelled&) { return {CheckStatus::Cancelled, {}, {}}; }
    catch (const std::exception& error) { return {CheckStatus::Failed, {}, error.what()}; }
}

StagedUpdate DownloadVerified(const Configuration& configuration, const Release& release,
                             const std::atomic_bool* cancelled, Progress progress) {
  if (!Configured(configuration)) throw std::runtime_error("Update service is not configured");
  ValidatePackage(release);
  if (!IsNewer(release.version, configuration.current_version)) throw std::runtime_error("Update is not newer than this build");
  Request request(detail::WideText(release.url), configuration, true, cancelled);
  if (request.length > kDownloadLimit) throw std::runtime_error("Update package exceeds the size limit");
  StagedUpdate staged{release, TemporaryDirectory(), {}};
  try {
    staged.archive = staged.directory / L"update.zip";
    std::ofstream file(staged.archive, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot create temporary update archive");
    std::array<char, 32768> buffer{}; uint64_t received{};
    while (const auto count = request.Read(buffer)) {
      if (received + count > kDownloadLimit) throw std::runtime_error("Update package exceeds the size limit");
      file.write(buffer.data(), count);
      if (!file) throw std::runtime_error("Could not write temporary update archive");
      received += count;
      if (progress) progress(received, request.length);
    }
    file.close();
    if (!file || !received || (request.length && received != request.length)) throw std::runtime_error("Incomplete update package");
    request.Check();
    if (detail::FileHash(staged.archive) != Digest(release.sha256)) throw std::runtime_error("Update package SHA-256 mismatch");
    request.Check();
    std::ifstream input(staged.archive, std::ios::binary); std::array<unsigned char, 4> signature{};
    input.read(reinterpret_cast<char*>(signature.data()), static_cast<std::streamsize>(signature.size()));
    if (!input || signature != std::array<unsigned char, 4>{'P', 'K', 3, 4}) throw std::runtime_error("Update package is not a ZIP archive");
    return staged;
  } catch (...) { DiscardStaged(staged); throw; }
}

void DiscardStaged(const StagedUpdate& staged) {
  if (staged.directory.empty()) return;
  // Limit cleanup to our direct, randomly named staging child of Windows TEMP.
  std::array<wchar_t, 32768> buffer{};
  const auto count = GetTempPathW(static_cast<DWORD>(buffer.size()), buffer.data());
  const auto name = staged.directory.filename().wstring();
  if (!count || count >= buffer.size() || !name.starts_with(kStagingPrefix) ||
      name.size() != std::wstring_view(kStagingPrefix).size() + 32 ||
      staged.directory.parent_path().lexically_normal() !=
          (std::filesystem::path(buffer.data()) / L"update.zip").parent_path().lexically_normal() ||
      !std::all_of(name.end() - 32, name.end(), [](wchar_t c) { return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f'); })) return;
  std::error_code error;
  std::filesystem::remove_all(staged.directory, error);
}
}  // namespace launcher::updates
