# Keep v0.10.0 plus the project's existing spatial/input/livery patches.
include("${CMAKE_CURRENT_LIST_DIR}/sdk-spatial-fsr.cmake")
if(WIN32 AND PROJECT_NAME STREQUAL "ReXGlue")
  set(MOTOGP_FFX_ROOT "${CMAKE_CURRENT_LIST_DIR}/../third_party/fidelityfx")
  set(MOTOGP_FFX_BRIDGE "${CMAKE_CURRENT_LIST_DIR}/../src/fsr_provider_bridge.cpp")
  function(motogp_add_temporal_fsr)
    add_library(motogp_fidelityfx_dx12 SHARED IMPORTED GLOBAL)
    set_target_properties(motogp_fidelityfx_dx12 PROPERTIES
      IMPORTED_LOCATION "${MOTOGP_FFX_ROOT}/amd_fidelityfx_dx12.dll"
      IMPORTED_IMPLIB "${MOTOGP_FFX_ROOT}/amd_fidelityfx_dx12.lib")
    target_include_directories(rexui PRIVATE "${MOTOGP_FFX_ROOT}/include")
    target_compile_definitions(rexui PRIVATE REX_HAS_FIDELITYFX_RUNTIME=1)
    target_link_libraries(rexui PRIVATE motogp_fidelityfx_dx12)
    target_sources(rexruntime PRIVATE "${MOTOGP_FFX_BRIDGE}")
    target_include_directories(rexruntime PRIVATE "${MOTOGP_FFX_ROOT}/include")
    target_link_libraries(rexruntime PRIVATE motogp_fidelityfx_dx12)
  endfunction()
  cmake_language(DEFER CALL motogp_add_temporal_fsr)
endif()
