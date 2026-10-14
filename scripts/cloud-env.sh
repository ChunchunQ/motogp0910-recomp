# Source from bash before configuring or building in the cloud workspace.
export REXGLUE_SDK=/workspace/shared/rexglue-nightly-packages/linux-amd64
export REXGLUE_SOURCE=/workspace/shared/rexglue-sdk-nightly
export REXGLUE_SYSROOT=/workspace/shared/sysroot
export PATH="$REXGLUE_SDK/bin:$REXGLUE_SYSROOT/usr/bin:$REXGLUE_SYSROOT/usr/lib/llvm-19/bin:$PATH"
export LD_LIBRARY_PATH="$REXGLUE_SDK/lib:$REXGLUE_SYSROOT/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export CMAKE_PREFIX_PATH="$REXGLUE_SDK${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
export CC="$REXGLUE_SYSROOT/usr/bin/clang-19"
export CXX="$REXGLUE_SYSROOT/usr/bin/clang++-19"
