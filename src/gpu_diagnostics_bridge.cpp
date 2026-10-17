// Compiled into the pinned GPU plugin. Read only; no engine flags or counters
// are changed. A versioned C boundary avoids crossing private C++ DLL state.
#include <rex/system/gpu_plugin.h>
#include <rex/perf/counter.h>
#include <cstdint>
extern "C" REX_GPU_PLUGIN_EXPORT uint32_t Motogp_ReadGpuCounters(uint32_t version,int64_t* values,uint32_t count){
 if(version!=1||!values||count!=static_cast<uint32_t>(rex::perf::CounterId::kCount))return 0;
#ifdef REXGLUE_ENABLE_PERF_COUNTERS
 for(uint32_t i=0;i<count;++i)values[i]=rex::perf::GetSnapshotCounter(static_cast<rex::perf::CounterId>(i));
 return 1;
#else
 for(uint32_t i=0;i<count;++i)values[i]=0;
 return 0;
#endif
}
