# Build and Run (Windows)

## Prerequisites
- Visual Studio 2022 (Desktop development with C++)
- CMake 3.28+
- Git
- SDL2 (fetched via CMake FetchContent)
- Dear ImGui (fetched via CMake FetchContent)
- nlohmann_json (fetched via CMake FetchContent)
- GLAD (vendored at `third_party/glad`)
- Catch2 (**not currently used**; test binaries are custom `main()` executables wired through CTest)
- SDL_mixer (**not currently used**; audio runtime uses SDL audio callback mixer in `AudioSystem`)

## Configure
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

## Build
```powershell
cmake --build build --config Debug
```

## Run (Entity + Pattern Sandbox)
```powershell
.\build\Debug\EngineDemo.exe
```

## Run (Stress Mode: 10,000 Projectiles)
```powershell
.\build\Debug\EngineDemo.exe --stress-10k
```

## Run (Headless Deterministic Simulation)
```powershell
.\build\Debug\EngineDemo.exe --headless --ticks 1200 --seed 1337
```

## Run (Renderer Smoke Test)
```powershell
.\build\Debug\EngineDemo.exe --renderer-smoke-test
```


## Run (Replay Record)
```powershell
.\build\Debug\EngineDemo.exe --replay-record runs\sample.rpl --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

## Run (Replay Playback)
```powershell
.\build\Debug\EngineDemo.exe --replay-playback runs\sample.rpl --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

## Run (Modern Renderer Path)
```powershell
.\build\Debug\EngineDemo.exe --modern-renderer
```

## Run (Legacy Renderer Path)
```powershell
.\build\Debug\EngineDemo.exe --legacy-renderer
```

## Run (Audio Volume Override Smoke)
```powershell
.\build\Debug\EngineDemo.exe --audio-master 0.8 --audio-music 0.5 --audio-sfx 1.0
```


## Product Validation Vertical Slice
Use the authored sample runbook for end-to-end product validation:
- `docs/SampleVerticalSlice.md`

Quick path:
```powershell
.\build\Debug\ContentPacker.exe --input data --output content.pak
.\build\Debug\EngineDemo.exe --content-pack content.pak
.\build\Debug\EngineDemo.exe --replay-verify --headless --ticks 7200 --seed 1337 --content-pack content.pak
./tools/package_dist.ps1
```

## Build ContentPacker
```powershell
cmake --build build --config Debug --target ContentPacker
```

## Compile Runtime Content Pack
```powershell
.\build\Debug\ContentPacker.exe --input data --output content.pak
```

## Run with Compiled Content Pack
```powershell
.\build\Debug\EngineDemo.exe --content-pack content.pak
```

## Run Tests
```powershell
ctest --test-dir build -C Debug --output-on-failure
```

## Controls (Windowed)
- Pan camera: `W/A/S/D` or arrow keys
- Zoom camera: mouse wheel or `Q/E`
- Toggle fullscreen/windowed: `F`
- Toggle projectile hitboxes: `H`
- Toggle collision grid overlay: `G`
- Pattern select: `1..9` or `TAB`
- Pattern pause/resume: `SPACE`
- Pattern reset (and clear projectiles): `R`
- Playback speed: `,` and `.`

## Pattern Composition Sandbox
Patterns are loaded from:
- `assets/patterns/sandbox_patterns.json`

Composition features:
- Layered patterns (multiple layers per pattern)
- Subpatterns (layer-to-layer references)
- Looping sequences
- Timelines with per-time speed/rotation overrides

Modifier features:
- rotation
- phase offsets
- speed curves
- angle oscillation
- deterministic jitter

Supported base pattern emitters:
- `radial`
- `spiral`
- `spread`
- `wave`
- `aimed`

## Expected Result
- Windowed mode:
  - A window titled **EngineDemo** opens.
  - Layered composition patterns are emitted from JSON-authored data.
  - Overlay displays pattern name, layer count, active sequence step/layer count, and playback status.
  - Projectile simulation remains deterministic and fixed-step.
- Stress mode:
  - Spawns and simulates 10,000 projectiles.
- Headless mode:
  - Runs deterministic fixed-step simulation without opening a window.
  - Exits after requested tick count.


See also: `docs/AuthoringPipeline.md` for the full authoring/packing workflow.


## Entity System
Data-driven entities are authored in `data/entities.json` and support:
- Player, Enemy, Boss, Resource Node, Hazard templates
- movement behaviors (`static`, `linear`, `sine`, `chase`)
- health/damage fields
- spawn rules
- enemy attack patterns via pattern names
- harvestable resource node types defined in data (currency, heal, temporary buff yields)


## Trait System
- During a run, trait offers are rolled deterministically as 1-of-3 choices.
- Choose with keys `1`, `2`, or `3` when an offer is pending.
- Traits apply rarity-weighted stat/behavior modifiers to projectiles, patterns, player harvest stats, and enemy interactions.
- Current trait effects are displayed in the runtime overlay.

## Release Packaging (Phase 18)
PowerShell scripts are provided at repository root to automate release outputs.

### 1) Build Release
```powershell
./tools/build_release.ps1
```
- Configures and builds a Release tree.
- Runs CTest in Release mode.

### 2) Create Portable Distribution + Zip
```powershell
./tools/package_dist.ps1
```
Outputs:
- Portable folder: `dist/portable`
- Zip archive: `dist/EngineDemo-portable.zip`

### 3) Build Optional Windows Installer
```powershell
./build_installer.ps1
```
- Uses NSIS (`makensis`) when available.
- If NSIS is not installed, writes a guidance note instead of failing hard.


See also: `docs/BuildAndRelease.md` for the scripted build/test/package pipeline.


## Replay Verify (Determinism Verifier)
```powershell
.\build\Debug\EngineDemo.exe --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```
- Runs simulation twice (record + playback verification).
- Reports first divergence tick and subsystem (`bullets`, `entities`, `run_state`) if detected.


## Compiled Pattern Graphs

Pattern graph authoring JSON supports a `graphs` array where each graph contains `id`, `nodes[]`, and per-node fields:
- `id`
- `type` (`emit_ring`, `emit_spread`, `emit_spiral`, `emit_wave`, `emit_aimed`, `wait`, `loop`, `modifier_rotate`, `modifier_phase`, `random_range`)
- `params` (numeric map)
- `targetNode` (for loop back-edges)
- `outputs` (optional edge list, first output is used as loop target fallback)

Compiler validation enforces bounded loops (`count <= 64`) and validates loop targets.

Run performance check:
- `ctest --test-dir build -R pattern_graph_perf_tests --output-on-failure`


## Stability + Diagnostics

- Structured errors are logged as JSON (`error_report=...`) with code/message/context/stack.
- Bad content load paths now fall back to last-good/default content and report an error instead of hard-crashing.
- In Release builds, crash handlers write reports under `crashes/` including build stamp and recent log tail; on Windows they also emit a minidump (`.dmp`).
- Toggle the on-screen debug HUD with the backquote key (`).


## Local CI Health Check
```powershell
./tools/ci_local.ps1
```


## Audio content paths
Runtime audio metadata is authored in:
- `data/audio.json`

Current clip paths in pack metadata:
- `music/main_loop.wav`
- `sfx/hit.wav`
- `sfx/graze.wav`
- `sfx/player_damage.wav`
- `sfx/enemy_death.wav`
- `sfx/boss_warning.wav`
- `ui/click.wav`
- `ui/confirm.wav`

## Catch2 tag filtering note
Catch2-style tag filtering is not currently applicable because the repository test harness uses standalone test executables + CTest entries rather than Catch2.

Equivalent filtering uses CTest regex selection:
```powershell
ctest --test-dir build -C Debug -R "pattern|collision|audio" --output-on-failure
```
## Audio prerequisites
- Build now fetches `SDL_mixer` via CMake `FetchContent`.
- Optional audio assets expected under `data/audio/` (e.g. `hit.wav`, `graze.wav`, `phase_warn.wav`, `special.wav`). Missing files are tolerated and runtime continues silently.
## Catch2 test usage
- Catch2 v3.5.2 is integrated through CMake `FetchContent` for selected test binaries.
- CTest still runs the full suite (both Catch2-discovered and legacy add_test targets):
  - `ctest --test-dir build --output-on-failure`
- Run deterministic-tagged Catch2 cases through CTest regex filtering:
  - `ctest --test-dir build --output-on-failure -R "\[determinism\]"`
- Use verbose CTest output when debugging failures:
  - `ctest --test-dir build --output-on-failure -V`
