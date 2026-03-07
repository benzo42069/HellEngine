# Implementation Plan

## Planning Constraints

## Execution Status Update (Persistence Baseline)
- Added `engine/persistence` module for product-oriented settings + profiles/save-slot file ownership.
- Defined schema-versioned load/save behavior with explicit migrators for legacy v1 documents.
- Added runtime integration hooks to load persisted settings/profiles at startup and write active profile/settings at shutdown.
- Added meta-progression snapshot hooks (`applyProgressSnapshot`/`makeProgressSnapshot`) to decouple progression persistence from runtime internals.
- Added persistence-focused tests for migration/roundtrip/corrupted fallback paths.

- Additive implementation only; keep repository buildable at each phase.
- Never rename modules once introduced.
- Deterministic sim/replay/hash verification precede advanced features.
- This phase remains documentation-only.





## Completion Ledger (Phases 1-10)
- Phase 1 — Spec Ingestion: **Completed**
- Phase 2 — CoreRuntime + Deterministic Clock Scaffold: **Completed (baseline in repo)**
- Phase 3 — Deterministic Kernel + Replay Hash Backbone: **Completed**
- Phase 4 — Bullet Motion Math + Emission Primitives: **Completed**
- Phase 5 — Hybrid ECS + Bullet World + Physics Core Vertical Slice: **Completed**
- Phase 6 — Pattern IR/VM + Authoring Runtime Binding: **Completed**
- Phase 7 — Gameplay Systems + Boss/Encounter Runtime Integration: **Completed**
- Phase 8 — Rendering + Debug/Profiler Introspection + Scalability Pass: **Completed**
- Phase 9 — Editor, Content Pipeline, and Commercial Tooling: **Completed**
- Phase 10 — Polish, Release Readiness, and Optional Extensions: **Completed (baseline docs and runtime scaffolding in place; optional items remain feature-flagged/roadmap tracked)**

### Graphics/shader delivery status
- OpenGL bullet rendering path (`GlBulletRenderer`) implemented.
- Shader management (`ShaderCache`) implemented.
- Grayscale atlas + palette ramp shader colorization path implemented.
- Modern post-processing chain (bloom/vignette/composite) implemented with fallback behavior.

## Program Milestones (Cross-Phase Gates)
- **M0 Architecture Lock**: determinism plan, bullet/collision architecture, Pattern Graph/IR specs approved.
- **M1 First Playable Simulation**: fixed-step loop + sandbox rendering + input movement.
- **M2 1,000 Bullet Stress**: no-allocation bullet runtime baseline.
- **M3 Collision + Graze Prototype**: deterministic overlap + graze events working.
- **M4 10,000 Bullet Stress**: sustained deterministic stress and hash stability.
- **M5 Pattern VM v1**: authored patterns compile and execute deterministically.
- **M6 Pattern Editor Prototype**: graph/timeline pattern authoring with live preview.
- **M7 Replay v1**: input/seed/content-hash replay with divergence diagnostics.
- **M8 Boss Vertical Slice**: full boss encounter authored via tools.
- **M9 Full Demo Loop**: stage map, encounters, rewards, boss, progression.
- **M10 1.0 RC**: docs/templates/plugin+mod baseline and release-readiness checks passed.

## Phase Framework Alignment
This plan uses additive buildable phases that map to the product program sequence:
- Foundations/pre-production intent is represented by Phase 1 documentation lock and milestone M0.
- Runtime, bullet, collision, pattern, tools, gameplay, pipeline, optimization, and polish intent are distributed across Phases 2–10 with explicit Windows validation commands.

## Execution Status Update (Phase 3)
- **Completed implementation in repository**:
  - Engine skeleton build graph with SDL2, Dear ImGui, stb_image, stb_truetype, nlohmann_json.
  - Fixed-step core runtime loop with render separation and headless mode.
  - Runtime services: logging, config, deterministic RNG streams, job system, frame allocator/object pool helpers.
  - Unit tests: timing and configuration (`ctest`).

## Phase 1 — Spec Ingestion (Complete)
### What changed
- Merged architecture and motion-math design documents into a single authoritative specification.
- Added decisions for deterministic motion stack ordering, RNG indexing discipline, and tie-break rules.
- Updated phased roadmap to include deterministic math/emission validation milestones.

### Files added/edited
- `docs/MasterSpec.md`
- `docs/DecisionLog.md`
- `docs/ImplementationPlan.md`

### Exact build/run commands (Windows)
```powershell
# Docs-only validation
git status
```

## Phase 2 — CoreRuntime + Deterministic Clock Scaffold
### Scope
- Create fixed module skeletons:
  - `CoreRuntime`
  - `Simulation`
  - `GameplaySystems`
  - `PatternRuntime`
  - `Render`
  - `AssetPipeline`
  - `EditorAPI`
- Implement fixed-step clock, deterministic utilities, logging/asserts, and memory telemetry scaffolding.

### Buildability target
- Executable boots and runs deterministic tick loop in headless mode.

### Exact build/run commands (Windows)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
.\build\Debug\EngineDemo.exe --headless --ticks 300
```

## Phase 3 — Deterministic Kernel + Replay Hash Backbone
### Scope
- Implement canonical tick order contract.
- Add deterministic input command buffer and snapshot/hash pipeline.
- Add replay record/playback skeleton and divergence reporting.

### Buildability target
- Same input stream produces matching hash sequence in replay verification.

### Exact build/run commands (Windows)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\EngineDemo.exe --record replay.rpl --ticks 3600
.\build\Debug\EngineDemo.exe --playback replay.rpl --verify-hash
```

## Phase 4 — Bullet Motion Math + Emission Primitives
### Scope
- Implement deterministic bullet kinematic modules:
  - Linear, acceleration (semi-implicit), angular turn, sinusoidal, orbit, homing clamp, Bézier path.
- Implement canonical motion stack order (steering -> integration -> offsets -> constraints).
- Implement emission primitives:
  - Radial, arc, aimed, shotgun, rotating ring, spiral families, lattice/grid.
- Add deterministic equation/parameter tests for tick-level reproducibility.

### Buildability target
- Headless math validation scenes pass deterministic motion/emission test suite.

### Exact build/run commands (Windows)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure -R Motion|Emission
.\build\Debug\EngineDemo.exe --scene motion_validation --verify-hash
```

## Phase 5 — Hybrid ECS + Bullet World + Physics Core Vertical Slice
### Scope
- Implement general ECS (non-bullet entities) and specialized Bullet World SoA pools.
- Add preallocated command buffers/free-list reuse/no-hot-alloc enforcement.
- Implement specialized overlap physics: circle/capsule baseline, hitbox/hurtbox/graze zones, uniform-grid broadphase, fixed-capacity buckets, deterministic overflow handling, and event-precedence resolution order.
- Add deterministic collision event pipeline with stable sort keys and graze tracking arrays.

### Buildability target
- Bullet stress scene (10k+) runs with stable hashes, collision consistency, and zero per-tick allocations in collision path.

### Exact build/run commands (Windows)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure -R Collision|BulletWorld|Graze|Physics
.\build\Debug\EngineDemo.exe --scene bullet_world_stress --verify-hash
.\build\Debug\EngineDemo.exe --scene collision_stress_lab --verify-hash --report collision_profile.json
```

## Phase 6 — Pattern IR/VM + DSL + Parameter Binding
### Scope
- Implement Pattern Graph dual-flow model (Exec + Data), typed pins, explicit conversion rules, and deterministic parallel-branch merge semantics.
- Implement graph/timeline/DSL compilation into unified Pattern IR with debug mapping (node GUID -> IR range).
- Implement deterministic VM instructions for loops/waits/bursts/shapes/aim/modifier stacks/subpatterns.
- Add parameter curves, deterministic contextual bindings, and indexed RNG substream derivation for pattern/emitter/burst.

### Buildability target
- Pattern assets from multiple authoring forms execute identically once compiled.

### Exact build/run commands (Windows)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure -R PatternIR|PatternVM|DSL
.\build\Debug\EngineDemo.exe --scene pattern_vm_demo --verify-hash
```

## Phase 7 — Gameplay Systems (Enemy/Boss/Wave/Event + PBAGS + WorldGen Core)
### Scope
- Implement HFSM-first enemy AI, movement timelines, attack scheduling.
- Add boss phase/timer/HP trigger system with deterministic trigger ordering.
- Implement PBAGS core: style profiles, module pools, deterministic weighted selection, deterministic parameter synthesis, pacing templates, and transition segment generation.
- Implement deterministic world generation core: DAG node-map synthesis, world progression constraints, encounter pool selection, hazard assignment, reward distribution, and threat-budget validation.
- Integrate waves/hazards/upgrades/scoring with deterministic event flush.

### Buildability target
- Playable deterministic progression prototype with reproducible world graph generation plus boss encounter replay parity, including at least one generated PBAGS phase plan.

### Exact build/run commands (Windows)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure -R Boss|Gameplay|Replay
.\build\Debug\EngineDemo.exe --scene boss_prototype --record boss_run.rpl
.\build\Debug\EngineDemo.exe --playback boss_run.rpl --verify-hash
.\build\Debug\EngineDemo.exe --generate-boss-plan helix_regent --difficulty Hard --verify-constraints
.\build\Debug\EngineDemo.exe --generate-world-run --seed 12345 --verify-constraints --report worldgen_report.json
```

## Phase 8 — Rendering + Debug/Profiler Introspection + Scalability Pass
### Scope
- Implement custom instanced bullet renderer and render instance buffer pipeline.
- Add debug stream plumbing, provenance/explain overlays, collision grid overlays.
- Add profiler counters for sim phases and memory pool telemetry.

### Buildability target
- Runtime visualization builds with deterministic sim untouched and introspection active, with profiling attribution suitable for scalability tuning.

### Exact build/run commands (Windows)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo --output-on-failure
.\build\RelWithDebInfo\EngineDemo.exe --scene debug_overlay_demo
```

## Phase 9 — Editor, Content Pipeline, and Commercial Tooling
### Scope
- Deliver editor shell with canonical dockable layout/workspaces (top/left/center/right/bottom regions), dashboard landing page, and command-palette-first navigation.
- Deliver Pattern Graph tooling: node highlights, timeline strip, modifier-stack UI, breakpoints/watches, provenance click-through, and deterministic preview controls.
- Add PBAGS authoring tooling: style-profile editor, module pool/weight editors, lock-regenerate UI, segment rationale reports, and variant comparison view.
- Add world-generation tooling: world map graph preview/editor, path-bias controls, node-distribution validators, encounter-pool weighting tools, and multi-run balance simulation dashboard.
- Add AI-assisted pattern generator tooling: intent prompt panel, hard-constraint sliders, style preset controls, reference/remix modes, multi-candidate generation, explainability metadata, and selective regenerate workflows.
- Add asset import/compile pipeline, schema migration, validation/linting, and safe hot reload.
- Add replay debugger UX, fairness analysis tools, density/safe-lane overlays, templates, docs viewer, issue capture, and profiler-to-asset jump workflows.

### Buildability target
- Editor authors/compiles content and previews deterministic sim end-to-end, including AI-assisted pattern generation outputs as normal graph assets.

### Exact build/run commands (Windows)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo --output-on-failure
.\build\RelWithDebInfo\EngineEditor.exe
.\build\RelWithDebInfo\EngineEditor.exe --open boss_generator
.\build\RelWithDebInfo\EngineEditor.exe --open world_generator
.\build\RelWithDebInfo\EngineEditor.exe --open pattern_ai_generator
```

## Phase 10 — Polish, Release Readiness, and Optional Extensions (Feature-Flagged)
### Scope
- Add optional lockstep/rollback networking.
- Add mod SDK/loader and plugin packaging/runtime hooks.
- Deliver release-readiness polish: workflow hardening, docs/tutorial/reference pass, sample/template packaging, and installer/package validation.
- Add optional analytics hooks (strict opt-in, sim-isolated).
- Add optional advanced generator tiers (LLM blueprint suggestion and learned ranking) behind deterministic template-concretization boundaries.
- Add advanced UI modules: time-travel debugger compare view, difficulty lab, fairness HUD, and parameter-surface tooling.

### Buildability target
- Core remains deterministic and buildable with extensions disabled; optional modules compile behind flags.

### Exact build/run commands (Windows)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DENABLE_NETWORKING=ON -DENABLE_MODS=ON -DENABLE_PLUGINS=ON
cmake --build build --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo --output-on-failure
.\build\RelWithDebInfo\EngineEditor.exe
.\build\RelWithDebInfo\EngineEditor.exe --open boss_generator
.\build\RelWithDebInfo\EngineEditor.exe --open world_generator
.\build\RelWithDebInfo\EngineEditor.exe --open pattern_ai_generator --profile benchmark_25k
```


## Phase 1 — Decompose Runtime (Complete)
### What changed
- Extracted input processing and replay input integration into `InputSystem`.
- Extracted render-context lifecycle and frame rendering into `RenderPipeline`.
- Extracted gameplay simulation/update and upgrade UI logic into `GameplaySession`.
- Reduced `runtime.cpp` to orchestration-oriented loop and system wiring.

### Files added/edited
- Added: `include/engine/input_system.h`, `src/engine/input_system.cpp`
- Added: `include/engine/render_pipeline.h`, `src/engine/render_pipeline.cpp`
- Added: `include/engine/gameplay_session.h`, `src/engine/gameplay_session.cpp`
- Edited: `include/engine/runtime.h`, `src/engine/runtime.cpp`, `CMakeLists.txt`
- Edited: `docs/DecisionLog.md`, `docs/Architecture.md`, `docs/MasterSpec.md`, `docs/ImplementationPlan.md`


## Phase 2 — Fix Collision Architecture (Complete)
### What changed
- Added `CollisionTarget` and `CollisionEvent` data types to projectile collision interface.
- Split projectile update into motion/grid/resolution stages and retained legacy `update()` wrapper for compatibility.
- Wired gameplay collision flow to use multi-target query pipeline with deterministic event sorting.
- Added `collision_correctness_tests` coverage for hit/graze/allegiance/brute-force parity/deterministic ordering.

### Files added/edited
- Added: `tests/collision_correctness_tests.cpp`
- Edited: `include/engine/projectiles.h`, `src/engine/projectiles.cpp`
- Edited: `include/engine/entities.h`, `src/engine/entities.cpp`
- Edited: `include/engine/gameplay_session.h`, `src/engine/gameplay_session.cpp`
- Edited: `CMakeLists.txt`
- Edited: `docs/DecisionLog.md`, `docs/Architecture.md`, `docs/MasterSpec.md`, `docs/ImplementationPlan.md`


## Phase 3 — Harden Floating-Point Determinism (Complete)
### What changed
- Added strict floating-point compiler options to `engine_core` only.
- Added `include/engine/deterministic_math.h` (`dmath::sin/cos/atan2/sqrt`) as the simulation math choke-point.
- Replaced direct `std::sin/cos/atan2/sqrt` calls with `dmath::` wrappers in projectile, pattern graph, entity, pattern, and GPU bullet simulation files.
- Added `tests/determinism_cross_config_test.cpp` with a 300-tick deterministic hash assertion.

### Files added/edited
- Added: `include/engine/deterministic_math.h`, `tests/determinism_cross_config_test.cpp`
- Edited: `CMakeLists.txt`
- Edited: `src/engine/projectiles.cpp`, `src/engine/pattern_graph.cpp`, `src/engine/entities.cpp`, `src/engine/patterns.cpp`, `src/engine/gpu_bullets.cpp`
- Edited: `docs/DecisionLog.md`, `docs/MasterSpec.md`, `docs/ImplementationPlan.md`


## Phase 4 — Fix GpuBulletSystem (Complete)
### What changed
- Added free-list backed slot allocation and cached active count to `GpuBulletSystem`.
- Replaced O(N) `emit()` scan with O(1) free-list pop.
- Replaced O(N) `activeCount()` scan with O(1) cached return.
- Updated `update()` and `clear()` to recycle slots deterministically.
- Expanded tests with 100k emit + expire + refill cycle.

### Files added/edited
- Edited: `include/engine/gpu_bullets.h`, `src/engine/gpu_bullets.cpp`, `tests/gpu_bullets_tests.cpp`
- Edited: `docs/DecisionLog.md`, `docs/MasterSpec.md`, `docs/GpuBulletAssumptions.md`, `docs/ImplementationPlan.md`


## Phase 5 — Compact Active-List Iteration (Complete)
### What changed
- Added compact active index list and cached active count to `ProjectileSystem`.
- Converted motion, grid build, render, debug draw, graze collection, and debug hash iteration to O(active) traversal.
- Added deterministic sort-on-removal for active index ordering at tick end to stabilize replay/hash behavior.

### Files added/edited
- Edited: `include/engine/projectiles.h`, `src/engine/projectiles.cpp`
- Edited: `docs/DecisionLog.md`, `docs/ImplementationPlan.md`
- Edited: `docs/PerfTargets.md` (benchmark note)

## 2026-03-07 — GameplaySession architecture refactor (runtime boundaries)

### Scope implemented
- Split `GameplaySession` runtime ownership into six state partitions:
  - `SessionSimulationState` (tick/clock/frame allocator/rng streams)
  - `PlayerCombatState` (player transform/aim/health/radius)
  - `ProgressionState` (upgrade screen + focus + progression memo indices)
  - `PresentationState` (camera shake queue, particle FX, danger-field runtime)
  - `DebugToolState` (tool suite, debug/perf flags, profiling labels)
  - `EncounterRuntimeState` (collision target/event buffers and rolling collision counters)
- Updated runtime integration (`Runtime`) to consume tool state and tick state through the new boundaries.
- Added `gameplay_session_state_tests` to validate that getter APIs and partitioned ownership remain aligned.

### Migration / integration notes
- Existing call-sites continue to use stable `GameplaySession` methods (`playerPos()`, `dangerFieldEnabled()`, `consumeCameraShakeEvents()`, etc.).
- Internal members formerly accessed as top-level `GameplaySession` fields now route through partition roots (e.g., `simulation_`, `playerState_`, `presentation_`, `debugTools_`, `encounter_`, `progression_`).
- Replay and deterministic systems were kept in place; tick/replay logic now reads tick from `simulation_.tickIndex`.


## Editor Tools Decomposition — Delivered
- [x] Broke the monolithic `drawControlCenter` flow into modular panel methods aligned with tool domains.
- [x] Added explicit shared editor service methods for encounter asset build and panel state seeding.
- [x] Kept existing panel behavior and menu toggles intact (content browser, graph editor, encounter editor, palette/FX, validator, profiler, debug).
- [x] Preserved runtime/editor contract through `ToolRuntimeSnapshot` read-only consumption in panel methods.

### Migration notes
- Existing external API remained stable (`ControlCenterToolSuite` public methods unchanged).
- Internal contributors should add new tools as dedicated panel methods and avoid adding large inline UI blocks to `drawControlCenter`.

### Future extension points
- Introduce a lightweight panel registry that maps menu toggles to panel method delegates.
- Move shared panel state structs into dedicated editor service files once additional tooling lands.

## 2026-03-07 — Asset Pipeline: Sprite/Texture Import (Completed)

Completed items:

- Added source asset import registry model (`SourceArtAssetRecord`, `ImportedArtAssetRecord`) in `content_pipeline`.
- Implemented manifest parsing + validation for sprite/texture source assets and import settings.
- Added import fingerprinting, previous-pack reimport comparison, and dependency invalidation output.
- Added atlas build planning output grouped by atlas group and color workflow.
- Integrated import metadata into generated runtime packs (`sourceAssetRegistry`, `importRegistry`, `importInvalidations`, `atlasBuild`).
- Added automated coverage:
  - `content_import_pipeline_tests` for parse/import/atlas/fingerprint behavior.
  - updated `content_packer_tests` to assert new import metadata fields in produced packs.
- Added authored sample manifest and source art examples under `data/`.


## 2026-03-07 — Asset Pipeline: Animation + Variant import completion
- Extended art import schema to support animation clip identity and frame-level metadata:
  - `animationSet`, `animationState`, `animationDirection`, `animationFrame`, `animationFps`
  - `animationSequenceFromFilename` + configurable `animationNamingRegex`
- Extended variant authoring metadata:
  - `variantGroup`, `variantName`, `variantWeight`, `paletteTemplate`
  - optional configurable `variantNamingRegex` for filename extraction
- Added deterministic build plans emitted into packs:
  - `animationBuild`: grouped clips + ordered frame asset GUIDs + clip FPS
  - `variantBuild`: grouped variant options + weighted selection metadata + palette-template compatibility
- Added validation guardrails:
  - malformed naming/groups, invalid fps/frame/weights, duplicate frame indexes, duplicate variant names, inconsistent clip FPS.
- Added tests to enforce parse/import/grouping output and pack metadata availability.

## Audio subsystem implementation status
- Implemented core `AudioSystem` runtime service with SDL audio callback mixer, content-driven clip/event registration, and music loop support.
- Wired gameplay event hooks for: hit, graze, player damage, enemy death, boss warning, UI click, UI confirm.
- Added content pipeline integration by allowing `ContentPacker` to include `audio` section in generated pack and by parsing audio metadata through `parseAudioContentDatabase`.
- Added runtime config volume controls (`audioMasterVolume`, `audioMusicVolume`, `audioSfxVolume`) and command-line overrides (`--audio-master`, `--audio-music`, `--audio-sfx`).
- Added smoke-level tests covering audio content parsing and config volume parsing/overrides.

## 2026-03-07 — Gameplay authoring architecture expansion (enemy/boss/encounter runtime)
- Implemented enemy runtime decomposition improvements:
  - Added richer boss phase state in runtime instances (telegraph lead, cadence timer, sequence cursor, intro/phase transitions).
  - Added explicit `EntityRuntimeEvent` channel for simulation-to-presentation hooks.
- Boss architecture upgrades:
  - Multi-pattern sequence support per phase (`patternSequence`).
  - Per-phase cadence support (`patternCadenceSeconds`).
  - Deterministic phase lifecycle events + aggregate counters (`telegraphEvents`, `hazardSyncEvents`).
- Encounter system upgrades:
  - Extended graph types to include `Telegraph`, `HazardSync`, and `PhaseGate` nodes.
  - Compiler now emits owner metadata for routing (`boss`, `hazards`, `encounter`) and preserves deterministic ordering.
- Session integration:
  - Gameplay session now consumes `EntityRuntimeEvent` stream and maps it to audio/camera presentation hooks.
  - Encounter runtime state now tracks encounter clock and synchronization counters.
- Authoring integration:
  - Updated `data/entities.json` boss phases to include pattern sequence and cadence examples.
- Validation:
  - Updated boss and encounter tests to validate new runtime event and schedule metadata behavior.


## Phase 5 — Reposition GPU Bullet Path to CPU Mass Render (Complete)
- Renamed runtime terminology to remove GPU-simulation ambiguity:
  - `BulletSimulationMode::CpuDeterministic` -> `CpuCollisionDeterministic`
  - `BulletSimulationMode::GpuMassHybrid` -> `CpuMassRender`
  - `GpuBulletSystem` implementation class -> `CpuMassBulletRenderSystem` (with compatibility alias retained).
- Upgraded CPU mass path internals:
  - Added compact active-slot tracking (`activeSlots_`, `slotToActiveIndex_`) to avoid full-capacity scans during update/render.
  - Added explicit `preparedQuadCount()` metric for render-prep profiling.
- Validation:
  - Updated `gpu_bullets_tests` to cover renamed class usage and clear/reset bookkeeping checks.
