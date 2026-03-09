# Sample Vertical Slice — Ember Crossing

This document defines the product-validation sample for HellEngine and the exact workflows used to run it.

## Slice contents
- **Stage 01: Ember Crossing**
  - Intro combat lane (baseline enemy pressure)
  - Elite lane (mixed enemy pressure: Shooter + Vanguard Lancer)
  - Event lane (short recovery window with resource node routing)
  - Boss lane: **Asterion Dreadnought** (3 authored phases)
- **Stage 02: Seraph Rematch**
  - Short combat warm-up
  - Boss lane: **Nyx Seraph** (alternate 3-phase cadence)

Authored content sources:
- `data/entities.json` (enemy/resource/boss definitions including `Vanguard Lancer` showcase pressure profile)
- `data/patterns.json` (projectile motifs used by enemy and boss attacks)
- `data/encounters.json` (zone/boss/replay metadata with final Stage 01 escalation tuning)
- `data/audio.json` (music + SFX events, including boss phase and clear cues)

## What this validates
- Runtime encounter flow is hydrated from packed `encounters[].zones[]` (no demo-only hardcoded stage graph).
- Player combat loop (movement, defensive special, projectile output)
- Projectile readability (palette differentiation + danger field support)
- Enemy encounter progression (combat/elite/event pacing)
- Boss phase flow (multi-phase sequence/cadence transitions with explicit phase-shift cue)
- Replay determinism workflow (record/playback/verify)
- Content pipeline usage (compile pack from authored JSON)
- Audio integration (combat/UI/boss cue event map including phase-shift transition cue)
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

## Stage 01 authored tuning snapshot (final polish)
- `lane_intro`: 50s warm-up against `Basic Enemy Shooter` for onboarding/readability.
- `lane_elite`: 65s mixed pressure (`Basic Enemy Shooter`, `Vanguard Lancer`) to validate layered projectile reads and movement discipline.
- `lane_event`: 30s resource-focused recovery window (`Currency Crystal`, `Med Bloom`, `Overclock Node`).
- `lane_boss`: Asterion flow authored as `Wave Weave` -> `Spread Lattice` -> `Composed Helix` with `boss_phase_shift` transition audio event.
