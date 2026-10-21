#include "updates.h"
#include "update_protocol.h"
#include "../../third_party/nlohmann/json.hpp"
#include <chrono>

namespace launcher::updates {
namespace {
using namespace detail;
std::wstring StageToken(const fs::path& directory){
  const auto name=directory.filename().wstring();const std::wstring prefix=L"MotoGP0910-update-";
  if(!name.starts_with(prefix)||!Token(std::wstring_view(name).substr(prefix.size())))throw std::runtime_error("Invalid update staging directory");
  return name.substr(prefix.size());
}
}
bool GameRunning(const std::filesystem::path& root){
  try{return detail::RunningGame(root);}catch(...){return true;}
}
bool InstallationActive(const std::filesystem::path& root,bool allow_restart){
  auto lock=detail::LockName(root);detail::Handle mutex(OpenMutexW(SYNCHRONIZE,FALSE,lock.c_str()));
  if(!mutex.value)return GetLastError()!=ERROR_FILE_NOT_FOUND;
  const auto token=detail::AckToken();
  if(allow_restart&&!token.empty()){
    detail::Handle acknowledgement(OpenEventW(EVENT_MODIFY_STATE,FALSE,detail::EventName(token,L"healthy").c_str()));
    detail::Handle permission(OpenEventW(SYNCHRONIZE,FALSE,detail::EventName(token,L"launch").c_str()));
    if(acknowledgement.value&&permission.value&&WaitForSingleObject(permission.value,0)==WAIT_OBJECT_0)return false;
  }
  return true;
}
Installation::~Installation(){
  if(cancel_&&!committed_)SetEvent(cancel_);
  if(helper_)CloseHandle(helper_);
  if(commit_)CloseHandle(commit_);
  if(cancel_)CloseHandle(cancel_);
}
std::unique_ptr<Installation> PrepareInstallation(const StagedUpdate& staged,
    const std::filesystem::path& root,const std::vector<std::filesystem::path>& protected_roots,
    bool turkish,const std::atomic_bool* cancelled){
  using namespace detail;
  // FIFA's NOBK protocol is unchanged. This helper adds a local rollback
  // fallback; it requires no extra server fields or endpoints.
  if(GameRunning(root))throw std::runtime_error("Close the game before updating");
  const auto token=StageToken(staged.directory);
  const auto helper_source=root/L"bin"/kHelper;
  if(!fs::is_regular_file(helper_source))throw std::runtime_error("Update helper is missing. Extract the complete package");
  // Recheck the archive before handing it to a separate process.
  if(FileHash(staged.archive)!=Utf8Text(Fold(WideText(staged.release.sha256))))throw std::runtime_error("Update archive changed after download");
  nlohmann::json job{{"root",Utf8Text(fs::canonical(root).wstring())},{"sha256",staged.release.sha256},
      {"parent",GetCurrentProcessId()},{"turkish",turkish},{"protected",nlohmann::json::array()}};
  FILETIME created{},exit{},kernel{},user{};
  if(!GetProcessTimes(GetCurrentProcess(),&created,&exit,&kernel,&user))throw std::runtime_error("Cannot identify current launcher");
  job["parent_created"]=(static_cast<uint64_t>(created.dwHighDateTime)<<32)|created.dwLowDateTime;
  for(const auto& path:protected_roots)job["protected"].push_back(Utf8Text(fs::weakly_canonical(path).wstring()));
  const auto job_path=staged.directory/L"install.json";
  {std::ofstream output(job_path,std::ios::binary);output<<job.dump();output.close();if(!output)throw std::runtime_error("Cannot prepare update job");}
  auto helper_path=staged.directory/kHelper;fs::copy_file(helper_source,helper_path,fs::copy_options::none);
  auto installation=std::unique_ptr<Installation>(new Installation);
  Handle ready(CreateEventW(nullptr,TRUE,FALSE,EventName(token,L"ready").c_str()));
  installation->commit_=CreateEventW(nullptr,TRUE,FALSE,EventName(token,L"commit").c_str());
  installation->cancel_=CreateEventW(nullptr,TRUE,FALSE,EventName(token,L"cancel").c_str());
  if(!ready.value||!installation->commit_||!installation->cancel_)throw std::runtime_error("Cannot initialize update handoff");
  auto helper=Spawn(helper_path,Quote(job_path.wstring()),staged.directory);
  installation->helper_=std::exchange(helper.value,nullptr);
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::minutes(3);
  HANDLE handles[]{ready.value,installation->helper_};
  for(;;){
    if(cancelled&&cancelled->load())throw std::runtime_error("Update cancelled");
    const auto result=WaitForMultipleObjects(2,handles,FALSE,100);
    if(result==WAIT_OBJECT_0)return installation;
    if(result==WAIT_OBJECT_0+1){
      std::ifstream input(staged.directory/L"error.txt");std::string error((std::istreambuf_iterator<char>(input)),{});
      throw std::runtime_error(error.empty()?"Update preparation failed":error);
    }
    if(result==WAIT_FAILED||std::chrono::steady_clock::now()>deadline)throw std::runtime_error("Update preparation timed out");
  }
}
void CommitInstallation(Installation& installation){
  if(!installation.helper_||WaitForSingleObject(installation.helper_,0)!=WAIT_TIMEOUT)throw std::runtime_error("Update helper closed before installation");
  if(!installation.commit_||!SetEvent(installation.commit_))throw std::runtime_error("Cannot authorize update installation");
  installation.committed_=true;
}
void AcknowledgeRestart(){
  const auto token=detail::AckToken();if(token.empty())return;
  detail::Handle event(OpenEventW(EVENT_MODIFY_STATE,FALSE,detail::EventName(token,L"healthy").c_str()));
  if(event.value)SetEvent(event.value);
}
}  // namespace launcher::updates
