# Build and Run (Windows)

## Prerequisites
- Visual Studio 2022 (Desktop development with C++)
- CMake 3.28+
- Git

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
./build_release.ps1
```
- Configures and builds a Release tree.
- Runs CTest in Release mode.

### 2) Create Portable Distribution + Zip
```powershell
./package_dist.ps1
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
