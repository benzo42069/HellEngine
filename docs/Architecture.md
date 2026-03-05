# Architecture

## Runtime layers

1. **Config/bootstrap** (`main.cpp`, `config.*`)
   - loads `engine_config.json`
   - applies CLI overrides
   - installs crash handlers (Release)

2. **Content pipeline** (`content_pipeline.*`, `tools_content_packer.cpp`)
   - migrates/validates pack schema
   - merges assets by GUID
   - computes `contentHash`

3. **Simulation core** (`runtime.cpp`)
   - fixed-step deterministic loop
   - pattern emission (legacy PatternPlayer or compiled PatternGraph VM)
   - entity updates, collisions, progression systems

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

