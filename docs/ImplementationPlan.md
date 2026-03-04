# Implementation Plan

## Planning Constraints
- Additive implementation only; keep repository buildable at each phase.
- Never rename modules once introduced.
- Deterministic sim/replay/hash verification precede advanced features.
- This phase remains documentation-only.



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
.\build\Debug\EngineApp.exe --headless --ticks 300
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
.\build\Debug\EngineApp.exe --record replay.rpl --ticks 3600
.\build\Debug\EngineApp.exe --playback replay.rpl --verify-hash
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
.\build\Debug\EngineApp.exe --scene motion_validation --verify-hash
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
.\build\Debug\EngineApp.exe --scene bullet_world_stress --verify-hash
.\build\Debug\EngineApp.exe --scene collision_stress_lab --verify-hash --report collision_profile.json
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
.\build\Debug\EngineApp.exe --scene pattern_vm_demo --verify-hash
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
.\build\Debug\EngineApp.exe --scene boss_prototype --record boss_run.rpl
.\build\Debug\EngineApp.exe --playback boss_run.rpl --verify-hash
.\build\Debug\EngineApp.exe --generate-boss-plan helix_regent --difficulty Hard --verify-constraints
.\build\Debug\EngineApp.exe --generate-world-run --seed 12345 --verify-constraints --report worldgen_report.json
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
.\build\RelWithDebInfo\EngineApp.exe --scene debug_overlay_demo
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
