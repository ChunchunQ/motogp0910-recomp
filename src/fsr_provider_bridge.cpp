// Linked into the custom v0.10.0 runtime, using AMD's pinned FidelityFX API.
#include <ffx_api/ffx_api.h>
#include <ffx_api/ffx_upscale.h>
#include <array>
#include <cstdint>
#include <cstring>
extern "C" __declspec(dllexport) uint64_t Motogp_SelectFsrProvider(uint32_t major,void* device) {
  if(major!=2 && major!=3)return 0;
  uint64_t count=16;std::array<uint64_t,16> ids{};std::array<const char*,16> names{};
  ffxQueryDescGetVersions query{};query.header.type=FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
  query.createDescType=FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;query.device=device;
  query.outputCount=&count;query.versionIds=ids.data();query.versionNames=names.data();
  if(ffxQuery(nullptr,&query.header)!=FFX_API_RETURN_OK)return 0;
  for(size_t i=0;i<count && i<ids.size();++i)
    if(names[i] && names[i][0]==static_cast<char>('0'+major) && names[i][1]=='.')return ids[i];
  return 0;
}
