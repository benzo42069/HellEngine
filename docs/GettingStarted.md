# Getting Started (External Creator Quickstart)

This guide is the fastest path from clone to a playable build that uses your own authored content.

## Who this guide is for
- Creators building encounters/patterns/content packs.
- Integrators evaluating engine runtime + deterministic replay workflow.
- Mod/plugin developers validating extension boundaries.

## 1) Build the engine

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 4
```

Expected binaries in `build/`:
- `EngineDemo`
- `ContentPacker`

## 2) Build a content pack

Runtime content is loaded from a compiled pack (`content.pak`).

Build from local authoring data:

```bash
./build/ContentPacker --input data --output content.pak
```

Or build from repo templates:

```bash
./build/ContentPacker --input examples/content_packs --output content.pak --pack-id starter
```

## 3) Run the game/runtime

```bash
./build/EngineDemo --content-pack content.pak
```

Headless smoke test:

```bash
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
```

## 4) Verify deterministic replay parity

```bash
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

Use this command as your baseline “did my content change break determinism?” check.

## 5) Author safely (edit -> repack -> verify loop)
1. Edit JSON in `data/` (or your own content source folder).
2. Rebuild the pack with `ContentPacker`.
3. Run a short headless smoke test.
4. Run `--replay-verify` before sharing/committing.

If content load fails, runtime reports a structured `error_report=...` log and keeps last-good/default fallback behavior instead of hard-crashing.

## 6) Where to go next
- Asset import workflow: `docs/AssetImportWorkflow.md`
- Pattern authoring: `docs/PatternAuthoringGuide.md`
- Boss + encounter authoring: `docs/BossEncounterAuthoring.md`
- Replay/debug workflow: `docs/ReplayAndDebugGuide.md`
- Plugin/mod extension overview: `docs/PluginAndModOverview.md`
- Performance guidance for creators: `docs/CreatorPerformanceGuide.md`
- Troubleshooting: `docs/Troubleshooting.md`
