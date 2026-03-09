# MasterSpec

## Build Notes
- 2026-03-09: Fixed `GameplaySession` upgrade-orchestration callback signature mismatch after refactor. Root cause: lambdas passed to `SessionOrchestrationSubsystem::updateUpgradeCadence(...)` and `applyUpgradeDebugOptions(...)` returned `TraitSystem::rollChoices()` (`std::array<Trait, 3>`) while subsystem contract expects `std::function<bool()>`. Minimal behavior-preserving fix: explicit `-> bool` lambdas now call `rollChoices()` for side effects and return `true` when invoked.

- 2026-03-09: Final external-facing documentation and package polish pass completed. Added first-class repository `README.md`, refreshed creator indexing (`AuthoringGuide` + `GettingStarted`), and aligned product-facing docs around the concrete build -> pack -> run -> replay-verify loop. Updated architecture/plan/decision/changelog/audit records to explicitly capture final creator workflow coverage and stale-doc cleanup outcomes.

- 2026-03-09: Polished vertical-slice content pass completed for showcase readiness. `sample_slice_stage_01` now escalates through a mixed elite lane (`Basic Enemy Shooter` + new `Vanguard Lancer`), tighter event downtime, and an authored boss phase order that explicitly includes `Spread Lattice` between `Wave Weave` and `Composed Helix`. This keeps encounter pacing authored through content-pack JSON while improving readability/variety and preserving deterministic replay workflow + packaging paths.
- 2026-03-09: Public API/extensibility closure finalized for commercial-v1 positioning. Public contract explicitly scoped to `include/engine/public/*`; plugin lifecycle expectations (metadata, compatibility preflight, registration/unregister/clear teardown, host ownership) and mod/content layering semantics (multi-pack order, GUID override, conflict logging) are now documented as the supported extension boundary.

- 2026-03-09: Audio subsystem finalization pass completed: authored audio parsing now enforces duplicate/unknown-reference/empty-path validation; runtime audio reconfiguration now frees old clip resources before reload and auto-starts configured looping music after content load. Audio dispatch remains presentation-side outside deterministic replay hashing.
- 2026-03-09: Finalized creator-facing content import closure for sprites/palettes/atlases/animation variants/audio. Art manifests now require `assetManifestType: "art-import"`, reject duplicate GUID/source-path records, normalize+validate atlas naming, and carry palette-template dependency tags into `importRegistry` invalidation metadata to make source-vs-runtime boundaries explicit in pack outputs.

- 2026-03-09: Editor tooling closure pass completed for commercial UX polish. Gameplay-focused tools were split from the pattern panel into a dedicated module (`editor_tools_gameplay_panel.cpp`) so responsibilities are now workspace shell/content flow, pattern authoring, gameplay encounter/projectile/trait editing, and shared services. Workspace now includes explicit workflow shortcuts (content/pattern/palette-diagnostics), dynamic content browser scanning, and clearer empty states for missing content selection.

- 2026-03-09: Renderer ownership closure pass finalized subsystem roles without behavioral changes: `render_pipeline` remains the sole projectile-path orchestrator, `gl_bullet_renderer` remains GL projectile draw-only, `render2d` remains shared SDL 2D primitive infrastructure, `modern_renderer` remains post-FX/compositing only, and `gpu_bullets` remains presentation-only CPU mass-render path (`CpuMassRender`) with no authoritative collision ownership.
- 2026-03-09: Finalized test infrastructure consistency model. All test executables now go through `engine_add_test_target(...)` + `engine_register_test(...)`; Catch classification uses source inspection only (no per-target safety overrides), Catch targets without `main(...)` link `Catch2::Catch2WithMain` and use `catch_discover_tests`, standalone tests keep explicit `main(...)` and register through `add_test(...)`. Windows runtime DLL deployment remains mandatory via `engine_deploy_runtime_dlls(...)` before test/discovery execution.
- 2026-03-09: Build/release reproducibility closure pass completed. CMake now deploys runtime DLL dependencies for top-level runtime tools (`EngineDemo`, `ContentPacker`) using the same `TARGET_RUNTIME_DLLS` post-build flow as tests, eliminating stale/manual DLL copy drift between clean and incremental builds. Portable manifest generation is now deterministic (stable sorted relative paths, static format header, no timestamp/path entropy) so repeated package runs from identical inputs produce identical manifest content.

- 2026-03-09: Immediate missing-main build fix completed for `content_packer_tests`, `entity_tests`, and `boss_phase_tests`. Root cause was helper safety-override drift: `engine_link_catch_main_if_needed(...)` only force-classified two legacy targets, leaving `boss_phase_tests` exposed when Catch classification bypassed local include detection. Applied minimal CMake change to extend the existing override set to include `boss_phase_tests` (still guarded by `NOT main(...)`) so all three targets reliably link `Catch2::Catch2WithMain` when needed. Preserve target names, test registration shape, and runtime-DLL deployment; perform a clean rebuild from a deleted build directory to fully validate regenerated link state.
- 2026-03-08: Finished missing-main follow-up for `content_packer_tests` and `entity_tests` by hardening `engine_link_catch_main_if_needed(...)` with a minimal target-name override: if either target does not define `main(...)`, CMake now forces Catch classification and links `Catch2::Catch2WithMain`. Root cause was test-target classification drift in the shared helper path for these named targets.
- 2026-03-08: Completed Catch2 main-link coverage across all test-target creation paths. CMake now detects both Catch includes and whether the source defines `main(...)`; only Catch tests without their own `main` are linked with `Catch2::Catch2WithMain`. This closes the remaining gap for manual targets such as `content_packer_tests` and prevents future missing-main drift outside `engine_add_test(...)`.
- 2026-03-08: Test registration now uses a single `engine_add_test(...)` path that auto-detects Catch-based sources by include (`#include <catch2/...>`), links them with `Catch2::Catch2WithMain`, and routes them through `catch_discover_tests`; non-Catch tests remain plain `add_test(...)`. Root cause was helper-path drift where Catch tests could be registered via plain-test wiring and miss a main at link time on Windows.
- 2026-03-08: Windows/MSVC build compatibility hardening: project-wide `NOMINMAX` and `WIN32_LEAN_AND_MEAN` compile definitions are now applied on `WIN32` to prevent `<windows.h>` `min`/`max` macros from colliding with `std::min`/`std::max` in engine headers/sources. Clean rebuild required after this definition change so all translation units pick up the new preprocessor state.
- 2026-03-08: Public plugin API now includes host-facing compatibility/introspection helpers (`isPluginTargetCompatible`, `pluginRegistrationErrorMessage`) and explicit ownership/ABI boundary notes in `engine/public/plugins.h`; this tightens lifecycle expectations without exposing registry internals or changing runtime behavior.
- 2026-03-08: Final creator-facing documentation pass completed. External onboarding now has explicit workflows for getting started, build/run, asset import, palette+grayscale shading, pattern authoring, encounter/boss authoring, replay/debug, audio, sample project usage, troubleshooting, plugin/mod integration, and creator performance guidance.
- 2026-03-08: Release packaging/productization hardening completed. Portable packaging now auto-bundles discovered runtime DLL dependencies from build output, emits a SHA-256 release manifest (`RELEASE_MANIFEST.txt`), and enforces sample-pack replay validation from the packaged bundle. Release validation now checks both artifact presence and manifest coverage for critical binaries/content.
- 2026-03-08: Runtime run-structure boot now consumes authored `encounters[].zones[]` from the selected content pack when available, falling back to defaults only if no encounter data is present. This aligns the playable vertical slice flow (combat/elite/event/boss sequencing) with content-pipeline authored data instead of hardcoded stage layouts.
- 2026-03-08: Build reliability verification pass completed. Clean reconfigure/build from a deleted `build/` directory and incremental rebuild checks (`touch` representative header/cpp + rebuild) both behaved consistently in Ninja; expected workflow remains delete build dir, reconfigure, rebuild, then run tests.
- 2026-03-08: Added `CMAKE_CONFIGURE_DEPENDS` tracking for `version/VERSION.txt` so project version and generated `generated/engine/version.h` are reconfigured automatically when the version file changes, reducing stale configure state risk.
- 2026-03-08: Public API/plugin boundary hardening added metadata-based plugin registration, compatibility gating against `publicApiVersion()`, duplicate-id protection, and explicit unregister/clear lifecycle endpoints while preserving existing engine run behavior.

## Public API / Extensibility Baseline (2026-03-08)
- Stable external boundary remains limited to `include/engine/public/*`; all `include/engine/internal/*` and `src/engine/*` details are explicitly non-contract internals.
- Plugin contract now uses metadata-first registration (`PluginMetadata`) with explicit target API version checks at registration time.
- Plugin lifecycle supports register, unregister-by-id, and `clearRegisteredPlugins()` for host-managed teardown and test isolation.
- Registration rejects null plugins, missing ids, duplicate ids/instances, and incompatible major/minor API targets to preserve runtime safety without exposing runtime internals.

- 2026-03-08: Editor tooling architecture was decomposed from the single `src/engine/editor_tools.cpp` unit into modular editor translation units under `src/engine/editor/` (`editor_tools_core`, `editor_tools_workspace_panel`, `editor_tools_pattern_panel`, and `editor_tools_services`).
- 2026-03-08: Shared editor services were formalized around content generation and validation (`generateDemoContent`, `runControlCenterValidation`) so panels consume service APIs instead of embedding filesystem/schema logic inline.
- 2026-03-08: Extension seam preserved and clarified: plugin panels still render through `public_api::toolPanelPlugins()`, while new editor domains can be added as isolated panel/source units without expanding a central monolith.

- 2026-03-08: Pattern editor panel was further split into focused submodules inside `editor_tools_pattern_panel.cpp` so generation controls, seed/testing controls, graph editing (palette + inspector), and preview/analysis each have dedicated draw/build helpers while keeping one coherent panel workflow.
- 2026-03-08: Pattern extension points are now explicit (`drawPatternGenerationControls`, `drawPatternSeedAndTestingControls`, `drawPatternGraphNodePalette`, `drawPatternGraphNodeInspector`, `buildPatternPreviewAsset`, `drawPatternPreviewAndAnalysis`) to reduce coupling and simplify future tooling additions (e.g., new analysis overlays or generator presets).


## Build Notes
- 2026-03-09: Audio subsystem finalization pass completed: authored audio parsing now enforces duplicate/unknown-reference/empty-path validation; runtime audio reconfiguration now frees old clip resources before reload and auto-starts configured looping music after content load. Audio dispatch remains presentation-side outside deterministic replay hashing.
- 2026-03-08: Removed generic content pipeline -> runtime audio header coupling by extracting audio pack schema types into `audio_content.h`; `content_pipeline.h` no longer includes `audio_system.h`, so SDL_mixer headers are no longer required for generic content/tool include paths. Follow-up: keep runtime playback APIs confined to `audio_system` and avoid reintroducing SDL-facing includes into content headers.
- 2026-03-08: Consolidated third-party CMake dependency registration into a single `engine_register_dependency(...)` workflow and one `FetchContent_MakeAvailable(${ENGINE_FETCHCONTENT_DEPENDENCIES})` materialization pass. This replaces fragmented declaration patterns with one auditable dependency list while preserving existing targets (`SDL2`, `SDL2_mixer`, `imgui`, `nlohmann_json`, `Catch2`) and downstream link wiring.
- 2026-03-08: `ContentPacker` dependency surface was re-audited after audio schema extraction to `audio_content.h`; it now links only `nlohmann_json` and no longer links SDL2/SDL2_mixer because pack-generation code does not invoke runtime audio APIs.
- 2026-03-07: Build hygiene hardening pass completed. CMake now rejects in-source builds to prevent cache/source-tree contamination, ensures `${CMAKE_BINARY_DIR}/generated/engine` exists before emitting configured headers, and removes duplicate `FetchContent_MakeAvailable` calls so dependency initialization is single-pass and deterministic across clean/incremental configure runs.
- 2026-03-07: Clean rebuild workflow baseline is now: remove build tree, reconfigure out-of-source, build, then run tests (`cmake -E remove_directory build`, `cmake -S . -B build ...`, `cmake --build build ...`, `ctest --test-dir build ...`).



## Vertical Slice Validation Baseline (2026-03-08)
- The product validation slice is defined as **Ember Crossing** (combat lanes + boss) and **Seraph Rematch** (replay-focused boss rematch) authored in pack content (`data/entities.json`, `data/patterns.json`, `data/encounters.json`, `data/audio.json`).
- Required validation coverage for this slice: player combat cadence, projectile readability (palette/shape differentiation + danger field), staged enemy encounters, boss multi-phase sequencing, deterministic replay record/playback, content pack compile/load, runtime audio events, HUD/upgrade UI flow, and packaging scripts.
- This slice is the release-candidate smoke baseline for product coherence checks and must remain data-authored (no one-off runtime hacks).

## 1. Scope and Authority
This is the merged authoritative specification for a standalone Windows bullet-hell engine. All provided design documents are source-of-truth and must be integrated additively.

## 2. Non-Negotiable Constraints
- Target: Windows 10/11 x64.
- Keep repository buildable after every phase.
- Additive evolution only; never rename created modules.
- Determinism, replay parity, and high-throughput projectile simulation are first-class constraints.

## 3. Layered Engine Architecture
### 3.1 Core Runtime (Platform + Kernel)
Responsibilities:
- Platform abstraction (window, input, audio, threads, files).
- Job system + synchronization primitives.
- Memory subsystem (arenas, pools, allocators, telemetry).
- Time/clocking and frame pacing.
- Logging, asserts, crash capture.
- Deterministic utilities (fixed RNG, fixed math helpers).

Rule: no gameplay dependencies.

### 3.2 Simulation Layer (Deterministic Sim)
Responsibilities:
- Fixed-step world simulation.
- Deterministic entity scheduling and updates.
- Specialized Bullet World.
- Collision and queries.
- Deterministic event queue.
- Snapshot/hash/checkpoint generation.

Rule: pure state output only (no render/platform calls).

### 3.3 Gameplay Systems Layer
Responsibilities:
- Player deterministic intent processing.
- Enemy AI (HFSM default; optional BT).
- Boss phase/timeline orchestration.
- Upgrades/mutations/scoring/economy hooks.

Rule: stable IDs and data-driven references.

### 3.4 Pattern Layer (Authoring IR + Runtime VM)
Responsibilities:
- Graph/timeline/DSL authoring inputs.
- Compilation to unified Pattern IR + bytecode.
- Deterministic VM execution and binding.

Rule: authoring flexibility; runtime predictability.

### 3.5 Rendering Layer
Responsibilities:
- Instanced/batched sprite/mesh rendering.
- Materials/atlases.
- Debug overlays.

Rule: render consumes sim snapshots; no sim feedback.

### 3.6 Asset/Content Pipeline
Responsibilities:
- Import authored content.
- Compile immutable runtime packs.
- Validation/lint/schema versioning/migrations.
- Optional mod pack production.

### 3.7 Editor/Tooling Layer
Responsibilities:
- Graph/timeline editors and inspectors.
- In-editor deterministic preview.
- Replay debugger + profiler.
- Hot reload with safe swap boundaries.

Rule: tooling integrates through stable Editor API.

## 4. Deterministic Simulation Contract

### 4.x Floating-Point Determinism Policy (Phase 3)
- `engine_core` is compiled with strict FP determinism flags:
  - MSVC: `/fp:strict`
  - GCC/Clang: `-ffp-contract=off -fno-fast-math`
- Simulation trig/root math routes through `engine::dmath` wrappers (`deterministic_math.h`) instead of direct `std::sin/cos/atan2/sqrt` calls in hot simulation code.
- Wrapper indirection exists as a deterministic choke-point for future fixed-precision or polynomial replacement without broad call-site churn.
- A cross-config determinism test (`determinism_cross_config_test`) runs 300 deterministic ticks and verifies a hard-coded hash to detect configuration drift.

### 4.1 Time and Tick Model
- 2D simulation with fixed timestep `Δt` (typical `1/60` or `1/120`).
- Integer tick index `n`; simulation time is `t = n·Δt`.
- Math mode:
  - Preferred fixed-point for strongest cross-platform parity.
  - Deterministic-float mode allowed with strict platform/toolchain restrictions and validation.
- Accumulator stepping: while accumulator >= Δt, run `SimTick()`.

### 4.2 Canonical Tick Order
1. Input sampling to deterministic command buffer.
2. PreSim flush (spawn/despawn/upgrades/difficulty scalars).
3. AI/scheduling (enemy AI, boss transitions, pattern VM).
4. Motion integration.
5. Collision broadphase+narrowphase and deterministic collision queue.
6. PostSim apply damage/deaths/score/events.
7. Emit snapshot + state hash.

### 4.3 Determinism Guarantees
- No variable-delta sim.
- Stable iteration/sort order.
- Deterministic RNG stream discipline.
- Deterministic math policy enforcement.
- Per-tick state hashing for replay/desync/debug.

## 5. Bullet Motion Mathematics
### 5.1 Core Kinematics
- Position `p(t)=(x(t),y(t))`, velocity `v(t)` in 2D.
- Linear motion:
  - Continuous: `p(t)=p0 + v·t`
  - Discrete: `p[n+1]=p[n] + v[n]·Δt`
- Constant acceleration:
  - `v[n+1]=v[n] + a·Δt`
  - `p[n+1]=p[n] + v[n+1]·Δt` (semi-implicit Euler).

### 5.2 Angular/Curvilinear Motion
- Heading form `v = s·(cos θ, sin θ)`.
- Angular velocity update: `θ[n+1]=θ[n] + ω·Δt`.
- Position: `p[n+1]=p[n] + s·(cos θ[n+1], sin θ[n+1])·Δt`.

### 5.3 Sinusoidal Models
- Lateral offset model around forward axis:
  - `p_base(t)=p0 + f·s·t`
  - `offset(t)=u·A·sin(2πft + φ)`
  - `p(t)=p_base(t)+offset(t)`
- Heading perturbation model:
  - `θ(t)=θ0 + A·sin(2πft + φ)`.

### 5.4 Orbit Motion
- Around center `c(t)` with radius `R` and phase `α`:
  - `p(t)=c(t)+R·(cos α, sin α)`
  - `α[n+1]=α[n] + ω·Δt`
- Supports drifting centers (`c(t)=c0 + vc·t`).

### 5.5 Homing with Turn Limits
- Desired heading from target direction.
- Per-tick angular clamp:
  - `Δθ = clamp(angleDiff(θd,θ), -ωmax·Δt, +ωmax·Δt)`.
- Update heading, then velocity from heading.

### 5.6 Bézier Paths
- Quadratic and cubic Bézier path support for authored trajectories.
- Lifetime-normalized parameter `u = clamp(age/L, 0..1)`.
- Position set by `B(u)` each tick; derivative `B'(u)` used when velocity is required.

### 5.7 Composable Motion Stack (Canonical Order)
1. Steering/turning (homing/turn modules).
2. Velocity integration (linear/accel/angular).
3. Parametric offsets (sin/orbit/path overlays).
4. Constraints (speed clamps, boundaries).
5. Final position commit.

## 6. Emission Pattern Algorithms
### 6.1 Canonical Emission Event
Each spawn event captures:
- Spawn tick.
- Origin.
- Angle/direction.
- Speed.
- Archetype and init params.
- RNG substream reference.

Emitter parameters include `N`, `θ0`, spread `Δ`, rotation rate `Ω`, and interval `I`.

### 6.2 Standard Emitters
- Radial burst: `θi = θ0 + 2π·i/N`.
- Arc burst: `θi = θ0 - Δ/2 + Δ·i/(N-1)`.
- Rotating rings: `θ0(n)=θbase + Ω·n·Δt`; spawn when `n mod I = 0`.
- Aimed shots: `θ0 = atan2(player.y-origin.y, player.x-origin.x)`.
- Shotgun variants: even or center-weighted spreads.
- Wave emitters: time-modulated angle/rate.
- Lattice/grid emitters for curtains and synchronized fields.

### 6.3 Spiral Families
- Burst spiral via rotating emitter (`θ0(k)=θstart + k·Δθ`).
- Parametric spiral loci (`r=a+b·t`, `θ=θstart+ω·t`) with spawn-on-curve or on-rail behavior.
- Spiral acceleration supported via cumulative/analytic integration of `Ω(t)`.

## 7. Parametric Pattern System
### 7.1 Exposed Parameters
Typical designer parameters: `N, Δ, Ω, I, S, A, L, a, ωturn, phase` and variant-specific controls.

### 7.2 Deterministic Curve/Binding Model
- Parameters can be time/phase functions sampled deterministically each tick.
- Deterministic bindings permitted from:
  - Difficulty tier.
  - Boss HP%/phase.
  - Player distance (if sampled deterministically).
  - Deterministic noise fields.

### 7.3 Useful Derived Metrics
- Burst throughput: bullets/sec ≈ `N / (I·Δt)`.
- Ring spacing at radius `r`: ≈ `(2πr)/N`.
- Arc spacing: ≈ `Δ/(N-1)`.

## 8. Procedural Pattern Modifiers
- Rotation and oscillation modifiers can alter angle/origin over time.
- Split modifiers (timer/hit/death) enqueue deterministic child-spawn commands at tick boundaries.
- Delayed telegraph spawning separates warning marker from delayed spawn using stored seeds/params.
- Layer phase offsets support interference/weave patterns.
- Constraint-based modifier (advanced) solves bounded params (`Δ,N,Ω` etc.) via deterministic iterations for fairness goals.

## 9. Pattern Composition
- Nested patterns (emitters spawn emitters/subpatterns).
- Seed derivation for child instances via deterministic hashing.
- Layered concurrent emitters with shared phase clock and independent params.
- Attack timelines as clip sequences with deterministic trigger order/tie-breaking by stable IDs.

## 10. Targeting Algorithms
- Direct aiming via `atan2`.
- Predictive lead by solving intercept quadratic for smallest positive time.
- Homing with bounded angular velocity and optional proportional steering.
- Difficulty-scaled adaptive tracking blends direct/predictive/lagged strategies deterministically.

## 11. Deterministic RNG Discipline
- Pattern-instance stream seeds derive from run seed + asset/instance IDs.
- Emitter and burst substreams derived from parent stream IDs.
- Prefer indexed randomness (hash-derived values) to avoid call-order divergence.
- Replay stores run seed + input stream + content hash/version (+ optional periodic hashes).

## 12. Entity and Bullet Runtime Model
### 12.1 Hybrid ECS + Bullet World
- ECS handles general entities.
- Bullet World handles projectile-heavy workloads.

### 12.2 Bullet Data Structures
- Immutable `BulletArchetype` runtime descriptors.
- SoA bullet pools with fixed-capacity arrays.
- Preallocated command buffers and free-list reuse.
- No per-bullet dynamic allocations in hot path.

### 12.x Runtime SoA and Spawn Queue Baseline (Phase 7)
- `ProjectileSystem` hot-path iteration uses a compact active-index list (`activeIndices_` + `indexInActive_`) over free-list slots to avoid full-capacity scans during update/render/debug traversal.
- Split/spawn deferrals use a fixed-capacity pending spawn queue (`kMaxPendingSpawns = 4096`), flushed at deterministic tick boundaries.
- Pending-spawn overflow policy is deterministic: additional requests beyond 4096 in a tick are dropped in-order (first 4096 retained), preserving replay parity.
- Current SoA additions used by rendering/FX include:
  - `paletteIndex_`
  - `shape_`
  - `trailX_` / `trailY_`
  - `enableTrails_`
  - `trailHead_`

### 12.3 Collision and Spatial Optimization
- Uniform grid/spatial hash broadphase.
- Narrowphase checks for circle/capsule/OBB; stable deterministic resolution ordering.
- Collision layers and cheap deterministic culling rules (bounds/lifetime).
- Batch updates by archetype/program/layer for locality and branch reduction.

## 13. Data-Driven Content and Asset Pipeline
- Author-time assets (graph/timeline/configs) compile to runtime packs.
- Runtime pack includes asset DB, string table, dependencies, schema version, content hash.
- Hot reload recompiles changed assets and swaps at safe deterministic points.
- Optional JSON export for modding/diff workflows.

## 14. Memory and Performance Contract
- No GC-dependent behavior in simulation runtime.
- Frame arenas, pools, chunk allocators, ring buffers.
- Telemetry: high-water marks, exhaustion, per-tick allocation violations.
- Pre-size pools by budgets; growth allowed in editor/loading only.
- Degradation strategy preserves sim correctness first (reduce visuals/effects before sim fidelity).

### 14.x Implemented Sim-Tick Allocation Contract (Phase 7)
- `Runtime::simTick()`/`GameplaySession::simulateTick()` deterministic-path execution is zero-allocation by contract.
- Fixed-capacity arrays/queues in hot simulation include (non-exhaustive):
  - projectile pending spawns (`4096`),
  - projectile despawn events (`4096`),
  - gameplay collision targets (`512`),
  - gameplay collision events (`8192`).
- Overflow behavior must remain deterministic and non-allocating:
  - cap writes at fixed buffer capacity,
  - increment diagnostics counters/logging hooks,
  - continue simulation without heap fallback.
- Any overflow-induced drops are treated as authoring/perf-budget violations, not runtime reasons to allocate.

## 15. Rendering Strategy
- OpenGL bullet renderer (`gl_bullet_renderer`) is the primary high-throughput path for gameplay-authoritative projectiles.
- CPU collision/determinism path (`CpuCollisionDeterministic`) remains replay-authoritative.
- CPU mass bullet render path (`CpuMassBulletRenderSystem`) is a presentation-oriented mass-quad path, not GPU compute simulation.
- Render remains presentation-only.

### 15.x Renderer Path Ownership Map (2026-03-08)
- `render_pipeline` owns render-frame orchestration and route selection between renderer backends; simulation ownership stays in `GameplaySession`/`ProjectileSystem`.
- `gl_bullet_renderer` owns only the OpenGL projectile draw backend (vertex/index generation + draw submission) for deterministic projectile snapshots.
- `render2d` owns generic 2D presentation primitives (`Camera2D`, `SpriteBatch`, `DebugDraw`, `TextureStore`) shared by gameplay and tooling overlays.
- `modern_renderer` owns optional scene/post-processing composition (offscreen buffers + post-fx), independent from projectile simulation/path decisions.
- `gpu_bullets` (current `CpuMassBulletRenderSystem`) is explicitly an alternate CPU mass-render path and does not own authoritative collision/deterministic projectile state.
- Roadmap note: if a future authoritative GPU-simulation path is introduced, it must be a distinct simulation mode with independent replay/hash contracts rather than an extension of current mass-render presentation systems.
- `RenderPipeline::ProjectileRenderPath` is the canonical projectile presentation selector (`Disabled`/`ProceduralSpriteBatch`/`GlInstanced`) to avoid split routing logic across call sites.

### 15.x Roadmap Notes
- Keep `GpuBulletSystem` alias only for compatibility; prefer `CpuMassBulletRenderSystem` in new code and docs until alias removal is scheduled.
- Revisit true GPU-sim naming only when compute/SSBO ownership lands as a separate simulation path with replay guarantees.

### 15.x Implemented GL Bullet Colorization Pipeline (Phase 7)
- Bullet visuals follow a deterministic authoring-to-render pipeline:
  1. `GrayscaleSpriteAtlas` generates grayscale SDF-like bullet shape atlas pages.
  2. `PaletteRampTexture` uploads per-palette ramp rows to GPU LUT texture.
  3. GLSL bullet shader samples atlas intensity + palette ramp row and applies runtime colorization/animation.
  4. `GlBulletRenderer` emits one indexed draw call for all active bullets in the frame (`glDrawElements`).
- CPU builds dynamic vertex/index data from projectile SoA each frame into preallocated GPU buffers.
- Hard fallback path remains available: if GL context/shader resources are unavailable, projectile rendering routes through `SpriteBatch::renderProcedural`.

### 15.x CPU Mass Bullet Render Runtime Note (Phase 5)
- `CpuMassBulletRenderSystem` replaces ambiguous naming previously implying GPU simulation.
- Runtime uses O(1) free-list emission and compact active-slot iteration for update/render traversal.
- Prepared quad count is tracked explicitly (`preparedQuadCount`) for profiling clarity.
- Mode naming is explicit: `CpuCollisionDeterministic` vs `CpuMassRender`.

## 16. Replay, Debugging, and Introspection
- Replay record/playback uses authoritative per-tick input stream and deterministic settings.
- Determinism checks compare recorded/current state hashes and capture first divergence tick.
- Debug Stream ring buffer keyed by tick + IDs captures spawn/kill/collision/state transitions/markers.
- Tools include bullet provenance, path history, AI trace, collision ordering/grid overlays, and timeline scrubbing.

## 17. Boss Pattern Design System
- Phases define enter conditions, active timelines, exit conditions, and transition rules.
- HP/timers sampled at tick boundaries (integer tick counters, no variable wall time).
- Escalation via controlled ramps (`N`, `I`, `S`, targeting sophistication, layered modifiers).
- Transitions require readability controls (telegraph windows, optional cancel/conversion behavior).
- Roguelite variation via deterministic pattern genomes/mutations with fairness constraints.

## 18. Extensibility, Modding, and Safety
- Plugin extension points for runtime systems, editor panels, importers, and pattern compiler modules.
- Mod packs can add/override data content with dependency/schema checks.
- Safety rules:
  - No wall-clock access in sim.
  - Deterministic RNG/tick-phase declarations.
  - No per-tick dynamic allocations in shipping builds.
  - Versioned schemas + migrations.

## 19. Commercial Tooling and UX Requirements
- Pattern/projectile/enemy/boss/wave editors with deterministic preview.
- Replay debugger, fairness tools, and bullet-domain profiler.
- Validation/linting for telegraphing, density, lane guarantees, and dependencies.
- Dockable pro editor UI and built-in docs/templates/benchmarks.
- CI gates for determinism/performance/fairness.
- Optional analytics must be explicit opt-in and separated from core sim.


## 20. Pattern Graph System Specification
### 20.1 Pattern Graph Purpose and Model
- Pattern Graph is a domain-specific visual scripting system specialized for bullet-hell authoring.
- It combines dual-flow semantics:
  - Execution flow: deterministic sequencing and scheduling of actions.
  - Data flow: typed parameter/value computation for emissions and modifiers.
- Authoring goals: fast iteration, deterministic replays, compile-time safety, and high runtime performance.
- Runtime model: all graphs compile to deterministic Pattern IR/bytecode executed by Pattern VM.

### 20.2 Graph Documents and Entry Context
Supported graph document types:
- Pattern Graph (primary runtime authoring document).
- Timeline Graph (embedded or referenced scheduling/keyframe tracks).
- Macro/Subgraph documents (reusable typed components).

Entry nodes include:
- `OnPatternStart`
- `OnPhaseEnter(PhaseName)`
- `OnMarker(MarkerName)`
- `OnEvent(EventType)`

Entry context pins include owner/target refs, difficulty tier, seed handle, tick index, and local phase time.

### 20.3 Exec/Data Flow and Deterministic Merge
- Execution links define deterministic control flow; parallel branches are compiled with stable merge rules (node GUID deterministic sort).
- Data links are strongly typed (`Scalar`, `Int`, `Bool`, `Angle`, `Vector2`, `Curve`, `RNGStream`, refs).
- Type conversion is explicit via conversion nodes only.

### 20.4 Pattern Outputs and Emission Semantics
Pattern graph outputs are deterministic side effects:
- Spawn commands (`SpawnBullet`, `SpawnLaser`, `SpawnHazard`).
- Deterministic events/markers.
- Parameter/state set requests.
- Non-sim FX requests.

Emission nodes execute only when Exec input fires, compute deterministic bullet batches, and write to preallocated spawn command buffers.

### 20.5 Core Node Families
#### Emission Nodes
Common pins: `ExecIn/Out`, `Origin`, `Aim`, `Projectile`, `Speed`, `Count`, optional `Seed`, `Tags`.
Required primitives:
- Radial Burst.
- Arc Spread.
- Spiral Emitter.
- Aimed Shot.
- Wave Emitter.
- Grid/Lattice Emitter.

#### Timing Nodes
- `Wait/Delay` (tick-based with explicit seconds->tick rounding policy).
- `IntervalLoop` (bounded/unbounded).
- `Timeline`.
- `PhaseTrigger`.

#### Transform/Targeting/Utility Nodes
- Transform: rotate, position offset, count scale (deterministic rounding), speed modify.
- Targeting: direct aim, predictive aim with fallback policy, deterministic random direction, nearest-target with stable tie-break.
- Utility: deterministic random node (indexed mode), parameter expose node, difficulty scaling, seed split.

### 20.6 Modifier Stack Contract
- Emission nodes expose `ModifiersIn` stack and optional per-bullet modifier pin.
- Modifier stack supports reorder/toggle with deterministic compile order.
- Required modifiers:
  - Spiral Rotation.
  - Oscillating Angle.
  - Split Bullet.
  - Delayed Spawn.
  - Speed Ramp.
- Canonical modifier execution order:
  1. Parameter modifiers.
  2. Angle modifiers.
  3. Position modifiers.
  4. Post-spawn behavior bindings.

### 20.7 Timeline Integration
- Embedded timeline strip supports playhead/scrub/zoom and tracks for exec clips, parameter curves, and markers.
- Clip types: burst, loop, ramp, gate.
- Curve/keyframe types: linear, smooth, stepped.
- Deterministic sampling via fixed-step lookup or deterministic interpolation.
- Timeline emits marker/phase start/end events with fixed-order trigger resolution.

### 20.8 Composition and Reuse
- Subpattern node instantiates child pattern instances with origin/aim/param overrides/seed.
- Library system requirements: tags, search, favorites, thumbnails, metadata, dependency view.
- Macro graphs use typed I/O and compile-time expansion (no runtime overhead).
- Layering: named subgraph layers compile to deterministic parallel streams merged by stable ordering.

### 20.9 Parameter and Inspector Model
- Parameter types: scalar, int, bool, angle, vector2, curve ref, enum.
- Metadata: name/description/range/step/grouping/per-difficulty overrides/runtime lock.
- Inspector panels must provide:
  - Exposed parameter editing.
  - Live resolved values during preview.
  - Validation warnings for budget/range risks.
- Binding sources: constants, curves, difficulty scalers, deterministic context vars, deterministic noise.

### 20.10 Visual Debugging and Preview UX
- Node execution highlighting, loop iteration counters, wait remaining time, timeline firing indicators.
- Bullet provenance drill-down: source node, applied modifier stack, resolved parameters at spawn tick.
- Playback controls: play/pause/step/scrub, slow-mo, seed reset/lock, bot profile selection.
- Heatmaps: parameter influence and spatial bullet density; optional safe-lane overlays.
- Breakpoints and watches on node/pin execution with short trace window.

### 20.11 Graph Compilation Pipeline
Stages:
1. Validation (types/cycles/forbidden nondeterminism/budget estimates).
2. Lowering (exec to schedule blocks; timeline to WAIT/LOOP + curves; data to constants/registers).
3. Optimization (constant fold, dead-node elimination, batch grouping).
4. Codegen (Pattern IR + tables + debug mapping node GUID to IR ranges).

Runtime requirements:
- IR cached by content hash.
- Shared immutable IR across instances.
- Small per-instance state (`PC`, locals, RNG counters, time cursor).
- No runtime dynamic allocations in VM hot path.
- If multithreaded, use thread-local spawn buffers merged by stable keys.

### 20.12 Designer Workflow Requirements
- New pattern flow: create graph, wire loop+emitters+modifiers, expose params, save with thumbnail.
- Instant preview: difficulty selection, overlays, tick stepping, provenance click-through, hot reload at safe boundaries.
- Reuse flow: drag from library into boss phases, override per phase, extract/reuse macros with versioned propagation.

### 20.13 Advanced/Innovative Graph Features
- AI-assisted constrained pattern generation with multi-candidate outputs and cost estimates.
- Deterministic mutation tools for roguelite variants.
- Auto-difficulty scaling over designated parameters with generated reports.
- Pre-sim density estimation and spike prediction overlays.
- Dodge-path/fairness simulation overlays linked to timeline timestamps and source nodes.
- 2D parameter surfaces for coherent multi-parameter tuning.


## 21. Procedural Boss Attack Generation System (PBAGS)
### 21.1 Definition and Goals
- PBAGS generates boss phases automatically from vetted modular pattern components under explicit constraints.
- Generated artifacts include:
  - Phase timelines (attack/recovery/transition sequencing).
  - Pattern instances (module selections + parameters + modifiers).
  - Transition logic (HP/time/event/performance-gated variants).
  - Deterministic variants across difficulty tiers and roguelite seeds.
- Outputs are editable assets: designers can lock, regenerate subsets, and hand-tune.

### 21.2 High-Level Generation Flow
1. Designer selects style profile, difficulty bounds, module pool, and constraints.
2. Generator creates phase plan (count, durations, escalation curve).
3. Each phase is synthesized into timeline segments from weighted attack pools.
4. Fast analyzers and optional deterministic bot simulation validate output.
5. System emits Boss Phase Asset + explainable generation report.

### 21.3 Runtime-Facing Data Model
Required engine representation:
- `BossAttackPlan`
  - `seed`, `difficultyTier`, `styleProfileId`
  - `phases[]`
- `PhasePlan`
  - `phaseId`, `hpRange`, `timeCap`, `escalationLevel`
  - `segments[]`, `transitionRules[]`
- `AttackSegment`
  - `segmentType` (`Primary`, `Secondary`, `Transition`, `Recovery`)
  - `startTick`, `durationTicks`
  - `patternInstances[]` (`moduleId`, params, modifiers, anchors)
  - `constraintsSnapshot`

### 21.4 Phase/Segment Architecture
- Boss fight is composed of phases.
- Each phase has:
  - Entry (telegraph/reposition/rule-change signal).
  - Active timeline loop (primary + optional secondary/hazard layering).
  - Exit (cleanup/cancel/convert/reposition).
- Attack segments contain:
  - Duration window.
  - One primary attack.
  - Optional secondary pressure layer.
  - Recovery/breather windows.

### 21.5 Deterministic Timing Structures
- Fixed-tick timelines only.
- Segment durations align to a beat grid (e.g., 0.25s quantization).
- Pulse windows define burst microstructure (0.2s, 1.0s cycles, etc.).
- Breather quota policy enforced: every X seconds provide at least Y seconds reduced pressure.

### 21.6 Pattern Building Blocks and Module Taxonomy
Modules are pre-authored, compile to Pattern Graph/IR templates, and expose parameters/modifier slots.

Baseline module families:
- Radial: ring, arc, rotating ring, petal/rose.
- Spiral: constant spiral, accelerating spiral, double helix, spiral+wave.
- Targeted: direct, predictive, turn-limited homing, delayed punish targeting.
- Barrage/Curtain: line sweeps, curtain walls, lattice grids, explicit lane gaps.
- Environmental hazards: persistent zones, moving hazards, arena constraints, bullet interactions.
- Transition modules: cancel/convert, telegraphed reposition, low-density interlude.

Standard module interface:
- Inputs: origin anchors, target refs, seed stream, difficulty scalar.
- Parameters: count, speed, spread, interval, rotation, lifetime, aim variance.
- Modifier slots: oscillation, split, delay, speed ramp, phase shift.
- Outputs: density estimate, safe-lane hints, performance estimate.

### 21.7 Combination and Fairness Rules
- Primary module defines core dodge problem.
- Secondary/hazard layering is allowed only if overlap constraints pass:
  - Must not remove all escape routes.
  - Must not exceed density/speed/perf budgets.
- Hazard presence constrains legal bullet pattern combinations.

### 21.8 Selection Algorithms and Weighted Pools
Attack pools by role/intensity:
- `Primary.Low/Med/High`
- `Secondary.Support`
- `Hazards`
- `Transitions`

Each candidate includes style weights, prerequisites, fatigue metadata, and synergy tags.

Deterministic selection process:
1. Filter by constraints and prerequisites.
2. Score candidates with `style * phase * novelty * synergy * feasibility` weighting.
3. Deterministically sample using seeded stream.
4. Apply repetition cooldowns by module/tag.

### 21.9 Deterministic Parameter Synthesis
- Parameters are selected within module/style bounds.
- Phase escalation curves map to parameter scaling.
- Constraint pass enforces global budgets (spawn/alive/speed/reaction/telegraph).
- Prefer indexed randomness: `param = lerp(min,max, hash(seed,moduleId,param,segmentIndex))`.
- Clamp/adjust deterministically if constraints are violated.

### 21.10 Sequencing and Escalation
- Sequence generated from pacing templates (e.g., Primary -> Recovery -> Layered Primary+Secondary -> Transition).
- Durations chosen from bounded beat-grid-aligned sets.
- Escalation levers:
  - Add secondary layers.
  - Increase rotation/amplitude.
  - Reduce interval.
  - Introduce modifiers/hazards in later phases.
- Avoid stacking many escalation levers simultaneously except late-phase climax.

### 21.11 Difficulty Scaling Model
Difficulty is multi-dimensional:
- Density, speed, entropy, aim aggression, hazard pressure, reaction time.

Budget-driven constraints per phase:
- `B_alive_max`, `B_spawn_rate_max`, `B_collision_checks_max`.
- `S_avg_max`, `S_peak_max`.
- `MinTelegraphTicks`, `MinBreatherRatio`.

If over budget, deterministic reduction priority:
1. Reduce secondary layers.
2. Reduce density.
3. Reduce speed.
4. Reduce aim aggression.
5. Last resort: change module selection.

Optional adaptive mode:
- Runtime selects among pre-generated variants (A/B/C) using deterministic performance bands.

### 21.12 Style Profiles and Motifs
Style profile is a declarative generation config:
- Category/module weights.
- Parameter ranges/signature motifs.
- Pacing templates.
- Modifier allowances/probabilities.
- Theming hooks.

Required support for motifs (“phrases”) to preserve identity across procedural output.

### 21.13 Designer Control and Regeneration Workflow
Designer controls:
- Module allow/deny lists.
- Category/module weights.
- Parameter caps and hard constraints.
- Pacing templates.
- Mandatory signature attacks.
- Seed/variation policy.

Lock/regenerate capabilities:
- Lock segment/motif.
- Regenerate unlocked sections only.
- Regenerate parameters only while preserving module picks.

Constraint authoring includes:
- Max bullets alive/spawn rate.
- Min safe-lane frequency.
- Min telegraph duration.
- Max homing turn rate.
- Optional strict “no unavoidable hits” mode.

### 21.14 Transition Generation
Transition segments are synthesized mini-sequences:
- Bullet clear/cancel/convert.
- Repositioning.
- Rule-change telegraphing.
- Low-density reset pattern.

Trigger types:
- HP thresholds (tick boundary evaluation).
- Time caps.
- Optional deterministic performance triggers selecting precomputed variants.

### 21.15 Playtest Simulation and Validation
Deterministic bot profiles:
- Cautious, grazer, aggressive, limited-skill.

Validation metrics:
- Survival distributions.
- Hit timestamps/locations.
- Required reaction-time estimates.
- Density heatmaps and safe-region estimates.
- Spike detection and segment difficulty scores.

Feedback loop:
- Hard validation failure for violated constraints.
- Optional bounded deterministic tuning pass with fixed iteration budget.
- Output diff report describing parameter changes and reasons.

### 21.16 Generation Reports and Explainability
Each generated segment report must include:
- Selected module and weighted-selection rationale.
- Final parameters + applied constraints/clamps.
- Predicted difficulty/performance metrics.
- Bot-simulation summary vs target bands.
- Any substitutions and deterministic causes.

### 21.17 Reference Example (Normative)
Engine/docs should support end-to-end generation equivalent in structure to a 3-phase “spiral-heavy with aimed punctuation” boss profile featuring:
- Establishment phase with motifs and recovery windows.
- Mid-phase layered entropy increases and transition hazards.
- Climax phase with constrained split crescendos and fairness-preserving telegraphs.

This example is normative for capabilities (not fixed content values).


## 22. AI-Assisted Pattern Generator (In-Editor Co-Pilot)
### 22.1 Definition and Scope
- The AI-assisted generator is an editor co-pilot that outputs engine-native Pattern Graph assets (or IR templates), not opaque runtime scripts.
- It proposes candidate structures, explains decisions, supports deterministic refinement, and enforces performance/fairness constraints.
- It is explicitly **not** a black-box runtime behavior controller.

### 22.2 Primary Goals
- Speed: idea to playable pattern in minutes.
- Quality: readability, fairness, style consistency.
- Control: hard constraints and deterministic reproducibility.
- Integration: outputs normal pattern assets with preview/profiling/report metadata.

### 22.3 Hybrid Generation Philosophy
Generation pipeline combines:
- AI-assisted structure proposals (optional tier).
- Rule/template-based parameterization and constraint solving.
- Deterministic simulation/analyzers for validation and bounded tuning.

### 22.4 Input Model
Supported inputs:
- Natural-language intent prompts.
- Hard-constraint sliders/targets (alive cap, spawn-rate cap, speed bands, aim aggression, telegraph strictness, entropy, complexity).
- Style presets.
- Existing-pattern reference/remix modes.
- Optional advanced inputs (safe-lane policy, arena constraints, boss socket map).

Prompt parsing produces a structured Intent Spec containing motifs, pacing, constraints, and style tags.

### 22.5 Three-Stage Generation Pipeline
1. Intent -> Blueprint
   - Generate timeline skeleton, layer plan, and weighted module palette.
2. Blueprint -> Graph Drafts
   - Assemble graph/timeline candidates from approved template library.
   - Produce multiple deterministic candidates (typical 3–6).
3. Parameter Solve + Validation
   - Deterministic constrained sampling with indexed randomness.
   - Run fast analyzers and optional deterministic bot simulation.
   - Apply bounded deterministic tuning iterations.

### 22.6 AI Tiers and Safety Boundary
- Tier 1: deterministic rule/template generator (shipping baseline).
- Tier 2: LLM-assisted blueprint suggestions (optional).
- Tier 3: learned candidate ranking model (optional).

Safety rule:
- AI/LLM may suggest composition only.
- Final runtime behavior must be concretized via approved templates, deterministic compilation, and constraints.

### 22.7 Engine-Compatible Output Contract
Generated output includes:
- Standard Pattern Graph asset.
- Timeline tracks/markers/keyframes.
- Exposed parameters (`Count`, `Speed`, `Interval`, `RotationSpeed`, `Spread`, `TelegraphDelay`, etc.).
- Embedded generation report metadata:
  - intent summary
  - constraints used
  - rationale for nodes/layers
  - predicted metrics

### 22.8 Style Library Integration
Style presets define:
- Module weights.
- Parameter ranges.
- Rhythm templates.
- Allowed modifiers.
- Readability rules.
- Optional visual theming hints.

Styles influence blueprint shape, module probabilities, parameter initialization, constraint priorities, and motif injection.

### 22.9 Refinement Workflow
Core loop: Generate -> Edit -> Selective Regenerate.

Required operations:
- Parameter editing with grouped controls and hot-reload preview.
- Node replacement while preserving compatible inputs.
- Timeline retiming and keyframe editing.
- Modifier stack tuning (toggle/reorder/intensity).
- Lock Structure / Lock Parameters modes per segment.

Explainability requirements:
- Node annotations for generated intent.
- Side panel summarizing blueprint and constraint satisfaction.

### 22.10 Preview and Debug UX
Always-on deterministic sandbox preview:
- play/pause/step/scrub
- speed scale, difficulty selector
- seed lock/reset

Required overlays:
- density heatmaps
- flow fields
- danger spike timeline
- bot reachable-region/dodge guidance

Graph coupling:
- node execution highlights
- click bullet to show spawn provenance and resolved spawn parameters
- timeline scrubbing reflects graph highlights.

### 22.11 Difficulty Estimation Model
Fast analytical metrics:
- bullets/sec
- peak alive estimate
- avg/peak speed
- aim aggression index
- entropy index
- telegraph compliance

Spatial/temporal metrics:
- density gradients
- gap-width estimates
- reaction-time approximation

Optional slower bot metrics:
- survivability curves across seeds/variants
- normalized score by tier

Supports explicit difficulty targets and deterministic tuning toward target bands.

### 22.12 Mutation System
Mutation modes:
- Parameter perturbation.
- Structural remix.
- Motif injection.

Controls:
- mutation intensity
- constraints always enforced
- lock list for protected nodes/segments

Mutation determinism:
- seeded mutation IDs (e.g., hash over run seed + pattern ID + mutation index).

### 22.13 Pattern Library Requirements
Generated patterns are stored as standard assets with metadata:
- style/motif tags
- estimated and verified difficulty
- budget metrics
- provenance snapshot (prompt + constraints)
- versioning

Library capabilities:
- tag/numeric filters
- similarity search via fingerprints
- favorites/collections
- phase-level reuse and override workflows

### 22.14 Future Extensions (Optional)
- ML models for structure proposal/ranking/difficulty prediction.
- Tight PBAGS integration for full-boss generation.
- Deterministic runtime selection among pre-generated adaptation variants.
- Community marketplace import/export with validation.
- “Explain and Teach” onboarding mode with before/after guidance.

### 22.15 Non-Code Enforcement Requirements
- Output must remain engine-native graph assets.
- Determinism via fixed ticks, seed discipline, indexed randomness, stable IR compilation.
- Performance via template whitelist + budget-aware solving + analyzer/bot validation.
- Designer trust via explainable reports, selective regeneration, locking, and integrated debugging.


## 23. Specialized Physics and Collision System
### 23.1 Purpose and Scope
Physics responsibilities are strictly deterministic, overlap-centric, and throughput-oriented:
- Determine overlaps per tick.
- Generate deterministic interaction events (`hit`, `graze`, `shield`, `pierce`, `cancel`).
- Sustain 10,000+ projectiles with predictable frame cost and zero per-tick allocations.

### 23.2 Boundary vs Traditional Physics
This engine intentionally excludes general rigidbody simulation:
- No impulse solver.
- No friction/constraint stacks.
- No general-purpose dynamic broadphase by default.

Focus area:
- Simple kinematics + overlap tests + deterministic special interaction rules.

### 23.3 Collision Shape Set
Minimal primitive set:
- Circle (default bullets/player/hurtboxes).
- Capsule (beams/elongated hurtboxes).
- OBB (optional rare hazards/enemies).
- Segment+radius (laser represented as capsule-like primitive).

Default gameplay mapping:
- Bullet hitbox: circle.
- Player hurtbox: small circle.
- Player graze zone: larger circle.
- Enemy hurtboxes: circle/capsule.

### 23.4 Hitbox/Hurtbox/Meta-Zone Model
Collision interactions are defined between:
- Bullet hitbox and target hurtbox.

Meta-zones include:
- Graze zone.
- Shield zone.
- Damage/hazard zone.

Each collider carries:
- shape type, layer/mask, team, policy, priority.

Policies include:
- destroy, pierce N, reflect, trigger-only, graze-only.

### 23.5 Optional Continuous Checks
Default is discrete overlap at fixed ticks.
Optional per-archetype swept checks for fast bullets only:
- Swept circle-vs-circle (prev to current segment test).
- Enabled selectively to preserve predictability.

### 23.6 Spatial Partitioning Strategy
Recommended broadphase:
- Uniform grid (or hash-grid variant for large/scrolling arenas).

Reasons:
- Predictable O(N)-like insertion behavior.
- Deterministic iteration.
- Dense-field suitability.
- Allocation-free fixed-array implementations.

Grid design requirements:
- Deterministic cell mapping via floor transform.
- Tunable cell size (~2–4× typical bullet radius).
- Bounds clamp/despawn policy for out-of-range bullets.

### 23.7 Bucket Capacity and Overflow Handling
Per-cell fixed-capacity buffers:
- `cellCounts[]`
- `cellItems[]`

Overflow policy must be deterministic:
- Prefer conservative capacities + fixed-size global overflow buffer + diagnostics.
- Dev builds must surface overflow hotspots.

Optional multi-grid mode:
- Fine grid for bullets.
- Coarse grid for large hazards/colliders.

### 23.8 Per-Tick Collision Pipeline
Stage 0: integrate motion (store previous positions when swept enabled).

Stage 1: build broadphase grid(s).

Stage 2: generate candidate queries per target by overlapping cell bounds.

Stage 3: narrowphase exact tests (circle-circle, circle-capsule, minimal extra pairs).

Stage 4: emit collision/graze/shield events into preallocated buffers.

Stage 5: deterministic resolution apply pass with stable ordering and one-hit-per-bullet rules as configured.

### 23.x Implemented Deterministic Collision Runtime Contract (Phase 7)
- Runtime pipeline execution is three-stage in `ProjectileSystem` API:
  1. `updateMotion(dt, enemyTimeScale, playerTimeScale)`
  2. `buildGrid()`
  3. `resolveCollisions(targets, outEvents)`
- `CollisionTarget` and `CollisionEvent` are explicit deterministic interchange types between entity/runtime ownership boundaries.
- `CollisionEvent` output is deterministically sorted by `(targetId, bulletIndex)` before resolution.
- Stamp-based deduplication is used in collision traversal:
  - `collisionVisitedStamp_` prevents duplicate narrowphase checks for the same bullet/target query window.
  - `collisionHitStamp_` enforces one-hit-per-bullet-per-tick semantics.

### 23.9 Deterministic Event and Resolution Rules
Event buffers are preallocated ring arrays (`hitEvents`, `grazeEvents`, etc.).

Stable ordering key is explicit (e.g., targetId + bulletPoolId + bulletIndex, or bullet-first equivalent) and must remain fixed per game ruleset.

Required precedence order:
1. Shield
2. Player hit (if vulnerable)
3. Graze
4. Enemy hit
5. Hazard interactions

### 23.10 Memory and Data Layout
Bullets use SoA hot-path arrays:
- positions, previous positions (optional), velocity, radius, flags, team mask, owner, lifetime, behavior id.

Targets/hazards may remain ECS/AoS, but collision uses compact deterministic proxy buffers built allocation-free each tick.

No per-tick dynamic allocation rules apply to:
- grid buffers
- event buffers
- temporary query scratch

### 23.11 Deterministic Multithreading Plan
Parallelization allowed with deterministic merge:
- thread-local preallocated buffers.
- stable merge keys at synchronization point.

Preferred partitioning:
- by target sets (player/enemy hurtboxes) for simpler deterministic ownership.

Parallel grid-build options:
- two-pass deterministic build (`bulletCellId` + stable radix/counting arrangement), preferred over atomics for reproducibility.

### 23.12 GPU Role (Optional)
Gameplay collision remains CPU-first for determinism/debuggability.
GPU is suitable for:
- rendering
- density heatmaps
- debug/analysis visualization

GPU collision for gameplay is optional/advanced and not baseline.

### 23.13 Graze Detection Model
Given player hit radius `R_hit`, graze radius `R_graze`, bullet radius `r`:
- Hit if `d^2 <= (R_hit + r)^2`
- Graze if `(R_hit + r)^2 < d^2 <= (R_graze + r)^2`

Graze throttling is allocation-free via per-bullet fields:
- once-per-life flags or
- `lastGrazeTick` cooldown checks.

### 23.14 Physics Debugging Tooling Requirements
Must provide overlays and introspection for:
- hitbox/hurtbox/graze bands
- collision markers
- grid occupancy + overflow cells
- per-tick resolution order keys
- first divergent replay tick event diffs

### 23.15 Collision Performance Counters
Per tick counters:
- bullets inserted
- cells touched per target
- narrowphase checks
- hits/grazes emitted
- stage timings
- overflow incidents

### 23.16 Collision Stress Lab and Acceptance Targets
Required benchmark scene/tool: deterministic Collision Stress Lab with fixed seeds.

Scenarios include:
- uniform dense fields
- clumped worst-case around player
- many-target scenes
- fast swept bullets
- large-hazard overlap

Reporting includes:
- per-stage ms
- checks/tick
- overflow incidents
- hash stability across runs

Example acceptance target (configurable):
- 10k bullets + player + ~200 enemies with collision stage under target frame budget, zero tick allocations, and controlled/diagnosed overflow behavior.


## 24. Procedural World and Encounter Generation System
### 24.1 Run Structure and Determinism
A run is a deterministic seeded sequence:
- World 1: introduction layer.
- World 2: systemic pressure layer.
- World 3: high intensity layer.
- World 4: endgame layer.
- Final boss encounter.

Each world is generated as a node map and must remain reproducible from run seed for:
- replay/debug parity,
- deterministic validation,
- daily challenge consistency.

### 24.2 World Composition Requirements
Per-world structure:
- 6–10 nodes.
- Exactly one mid-boss and one boss encounter.
- Branching paths with meaningful content diversity.

Node types:
- standard combat,
- elite combat,
- event,
- mutation/reward,
- recovery,
- optional shop.

Each world introduces additional mechanics/archetypes to increase systemic complexity.

### 24.3 World Generation Pipeline
Required generation stages:
1. Seed initialization.
2. World theme/style selection.
3. Node DAG generation.
4. Encounter pool selection.
5. Hazard assignment.
6. Reward distribution.
7. Difficulty-budget validation.

Pipeline outputs must include diagnostics suitable for offline balancing and replay-audit.

### 24.4 World Node Graph Model
World maps are directed acyclic graphs (DAGs) with branching/rejoin structures.

Node distribution constraints:
- at least one elite node,
- at least one mutation node,
- mid-boss appears before boss,
- balanced spacing for combat, event, recovery pressure.

Path bias classes are supported:
- combat-biased,
- event-biased,
- mutation-biased.

### 24.5 Encounter Composition and Threat Budgets
Encounters are generated from bounded threat budgets per world and per encounter.

Threat allocation dimensions:
- enemy role mix,
- wave count/duration,
- bullet density,
- hazard pressure.

Wave structure baseline:
- 2–4 waves per standard encounter,
- pacing includes pressure/recovery alternation,
- elite encounters include mutator layers and elevated rewards.

### 24.6 Enemy Role Taxonomy for Generation
Generation pools classify enemies by functional role:
- Swarmers,
- Snipers,
- Emitters,
- Bruisers.

Wave synthesis must combine roles with synergy-aware composition to avoid monotony and maintain dodge readability.

### 24.7 Event and Non-Combat Systems
Event subsystem supports deterministic randomization of:
- mutation chambers (choice sets + skip option),
- parasite nests (risk/reward combat events),
- dormant tissue zones (healing/lore/latent modifiers),
- temporary surge events (buff/debuff windows).

Event outcomes must be serialized in run state for replay consistency.

### 24.8 Environmental Hazard Integration
Hazard families include:
- blood currents,
- antiseptic bursts,
- slow zones,
- shrinking safe zones.

Hazards are introduced progressively by world and integrated into encounter generation budgets.

Hazard interactions must obey determinism contracts already defined for collision/simulation.

### 24.9 Difficulty Curve and Adaptive Control
Difficulty is multi-dimensional across:
- enemy durability,
- bullet density,
- projectile speed,
- hazard complexity,
- encounter duration.

World-over-world scaling increases complexity gradually, with optional deterministic adaptive adjustments based on performance metrics (damage taken, clear times, build strength).

Adaptive logic must be subtle and deterministic; no runtime nondeterministic tuning.

### 24.10 Reward and Mutation Economy
Mutation categories:
- offense,
- defense,
- mobility,
- utility.

Rarity tiers:
- common, rare, epic, legendary.

Reward flow emphasizes player agency via choice-based presentation (e.g., choose 1 of 3), with deterministic pool sampling and provenance logging.

### 24.11 Variation Systems
Run-to-run variation mechanisms:
- world-specific encounter pools,
- enemy modifiers,
- hazard combinations,
- mutation pool rotation.

Variation remains bounded by world budgets, pacing rules, and fairness constraints.

### 24.12 Designer Control Surface
Designer-authorable controls include:
- per-content weights (encounters/enemy roles/events/hazards),
- world/encounter threat budget ranges,
- pacing constraints (elite spacing, mutation frequency, hazard intro timing),
- progression and gating rules.

Generator must support lock/override mechanisms for manual curation where needed.

### 24.13 Validation and Tooling Requirements
World-generation validation tooling must support:
- large-run simulation sweeps,
- difficulty-distribution analysis,
- encounter frequency and imbalance detection,
- hazard/reward pacing audits,
- deterministic seed replay of generated world plans.

Outputs must include explainable generation reports for each world and encounter.

### 24.14 Summary Contract
Procedural world generation is a deterministic, designer-governed system combining:
- DAG map generation,
- budget-driven encounter synthesis,
- controlled event/hazard variation,
- player-choice rewards.

The resulting runs must differ meaningfully while preserving fairness, pacing quality, and reproducible debugging.


## 25. Product Vision, Delivery Roadmap, and Release Criteria
### 25.1 Product Vision and Scope
The product is a commercial-grade bullet-hell engine prioritizing:
- 10,000+ projectiles with predictable performance.
- Deterministic simulation and replay reproducibility.
- Data-driven content authoring with minimal code requirements.
- Professional editor workflows (graph/timeline/debug/profiling).
- Fast iteration via hot reload and live preview.

In-scope pillars:
- Runtime (simulation, rendering, runtime asset consumption).
- Editor tooling (pattern/boss/wave/debug/profiler).
- Content pipeline (author data -> validated runtime packs).
- Sample projects/templates and documentation.

Explicit initial out-of-scope items:
- general-purpose 3D physics,
- AAA cinematic/film pipelines,
- large-scale open-world streaming.

### 25.2 Target Users
Primary users:
- indie and small studios building bullet-hell roguelites,
- technical designers authoring patterns without programming,
- engine/tools programmers extending via plugin interfaces.

### 25.3 Core Principles (Cross-Cutting)
- Determinism-first.
- Performance-first.
- Tools-first.
- Data-first.
- Explainability-first (cost/difficulty attribution to authored assets).

### 25.4 Program Phase Model (Execution Intent)
Program intent aligns to the following sequence:
- Foundation pre-production (architecture lock and budgets).
- Core runtime.
- Bullet world throughput.
- Collision/graze.
- Pattern VM/IR.
- Editor productionization.
- Enemy/boss/wave gameplay tools.
- Author->build->runtime content pipeline.
- Optimization/scalability pass.
- Polish/docs/templates/release readiness.

ImplementationPlan maps this intent into additive buildable phases and exact validation commands.

### 25.5 Milestone Contracts
Required milestone progression:
- M0 Architecture lock.
- M1 First playable deterministic simulation sandbox.
- M2 1,000-bullet no-allocation stress validation.
- M3 Collision+graze prototype.
- M4 10,000-bullet sustained stress with stable hashes.
- M5 Pattern VM v1.
- M6 Pattern editor prototype.
- M7 Replay system v1 with divergence reporting.
- M8 Boss-fight vertical slice built through tools.
- M9 Full roguelite-loop demo.
- M10 1.0 release candidate readiness.

### 25.6 Tooling Co-Evolution Requirement
Tooling must co-evolve with runtime from early phases:
- early sandbox + inspector,
- debug overlays + profiler skeleton,
- pattern graph + timeline editing,
- replay debugger and divergence detector,
- boss/wave/enemy editors,
- validation/lint/build pipeline UI.

Tooling is a parallel product pillar, not a late-stage add-on.

### 25.7 Performance Targets
Core 1.0 targets:
- 10,000 projectiles at target framerate on defined reference hardware.
- zero allocations in simulation ticks (shipping builds).
- deterministic replay hash stability for identical input/seed/content.
- bounded worst-case collision stage cost.
- hot-reload preview responsiveness suitable for interactive authoring.

Secondary targets:
- performance mode support for higher projectile counts,
- deterministic multithreaded collision,
- profiler attribution down to authored-pattern granularity.

### 25.8 Testing and Verification Strategy
Continuous automation requirements:
- unit tests (RNG, clocks, collision primitives, grid operations),
- property tests (determinism invariants, numeric safety),
- golden tests for Pattern IR compilation output,
- replay verification suites in CI (hash checks + first divergence tick diagnostics),
- stress/soak suites (1k/10k/20k bullets, collision clumps, tool library scale),
- memory and overflow policy validation.

### 25.9 Documentation Program
Documentation begins early and evolves with releases:
- getting started,
- designer guides,
- engineering references,
- performance guides,
- troubleshooting playbooks.

In-editor docs (tooltips/examples/warnings) and templates are first-class documentation channels.

### 25.10 Community, Plugin, and Mod Baselines
1.0 plugin baseline:
- register pattern nodes, projectile behaviors, editor extensions,
- deterministic contract declarations enforced by validation.

Initial mod support:
- data-only packs,
- schema/dependency validation,
- runtime-pack format compatibility.

### 25.11 Post-1.0 Direction
Post-1.0 roadmap includes:
- advanced AI copilot enhancements,
- deeper procedural boss generation,
- optional deterministic co-op/networking,
- advanced mod workflows and publishing,
- GPU-assisted analysis,
- stricter cross-platform determinism hardening.

### 25.12 Vertical Slice Gating Rule
Two gating vertical slices are mandatory before expansion:
1. 10k bullets + collision + replay hash stability.
2. Designer-authored boss fight built end-to-end in tools.


## 26. Editor UX Architecture and Workflow Specification
### 26.1 Overall Layout and Workspace Model
Default authoring workspace includes:
- Top global menu/toolbar with command palette/search, preview controls, difficulty/bot selectors, determinism badge, and overlay toggles.
- Left dock for Content Browser and Outline hierarchy.
- Center document-tab work area (stage view, pattern graph, timeline, boss phases, projectile editor, etc.).
- Right dock for Inspector, live Parameters, and contextual Docs.
- Bottom dock for Console, Validation, Profiler, Replay, Tasks, and Build Output.

Workspace presets are first-class:
- save/load per-role layouts,
- panel docking/splitting/floating,
- per-workspace overlay presets,
- reset options (workspace or global).

### 26.2 Dashboard / Project Overview
Project-open landing dashboard must provide:
- quick navigation by asset type (stages/patterns/projectiles/enemies/bosses/hazards/replays/profiling/modding),
- recent assets + quick-open/run actions,
- quick-create actions/templates,
- project health panel (validation, determinism, perf budget, missing refs),
- last-run metrics (peak bullets, worst frame, hottest assets).

Primary goal: minimize time-to-action for both new and experienced users.

### 26.3 Pattern Graph Editor UX
Pattern authoring tool is hybrid Graph + Timeline + Preview:
- typed node palette,
- graph canvas with minimap,
- timeline scrub/clip area,
- integrated deterministic preview viewport,
- inspector/parameters/docs docks,
- validation/profiler micro-views.

Node families include:
- flow/scheduling,
- emitters,
- parameters/signals,
- spawn/projectile,
- transform/anchor utilities.

Connections:
- thick execution links for order,
- typed thin data links for values.

Supports layering (mute/solo/cost per layer), marker-driven triggers, and expose-parameter workflows.

### 26.4 Pattern Preview and Explainability UX
Pattern preview must support:
- play/pause/step/scrub,
- seed lock/reset,
- bot profile selection,
- overlay toggles,
- live hot-reload at safe boundaries.

Click-to-explain flow:
- selecting a bullet highlights origin node chain,
- displays resolved spawn parameters and modifier influences.

### 26.5 Projectile Editor UX
Projectile editor centers on reusable archetype validation:
- reorderable module stack with searchable add-module palette,
- dedicated motion/collision/visual/gameplay modules,
- collision shape editing overlays,
- split/spawn behavior modules,
- live preview with spawn modes and target dummies,
- collision test mode with event log.

Goal: ensure hitbox fairness, readability, deterministic behavior, and per-archetype performance transparency.

### 26.6 Enemy and Boss Authoring UX
Enemy editor (choreography-first) integrates:
- HFSM/BT behavior graph,
- movement timeline,
- attack slots with pattern refs and overrides,
- sandbox preview with bots,
- validation/event diagnostics.

Boss editor uses synchronized views:
- phase map for high-level progression/transitions,
- phase timeline for attack/hazard/movement/UI tracks,
- rehearsal controls (jump-to-phase/time with preset state),
- anchor/socket visualization with live pattern linkage.

### 26.7 Wave/Level Timeline Tool UX
Stage editor is timeline-driven with multi-tracks:
- enemy spawns,
- boss entries,
- hazards,
- audio/UI cues,
- difficulty curves.

Supports:
- marker events referenced by patterns/boss logic,
- spawn-clip visual metadata,
- predicted-vs-actual bullet load views,
- bot-assisted preview and quick marker jumps.

### 26.8 Debug and Overlay System UX
Overlay system is globally accessible and workspace-aware.

Overlay categories:
- collision (hit/graze/hurt boxes),
- bullet trajectories/lifetimes/ownership,
- density/flow/safe-zone analysis,
- AI state traces,
- performance HUD.

Requirements:
- preset sets (designer/QA/etc.),
- hotkey binding,
- contextual linkage back to source assets.

### 26.9 Profiler UI Requirements
Bullet-hell-specific profiler must provide:
- frame timeline lanes (CPU/GPU/Sim/Render),
- stage breakdown (motion/VM/broadphase/narrowphase/render),
- hotspot ranking by pattern/archetype/cell groups,
- allocation visibility (should remain zero in sim tick),
- click-through asset attribution and “open in editor” actions.

### 26.10 UX Core Behaviors
Mandatory UX fundamentals:
- robust shortcuts and command palette,
- fast search across assets/tools/commands,
- per-document undo/redo (optionally unified global stack),
- autosave, dirty-state indicators, crash recovery,
- non-intrusive smart warnings with actionable fixes and doc links,
- inline parameter docs with ranges/examples/perf notes.

### 26.11 Advanced UI Features (Roadmap-Tracked)
Editor must support roadmap hooks for:
- time-travel replay/pattern debugger with run-compare mode,
- UI-driven AI pattern generation panel with constraints/candidates,
- difficulty lab with bot curves and accessibility deltas,
- player path prediction and safe-corridor visualization,
- pre-sim trajectory concentration analysis,
- 2D macro parameter surfaces for rapid tuning,
- fairness HUD with drill-down reports.


## 27. Implemented Runtime Baseline (Phase 3 Delivery Snapshot)
Current repository implementation now includes:
- C++20 CMake engine skeleton (`EngineDemo`) with SDL2 + Dear ImGui integration.
- Fixed-step simulation loop with render-loop separation and optional headless execution.
- Core runtime services:
  - logging subsystem,
  - JSON-backed configuration loading + command-line overrides,
  - deterministic RNG stream manager,
  - thread-pool job system,
  - frame allocator and object-pool helpers.
- Automated unit tests for timing-step consumption and configuration parsing.

### 27.x Current module inventory snapshot (Phase 7)
Implemented runtime/tooling modules currently present in repository:
- `InputSystem`
- `GameplaySession`
- `RenderPipeline`
- `GlBulletRenderer`
- `ShaderCache`
- `GrayscaleSpriteAtlas`
- `PaletteRampTexture`
- `BulletPaletteTable`
- `BulletSpriteGenerator`
- `GradientAnimator`
- `ParticleFxSystem`
- `BackgroundSystem`
- `DangerFieldOverlay`
- `LevelTileGenerator`
- `PatternSignatureGenerator`
- `CameraShakeSystem` (session-owned presentation event channel)
- `AudioSystem`
- `ContentWatcher` (hot-reload responsibility currently integrated through content/shader/palette reload flows; dedicated watcher module is roadmap-tracked)

Headless mode contract:
- `EngineDemo --headless --ticks <N> --seed <S>` runs deterministic simulation ticks without opening a window.


### Runtime decomposition update (Phase 1 refactor)
- Runtime orchestration now delegates to three focused modules:
  - `InputSystem` (`input_system.h/.cpp`) for frame input polling, event processing, and replay input plumbing.
  - `GameplaySession` (`gameplay_session.h/.cpp`) for deterministic gameplay simulation state updates and upgrade-screen state.
  - `RenderPipeline` (`render_pipeline.h/.cpp`) for render-context lifecycle and frame rendering.
- `SimSnapshot` defines the explicit sim->render contract used by the render pipeline.


### 23.17 Implemented Collision Runtime Contract (Phase 2)
- `CollisionTarget` contains `{ pos, radius, id, team }` where `id` is a stable deterministic key and `team` filters friendly-fire.
- `CollisionEvent` contains `{ bulletIndex, targetId, graze }`; events are sorted by `(targetId, bulletIndex)` before processing.
- Runtime collision pipeline is now three-stage:
  1. `updateMotion(dt, enemyTimeScale, playerTimeScale)`
  2. `buildGrid()`
  3. `resolveCollisions(targets, outEvents)`
- `resolveCollisions()` performs target AABB -> grid-cell range query, traverses linked lists in overlapping cells, runs circle-circle narrowphase, and enforces one-hit-per-bullet per tick.

### Runtime Architecture Update — GameplaySession State Partitioning (2026-03-07)
- `GameplaySession` now owns explicit state partitions to enforce clearer runtime boundaries:
  - `SessionSimulationState`: deterministic tick-clock and scratch/frame allocation state.
  - `PlayerCombatState`: player runtime combat data (position, aim target, radius, health).
  - `ProgressionState`: upgrade/progression UI navigation state.
  - `PresentationState`: presentation-event sinks and visual runtime state (shake queue, particle effects, danger-field overlay data).
  - `DebugToolState`: debug/perf/tool integration state.
  - `EncounterRuntimeState`: deterministic encounter collision scratch buffers/counters.
- Deterministic simulation behavior is preserved; partitioning is architectural/ownership-focused and keeps replay/hash flow intact.


## Editor Tooling Modular Panel Architecture (Update)
- `ControlCenterToolSuite` now composes focused panel methods instead of a single monolithic draw body.
- Panel ownership is grouped into: workspace shell/content browser, pattern graph editor, encounter/wave editor, projectile+trait tooling, palette/FX editor, and validation diagnostics.
- Shared editor service helpers are explicit (`buildEncounterAsset`, `ensurePatternGraphSeeded`, `ensureEncounterNodesSeeded`) to keep deterministic runtime state separate from persistent UI authoring state.
- Runtime/editor boundaries remain telemetry-driven: panel rendering consumes `ToolRuntimeSnapshot` and mutation hand-off fields (`UpgradeDebugOptions`, `ProjectileDebugOptions`, `PatternGeneratorDebugState`) without embedding simulation execution into panel code.
- Extension point: future tools should add one panel method + one state/service surface instead of growing `drawControlCenter`.

## 13. Asset Import Pipeline Requirements (Implemented)
- Source-art import is manifest-driven and integrated directly into the pack build stage.
- Sprite + texture authored assets are supported with deterministic GUID assignment.
- Import settings include: classification, grayscale/monochrome workflow, pivot, collision policy, atlas group, filtering/mips, variant group, animation grouping.
- Reimport behavior is fingerprint-based and emits explicit dependency invalidation records.
- Atlas build grouping is deterministic and represented in pack metadata for downstream consumers.
- Runtime pack integration includes full source/import registries for tooling diagnostics and cache invalidation behavior.


## Content Pipeline Addendum — Animation and Variant Import Workflow
- Art import manifests support animation clip metadata and grouping:
  - `animationSet`, `animationState`, `animationDirection`, `animationFrame`, `animationFps`
  - optional filename-derived grouping using `animationSequenceFromFilename` + `animationNamingRegex`
- Art import manifests support variant grouping for biome/themed/procedural swaps:
  - `variantGroup`, `variantName`, `variantWeight`, `paletteTemplate`
  - optional filename-derived grouping using `variantNamingRegex`
- Build output includes:
  - `animationBuild`: deterministic clip plans with ordered frame GUIDs and frame timing.
  - `variantBuild`: deterministic variant groups/options with weights and palette-template compatibility metadata.
- Validation requirements:
  - detect malformed naming/group values, duplicate frame indices, duplicate variant names, invalid frame/fps/weight values, and inconsistent clip FPS within a clip.
- Runtime expectations:
  - runtime/editor consume `animationBuild` and `variantBuild` directly instead of requiring per-frame manual setup.
  - procedural selection uses variant `weight` under deterministic RNG, with optional palette-template application where authored.

## Audio Runtime and Event Routing
- `AudioSystem` is a presentation-only runtime service; simulation never depends on audio state.
- Gameplay emits deterministic audio events (`hit`, `graze`, `player_damage`, `enemy_death`, `boss_warning`, `boss_phase_shift`, `defensive_special`, `run_clear`, `ui_click`, `ui_confirm`) and runtime flushes them after each simulation tick.
- Audio content is data-driven via pack `audio` section (`clips`, `events`, `music`) and loaded at runtime from content pack JSON.
- Runtime configures playback from authored audio pack records at boot (clip registry + event bindings + optional music loop id) and applies bus/category mix controls (`master/music/sfx`) before event dispatch.
- Mix model supports buses (`master`, `music`, `sfx`) with independent volume controls (`audioMasterVolume`, `audioMusicVolume`, `audioSfxVolume`).
- Supported source asset type is WAV via SDL decode/conversion, with deterministic fallback tones when source files are unavailable.

## Enemy/Boss Runtime Ownership Model (DL-0021)
- Enemy authoring data (`EntityTemplate`, `BossPhase`) is immutable runtime input loaded from content packs.
- Behavior state is isolated in `EntitySystem::EntityInstance` (phase timers, telegraph lead timers, phase pattern cursor, cooldown state).
- Pattern firing/orchestration is split between:
  - generic `emitPatternFromTemplate` for normal entities.
  - boss-specific `emitBossPhasePattern` for phase sequence/cadence orchestration.
- Encounter ownership boundary is explicit:
  - `EntitySystem` owns enemy/boss lifecycle + deterministic runtime events.
  - `GameplaySession` owns presentation reactions (camera shake + audio) to runtime events.
- Combat presentation hooks are eventized via `EntityRuntimeEvent` (`Telegraph`, `HazardSync`, `BossPhaseStarted`, `BossDefeated`, etc.).
- Boss phases support multi-pattern sequence authoring with per-phase cadence (`patternSequence`, `patternCadenceSeconds`) while preserving deterministic update order.
- Encounter synchronization hooks:
  - authored encounter schedule now supports `telegraph`, `hazardSync`, and `phaseGate` node types.
  - compiled encounter events carry `owner` domain metadata (`encounter`, `boss`, `hazards`) for orchestration routing.
## Release Engineering Contract (Packaging + Distribution)
- Canonical release validation entrypoint is `tools/release_validate.ps1`.
- Release artifacts MUST include: runtime binaries (`EngineDemo`, `ContentPacker`), built content packs (`content.pak`, `sample-content.pak`), runtime data/assets/examples, version stamp file, and release docs.
- Portable packaging executes in-bundle validation (sample pack rebuild + engine headless smoke run) before archive generation.
- Runtime content compatibility is enforced by a centralized `kRuntimePackVersion` and pack compatibility bounds.
- Deterministic release mode (`ENGINE_DETERMINISTIC_BUILD=ON`) is mandatory for candidate builds.

## External Documentation Baseline (2026-03-07)

The product now maintains an explicit external-user documentation set aligned with implemented workflows:

- Getting started and onboarding: `docs/GettingStarted.md`
- Asset import workflow: `docs/AssetImportWorkflow.md`
- Pattern authoring workflow: `docs/PatternAuthoringGuide.md`
- Boss/encounter authoring workflow: `docs/BossEncounterAuthoring.md`
- Replay/debug workflow: `docs/ReplayAndDebugGuide.md`
- Plugin/mod extension overview: `docs/PluginAndModOverview.md`
- Performance guidance for creators: `docs/CreatorPerformanceGuide.md`
- Troubleshooting: `docs/Troubleshooting.md`

Documentation policy:
- External docs must describe validated runtime/packer commands and currently implemented data boundaries.
- Deprecated or internal-only assumptions must be labeled or moved to internal docs.
- Docs updates for user-facing workflows must include synchronized entries in `DecisionLog`, `ImplementationPlan`, and `CHANGELOG`.

## Persistence Baseline (Settings, Profiles, Runtime Meta)
- Persistence is **outside** deterministic simulation ownership; simulation consumes resolved runtime config/snapshots and never performs direct file I/O during deterministic tick execution.
- Two baseline persisted products:
  - **User settings** (`user_settings.json`): audio/video/gameplay preferences with schema versioning.
  - **Profiles** (`profiles.json`): save-slot list, active profile pointer, and runtime/meta progression snapshot payload.
- Schema policy:
  - Each persisted document includes `schemaVersion`.
  - Current baseline schema is `2`.
  - Loader must run explicit migration for known legacy versions (v1→v2).
  - Unknown future versions must fail safely with fallback behavior (do not silently coerce unknown schemas).
- Corruption policy:
  - Invalid JSON or structurally invalid persisted files must return a fallback load status and default in-memory state, not crash runtime startup.
  - Runtime should log fallback reasons for diagnostics/QA triage.
- Save-slot baseline payload contains minimum product scaffolding:
  - Profile identity metadata (`id`, display label, last-play marker).
  - Meta progression snapshot (`progressionPoints`, purchased node IDs) for cross-run progression continuity.
  - Extension-ready counters for run-level lifetime stats (started/cleared) for future progression systems.

- Section 13 update: Data-driven content now supports deterministic tick-boundary hot-reload for patterns, entities, traits, difficulty profiles, and palette templates via `ContentWatcher`; failed loads keep prior content active and emit editor-visible errors.


### Section 27 — Implemented Runtime Baseline (Update)
- Added presentation-only SDL_mixer audio runtime with graceful fallback when device init or `.wav` loading fails.
- Gameplay simulation emits audio events only; `Runtime` dispatches playback outside `simTick()` to preserve deterministic state/replay parity.
## Section 15 (Rendering Strategy) — Runtime baseline amendment
- Bullet presentation now supports a single-draw-call OpenGL path (`GlBulletRenderer`) using grayscale SDF atlas sampling plus palette ramp-compatible pipeline integration.
- CPU simulation remains authoritative; GL bullet buffers are rebuilt from projectile SoA each frame with no persistent GPU gameplay state.
- Non-GL platforms and GL init failures continue to use SpriteBatch procedural rendering fallback.

## Section 27 (Implemented Runtime Baseline) — Runtime feature amendment
- Integrated `GlBulletRenderer` into `RenderPipeline` for deterministic-sim-safe, presentation-only acceleration of bullets and trails.
- Added per-frame CPU vertex/index rebuild with preallocated GL buffers and one draw call for active projectiles.

## Section 15 (Rendering Strategy) — Compile hotfix note (2026-03-07)
- Resolved a palette-ramp header merge regression that duplicated `PaletteRampTexture` members (`textureId`, `rowV`, `shutdown`, and `texture_`) and blocked renderer compilation.
- `PaletteRampTexture::animationFor` is now exposed in the public interface to match existing `GlBulletRenderer` usage.
- `GlBulletRenderer` now explicitly includes `projectiles.h` so `ProjectileAllegiance::Enemy` resolves from the canonical projectile allegiance enum.

## GameplaySession Runtime Ownership (2026-03-08 update)

`GameplaySession` is now treated as a deterministic runtime coordinator with explicit concern facets:
- **PlayerCombatSubsystem**: updates aim target, movement clamp, defensive-special trigger gating, and graze point collection.
- **ProgressionSubsystem**: applies upgrade screen navigation and selection/reroll transitions.
- **PresentationSubsystem**: emits camera-shake/audio feedback events for gameplay-triggered presentation signals.

Behavioral contract remains unchanged: the owning simulation tick order is still orchestrated by `GameplaySession::updateGameplay()`, and replay determinism contracts are preserved.

### Runtime Architecture Update — GameplaySession subsystem continuation (2026-03-08)
- `GameplaySession` remains deterministic phase orchestrator, but encounter-focused runtime/presentation glue has been extracted into `EncounterSimulationSubsystem`.
- `EncounterSimulationSubsystem` owns:
  - projectile despawn presentation reaction shaping,
  - deterministic CPU collision pipeline coordination (danger-field build + collision target/event processing),
  - runtime-event to presentation feedback fanout for encounter events,
  - zone transition and ambient zone presentation emissions.
- `GameplaySession` now coordinates subsystem calls rather than embedding full encounter implementation blocks inline.
- Determinism/replay contract remains unchanged: update order and core runtime systems are preserved.

## Section 15 (Rendering Strategy) — Compile hotfix note (2026-03-08)
- Root cause: a duplicated `RenderPipeline::buildSceneOverlay(...)` function signature in `src/engine/render_pipeline.cpp` introduced an unmatched opening brace, which caused following member definitions (including `renderFrame`) to be parsed as local functions.
- Syntax fix applied: removed the stray duplicate opening signature/block and kept the existing `buildSceneOverlay(const SimSnapshot&, const double /*frameDelta*/)` body intact.
- Behavior impact: none intended; this is a parsing/structure correction only.

## Test Build Hotfix (2026-03-08)
- Affected file: `tests/modern_renderer_tests.cpp`.
- Root cause: helper function named `near` conflicted with a Windows macro (`near`), corrupting parsing near the top-of-file helper signature and cascading into syntax errors around `const`.
- Exact fix: renamed helper to `nearlyEqual` and updated its call sites; no test logic changes.

## 29. Test Build Contract (Windows + Catch2)
- Catch-based tests must link `Catch2::Catch2WithMain` unless a target intentionally supplies its own `main`.
- CMake test target creation should use shared helpers to avoid per-target setup drift.
- On Windows, each test executable must have runtime DLL dependencies deployed into its output directory prior to execution (including discovery-time execution for `catch_discover_tests`).
- Runtime DLL deployment should be implemented once at the build-system layer using target runtime dependency introspection (`TARGET_RUNTIME_DLLS`) and reused by all test targets.

## Test Build Hotfix — Catch2 main linkage consistency (2026-03-08)
- Root cause: `render2d_tests` and `pattern_tests` were configured through `engine_add_plain_test(...)` while their intended Catch-style test registration path requires `Catch2::Catch2WithMain` to provide the executable entry point.
- Affected test targets: `render2d_tests`, `pattern_tests`.
- Exact CMake fix: switched both targets to `engine_add_catch_test(...)`, which centrally applies `Catch2::Catch2WithMain` and `catch_discover_tests(...)`.
- Follow-up consistency fix: converted both files from ad-hoc `int main()` checks to Catch `TEST_CASE` definitions so entrypoint ownership is consistently provided by Catch2.

## GameplaySession Runtime Ownership (2026-03-09 finalization)

`GameplaySession` now acts as deterministic runtime phase coordinator with remaining policy-heavy orchestration moved into explicit subsystem ownership.

Final concern split:
- **GameplaySession**: deterministic update sequencing, subsystem handoff, run-structure integration.
- **SessionOrchestrationSubsystem**: content hot-reload polling cadence + content-type reload callback fanout; periodic upgrade cadence and debug-policy application.
- **PlayerCombatSubsystem**: aim, movement, defensive-special activation gating, graze point collection.
- **ProgressionSubsystem**: upgrade navigation transitions.
- **EncounterSimulationSubsystem**: deterministic collision/event flow and encounter-sourced presentation feedback.
- **PresentationSubsystem**: camera/audio event shaping for defensive-special and graze responses.

Behavioral guarantee remains unchanged:
- replay/determinism-sensitive tick ordering remains owned and invoked by `GameplaySession::updateGameplay()`.
- no public runtime API break at call-sites.

Extensibility note:
- future session-level cadence/policy additions (e.g., scheduled tutorial prompts, deterministic telemetry windows) should be added to `SessionOrchestrationSubsystem` rather than inlining additional policy branches into `GameplaySession::updateGameplay()`.
