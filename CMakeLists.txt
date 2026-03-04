cmake_minimum_required(VERSION 3.28)
project(EngineDemo VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

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
  src/engine/config.cpp
  src/engine/deterministic_rng.cpp
  src/engine/editor_tools.cpp
  src/engine/entities.cpp
  src/engine/job_system.cpp
  src/engine/logging.cpp
  src/engine/memory.cpp
  src/engine/meta_progression.cpp
  src/engine/patterns.cpp
  src/engine/projectiles.cpp
  src/engine/replay.cpp
  src/engine/render2d.cpp
  src/engine/run_structure.cpp
  src/engine/runtime.cpp
  src/engine/timing.cpp
  src/engine/traits.cpp
  src/stb_impl.cpp
)

target_include_directories(engine_core
  PUBLIC
    include
    third_party/stb
)

target_link_libraries(engine_core
  PUBLIC
    ${ENGINEDEMO_SDL_TARGET}
    imgui_sdl2
    nlohmann_json::nlohmann_json
)


add_executable(ContentPacker
  src/tools_content_packer.cpp
)

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

add_executable(editor_tools_tests tests/editor_tools_tests.cpp)
target_link_libraries(editor_tools_tests PRIVATE engine_core)
add_test(NAME editor_tools_tests COMMAND editor_tools_tests)

add_executable(replay_tests tests/replay_tests.cpp)
target_link_libraries(replay_tests PRIVATE engine_core)
add_test(NAME replay_tests COMMAND replay_tests)
