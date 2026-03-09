# HellEngine

HellEngine is a deterministic 2D bullet-hell engine package focused on creator workflows: authored content packs, replay-safe iteration, performance-aware rendering paths, and a constrained plugin boundary for productized extension.

Current engine version: **0.2.1** (`version/VERSION.txt`).

## Key Features
- Deterministic simulation loop with replay record/playback/verify workflows.
- JSON-authored content pipeline for patterns, encounters, entities, projectiles, palettes, traits, and audio event bindings.
- Dual runtime rendering paths (`--modern-renderer` / `--legacy-renderer`) with stress/smoke runtime checks.
- Creator-facing authoring docs for pattern graphs, palette workflows, boss/encounter authoring, and sample-pack extension.
- Release scripts for build validation, packaging, portable verification, and manifest hashing.
- Public API/plugin boundary under `include/engine/public/*` for host-side integration.

## Getting Started
1. Build runtime + tools:
   - Windows: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 && cmake --build build --config Debug`
   - Linux/macOS: `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j 4`
2. Build a pack: `./build/ContentPacker --input data --output content.pak`
3. Run: `./build/EngineDemo --content-pack content.pak`
4. Verify determinism: `./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak`

For full onboarding, see `docs/GettingStarted.md`.

## Creator Documentation Map
- Getting started: `docs/GettingStarted.md`
- Build/run command reference: `docs/BuildAndRun.md`
- Project/content structure + import: `docs/AssetImportWorkflow.md`, `docs/ContentPipeline.md`
- Palette/grayscale visual workflow: `docs/PaletteAndGrayscaleWorkflow.md`
- Pattern authoring: `docs/PatternAuthoringGuide.md`
- Enemy/boss/encounter authoring: `docs/BossEncounterAuthoring.md`
- Replay/debug workflow: `docs/ReplayAndDebugGuide.md`
- Audio workflow: `docs/AudioWorkflow.md`
- Sample usage: `docs/SampleProjectUsage.md`
- Testing/build validation: `docs/BuildAndRun.md`, `tools/run_tests.ps1`, `tools/release_validate.ps1`
- Packaging/release workflow: `docs/BuildAndRelease.md`
- Plugin/mod overview: `docs/PluginAndModOverview.md`
- Troubleshooting + performance: `docs/Troubleshooting.md`, `docs/CreatorPerformanceGuide.md`

## Repository Layout
- `src/engine/` - internal runtime systems (non-public API).
- `include/engine/public/` - stable public API + plugin interfaces.
- `data/` - authored source content used to build packs.
- `examples/content_packs/` - sample authoring templates.
- `docs/` - creator-facing and engineering docs.
- `tools/` - scripted build/test/release workflows.
- `tests/` - determinism/content/runtime validation executables.

## Productization Notes
- Runtime/testing/package scripts are intended to be run from a clean out-of-source build tree.
- Release packaging emits `RELEASE_MANIFEST.txt` with deterministic ordering + SHA-256 hashes.
- Portable packaging includes runtime dependencies and validates generated sample content before distribution.

## License
No explicit OSS license file is currently included in this repository snapshot.
