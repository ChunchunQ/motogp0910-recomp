#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$project_dir"
source scripts/cloud-env.sh
[[ "$(rexglue --version)" == "0.10.0" ]]
mkdir -p out/smoke/asm out/smoke/bin out/smoke/generated
for name in instr_add instr_addic instr_fadd; do
  cp "$REXGLUE_SOURCE/tests/ppc/asm/$name.s" "out/smoke/asm/$name.s"
  "$REXGLUE_SOURCE/tools/binutils/powerpc-none-elf-as" \
    -a32 -be -mregnames -mpower7 -maltivec -mvsx -mvmx128 -R \
    -o "out/smoke/bin/$name.o" "out/smoke/asm/$name.s"
  "$REXGLUE_SOURCE/tools/binutils/powerpc-none-elf-ld" \
    -A powerpc:common32 -melf32ppc -EB -nostdlib --oformat=binary \
    -Ttext=0x82010000 -e 0x82010000 \
    -o "out/smoke/bin/$name.bin" "out/smoke/bin/$name.o"
  "$REXGLUE_SOURCE/tools/binutils/powerpc-none-elf-nm" --numeric-sort \
    "out/smoke/bin/$name.o" > "out/smoke/bin/$name.map"
done
rexglue recompile-tests --output out/smoke/generated \
  --bin-dir out/smoke/bin --asm-dir out/smoke/asm
cat > out/smoke/CMakeLists.txt <<'EOF'
cmake_minimum_required(VERSION 3.25)
project(ReXGlueSmoke LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
find_package(rexglue 0.10.0 EXACT REQUIRED CONFIG)
set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(/workspace/shared/rexglue-sdk/thirdparty/catch2 catch2)
add_executable(ppc_smoke generated/ppc_test_functions.cpp generated/ppc_test_cases.cpp
 /workspace/shared/rexglue-sdk/tests/unit/core/chrono_test.cpp)
target_include_directories(ppc_smoke PRIVATE generated)
target_compile_options(ppc_smoke PRIVATE -msse4.1 -mssse3 -ffp-model=strict)
target_link_libraries(ppc_smoke PRIVATE rex::runtime Catch2::Catch2WithMain)
EOF
cmake -S out/smoke -B out/smoke/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="$CXX"
cmake --build out/smoke/build --parallel 4
out/smoke/build/ppc_smoke
