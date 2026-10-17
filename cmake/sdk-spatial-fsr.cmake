# ReXGlue v0.10.0 already ships spatial FSR1/CAS shader bytecode and CPU
# constants. Enable that path only; no temporal backend or frame generation.
add_compile_definitions(REX_HAS_FIDELITYFX_SDK=1)
# ReXGlue's byte swapping and vector helpers require SSSE3 on x64.
# clang targeting the MSVC ABI otherwise defaults to baseline x86-64.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mssse3>)
endif()

# Readback is required only while the game builds CPU-copied career liveries.
# A C ABI backed by an atomic scope counter avoids changing user GPU flags.
if(PROJECT_NAME STREQUAL "ReXGlue")
  # SDK Release normally compiles counter instrumentation out. Keep the
  # lightweight counters (without Tracy) for the optional diagnostic overlay.
  add_compile_definitions(REXGLUE_ENABLE_PERF_COUNTERS=1)
  set(MOTOGP_GPU_DIAGNOSTICS_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../src/gpu_diagnostics_bridge.cpp")
  set(MOTOGP_LIVERY_BRIDGE_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../src/livery_readback_bridge.cpp")
  function(motogp_add_livery_bridge)
    target_sources(rexgpu-xenos PRIVATE "${MOTOGP_LIVERY_BRIDGE_SOURCE}" "${MOTOGP_GPU_DIAGNOSTICS_SOURCE}")
  endfunction()
  cmake_language(DEFER CALL motogp_add_livery_bridge)
endif()

# Keep input observers in the runtime's own SDL instance on Windows. Adding
# a C export preserves the existing v0.10.0 SDK/plugin C++ ABI and import lib.
if(WIN32 AND PROJECT_NAME STREQUAL "ReXGlue")
  set(MOTOGP_SHARED_SDL_BRIDGE_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../src/shared_sdl_bridge.cpp")
  function(motogp_add_shared_sdl_bridge)
    target_sources(rexruntime PRIVATE "${MOTOGP_SHARED_SDL_BRIDGE_SOURCE}")
  endfunction()
  cmake_language(DEFER CALL motogp_add_shared_sdl_bridge)
endif()

if(PROJECT_NAME STREQUAL "ReXGlue")
  set(MOTOGP_ASPECT_BRIDGE_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../src/presentation_aspect_bridge.cpp")
  function(motogp_add_aspect_bridge)
    target_sources(rexruntime PRIVATE "${MOTOGP_ASPECT_BRIDGE_SOURCE}")
  endfunction()
  cmake_language(DEFER CALL motogp_add_aspect_bridge)
endif()
