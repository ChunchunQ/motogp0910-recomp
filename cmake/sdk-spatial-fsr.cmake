# ReXGlue v0.10.0 already ships spatial FSR1/CAS shader bytecode and CPU
# constants. Enable that path only; no temporal backend or frame generation.
add_compile_definitions(REX_HAS_FIDELITYFX_SDK=1)
# ReXGlue's byte swapping and vector helpers require SSSE3 on x64.
# clang targeting the MSVC ABI otherwise defaults to baseline x86-64.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mssse3>)
endif()
