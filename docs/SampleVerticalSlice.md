# Sample Vertical Slice — Ember Crossing

This document defines the product-validation sample for HellEngine and the exact workflows used to run it.

## Slice contents
- **Stage 01: Ember Crossing**
  - Intro combat lane (baseline enemy pressure)
  - Elite lane (increased fire-rate and projectile speed pressure)
  - Event lane (resource node recovery window)
  - Boss lane: **Asterion Dreadnought** (3 authored phases)
- **Stage 02: Seraph Rematch**
  - Short combat warm-up
  - Boss lane: **Nyx Seraph** (alternate 3-phase cadence)

Authored content sources:
- `data/entities.json` (enemy, resource, boss definitions + phase sequences)
- `data/patterns.json` (projectile motifs used by enemy and boss attacks)
- `data/encounters.json` (zone/boss/replay metadata for the sample)
- `data/audio.json` (music + SFX events, including boss phase and clear cues)

## What this validates
- Runtime encounter flow is hydrated from packed `encounters[].zones[]` (no demo-only hardcoded stage graph).
- Player combat loop (movement, defensive special, projectile output)
- Projectile readability (palette differentiation + danger field support)
- Enemy encounter progression (combat/elite/event pacing)
- Boss phase flow (multi-phase sequence and cadence transitions)
- Replay determinism workflow (record/playback/verify)
- Content pipeline usage (compile pack from authored JSON)
- Audio integration (combat/UI/boss cue event map)
- HUD/UI operation (runtime overlay + upgrade UI flow)
- Packaging viability (portable zip + installer script path)

## Run/build instructions (Windows)
1. Build + tests
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

2. Build content pack from authored sample data
```powershell
.\build\Debug\ContentPacker.exe --input data --output content.pak
```

3. Run playable slice
```powershell
.\build\Debug\EngineDemo.exe --content-pack content.pak
```

4. Validate replay loop
```powershell
.\build\Debug\EngineDemo.exe --replay-record runs\vertical_slice_sample.rpl --headless --ticks 7200 --seed 1337 --content-pack content.pak
.\build\Debug\EngineDemo.exe --replay-playback runs\vertical_slice_sample.rpl --headless --ticks 7200 --seed 1337 --content-pack content.pak
.\build\Debug\EngineDemo.exe --replay-verify --headless --ticks 7200 --seed 1337 --content-pack content.pak
```

5. Validate packaging
```powershell
./tools/build_release.ps1
./tools/package_dist.ps1
./build_installer.ps1
```

## Systems validated checklist
- [x] Combat + projectiles
- [x] Encounters + boss phases
- [x] Replay record/playback/verify
- [x] Content authoring + pack compile/load
- [x] Audio event metadata
- [x] HUD/UI (runtime + upgrades)
- [x] Packaging scripts
