cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(silicate VERSION 1.0.0)

set(ARC_FEATURE_FULL OFF CACHE BOOL "" FORCE)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp

    src/bot/bot.cpp
    src/bot/updater.cpp
    src/bot/scheduler.cpp

    src/cbf/cbf.cpp
    src/cbf/engine.cpp
    src/cbf/sidecar.cpp

    src/assist/autoclicker.cpp
    src/assist/hitboxes.cpp
    src/assist/hitzone_geometry.cpp
    src/assist/hitzone_renderer.cpp
    src/assist/hitzones.cpp
    src/assist/cps.cpp

    src/hooks/CCActionManager.cpp
    src/hooks/CCDirector.cpp
    src/hooks/CCEGLView.cpp
    src/hooks/CCScheduler.cpp
    src/hooks/EnhancedGameObject.cpp
    src/hooks/FMODAudioEngine.cpp
    src/hooks/GJBaseGameLayer.cpp
    src/hooks/GameObject.cpp
    src/hooks/LevelEditorLayer.cpp
    src/hooks/PauseLayer.cpp
    src/hooks/PlayLayer.cpp
    src/hooks/PlayerObject.cpp
    src/hooks/RandTriggerGameObject.cpp
    src/hooks/ShaderLayer.cpp
    src/hooks/VideoOptionsLayer.cpp

    src/ui/manager.cpp
    src/ui/hook.cpp
    src/ui/theme.cpp
    src/ui/widgets/info_modal.cpp
    src/ui/widgets/tooltip.cpp
    src/ui/widgets/yes_no_modal.cpp

    src/replay/replay.cpp
    src/replay/system.cpp

    src/physics/collisions.cpp
    src/physics/gjbasegamelayer.cpp
    src/physics/gravity.cpp
    src/physics/object.cpp
    src/physics/player.cpp

    src/label/label.cpp

    src/checkpoint/fix.cpp
    src/checkpoint/checkpoint.cpp

    src/trajectory/trajectory.cpp

    src/render/renderer.cpp
    src/render/texture.cpp
    src/render/dsp.cpp
    src/render/pass.cpp

    src/util/midhook.cpp
    src/hooks/GJEffectManager.cpp
)

# /W4 is an MSVC-only flag. The Windows cross-build uses clang++ with an
# MSVC target, which treats an unguarded /W4 as an input file.
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
target_include_directories(${PROJECT_NAME} PRIVATE src include)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

CPMAddPackage("gh:silicate-bot/tabby#legacy-v1")
set(TABBY_USE_WIN32 ON)
set(TABBY_USE_OPENGL3 ON)
add_compile_definitions(TABBY_USE_WIN32)
add_compile_definitions(TABBY_USE_OPENGL3)
target_link_libraries(${PROJECT_NAME} tabby)

# Tabby's drag.cpp has an extra qualification on a member definition. It is a
# harmless warning, but Clang reports it on every build.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(tabby PRIVATE -Wno-extra-qualification)
endif()

CPMAddPackage("gh:stephenberry/glaze#dce5e0f")
target_link_libraries(${PROJECT_NAME} glaze::glaze)

CPMAddPackage("gh:zyantific/zydis#bffbb61")
target_link_libraries(${PROJECT_NAME} Zydis)

CPMAddPackage("gh:cursey/safetyhook#2f28386")
target_link_libraries(${PROJECT_NAME} safetyhook)

CPMAddPackage("https://git.puppy.lgbt/silicate/slc.git#main")

# libslc exports -Werror to every consumer. Geode's SDK headers intentionally
# use GNU/MSVC-compatibility constructs that Clang warns about, so keeping
# -Werror here turns third-party header noise into build failures.
get_target_property(SILICATE_LIBSLC_OPTIONS libslc INTERFACE_COMPILE_OPTIONS)
if (SILICATE_LIBSLC_OPTIONS)
    list(REMOVE_ITEM SILICATE_LIBSLC_OPTIONS "-Werror")
    set_target_properties(libslc PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${SILICATE_LIBSLC_OPTIONS}")
endif()

target_link_libraries(${PROJECT_NAME} libslc)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # These warnings come from Geode's public headers and generated bindings
    # rather than from this mod.
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wno-dollar-in-identifier-extension
        -Wno-ignored-qualifiers
        -Wno-overloaded-virtual
        -Wno-unused-parameter)
    target_compile_options(GeodeBindings PRIVATE
        -Wno-overloaded-virtual
        -Wno-unused-variable)
endif()

setup_geode_mod(${PROJECT_NAME})
