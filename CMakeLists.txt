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
    src/button_prompts.cpp
    src/ps3_prompt_textures.cpp
    src/physics_assets.cpp
    src/livery_readback.cpp
)

if(WIN32)
    add_executable(motogp0910 WIN32 ${MOTOGP0910_SOURCES})
    target_link_libraries(motogp0910 PRIVATE xinput)
    add_executable(motogp0910-launcher WIN32 src/launcher/main.cpp)
    target_compile_definitions(motogp0910-launcher PRIVATE UNICODE _UNICODE)
    target_include_directories(motogp0910-launcher SYSTEM PRIVATE
        $<TARGET_PROPERTY:rex::runtime,INTERFACE_INCLUDE_DIRECTORIES>)
    target_link_libraries(motogp0910-launcher PRIVATE user32 shell32 ole32 comctl32 bcrypt uuid dxgi gdiplus gdi32 dwmapi uxtheme)
    add_executable(launcher_layout_probe EXCLUDE_FROM_ALL tools/launcher_layout_probe.cpp)
    target_compile_definitions(launcher_layout_probe PRIVATE UNICODE _UNICODE)
    target_include_directories(launcher_layout_probe SYSTEM PRIVATE
        $<TARGET_PROPERTY:rex::runtime,INTERFACE_INCLUDE_DIRECTORIES>)
    target_link_libraries(launcher_layout_probe PRIVATE user32 shell32 ole32 comctl32 bcrypt uuid dxgi gdiplus gdi32 dwmapi uxtheme)
    rexglue_apply_target_settings(launcher_layout_probe)
    add_executable(launcher_probe EXCLUDE_FROM_ALL tools/launcher_probe.cpp)
    target_link_libraries(launcher_probe PRIVATE rex::runtime shell32 bcrypt)
    add_executable(launcher_runtime_probe EXCLUDE_FROM_ALL tools/launcher_runtime_probe.cpp)
    target_link_libraries(launcher_runtime_probe PRIVATE rex::runtime)
    target_compile_definitions(launcher_runtime_probe PRIVATE REX_HAS_FIDELITYFX_SDK=1)
    target_compile_options(launcher_runtime_probe PRIVATE $<$<CXX_COMPILER_ID:Clang>:-mssse3>)
    add_executable(launcher_ui_probe EXCLUDE_FROM_ALL tools/launcher_ui_probe.cpp)
    target_link_libraries(launcher_ui_probe PRIVATE user32)
else()
    add_executable(motogp0910 ${MOTOGP0910_SOURCES})
endif()

rexglue_setup_target(motogp0910 GPU_PLUGINS xenos)

add_executable(button_prompt_input_probe EXCLUDE_FROM_ALL tools/button_prompt_input_probe.cpp)
target_link_libraries(button_prompt_input_probe PRIVATE rex::runtime)
rexglue_apply_target_settings(button_prompt_input_probe)

add_executable(livery_readback_probe EXCLUDE_FROM_ALL tools/livery_readback_probe.cpp src/livery_readback.cpp)
target_link_libraries(livery_readback_probe PRIVATE rex::runtime ${CMAKE_DL_LIBS})
rexglue_apply_target_settings(livery_readback_probe)

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
    add_executable(physics_asset_probe EXCLUDE_FROM_ALL tools/physics_asset_probe.cpp src/physics_assets.cpp)
    target_include_directories(physics_asset_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(physics_asset_probe PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(physics_asset_probe)
    add_executable(button_prompt_window_probe EXCLUDE_FROM_ALL tools/button_prompt_window_probe.cpp src/ps3_prompt_textures.cpp)
    target_link_libraries(button_prompt_window_probe PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(button_prompt_window_probe)
    add_executable(button_prompt_gpu_probe EXCLUDE_FROM_ALL tools/button_prompt_gpu_probe.cpp src/ps3_prompt_textures.cpp src/physics_assets.cpp src/livery_readback.cpp)
    target_include_directories(button_prompt_gpu_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(button_prompt_gpu_probe PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(button_prompt_gpu_probe)
    rexglue_configure_target(button_prompt_gpu_probe GPU_PLUGINS xenos)
    if(WIN32)
        target_link_libraries(button_prompt_window_probe PRIVATE xinput)
        target_link_libraries(button_prompt_gpu_probe PRIVATE xinput)
    endif()
    add_executable(button_prompt_probe EXCLUDE_FROM_ALL
        tools/button_prompt_probe.cpp src/button_prompts.cpp src/ps3_prompt_textures.cpp)
    target_include_directories(button_prompt_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(button_prompt_probe PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(button_prompt_probe)
    if(WIN32)
        target_link_libraries(button_prompt_probe PRIVATE xinput)
    endif()
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

foreach(artwork_target motogp0910 button_prompt_probe button_prompt_window_probe button_prompt_gpu_probe)
    if(TARGET ${artwork_target})
        set_property(TARGET ${artwork_target} APPEND PROPERTY LINK_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/controller/ps3-prompts.bin")
        add_custom_command(TARGET ${artwork_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${artwork_target}>/resources/controller"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/controller/ps3-prompts.bin"
                "$<TARGET_FILE_DIR:${artwork_target}>/resources/controller/ps3-prompts.bin"
            VERBATIM)
    endif()
endforeach()

# Keep the runtime pair consistent even when building directly from an IDE.
# The official v0.10.0 code generator/import library remain pinned; the
# separately verified runtime enables that version's bundled spatial shaders.
if(WIN32)
    if(CMAKE_CROSSCOMPILING)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        foreach(stage_target motogp0910 motogp_headless_probe button_prompt_probe button_prompt_input_probe button_prompt_window_probe physics_asset_probe livery_readback_probe input_defaults_probe launcher_runtime_probe launcher_probe)
            if(TARGET ${stage_target})
                add_custom_command(TARGET ${stage_target} POST_BUILD
                    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/stage-spatial-runtime.py
                        $<TARGET_FILE_DIR:${stage_target}>
                    VERBATIM)
            endif()
        endforeach()
    else()
        find_program(MOTOGP_POWERSHELL NAMES powershell pwsh REQUIRED)
        foreach(stage_target motogp0910 motogp_headless_probe button_prompt_probe button_prompt_input_probe button_prompt_window_probe physics_asset_probe livery_readback_probe input_defaults_probe launcher_runtime_probe launcher_probe)
            if(TARGET ${stage_target})
                add_custom_command(TARGET ${stage_target} POST_BUILD
                    COMMAND ${MOTOGP_POWERSHELL} -NoProfile -ExecutionPolicy Bypass -File
                        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/stage-spatial-runtime.ps1
                        -Destination $<TARGET_FILE_DIR:${stage_target}>
                    VERBATIM)
            endif()
        endforeach()
    endif()
endif()

set(MOTOGP_LIVERY_GPU_PLUGIN "" CACHE FILEPATH "Linux v0.10.0 GPU plugin built with the career readback patch")
if(NOT WIN32 AND MOTOGP_LIVERY_GPU_PLUGIN)
    if(NOT EXISTS "${MOTOGP_LIVERY_GPU_PLUGIN}")
        message(FATAL_ERROR "MOTOGP_LIVERY_GPU_PLUGIN does not exist")
    endif()
    foreach(stage_target motogp0910 button_prompt_probe button_prompt_window_probe button_prompt_gpu_probe livery_readback_probe)
        if(TARGET ${stage_target})
            add_custom_command(TARGET ${stage_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MOTOGP_LIVERY_GPU_PLUGIN}" "$<TARGET_FILE_DIR:${stage_target}>/librexgpu-xenos.so"
                VERBATIM)
        endif()
    endforeach()
endif()
