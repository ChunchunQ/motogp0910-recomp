# motogp0910 - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(motogp0910 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REXSDK_VERSION "0.10.0" CACHE STRING "Required ReXGlue SDK version")
include(generated/rexglue.cmake)

# Sources
set(MOTOGP0910_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(motogp0910 WIN32 ${MOTOGP0910_SOURCES})
    add_executable(motogp0910-launcher WIN32 src/launcher/main.cpp)
    target_compile_definitions(motogp0910-launcher PRIVATE UNICODE _UNICODE)
    target_include_directories(motogp0910-launcher SYSTEM PRIVATE
        $<TARGET_PROPERTY:rex::runtime,INTERFACE_INCLUDE_DIRECTORIES>)
    target_link_libraries(motogp0910-launcher PRIVATE user32 shell32 ole32 comctl32 bcrypt uuid)
    add_executable(launcher_probe EXCLUDE_FROM_ALL tools/launcher_probe.cpp)
    target_link_libraries(launcher_probe PRIVATE rex::runtime shell32 bcrypt)
    add_executable(launcher_ui_probe EXCLUDE_FROM_ALL tools/launcher_ui_probe.cpp)
    target_link_libraries(launcher_ui_probe PRIVATE user32)
else()
    add_executable(motogp0910 ${MOTOGP0910_SOURCES})
endif()

rexglue_setup_target(motogp0910 GPU_PLUGINS xenos)

# Optional check of direct-launch input defaults against the actual SDK.
add_executable(input_defaults_probe EXCLUDE_FROM_ALL tools/input_defaults_probe.cpp)
target_link_libraries(input_defaults_probe PRIVATE rex::runtime)
rexglue_apply_target_settings(input_defaults_probe)
if(WIN32)
    add_custom_command(TARGET input_defaults_probe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:rex::runtime> $<TARGET_FILE_DIR:input_defaults_probe>)
endif()

# Optional integration check: real generated game code + patched XEX data,
# without initializing presentation or claiming that gameplay works.
if(TARGET motogp0910_recomp)
    add_executable(motogp_headless_probe EXCLUDE_FROM_ALL tools/headless_probe.cpp)
    target_include_directories(motogp_headless_probe PRIVATE
        ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(motogp_headless_probe PRIVATE
        motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(motogp_headless_probe)
    if(WIN32)
        add_custom_command(TARGET motogp_headless_probe POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:rex::runtime> $<TARGET_FILE_DIR:motogp_headless_probe>)
    endif()
endif()
