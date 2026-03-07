cmake_minimum_required(VERSION 3.28)

set(ENGINE_VERSION_FILE "${CMAKE_SOURCE_DIR}/version/VERSION.txt")
if(EXISTS "${ENGINE_VERSION_FILE}")
  file(READ "${ENGINE_VERSION_FILE}" ENGINE_PROJECT_VERSION_RAW)
  string(STRIP "${ENGINE_PROJECT_VERSION_RAW}" ENGINE_PROJECT_VERSION)
else()
  set(ENGINE_PROJECT_VERSION "0.2.0")
endif()

project(EngineDemo VERSION ${ENGINE_PROJECT_VERSION} LANGUAGES CXX)

execute_process(
  COMMAND git -C "${CMAKE_SOURCE_DIR}" rev-parse --short=12 HEAD
  OUTPUT_VARIABLE ENGINE_GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(ENGINE_GIT_HASH STREQUAL "")
  set(ENGINE_GIT_HASH "nogit")
endif()

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

# Avoid building both SDL shared and static variants in this project to reduce
# archive/write races under parallel generators and to keep linkage explicit.
set(SDL_SHARED ON CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Build SDL static library" FORCE)
set(SDL_TEST OFF CACHE BOOL "Build SDL tests" FORCE)

FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.30.5
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.0
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(SDL2 imgui nlohmann_json)

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
  src/engine/config.cpp
  src/engine/crash_handler.cpp
  src/engine/diagnostics.cpp
  src/engine/gameplay_session.cpp
  src/engine/content_pipeline.cpp
  src/engine/content_watcher.cpp
  src/engine/deterministic_rng.cpp
  src/engine/defensive_special.cpp
  src/engine/danger_field.cpp
  src/engine/difficulty_scaling.cpp
  src/engine/encounter_graph.cpp
  src/engine/editor_tools.cpp
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
target_link_libraries(ContentPacker PRIVATE nlohmann_json::nlohmann_json)

add_executable(EngineDemo
  src/main.cpp
)

target_link_libraries(EngineDemo PRIVATE engine_core)

if(WIN32)
  target_compile_definitions(EngineDemo PRIVATE SDL_MAIN_HANDLED)
endif()

enable_testing()

add_executable(timing_tests tests/timing_tests.cpp)
target_link_libraries(timing_tests PRIVATE engine_core)
add_test(NAME timing_tests COMMAND timing_tests)

add_executable(config_tests tests/config_tests.cpp)
target_link_libraries(config_tests PRIVATE engine_core)
add_test(NAME config_tests COMMAND config_tests)

add_executable(render2d_tests tests/render2d_tests.cpp)
target_link_libraries(render2d_tests PRIVATE engine_core)
add_test(NAME render2d_tests COMMAND render2d_tests)

add_executable(projectile_tests tests/projectile_tests.cpp)
target_link_libraries(projectile_tests PRIVATE engine_core)
add_test(NAME projectile_tests COMMAND projectile_tests)

add_executable(pattern_tests tests/pattern_tests.cpp)
target_link_libraries(pattern_tests PRIVATE engine_core)
add_test(NAME pattern_tests COMMAND pattern_tests)

add_executable(content_packer_tests tests/content_packer_tests.cpp)
target_link_libraries(content_packer_tests PRIVATE engine_core)
add_test(NAME content_packer_generate COMMAND ContentPacker --input ${CMAKE_SOURCE_DIR}/data --output ${CMAKE_BINARY_DIR}/content_test.pak)
add_test(NAME content_packer_tests COMMAND content_packer_tests ${CMAKE_BINARY_DIR}/content_test.pak)
set_tests_properties(content_packer_tests PROPERTIES DEPENDS content_packer_generate)


add_executable(content_import_pipeline_tests tests/content_import_pipeline_tests.cpp)
target_link_libraries(content_import_pipeline_tests PRIVATE engine_core)
add_test(NAME content_import_pipeline_tests COMMAND content_import_pipeline_tests)

add_executable(entity_tests tests/entity_tests.cpp)
target_link_libraries(entity_tests PRIVATE engine_core)
add_test(NAME entity_tests COMMAND entity_tests)

add_executable(trait_tests tests/trait_tests.cpp)
target_link_libraries(trait_tests PRIVATE engine_core)
add_test(NAME trait_tests COMMAND trait_tests)

add_executable(archetype_tests tests/archetype_tests.cpp)
target_link_libraries(archetype_tests PRIVATE engine_core)
add_test(NAME archetype_tests COMMAND archetype_tests)

add_executable(boss_phase_tests tests/boss_phase_tests.cpp)
target_link_libraries(boss_phase_tests PRIVATE engine_core)
add_test(NAME boss_phase_tests COMMAND boss_phase_tests)

add_executable(run_structure_tests tests/run_structure_tests.cpp)
target_link_libraries(run_structure_tests PRIVATE engine_core)
add_test(NAME run_structure_tests COMMAND run_structure_tests)

add_executable(meta_progression_tests tests/meta_progression_tests.cpp)
target_link_libraries(meta_progression_tests PRIVATE engine_core)
add_test(NAME meta_progression_tests COMMAND meta_progression_tests)

add_executable(persistence_tests tests/persistence_tests.cpp)
target_link_libraries(persistence_tests PRIVATE engine_core)
add_test(NAME persistence_tests COMMAND persistence_tests)

add_executable(editor_tools_tests tests/editor_tools_tests.cpp)
target_link_libraries(editor_tools_tests PRIVATE engine_core)
add_test(NAME editor_tools_tests COMMAND editor_tools_tests)

add_executable(replay_tests tests/replay_tests.cpp)
target_link_libraries(replay_tests PRIVATE engine_core)
add_test(NAME replay_tests COMMAND replay_tests)

add_executable(upgrade_ui_tests tests/upgrade_ui_tests.cpp)
target_link_libraries(upgrade_ui_tests PRIVATE engine_core)
add_test(NAME upgrade_ui_tests COMMAND upgrade_ui_tests)

add_executable(upgrade_ui_model_tests tests/upgrade_ui_model_tests.cpp)
target_link_libraries(upgrade_ui_model_tests PRIVATE engine_core)
add_test(NAME upgrade_ui_model_tests COMMAND upgrade_ui_model_tests)

add_executable(projectile_behavior_tests tests/projectile_behavior_tests.cpp)
target_link_libraries(projectile_behavior_tests PRIVATE engine_core)
add_test(NAME projectile_behavior_tests COMMAND projectile_behavior_tests)

add_executable(pattern_graph_tests tests/pattern_graph_tests.cpp)
target_link_libraries(pattern_graph_tests PRIVATE engine_core)
add_test(NAME pattern_graph_tests COMMAND pattern_graph_tests)

add_executable(pattern_generator_tests tests/pattern_generator_tests.cpp)
target_link_libraries(pattern_generator_tests PRIVATE engine_core)
add_test(NAME pattern_generator_tests COMMAND pattern_generator_tests)


add_executable(pattern_graph_perf_tests tests/pattern_graph_perf_tests.cpp)
target_link_libraries(pattern_graph_perf_tests PRIVATE engine_core)
add_test(NAME pattern_graph_perf_tests COMMAND pattern_graph_perf_tests)
add_executable(encounter_graph_tests tests/encounter_graph_tests.cpp)
target_link_libraries(encounter_graph_tests PRIVATE engine_core)
add_test(NAME encounter_graph_tests COMMAND encounter_graph_tests)

add_executable(difficulty_scaling_tests tests/difficulty_scaling_tests.cpp)
target_link_libraries(difficulty_scaling_tests PRIVATE engine_core)
add_test(NAME difficulty_scaling_tests COMMAND difficulty_scaling_tests)


add_executable(diagnostics_tests tests/diagnostics_tests.cpp)
target_link_libraries(diagnostics_tests PRIVATE engine_core)
add_test(NAME diagnostics_tests COMMAND diagnostics_tests)

add_executable(gpu_bullets_tests tests/gpu_bullets_tests.cpp)
target_link_libraries(gpu_bullets_tests PRIVATE engine_core)
add_test(NAME gpu_bullets_tests COMMAND gpu_bullets_tests)




add_executable(golden_replay_tests tests/golden_replay_tests.cpp)
target_link_libraries(golden_replay_tests PRIVATE engine_core)
add_test(NAME golden_replay_tests COMMAND golden_replay_tests)

add_executable(benchmark_suite_tests tests/benchmark_suite.cpp)
target_link_libraries(benchmark_suite_tests PRIVATE engine_core)
add_test(NAME benchmark_suite_tests COMMAND benchmark_suite_tests)

add_executable(public_api_tests tests/public_api_tests.cpp)
target_link_libraries(public_api_tests PRIVATE engine_core)
add_test(NAME public_api_tests COMMAND public_api_tests)

add_executable(renderer_smoke_tests tests/renderer_smoke_tests.cpp)
target_link_libraries(renderer_smoke_tests PRIVATE engine_core)
add_test(NAME renderer_smoke_tests COMMAND renderer_smoke_tests)

add_executable(palette_fx_templates_tests tests/palette_fx_templates_tests.cpp)
target_link_libraries(palette_fx_templates_tests PRIVATE engine_core)
add_test(NAME palette_fx_templates_tests COMMAND palette_fx_templates_tests)

add_executable(modern_renderer_tests tests/modern_renderer_tests.cpp)
target_link_libraries(modern_renderer_tests PRIVATE engine_core)
add_test(NAME modern_renderer_tests COMMAND modern_renderer_tests)

add_executable(defensive_special_tests tests/defensive_special_tests.cpp)
target_link_libraries(defensive_special_tests PRIVATE engine_core)
add_test(NAME defensive_special_tests COMMAND defensive_special_tests)


add_executable(gameplay_session_state_tests tests/gameplay_session_state_tests.cpp)
target_link_libraries(gameplay_session_state_tests PRIVATE engine_core)
add_test(NAME gameplay_session_state_tests COMMAND gameplay_session_state_tests)

add_executable(standards_tests tests/standards_tests.cpp)
target_link_libraries(standards_tests PRIVATE engine_core)
add_test(NAME standards_tests COMMAND standards_tests)

add_executable(collision_correctness_tests tests/collision_correctness_tests.cpp)
target_link_libraries(collision_correctness_tests PRIVATE engine_core)
add_test(NAME collision_correctness_tests COMMAND collision_correctness_tests)

add_executable(determinism_cross_config_test tests/determinism_cross_config_test.cpp)
target_link_libraries(determinism_cross_config_test PRIVATE engine_core)
add_test(NAME determinism_cross_config_test COMMAND determinism_cross_config_test)
