# Architecture

## Runtime layers

1. **Config/bootstrap** (`main.cpp`, `config.*`)
   - loads `engine_config.json`
   - applies CLI overrides
   - installs crash handlers (Release)

2. **Simulation core** (`runtime.cpp`, `gameplay_session.*`, `input_system.*`)
   - fixed-step deterministic loop orchestration in `Runtime`
   - gameplay simulation update and progression orchestration in `GameplaySession`
   - input polling/event processing + replay input injection/recording in `InputSystem`

3. **Render pipeline** (`render_pipeline.*`, `render2d.*`, `modern_renderer.*`, `bullet_palette.*`)
   - render-context lifecycle (init/recreate/shutdown)
   - `BulletPaletteTable` maps palette template registry entries to projectile render colors
   - scene overlay composition through `SimSnapshot` sim->render contract
   - sprite/debug draw + tool HUD composition
   - `ParticleFxSystem` consumes projectile despawn events and renders short-lived impact bursts using frame-time updates (visual-only, no sim coupling)

4. **Diagnostics/replay** (`diagnostics.*`, `replay.*`, `logging.*`)
   - structured error reports (`code/context/stack`)
   - replay record/playback verification with subsystem mismatch reporting
   - logger tail buffering for crash reports

5. **Tooling/editor** (`editor_tools.*`)
   - in-engine control center
   - pattern graph editing/preview
   - validation status and debug controls

## Data flow

- Author JSON content (`patterns`, `entities`, `traits`, `archetypes`, `encounters`, optional `graphs`).
- Pack with `ContentPacker` -> `content.pak`.
- Runtime loads pack(s) and caches pattern/entity databases.
- Deterministic systems run from seed + fixed dt.
- Collision pipeline executes in three explicit stages: `updateMotion` -> `buildGrid` -> `resolveCollisions` (targets queried per overlapping grid cell, then deterministic event sort).

## Safety model

- Bad or missing content should not hard-crash gameplay startup.
- Load errors are logged as structured JSON error reports.
- Runtime falls back to last-good or built-in/default content where available.
- Release crashes emit crash reports (`crashes/`) and Windows minidumps.

## Key extension points

- Add new asset types in `content_pipeline` + packer merge rules.
- Add new pattern graph opcodes in `pattern_graph.h/.cpp`.
- Add authoring UI in `editor_tools.cpp`.
- Add deterministic verifier probes in `runtime.cpp` + `replay.cpp`.

