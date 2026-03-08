# Getting Started (Creator Onboarding)

This guide is the fastest path from clone to a playable HellEngine run using authored content.

## Audience
- Content creators (patterns, encounters, bosses, palettes, audio).
- Technical designers validating deterministic replay.
- Mod/plugin developers validating extension boundaries.

## 1) Build engine tools and runtime

### Linux/macOS (Ninja)
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 4
```

### Windows (Visual Studio)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

Expected binaries:
- `EngineDemo`
- `ContentPacker`

## 2) Build a runtime content pack

Build from repository authoring data:

```bash
./build/ContentPacker --input data --output content.pak
```

Build from sample templates:

```bash
./build/ContentPacker --input examples/content_packs --output content.pak --pack-id starter
```

## 3) Run the runtime

```bash
./build/EngineDemo --content-pack content.pak
```

Headless creator smoke test:

```bash
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
```

## 4) Validate deterministic replay parity

```bash
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

Run this before publishing content changes.

## 5) Daily creator loop
1. Edit authored JSON in your source folder (`data/` or your own pack source tree).
2. Rebuild with `ContentPacker`.
3. Run a short headless smoke test.
4. Run replay verification.
5. Fix warnings/errors before sharing.

## 6) Creator documentation map
- Build/run options: `docs/BuildAndRun.md`
- Asset import: `docs/AssetImportWorkflow.md`
- Palette + grayscale shader workflow: `docs/PaletteAndGrayscaleWorkflow.md`
- Pattern authoring: `docs/PatternAuthoringGuide.md`
- Encounter/boss authoring: `docs/BossEncounterAuthoring.md`
- Replay/debug workflow: `docs/ReplayAndDebugGuide.md`
- Audio workflow: `docs/AudioWorkflow.md`
- Sample project usage: `docs/SampleProjectUsage.md`
- Plugin/mod overview: `docs/PluginAndModOverview.md`
- Creator performance guidance: `docs/CreatorPerformanceGuide.md`
- Troubleshooting: `docs/Troubleshooting.md`
