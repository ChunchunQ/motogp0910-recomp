#pragma once
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace launcher::updates {
struct Configuration {
  std::string current_version;
  // Same version endpoint contract as FIFA 09's Updater.VersionUrl.
  // The request appends ?v=major.minor.patch (or &v= after an existing query).
  std::wstring manifest_url;
};

// Approved launcher release version. Checks remain inactive until the NOBK
// service operator supplies a documented MotoGP-specific manifest endpoint.
inline const Configuration kConfiguration{"1.1.11", L""};

struct Release {
  std::string version, url, sha256;
  std::map<std::string, std::string> notes;
  std::string NotesFor(std::string_view language) const;
  bool CanInstall() const;
};

enum class CheckStatus { Disabled, UpToDate, Available, Failed, Cancelled };
struct CheckResult {
  CheckStatus status = CheckStatus::Disabled;
  std::optional<Release> release;
  std::string error;
};
struct StagedUpdate {
  Release release;
  std::filesystem::path directory, archive;
};
using Progress = std::function<void(uint64_t received, uint64_t total)>;

bool Configured(const Configuration& configuration);
std::wstring VersionRequestUrl(const Configuration& configuration);
bool IsNewer(std::string_view candidate, std::string_view installed);
Release ParseManifest(std::string_view json);

// Transport functions are synchronous; the launcher invokes them on workers.
// These transport functions alone do not display UI or modify installed files.
CheckResult CheckForUpdate(const Configuration& configuration = kConfiguration,
                           const std::atomic_bool* cancelled = nullptr);
StagedUpdate DownloadVerified(const Configuration& configuration, const Release& release,
                             const std::atomic_bool* cancelled = nullptr, Progress progress = {});
// Removes only the temporary directory returned by DownloadVerified.
void DiscardStaged(const StagedUpdate& staged);
bool GameRunning(const std::filesystem::path& root);
bool InstallationActive(const std::filesystem::path& root, bool allow_restart = true);
// The old launcher stays open until the helper completes its preflight.
class Installation {
 public:
  ~Installation();
  Installation(const Installation&) = delete;
  Installation& operator=(const Installation&) = delete;
 private:
  Installation() = default;
  void* helper_{};
  void* commit_{};
  void* cancel_{};
  bool committed_ = false;
  friend std::unique_ptr<Installation> PrepareInstallation(const StagedUpdate&,
      const std::filesystem::path&, const std::vector<std::filesystem::path>&,
      bool, const std::atomic_bool*);
  friend void CommitInstallation(Installation&);
};
std::unique_ptr<Installation> PrepareInstallation(const StagedUpdate& staged,
    const std::filesystem::path& root, const std::vector<std::filesystem::path>& protected_roots,
    bool turkish, const std::atomic_bool* cancelled = nullptr);
void CommitInstallation(Installation& installation);
// Called only after the restarted launcher's real window was created/painted.
void AcknowledgeRestart();
}  // namespace launcher::updates
