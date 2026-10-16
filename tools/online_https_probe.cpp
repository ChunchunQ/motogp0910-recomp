// QA only: adds one disposable local test CA to CURRENT_USER ROOT, removes it
// on every exit. This executable and its trust operation are never packaged.
#include "online/client.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif
namespace {
unsigned checks=0;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);++checks;}
template<class F>bool Reject(F f){try{f();return false;}catch(const std::exception&){return true;}}
#ifdef _WIN32
struct TestTrust {
 HCERTSTORE store=nullptr;PCCERT_CONTEXT certificate=nullptr;
 void Remove(){if(certificate){const auto current=certificate;certificate=nullptr;Check(CertDeleteCertificateFromStore(current)!=FALSE,"Disposable test CA removed");}}
 ~TestTrust(){if(certificate)CertDeleteCertificateFromStore(certificate);if(store)CertCloseStore(store,0);}
 void Add(const std::filesystem::path& file){
  std::ifstream input(file,std::ios::binary);std::vector<uint8_t> der((std::istreambuf_iterator<char>(input)),{});
  if(der.empty() || der.size()>8192)throw std::runtime_error("Invalid disposable test CA");
  store=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,0,0,CERT_SYSTEM_STORE_CURRENT_USER,L"ROOT");
  if(!store || !CertAddEncodedCertificateToStore(store,X509_ASN_ENCODING,der.data(),static_cast<DWORD>(der.size()),CERT_STORE_ADD_NEW,&certificate))throw std::runtime_error("Cannot install disposable local test CA");
 }
};
#endif
}
int main(int argc,char** argv){try{
#ifdef _WIN32
 if(argc!=7 && argc!=8)throw std::runtime_error("Usage: online_https_probe GOOD_HTTPS WRONG_HOST REDIRECT_HTTPS TEST_CA_DER PROFILE_FOLDER BAD_CONTROL_HTTPS");
 const auto folder=std::filesystem::path(argv[5]);std::filesystem::create_directories(folder);
 if(argc==8){
  Check(Reject([&]{motogp::online::Client c(argv[1],"Untrusted",folder/"untrusted.profile");}),"Real Windows client rejects an untrusted certificate");
  std::cout<<checks<<" isolated untrusted certificate check passed\n";return 0;
 }
 TestTrust trust;trust.Add(argv[4]);
 {motogp::online::Client client(argv[1],"Trusted",folder/"trusted.profile");Check(client.Connection().peer!=0,"Real Windows client accepts explicitly trusted localhost TLS");}
 Check(Reject([&]{motogp::online::Client c(argv[2],"BadHost",folder/"host.profile");}),"Real Windows client rejects trusted certificate with wrong hostname");
 Check(Reject([&]{motogp::online::Client c(argv[3],"Redirect",folder/"redirect.profile");}),"Real Windows client refuses HTTPS redirect response");
 Check(Reject([&]{motogp::online::Client c(argv[6],"Broken",folder/"broken.profile");}),"Real Windows client rejects malformed TLS control response");
 trust.Remove();
 std::cout<<checks<<" actual Windows WinHTTP certificate/redirect checks passed\n";
 return 0;
#else
 (void)argc;(void)argv;throw std::runtime_error("Windows QA target only");
#endif
}catch(const std::exception& error){std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;}}
