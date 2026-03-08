cmake_minimum_required(VERSION 3.28)

set(ENGINE_VERSION_FILE "${CMAKE_SOURCE_DIR}/version/VERSION.txt")
if(EXISTS "${ENGINE_VERSION_FILE}")
  file(READ "${ENGINE_VERSION_FILE}" ENGINE_PROJECT_VERSION_RAW)
  string(STRIP "${ENGINE_PROJECT_VERSION_RAW}" ENGINE_PROJECT_VERSION)
else()
  set(ENGINE_PROJECT_VERSION "0.2.0")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ENGINE_VERSION_FILE}")

project(EngineDemo VERSION ${ENGINE_PROJECT_VERSION} LANGUAGES CXX)

if(WIN32)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not supported. Use: cmake -S . -B build")
endif()

execute_process(
  COMMAND git -C "${CMAKE_SOURCE_DIR}" rev-parse --short=12 HEAD
  OUTPUT_VARIABLE ENGINE_GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(ENGINE_GIT_HASH STREQUAL "")
  set(ENGINE_GIT_HASH "nogit")
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated/engine")

configure_file(
  "${CMAKE_SOURCE_DIR}/include/engine/version.h.in"
  "${CMAKE_BINARY_DIR}/generated/engine/version.h"
  @ONLY
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENGINE_DETERMINISTIC_BUILD "Enable deterministic/reproducible compiler and linker flags" OFF)
option(HELLEngine_ENABLE_MODERN_RENDERER "Build experimental modern material/post renderer path" OFF)
if(ENGINE_DETERMINISTIC_BUILD)
  if(MSVC)
    add_compile_options(/Brepro)
    add_link_options(/Brepro)
  else()
    add_compile_options(-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.)
  endif()
endif()

include(FetchContent)

# Third-party dependency setup (single registration/materialization flow).
set(ENGINE_FETCHCONTENT_DEPENDENCIES)

function(engine_register_dependency dependency_name)
  cmake_parse_arguments(ENGINE_DEP "" "" "FETCH_ARGS" ${ARGN})
  if(NOT ENGINE_DEP_FETCH_ARGS)
    message(FATAL_ERROR "engine_register_dependency(${dependency_name}) requires FETCH_ARGS")
  endif()

  FetchContent_Declare(${dependency_name} ${ENGINE_DEP_FETCH_ARGS})
  list(APPEND ENGINE_FETCHCONTENT_DEPENDENCIES ${dependency_name})
  set(ENGINE_FETCHCONTENT_DEPENDENCIES "${ENGINE_FETCHCONTENT_DEPENDENCIES}" PARENT_SCOPE)
endfunction()

# SDL2 policy: prefer shared-only to avoid duplicate archive generation.
set(SDL_SHARED ON CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Build SDL static library" FORCE)
set(SDL_TEST OFF CACHE BOOL "Build SDL tests" FORCE)

engine_register_dependency(SDL2
  FETCH_ARGS
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.5
)

# Keep SDL_mixer self-contained in CI/dev environments without system codec dev packages.
set(SDL2MIXER_VENDORED ON CACHE BOOL "Use vendored dependencies for SDL2_mixer" FORCE)
set(SDL2MIXER_SAMPLES OFF CACHE BOOL "Disable SDL2_mixer sample binaries" FORCE)

engine_register_dependency(SDL2_mixer
  FETCH_ARGS
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG release-2.8.1
)

engine_register_dependency(imgui
  FETCH_ARGS
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.0
)

engine_register_dependency(nlohmann_json
  FETCH_ARGS
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

engine_register_dependency(Catch2
  FETCH_ARGS
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)

FetchContent_MakeAvailable(${ENGINE_FETCHCONTENT_DEPENDENCIES})
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

find_package(OpenGL REQUIRED)

if(TARGET SDL2::SDL2)
  set(ENGINEDEMO_SDL_TARGET SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
  set(ENGINEDEMO_SDL_TARGET SDL2::SDL2-static)
else()
  message(FATAL_ERROR "No SDL2 target found from fetched SDL2")
endif()

add_library(imgui_sdl2 STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

target_include_directories(imgui_sdl2
  PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui_sdl2 PUBLIC ${ENGINEDEMO_SDL_TARGET})

add_library(engine_core STATIC
  src/engine/archetypes.cpp
  src/engine/background_layers.cpp
  src/engine/audio_system.cpp
  src/engine/bullet_palette.cpp
  src/engine/bullet_sprite_gen.cpp
  src/engine/camera_shake.cpp
  src/engine/config.cpp
  src/engine/crash_handler.cpp
  src/engine/diagnostics.cpp
  src/engine/gameplay_session.cpp
  src/engine/gameplay_session_subsystems.cpp
  src/engine/content_pipeline.cpp
  src/engine/content_watcher.cpp
  src/engine/deterministic_rng.cpp
  src/engine/defensive_special.cpp
  src/engine/danger_field.cpp
  src/engine/difficulty_scaling.cpp
  src/engine/encounter_graph.cpp
  src/engine/editor_tools.cpp
  src/engine/editor/editor_tools_core.cpp
  src/engine/editor/editor_tools_services.cpp
  src/engine/editor/editor_tools_workspace_panel.cpp
  src/engine/editor/editor_tools_pattern_panel.cpp
  src/engine/entities.cpp
  src/engine/gpu_bullets.cpp
  src/engine/gl_bullet_renderer.cpp
  src/engine/gradient_animator.cpp
  src/engine/input_system.cpp
  src/engine/job_system.cpp
  src/engine/level_tile_gen.cpp
  src/engine/logging.cpp
  src/engine/memory.cpp
  src/engine/meta_progression.cpp
  src/engine/modern_renderer.cpp
  src/engine/perf_profiler.cpp
  src/engine/public_api.cpp
  src/engine/plugin_registry.cpp
  src/engine/pattern_generator.cpp
  src/engine/pattern_graph.cpp
  src/engine/pattern_signature.cpp
  src/engine/patterns.cpp
  src/engine/particle_fx.cpp
  src/engine/palette_fx_templates.cpp
  src/engine/persistence.cpp
  src/engine/palette_ramp.cpp
  src/engine/projectiles.cpp
  src/engine/replay.cpp
  src/engine/render_pipeline.cpp
  src/engine/render2d.cpp
  src/engine/shader_cache.cpp
  src/engine/sprite_atlas_gen.cpp
  src/engine/run_structure.cpp
  src/engine/runtime.cpp
  src/engine/standards.cpp
  src/engine/timing.cpp
  src/engine/traits.cpp
  src/engine/upgrade_ui_model.cpp
  src/stb_impl.cpp
  third_party/glad/src/glad.cpp
)

target_include_directories(engine_core
  PUBLIC
    include
    third_party/stb
    third_party/glad/include
    "${CMAKE_BINARY_DIR}/generated"
)

target_link_libraries(engine_core
  PUBLIC
    ${ENGINEDEMO_SDL_TARGET}
    SDL2_mixer::SDL2_mixer
    imgui_sdl2
    nlohmann_json::nlohmann_json
    OpenGL::GL
)

target_compile_definitions(engine_core PUBLIC HELLEngine_ENABLE_MODERN_RENDERER=$<BOOL:${HELLEngine_ENABLE_MODERN_RENDERER}>)

if(MSVC)
  target_compile_options(engine_core PRIVATE /fp:strict)
else()
  target_compile_options(engine_core PRIVATE -ffp-contract=off -fno-fast-math)
endif()


add_executable(ContentPacker
  src/tools_content_packer.cpp
  src/engine/content_pipeline.cpp
  src/engine/content_watcher.cpp
  src/engine/palette_fx_templates.cpp
)

target_include_directories(ContentPacker PRIVATE include)
target_link_libraries(ContentPacker
  PRIVATE
    nlohmann_json::nlohmann_json
)

add_executable(EngineDemo
  src/main.cpp
)

target_link_libraries(EngineDemo PRIVATE engine_core)

if(WIN32)
  target_compile_definitions(EngineDemo PRIVATE SDL_MAIN_HANDLED)
endif()

function(engine_deploy_runtime_dlls target_name)
  if(WIN32)
    add_custom_command(
      TARGET ${target_name}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_RUNTIME_DLLS:${target_name}>
              $<TARGET_FILE_DIR:${target_name}>
      COMMAND_EXPAND_LISTS
    )
  endif()
endfunction()

function(engine_add_plain_test target_name source_file)
  add_executable(${target_name} ${source_file})
  target_link_libraries(${target_name} PRIVATE engine_core)
  engine_deploy_runtime_dlls(${target_name})
  add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

function(engine_add_catch_test target_name source_file)
  add_executable(${target_name} ${source_file})
  target_link_libraries(${target_name} PRIVATE engine_core Catch2::Catch2WithMain)
  engine_deploy_runtime_dlls(${target_name})
  catch_discover_tests(${target_name})
endfunction()

engine_add_plain_test(timing_tests tests/timing_tests.cpp)
engine_add_plain_test(config_tests tests/config_tests.cpp)
engine_add_catch_test(render2d_tests tests/render2d_tests.cpp)
engine_add_catch_test(projectile_tests tests/projectile_tests.cpp)
engine_add_catch_test(pattern_tests tests/pattern_tests.cpp)

add_executable(content_packer_tests tests/content_packer_tests.cpp)
target_link_libraries(content_packer_tests PRIVATE engine_core)
engine_deploy_runtime_dlls(content_packer_tests)
add_test(NAME content_packer_generate COMMAND ContentPacker --input ${CMAKE_SOURCE_DIR}/data --output ${CMAKE_BINARY_DIR}/content_test.pak)
add_test(NAME content_packer_tests COMMAND content_packer_tests ${CMAKE_BINARY_DIR}/content_test.pak)
set_tests_properties(content_packer_tests PROPERTIES DEPENDS content_packer_generate)

engine_add_plain_test(content_import_pipeline_tests tests/content_import_pipeline_tests.cpp)
engine_add_plain_test(entity_tests tests/entity_tests.cpp)
engine_add_plain_test(trait_tests tests/trait_tests.cpp)
engine_add_plain_test(archetype_tests tests/archetype_tests.cpp)
engine_add_plain_test(boss_phase_tests tests/boss_phase_tests.cpp)
engine_add_plain_test(run_structure_tests tests/run_structure_tests.cpp)
engine_add_plain_test(meta_progression_tests tests/meta_progression_tests.cpp)
engine_add_plain_test(persistence_tests tests/persistence_tests.cpp)
engine_add_plain_test(editor_tools_tests tests/editor_tools_tests.cpp)
engine_add_catch_test(replay_tests tests/replay_tests.cpp)
engine_add_plain_test(upgrade_ui_tests tests/upgrade_ui_tests.cpp)
engine_add_plain_test(upgrade_ui_model_tests tests/upgrade_ui_model_tests.cpp)
engine_add_catch_test(projectile_behavior_tests tests/projectile_behavior_tests.cpp)
engine_add_catch_test(pattern_graph_tests tests/pattern_graph_tests.cpp)
engine_add_plain_test(pattern_generator_tests tests/pattern_generator_tests.cpp)
engine_add_plain_test(pattern_graph_perf_tests tests/pattern_graph_perf_tests.cpp)
engine_add_plain_test(encounter_graph_tests tests/encounter_graph_tests.cpp)
engine_add_plain_test(difficulty_scaling_tests tests/difficulty_scaling_tests.cpp)
engine_add_plain_test(diagnostics_tests tests/diagnostics_tests.cpp)
engine_add_plain_test(gpu_bullets_tests tests/gpu_bullets_tests.cpp)
engine_add_plain_test(golden_replay_tests tests/golden_replay_tests.cpp)
engine_add_plain_test(benchmark_suite_tests tests/benchmark_suite.cpp)
engine_add_plain_test(public_api_tests tests/public_api_tests.cpp)
engine_add_plain_test(renderer_smoke_tests tests/renderer_smoke_tests.cpp)
engine_add_plain_test(palette_fx_templates_tests tests/palette_fx_templates_tests.cpp)
engine_add_plain_test(modern_renderer_tests tests/modern_renderer_tests.cpp)
engine_add_plain_test(defensive_special_tests tests/defensive_special_tests.cpp)
engine_add_plain_test(gameplay_session_state_tests tests/gameplay_session_state_tests.cpp)
engine_add_plain_test(standards_tests tests/standards_tests.cpp)
engine_add_catch_test(collision_correctness_tests tests/collision_correctness_tests.cpp)
engine_add_catch_test(determinism_property_tests tests/determinism_property_tests.cpp)
engine_add_catch_test(content_fuzz_tests tests/content_fuzz_tests.cpp)
engine_add_plain_test(determinism_cross_config_test tests/determinism_cross_config_test.cpp)
