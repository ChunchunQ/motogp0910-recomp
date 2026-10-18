# motogp0910 - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(motogp0910 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/sdk-version.json" MOTOGP_SDK_METADATA)
string(JSON MOTOGP_SDK_VERSION GET "${MOTOGP_SDK_METADATA}" version)
string(JSON MOTOGP_SDK_CMAKE_VERSION GET "${MOTOGP_SDK_METADATA}" cmake_version)
set(REXSDK_VERSION "${MOTOGP_SDK_CMAKE_VERSION}" CACHE STRING "Required ReXGlue SDK version")
include(generated/rexglue.cmake)
if(NOT REXGLUE_VERSION_STRING STREQUAL MOTOGP_SDK_VERSION)
    message(FATAL_ERROR "SDK must match ${MOTOGP_SDK_VERSION}; found ${REXGLUE_VERSION_STRING}")
endif()
option(MOTOGP_STAGE_RUNTIME "Stage the verified matching Windows runtime" ON)
add_library(motogp_config STATIC src/config_file.cpp)
target_include_directories(motogp_config SYSTEM PRIVATE $<TARGET_PROPERTY:rex::runtime,INTERFACE_INCLUDE_DIRECTORIES>)
add_executable(config_file_probe EXCLUDE_FROM_ALL tools/config_file_probe.cpp)
target_link_libraries(config_file_probe PRIVATE motogp_config rex::runtime)
rexglue_apply_target_settings(config_file_probe)

# Native LAN protocol/service, separate from guest memory and GPU presentation.
add_library(motogp_lan STATIC src/lan/protocol.cpp src/lan/node.cpp src/online/relay.cpp src/online/client.cpp)
target_include_directories(motogp_lan PUBLIC src)
if(WIN32)
    target_link_libraries(motogp_lan PUBLIC ws2_32 iphlpapi bcrypt winhttp crypt32)
else()
    find_package(Threads REQUIRED)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(motogp_lan PUBLIC Threads::Threads OpenSSL::Crypto)
endif()
add_executable(lan_service_probe EXCLUDE_FROM_ALL tools/lan_service_probe.cpp)
target_link_libraries(lan_service_probe PRIVATE motogp_lan)
add_executable(online_service_probe EXCLUDE_FROM_ALL tools/online_service_probe.cpp)
target_link_libraries(online_service_probe PRIVATE motogp_lan)
add_executable(relay_crypto_probe EXCLUDE_FROM_ALL tools/relay_crypto_probe.cpp)
target_link_libraries(relay_crypto_probe PRIVATE motogp_lan)
if(WIN32)
    add_executable(online_https_probe EXCLUDE_FROM_ALL tools/online_https_probe.cpp)
    target_link_libraries(online_https_probe PRIVATE motogp_lan)
endif()
add_library(motogp_lan_guest STATIC src/lan/guest_bridge.cpp src/lan/socket_bridge.cpp src/lan/metadata.cpp src/online/stats.cpp)
target_link_libraries(motogp_lan_guest PUBLIC motogp_lan rex::runtime)
rexglue_apply_target_settings(motogp_lan_guest)

add_library(motogp_achievements STATIC src/achievement_ui.cpp src/achievement_sound.cpp)
target_link_libraries(motogp_achievements PUBLIC rex::runtime motogp_config)
rexglue_apply_target_settings(motogp_achievements)
if(WIN32)
    target_link_libraries(motogp_achievements PUBLIC winmm)
endif()
add_executable(achievement_ui_probe EXCLUDE_FROM_ALL tools/achievement_ui_probe.cpp)
target_link_libraries(achievement_ui_probe PRIVATE motogp_achievements)
rexglue_apply_target_settings(achievement_ui_probe)

add_library(motogp_adaptive STATIC src/adaptive_triggers.cpp)
target_link_libraries(motogp_adaptive PUBLIC rex::runtime)
rexglue_apply_target_settings(motogp_adaptive)
if(WIN32)
    add_executable(adaptive_triggers_probe EXCLUDE_FROM_ALL tools/adaptive_triggers_probe.cpp)
    target_link_libraries(adaptive_triggers_probe PRIVATE motogp_adaptive bcrypt)
    rexglue_apply_target_settings(adaptive_triggers_probe)
endif()

add_library(motogp_settings STATIC src/in_game_settings.cpp)
target_compile_definitions(motogp_settings PUBLIC REX_HAS_FIDELITYFX_SDK=1)
target_link_libraries(motogp_settings PUBLIC rex::runtime motogp_adaptive motogp_config)
rexglue_apply_target_settings(motogp_settings)

# Text overlays are separate from the original game and its voice selection.
add_library(motogp_localization STATIC src/localization.cpp)
target_link_libraries(motogp_localization PUBLIC rex::runtime)
rexglue_apply_target_settings(motogp_localization)

# Sources
set(MOTOGP0910_SOURCES
    src/main.cpp
    src/debug_overlay.cpp src/diagnostic_options.cpp
    src/lan/runtime.cpp
    src/button_prompts.cpp src/control_diagram.cpp
    src/ps3_prompt_textures.cpp
    src/physics_assets.cpp
    src/livery_readback.cpp
    src/game_language.cpp
    src/options_menu.cpp
    src/frame_pacing.cpp
    src/display_settings.cpp
    src/engine_settings.cpp
    src/ultrawide.cpp
    src/quit_menu.cpp
    src/in_game_settings_input.cpp
)

add_executable(gpu_diagnostics_bridge_probe EXCLUDE_FROM_ALL tools/gpu_diagnostics_bridge_probe.cpp src/gpu_diagnostics_bridge.cpp)
target_compile_definitions(gpu_diagnostics_bridge_probe PRIVATE REXGLUE_ENABLE_PERF_COUNTERS=1)
target_link_libraries(gpu_diagnostics_bridge_probe PRIVATE rex::runtime)
rexglue_apply_target_settings(gpu_diagnostics_bridge_probe)

add_executable(debug_overlay_probe EXCLUDE_FROM_ALL tools/debug_overlay_probe.cpp src/debug_overlay.cpp src/diagnostic_options.cpp src/frame_pacing.cpp)
target_link_libraries(debug_overlay_probe PRIVATE rex::runtime)
rexglue_apply_target_settings(debug_overlay_probe)

add_executable(nightly_input_probe EXCLUDE_FROM_ALL tools/nightly_input_probe.cpp)
target_link_libraries(nightly_input_probe PRIVATE rex::runtime)
rexglue_apply_target_settings(nightly_input_probe)

if(WIN32)
    enable_language(RC)
    function(motogp_embed_icon target artwork)
        set(MOTOGP_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/${artwork}.ico")
        configure_file(resources/icons/app.rc.in "${CMAKE_CURRENT_BINARY_DIR}/${target}-icon.rc" @ONLY)
        target_sources(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${target}-icon.rc")
        set_property(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${target}-icon.rc" APPEND PROPERTY
            OBJECT_DEPENDS "${MOTOGP_ICON_FILE}")
    endfunction()
    add_library(motogp_crash_report STATIC src/crash_report.cpp)
    target_link_libraries(motogp_crash_report PUBLIC dbghelp bcrypt)
    add_executable(crash_report_probe EXCLUDE_FROM_ALL tools/crash_report_probe.cpp)
    target_include_directories(crash_report_probe SYSTEM PRIVATE $<TARGET_PROPERTY:rex::runtime,INTERFACE_INCLUDE_DIRECTORIES>)
    target_link_libraries(crash_report_probe PRIVATE motogp_crash_report shell32)
    add_executable(motogp0910-crash-monitor WIN32 src/crash_monitor.cpp)
    target_include_directories(motogp0910-crash-monitor SYSTEM PRIVATE $<TARGET_PROPERTY:rex::runtime,INTERFACE_INCLUDE_DIRECTORIES>)
    target_link_libraries(motogp0910-crash-monitor PRIVATE motogp_crash_report shell32)
    add_executable(motogp0910 WIN32 ${MOTOGP0910_SOURCES})
    target_link_libraries(motogp0910 PRIVATE motogp_crash_report)
    target_link_options(motogp0910 PRIVATE "LINKER:/MAP:${CMAKE_CURRENT_BINARY_DIR}/motogp0910.map")
    motogp_embed_icon(motogp0910 game)
    target_link_libraries(motogp0910 PRIVATE xinput)
    add_library(motogp_launcher_achievements STATIC src/launcher/achievements.cpp)
    target_link_libraries(motogp_launcher_achievements PUBLIC rex::runtime motogp_config shell32 bcrypt ole32 uuid)
    rexglue_apply_target_settings(motogp_launcher_achievements)
    add_library(motogp0910-launcher-ui SHARED src/launcher/main.cpp)
    set_target_properties(motogp0910-launcher-ui PROPERTIES OUTPUT_NAME "motogp0910-ui" PREFIX "")
    target_compile_definitions(motogp0910-launcher-ui PRIVATE MOTOGP_LAUNCHER_DLL=1)
    add_executable(motogp0910-launcher WIN32 src/launcher/bootstrap.cpp)
    set_target_properties(motogp0910-launcher PROPERTIES OUTPUT_NAME "MotoGP09-10Launcher")
    motogp_embed_icon(motogp0910-launcher launcher)
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(motogp0910-launcher PRIVATE /GS- /Zl /Oi-)
    else()
        target_compile_options(motogp0910-launcher PRIVATE -fno-stack-protector -fno-builtin -fno-exceptions -fno-rtti)
    endif()
    target_link_options(motogp0910-launcher PRIVATE "LINKER:/NODEFAULTLIB" "LINKER:/ENTRY:LauncherEntry")
    target_link_libraries(motogp0910-launcher PRIVATE kernel32 user32)
    add_dependencies(motogp0910-launcher motogp0910-launcher-ui)
    add_executable(windows_icon_probe EXCLUDE_FROM_ALL tools/windows_icon_probe.cpp)
    target_link_libraries(windows_icon_probe PRIVATE user32 gdi32)
    target_compile_definitions(motogp0910-launcher-ui PRIVATE UNICODE _UNICODE)
    target_include_directories(motogp0910-launcher-ui SYSTEM PRIVATE
        $<TARGET_PROPERTY:rex::runtime,INTERFACE_INCLUDE_DIRECTORIES>)
    target_link_libraries(motogp0910-launcher-ui PRIVATE motogp_launcher_achievements user32 shell32 ole32 comctl32 bcrypt uuid dxgi gdiplus gdi32 dwmapi uxtheme shlwapi)
    add_executable(launcher_layout_probe EXCLUDE_FROM_ALL tools/launcher_layout_probe.cpp)
    motogp_embed_icon(launcher_layout_probe launcher)
    target_compile_definitions(launcher_layout_probe PRIVATE UNICODE _UNICODE)
    target_include_directories(launcher_layout_probe SYSTEM PRIVATE
        $<TARGET_PROPERTY:rex::runtime,INTERFACE_INCLUDE_DIRECTORIES>)
    target_link_libraries(launcher_layout_probe PRIVATE motogp_launcher_achievements user32 shell32 ole32 comctl32 bcrypt uuid dxgi gdiplus gdi32 dwmapi uxtheme shlwapi)
    rexglue_apply_target_settings(launcher_layout_probe)
    configure_file(resources/launcher/assets.rc.in "${CMAKE_CURRENT_BINARY_DIR}/launcher-assets.rc" @ONLY)
    set_property(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/launcher-assets.rc" APPEND PROPERTY OBJECT_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/launcher/background.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/verification/game-files.sha256"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/launcher/third-party-notices.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/launcher-fonts/OpenSans-Regular.ttf"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/launcher-fonts/OpenSans-SemiBold.ttf"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/launcher-fonts/OpenSans-Bold.ttf")
    foreach(launcher_asset_target motogp0910-launcher launcher_layout_probe)
        target_sources(${launcher_asset_target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/launcher-assets.rc")
    endforeach()
    add_executable(launcher_probe EXCLUDE_FROM_ALL tools/launcher_probe.cpp src/diagnostic_options.cpp src/frame_pacing.cpp src/display_settings.cpp)
    add_library(launcher_entry_probe SHARED EXCLUDE_FROM_ALL tools/launcher_entry_probe.cpp)
    set_target_properties(launcher_entry_probe PROPERTIES PREFIX "")
    target_link_libraries(launcher_entry_probe PRIVATE shell32)
    add_dependencies(launcher_probe motogp0910-launcher launcher_entry_probe)
    target_link_libraries(launcher_probe PRIVATE rex::runtime motogp_config shell32 bcrypt)
    add_executable(fsr_provider_probe EXCLUDE_FROM_ALL tools/fsr_provider_probe.cpp)
    target_link_libraries(fsr_provider_probe PRIVATE kernel32)
    add_executable(display_settings_probe EXCLUDE_FROM_ALL tools/display_settings_probe.cpp src/frame_pacing.cpp src/display_settings.cpp)
    target_link_libraries(display_settings_probe PRIVATE rex::runtime user32)
    rexglue_apply_target_settings(display_settings_probe)
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
add_custom_command(TARGET motogp0910 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:motogp0910>/resources/ui"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/open-sans/OpenSans-Semibold.ttf"
        "$<TARGET_FILE_DIR:motogp0910>/resources/ui/OpenSans-Semibold.ttf")
target_link_libraries(motogp0910 PRIVATE motogp_lan_guest motogp_achievements motogp_settings)

add_executable(button_prompt_input_probe EXCLUDE_FROM_ALL tools/button_prompt_input_probe.cpp)
target_link_libraries(button_prompt_input_probe PRIVATE rex::runtime)
rexglue_apply_target_settings(button_prompt_input_probe)

add_executable(livery_readback_probe EXCLUDE_FROM_ALL tools/livery_readback_probe.cpp src/livery_readback.cpp)
target_link_libraries(livery_readback_probe PRIVATE rex::runtime ${CMAKE_DL_LIBS})
rexglue_apply_target_settings(livery_readback_probe)

# CPU-only validation of the exact patched texture parser. Requires the pinned
# SDK source tree used by scripts/build-spatial-sdk.py; no GPU device is opened.
set(MOTOGP_SDK_VALIDATION_SOURCE "" CACHE PATH "Pinned patched SDK sources for texture-key regression checks")
if(MOTOGP_SDK_VALIDATION_SOURCE)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(MOTOGP_TEXTURE_PARSER_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/texture-parser-check.cpp")
    add_custom_command(OUTPUT "${MOTOGP_TEXTURE_PARSER_SOURCE}"
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/scripts/extract-texture-parser-check.py"
            --source "${MOTOGP_SDK_VALIDATION_SOURCE}/src/graphics/pipeline/texture/cache.cpp"
            --output "${MOTOGP_TEXTURE_PARSER_SOURCE}"
        DEPENDS "${MOTOGP_SDK_VALIDATION_SOURCE}/src/graphics/pipeline/texture/cache.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/extract-texture-parser-check.py" VERBATIM)
    add_executable(texture_binding_probe EXCLUDE_FROM_ALL tools/texture_binding_probe.cpp
        "${MOTOGP_TEXTURE_PARSER_SOURCE}"
        "${MOTOGP_SDK_VALIDATION_SOURCE}/src/graphics/pipeline/texture/util.cpp"
        "${MOTOGP_SDK_VALIDATION_SOURCE}/src/graphics/flags.cpp")
    target_include_directories(texture_binding_probe SYSTEM PRIVATE "${MOTOGP_SDK_VALIDATION_SOURCE}/include" "${MOTOGP_SDK_VALIDATION_SOURCE}/thirdparty/renderdoc")
    target_link_libraries(texture_binding_probe PRIVATE rex::runtime)
    rexglue_apply_target_settings(texture_binding_probe)
    if(MSVC)
        target_compile_options(texture_binding_probe PRIVATE /Gy /Gw)
    else()
        target_compile_options(texture_binding_probe PRIVATE -ffunction-sections -fdata-sections)
    endif()
    if(WIN32)
        target_link_options(texture_binding_probe PRIVATE "LINKER:/OPT:REF")
    else()
        target_link_options(texture_binding_probe PRIVATE -Wl,--gc-sections)
    endif()
endif()

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
    if(WIN32)
        add_executable(in_game_settings_probe EXCLUDE_FROM_ALL tools/in_game_settings_probe.cpp src/in_game_settings_input.cpp src/frame_pacing.cpp)
        target_include_directories(in_game_settings_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
        target_link_libraries(in_game_settings_probe PRIVATE motogp0910_recomp motogp_settings bcrypt)
        rexglue_apply_target_settings(in_game_settings_probe)
    endif()
    add_executable(localization_probe EXCLUDE_FROM_ALL tools/localization_probe.cpp src/options_menu.cpp src/game_language.cpp)
    target_include_directories(localization_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(localization_probe PRIVATE motogp0910_recomp motogp_localization)
    rexglue_apply_target_settings(localization_probe)
    add_executable(localization_asset_tool EXCLUDE_FROM_ALL tools/localization_asset_tool.cpp)
    target_include_directories(localization_asset_tool PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(localization_asset_tool PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(localization_asset_tool)
    add_executable(quit_menu_probe EXCLUDE_FROM_ALL tools/quit_menu_probe.cpp src/quit_menu.cpp src/diagnostic_options.cpp)
    target_include_directories(quit_menu_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(quit_menu_probe PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(quit_menu_probe)
    add_executable(lan_import_probe EXCLUDE_FROM_ALL tools/lan_import_probe.cpp src/lan/runtime.cpp)
    target_include_directories(lan_import_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(lan_import_probe PRIVATE motogp0910_recomp motogp_lan_guest)
    rexglue_apply_target_settings(lan_import_probe)
    add_executable(online_guest_probe EXCLUDE_FROM_ALL tools/online_guest_probe.cpp src/lan/runtime.cpp)
    target_include_directories(online_guest_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(online_guest_probe PRIVATE motogp0910_recomp motogp_lan_guest)
    rexglue_apply_target_settings(online_guest_probe)
    add_executable(lan_guest_probe EXCLUDE_FROM_ALL tools/lan_guest_probe.cpp)
    target_include_directories(lan_guest_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(lan_guest_probe PRIVATE motogp0910_recomp motogp_lan_guest)
    rexglue_apply_target_settings(lan_guest_probe)
    add_executable(file_compat_probe EXCLUDE_FROM_ALL tools/file_compat_probe.cpp)
    target_include_directories(file_compat_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(file_compat_probe PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(file_compat_probe)
    add_executable(runtime_stability_probe EXCLUDE_FROM_ALL tools/runtime_stability_probe.cpp)
    target_include_directories(runtime_stability_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(runtime_stability_probe PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(runtime_stability_probe)
    add_executable(game_language_probe EXCLUDE_FROM_ALL tools/game_language_probe.cpp src/game_language.cpp)
    target_include_directories(game_language_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(game_language_probe PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(game_language_probe)
    add_executable(engine_settings_probe EXCLUDE_FROM_ALL tools/engine_settings_probe.cpp src/engine_settings.cpp src/ultrawide.cpp)
    target_include_directories(engine_settings_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(engine_settings_probe PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(engine_settings_probe)
    add_executable(physics_asset_probe EXCLUDE_FROM_ALL tools/physics_asset_probe.cpp src/physics_assets.cpp)
    target_include_directories(physics_asset_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(physics_asset_probe PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(physics_asset_probe)
    add_executable(button_prompt_window_probe EXCLUDE_FROM_ALL tools/button_prompt_window_probe.cpp src/ultrawide.cpp src/control_diagram.cpp src/ps3_prompt_textures.cpp src/frame_pacing.cpp)
    target_link_libraries(button_prompt_window_probe PRIVATE motogp0910_recomp rex::runtime)
    rexglue_apply_target_settings(button_prompt_window_probe)
    add_executable(button_prompt_gpu_probe EXCLUDE_FROM_ALL tools/button_prompt_gpu_probe.cpp src/ultrawide.cpp src/control_diagram.cpp src/ps3_prompt_textures.cpp src/physics_assets.cpp src/livery_readback.cpp src/frame_pacing.cpp src/display_settings.cpp src/lan/runtime.cpp src/quit_menu.cpp src/diagnostic_options.cpp src/debug_overlay.cpp)
    target_include_directories(button_prompt_gpu_probe PRIVATE ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_sources(button_prompt_gpu_probe PRIVATE src/in_game_settings_input.cpp)
    target_link_libraries(button_prompt_gpu_probe PRIVATE motogp0910_recomp rex::runtime motogp_lan_guest motogp_achievements motogp_settings)
    rexglue_apply_target_settings(button_prompt_gpu_probe)
    rexglue_configure_target(button_prompt_gpu_probe GPU_PLUGINS xenos)
    if(WIN32)
        target_link_libraries(button_prompt_window_probe PRIVATE xinput)
        set_target_properties(button_prompt_gpu_probe PROPERTIES WIN32_EXECUTABLE TRUE)
        target_link_libraries(button_prompt_gpu_probe PRIVATE xinput motogp_crash_report)
    endif()
    add_executable(button_prompt_probe EXCLUDE_FROM_ALL
        tools/button_prompt_probe.cpp src/ultrawide.cpp src/button_prompts.cpp src/control_diagram.cpp src/ps3_prompt_textures.cpp src/frame_pacing.cpp)
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
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/controller/ps3-prompts.bin"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/controller/ps3-controls.bin")
        add_custom_command(TARGET ${artwork_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${artwork_target}>/resources/controller"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/controller/ps3-prompts.bin"
                "$<TARGET_FILE_DIR:${artwork_target}>/resources/controller/ps3-prompts.bin"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/controller/ps3-controls.bin"
                "$<TARGET_FILE_DIR:${artwork_target}>/resources/controller/ps3-controls.bin"
            VERBATIM)
    endif()
endforeach()

# Keep the runtime pair consistent even when building directly from an IDE.
# The exact SDK code generator/import library remain pinned by sdk-version.json; the
# separately verified runtime enables bundled spatial shaders and experimental AMD providers.
if(WIN32 AND MOTOGP_STAGE_RUNTIME)
    if(CMAKE_CROSSCOMPILING)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        foreach(stage_target file_compat_probe online_guest_probe localization_probe localization_asset_tool adaptive_triggers_probe in_game_settings_probe quit_menu_probe launcher_layout_probe lan_guest_probe lan_import_probe runtime_stability_probe motogp0910 motogp_headless_probe button_prompt_probe button_prompt_input_probe button_prompt_window_probe button_prompt_gpu_probe physics_asset_probe game_language_probe engine_settings_probe livery_readback_probe input_defaults_probe nightly_input_probe launcher_runtime_probe fsr_provider_probe launcher_probe display_settings_probe)
            if(TARGET ${stage_target})
                add_custom_command(TARGET ${stage_target} POST_BUILD
                    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/stage-spatial-runtime.py
                        $<TARGET_FILE_DIR:${stage_target}>
                    VERBATIM)
            endif()
        endforeach()
    else()
        find_program(MOTOGP_POWERSHELL NAMES powershell pwsh REQUIRED)
        foreach(stage_target file_compat_probe online_guest_probe localization_probe localization_asset_tool adaptive_triggers_probe in_game_settings_probe quit_menu_probe launcher_layout_probe lan_guest_probe lan_import_probe runtime_stability_probe motogp0910 motogp_headless_probe button_prompt_probe button_prompt_input_probe button_prompt_window_probe button_prompt_gpu_probe physics_asset_probe game_language_probe engine_settings_probe livery_readback_probe input_defaults_probe nightly_input_probe launcher_runtime_probe fsr_provider_probe launcher_probe display_settings_probe)
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

set(MOTOGP_LIVERY_GPU_PLUGIN "" CACHE FILEPATH "Matching Linux GPU plugin built with the career readback patch")
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

set(MOTOGP_RUNTIME_LIBRARY "" CACHE FILEPATH "Matching custom Linux runtime library")
if(NOT WIN32 AND MOTOGP_RUNTIME_LIBRARY)
    foreach(stage_target motogp0910 engine_settings_probe button_prompt_probe launcher_runtime_probe)
        if(TARGET ${stage_target})
            add_custom_command(TARGET ${stage_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MOTOGP_RUNTIME_LIBRARY}"
                    "$<TARGET_FILE_DIR:${stage_target}>/librexruntime.so" VERBATIM)
        endif()
    endforeach()
endif()

# Build the CPU/UI suite explicitly; probes stay out of ordinary release builds.
if(WIN32 AND TARGET motogp0910_recomp)
    add_custom_target(windows_validation_probes DEPENDS
        motogp0910 localization_probe runtime_stability_probe nightly_input_probe fsr_provider_probe
        engine_settings_probe game_language_probe physics_asset_probe
        button_prompt_probe livery_readback_probe motogp_headless_probe
        launcher_runtime_probe display_settings_probe button_prompt_input_probe
        launcher_probe launcher_layout_probe input_defaults_probe achievement_ui_probe quit_menu_probe in_game_settings_probe adaptive_triggers_probe)
endif()

foreach(locale_target motogp0910 quit_menu_probe button_prompt_probe button_prompt_gpu_probe game_language_probe)
    if(TARGET ${locale_target})
        target_link_libraries(${locale_target} PRIVATE motogp_localization)
    endif()
endforeach()
foreach(locale_target motogp0910 motogp0910-launcher localization_probe)
    if(TARGET ${locale_target})
        file(GLOB_RECURSE locale_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/localization/tr/overlay/*")
        set_property(TARGET ${locale_target} APPEND PROPERTY LINK_DEPENDS ${locale_files})
        add_custom_command(TARGET ${locale_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/localization/tr/overlay"
                "$<TARGET_FILE_DIR:${locale_target}>/resources/localization/tr/overlay"
            VERBATIM)
    endif()
endforeach()
