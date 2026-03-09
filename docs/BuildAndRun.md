# Build and Run

This page is the command reference for compiling and running HellEngine locally.

## Prerequisites
- CMake 3.28+
- C++ toolchain (Visual Studio 2022 on Windows, or Clang/GCC + Ninja)
- Git
- SDL2 (fetched via CMake FetchContent)
- Dear ImGui (fetched via CMake FetchContent)
- nlohmann_json (fetched via CMake FetchContent)
- GLAD (vendored at `third_party/glad`)
- Catch2 (used for Catch-authored tests with auto-linked `Catch2::Catch2WithMain`; non-Catch tests remain custom `main()` executables wired through CTest)
- SDL_mixer (actively used by `AudioSystem` for runtime WAV playback and event-driven bus/category mixing)

Dependencies are fetched via CMake (`SDL2`, `SDL2_mixer`, Dear ImGui, nlohmann_json, Catch2).

## Configure + build

### Windows
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

### Linux/macOS
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 4
```

## Build content pack
```bash
./build/ContentPacker --input data --output content.pak
```

## Core runtime commands

Run normally:
```bash
./build/EngineDemo --content-pack content.pak
```

Run headless smoke:
```bash
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
```

Run replay verification:
```bash
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

Run record/playback explicitly:
```bash
./build/EngineDemo --replay-record runs/sample.rpl --headless --ticks 1200 --seed 1337 --content-pack content.pak
./build/EngineDemo --replay-playback runs/sample.rpl --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

Renderer smoke:
```bash
./build/EngineDemo --headless --renderer-smoke-test --ticks 10 --content-pack content.pak
```

Stress mode:
```bash
./build/EngineDemo --stress-10k --content-pack content.pak
```

## Useful CLI overrides
- `--modern-renderer` / `--legacy-renderer`
- `--ticks <n>`
- `--seed <u64>`
- `--dt <seconds>`
- `--projectiles <count>`
- `--spawn-burst <count>`
- `--audio-master <0..1>`
- `--audio-music <0..1>`
- `--audio-sfx <0..1>`
- `--difficulty <name>`

## Run tests

### Windows
```powershell
ctest --test-dir build -C Debug --output-on-failure
```

### Linux/macOS
```bash
ctest --test-dir build --output-on-failure
```

## Release validation entry points
- `tools/build_release.ps1`
- `tools/package_dist.ps1`
- `tools/release_validate.ps1`
