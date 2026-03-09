# Renderer ownership closure update (2026-03-09)

A targeted closure pass confirmed renderer stack boundaries are now explicit and intentionally stable without functional redesign:
- `render_pipeline` owns path selection + frame composition order.
- `render2d` owns reusable SDL 2D drawing primitives/infrastructure.
- `modern_renderer` owns post-processing/compositing resources and passes.
- `gl_bullet_renderer` owns OpenGL projectile draw prep/submission only.
- `gpu_bullets` (`CpuMassBulletRenderSystem`) remains a non-authoritative presentation path.

Residual intentional note: `CpuMassRender` is presentation-focused and must not be treated as gameplay-authoritative collision ownership unless a future explicit mode is introduced.

---

## 2026-03-09 Build/Release reliability closure update
- Top-level runtime binaries now share the same runtime-DLL deployment mechanism as tests (`TARGET_RUNTIME_DLLS` via `engine_deploy_runtime_dlls`), reducing clean/incremental drift risk on Windows.
- Portable release manifest generation no longer includes timestamp/path entropy; inventory is stable-sorted by relative path with fixed format metadata for reproducible release validation diffs.

> **2026-03-08 extensibility update:** Public/plugin/mod boundaries were tightened without widening internals: host-facing compatibility/error helper APIs were added to the public plugin contract, while registry storage/order remains internal and non-contract.


> **2026-03-08 follow-up:** The prior `editor_tools.cpp` monolith risk has been mitigated by splitting editor tooling implementation into focused modules and shared services under `src/engine/editor/`. Functionality is preserved while improving maintainability and extension boundaries.

> **2026-03-08 CMake/tests follow-up:** Remaining Catch2 missing-main risk was closed by applying shared Catch/main detection to every test-target creation path (including manual `content_packer_tests`), so only Catch sources without a local `main(...)` link `Catch2::Catch2WithMain`.

# HellEngine — Pre-Finalization Architecture Audit

**Auditor role:** Principal Engine Architect / Technical Director  
**Date:** 2026-03-05  
**Codebase snapshot:** HellEngine-main (v0.2.0)  
**Total implementation:** ~7,700 LOC engine source, ~2,600 LOC headers, ~1,800 LOC tests, ~480K total including third-party


## Audit Follow-up Update (2026-03-08)
- A product-validation vertical slice was authored and documented to close the previously noted "engine-only" perception risk.
- Validation runbook now covers combat readability, enemy encounter progression, boss phase flow, deterministic replay, content pack pipeline, HUD/upgrade UI visibility, audio event wiring, and packaging script viability.
- Follow-up recommendation: preserve this slice as a standing RC smoke gate and add golden replay artifacts to CI.
**Update 2026-03-08:** Renderer ownership clarification landed with minimal structural change. `RenderPipeline` now owns projectile path selection (`Disabled`/`ProceduralSpriteBatch`/`GlInstanced`), `gl_bullet_renderer` is documented as the GL projectile backend only, `modern_renderer` as compositing/post-fx, `render2d` as shared 2D primitives, and `gpu_bullets` as CPU mass-render alternate mode.

## Audit Addendum — 2026-03-08 (Build/Dependencies)
- Previous issue: Top-level CMake dependency declarations were operational but fragmented enough to make duplication and wiring audits difficult over time.
- Consolidation: Third-party registration now uses a single helper and canonical list (`ENGINE_FETCHCONTENT_DEPENDENCIES`) with one materialization point (`FetchContent_MakeAvailable(...)`).
- Wiring impact: No target link contract changes; `engine_core`, `ContentPacker`, and test targets consume the same dependency targets as before.

---

## A) Repo Snapshot

```
HellEngine-main/
├── src/
│   ├── main.cpp                     (71 LOC – bootstrap)
│   ├── stb_impl.cpp                 (STB implementation unit)
│   ├── tools_content_packer.cpp     (295 LOC – offline tool)
│   └── engine/
│       ├── runtime.cpp              (1,234 LOC – GOD MODULE ⚠️)
│       ├── editor_tools.cpp         (882 LOC – ImGui control center)
│       ├── patterns.cpp             (505 LOC – legacy pattern player)
│       ├── entities.cpp             (477 LOC – entity database + system)
│       ├── palette_fx_templates.cpp (432 LOC – material/palette system)
│       ├── render2d.cpp             (413 LOC – 2D renderer, sprite batch, camera)
│       ├── pattern_graph.cpp        (385 LOC – compiler + VM)
│       ├── projectiles.cpp          (367 LOC – SoA bullet world)
│       ├── modern_renderer.cpp      (299 LOC – post-fx pipeline)
│       ├── traits.cpp               (259 LOC – upgrade/trait system)
│       ├── replay.cpp               (187 LOC – record/playback/verify)
│       ├── content_pipeline.cpp     (106 LOC – pack merge/migrate)
│       └── [15 other modules: config, rng, timing, memory, jobs, ...]
├── include/engine/                  (~2,600 LOC total headers)
│   ├── public/                      (api.h, engine.h, plugins.h, versioning.h)
│   └── internal/                    (plugin_registry.h)
├── tests/                           (~1,800 LOC across 32 test executables)
├── data/                            (JSON content: entities, patterns, traits, etc.)
├── docs/                            (MasterSpec, Architecture, PerfTargets, etc.)
├── tools/                           (PowerShell CI/build/verify scripts)
└── CMakeLists.txt                   (comprehensive CMake, FetchContent deps)
```

**Dependencies:** SDL2 2.30.5, Dear ImGui 1.91, nlohmann/json 3.11.3, stb_image, stb_truetype  
**Build system:** CMake 3.28+, C++20, Windows-primary target  
**Test framework:** Custom (no gtest/Catch2 — raw `main()` per test)

---

## B) Current Strengths

### 1. Spec-Driven Development
The MasterSpec is extraordinarily thorough — 27 sections covering determinism contracts, bullet kinematics, pattern graph IR, PBAGS, collision system, editor UX, and release criteria. This is rare and valuable. The decision log tracks key architectural choices with rationale. This is the kind of foundation that prevents expensive mid-project rewrites.

### 2. Determinism Architecture is Sound
The determinism discipline is taken seriously at every layer: fixed-step simulation, RNG stream manager with seed derivation, per-tick state hashing, replay record/playback with per-subsystem mismatch reporting (bullets, entities, run state). The `deterministic_rng.h` uses proper stream isolation. The headless mode (`--headless --ticks N --seed S`) enables CI determinism verification. Golden replay tests exist.

### 3. SoA Bullet World with Spatial Grid
`ProjectileSystem` uses proper Structure-of-Arrays layout with preallocated parallel arrays, free-list reuse, and a linked-list spatial hash grid for broadphase collision. This is the correct architecture for the domain. Graze detection uses the collision proximity band model exactly as specified.

### 4. Pattern Graph Compiler + Bytecode VM
The `PatternGraphCompiler` lowers graph nodes to a compact `PatternOp` bytecode with compile-time validation, constant-angle precomputation, loop-target patching, and spawn rate estimation. The VM is tight: wait-timer suspension, loop counters, deterministic RNG streams per graph instance, safety iteration limits. The dual execute paths (std::function and raw function pointer) avoid virtual call overhead in the hot path.

### 5. Comprehensive Tooling Shell
The editor tools (882 LOC) already implement: content browser, asset inspector, pattern graph visual editor, encounter wave editor, validation runner, profiler overlay, palette/FX template editor, demo content generator, console, layout persistence, and plugin panel hooks. For an engine at this stage, this is unusually mature.

### 6. Multi-Layer Gameplay Systems
Traits, archetypes, meta-progression, run structure, difficulty scaling, defensive special (time dilation), encounter graphs — these are all in place and wired together. The upgrade UI has card animations, synergy detection, stat previews, and gamepad navigation. This is well past "engine-only" into "shippable game loop" territory.

### 7. Build/CI Pipeline
One-command local CI (`ci_local.ps1`), benchmark suite with threshold assertions, content packer pipeline, replay verification tests, installer/packaging scripts, version management from `VERSION.txt` with git hash stamping.

---

## C) Current Risks / Bottlenecks

### RED FLAG 1: `runtime.cpp` Is a 1,234-Line God Object

This is the single biggest architectural risk. The `Runtime` class owns **everything**: SDL window, renderer, camera, projectiles, patterns, entities, traits, archetypes, meta-progression, run structure, difficulty, defensive special, replay, GPU bullets, editor tools, upgrade UI, player state, input handling, and rendering. The `simTick()` method alone is ~300 lines mixing simulation logic, UI debug spawning, hot reload, trait rolling, zone modifiers, and hash computation.

**Impact:** Every change to any system risks breaking unrelated systems. Testing requires constructing the entire world. Parallelization is impossible. A new developer cannot work on entities without understanding the full runtime.

### RED FLAG 2: Collision Is Not Using the Spatial Grid

The current `ProjectileSystem::update()` iterates **all** capacity slots linearly and performs collision checks inline during the motion update loop. The spatial grid (`gridHead_`/`gridNext_`) is built but **never queried** for collision — collisions happen via direct distance checks against a single `playerPos`. For player-vs-bullet (1 target, N bullets), this is O(N) anyway, but the grid is useless for bullet-vs-enemy, bullet-vs-bullet, or multi-target scenarios. When enemies shoot back and you need bullet-vs-player AND player-bullet-vs-enemies, this becomes a real problem.

**Impact:** The 10k bullet target holds for the trivial single-target case but will collapse when the entity system has 200+ enemy hurtboxes to check against.

### RED FLAG 3: `GpuBulletSystem::emit()` Is O(N) Linear Scan

```cpp
bool GpuBulletSystem::emit(const GpuBullet& bullet) {
    for (GpuBullet& b : bullets_) {   // scans entire 500K array
        if ((b.flags & 1U) == 0U) { ... }
    }
}
```

With a 500,000-element array, every emit call scans from the beginning. At 2,000 emits/tick this is catastrophic. `activeCount()` also scans the full array every call. This is not GPU-driven — it's CPU with "GPU" in the name. No compute shaders, no instanced draw, no SSBO.

### RED FLAG 4: Floating-Point Determinism Is Not Hardened

The decision log says "prefer fixed-point-capable implementation, retain deterministic-float compatibility mode under strict validation." In practice, everything is `float` with standard `std::cos`, `std::sin`, `std::sqrt`, `std::atan2`. These are not reproducible across compilers/platforms without `/fp:strict` or equivalent. The `ENGINE_DETERMINISTIC_BUILD` CMake flag only sets `/Brepro` (binary reproducibility) — not `/fp:strict` or `/fp:deterministic`. Cross-compiler replay will break.

### RED FLAG 5: No Test Framework, Low Coverage Depth

32 test executables averaging ~55 LOC each, with no test framework (raw `main()`, manual assertions). Many tests verify "does it load without crashing" rather than behavioral invariants. Missing: collision correctness tests, pattern VM edge case tests, determinism property tests, fuzz testing of JSON content loading, integration replay regression suite.

### RED FLAG 6: Memory Allocation in Hot Path

`pendingSpawns_` is a `std::vector<ProjectileSpawn>` that gets `push_back()`ed during the update loop (splits, explosions). While it's pre-reserved, splits and explosions can exceed the reserve and trigger reallocation mid-tick. The spec explicitly forbids per-tick dynamic allocations in shipping builds. `std::ostringstream` is used in `simTick()` for logging, which also allocates.

### RED FLAG 7: Render Path Iterates Dead Slots

```cpp
void ProjectileSystem::render(...) const {
    for (std::uint32_t i = 0; i < capacity_; ++i) {
        if (!active_[i]) continue;  // skipping thousands of dead slots
```

At 20k capacity with 5k active, this touches 20k cache lines for 5k draws. The render should iterate a compact active-list or use the SoA arrays with an active count prefix.

### RED FLAG 8: No Audio System

The spec mentions "audio cues" in wave timelines and the editor. There is zero audio infrastructure. For a bullet-hell game, audio feedback (hit confirms, graze sounds, pattern warnings) is essential for commercial quality and needs to be deterministically synced.

---

## D) Top 10 "Before Finalizing" Improvements (Ranked)

### 1. Decompose Runtime (God Object Kill)

**Impact:** Unlocks parallel development, testability, and maintainability for every future change  
**Risk:** Medium — requires careful interface extraction but no behavior changes  
**Complexity:** ~3-5 days  

**Approach:** Extract from `Runtime` into standalone systems with clean interfaces:
- `SimulationOrchestrator` — owns tick order, calls subsystems in sequence
- `InputSystem` — polling, replay injection, command buffer
- `RenderPipeline` — scene building, sprite batch, debug draw, post-fx
- `GameplaySession` — player state, health, run structure, traits, archetypes
- `UpgradeUiController` — card UI, navigation, stat preview

`Runtime` becomes a thin coordinator: init → run loop → shutdown. Each extracted system gets its own header, cpp, and test file.

### 2. Fix Collision Architecture for Multi-Target

**Impact:** Enables the game to actually function (enemy shooting, bullet-vs-enemy)  
**Risk:** Medium — collision ordering changes can break determinism  
**Complexity:** ~3-4 days  

**Approach:** Implement proper two-phase collision:
- Stage 1: Build grid from all active bullets (already done)
- Stage 2: For each target (player + enemies), query overlapping grid cells, run narrowphase
- Separate `CollisionPipeline` class with deterministic event buffer, stable sort by `(targetId, bulletIndex)`, and one-hit-per-bullet policy
- Add collision layer masks (player bullets only hit enemies, enemy bullets only hit player)

### 3. Harden Floating-Point Determinism

**Impact:** Prevents replay desync across builds/compilers, which is a spec non-negotiable  
**Risk:** Low — compiler flags + validation  
**Complexity:** ~1-2 days  

**Approach:**
- Add `/fp:strict` (MSVC) or `-ffp-contract=off -fno-fast-math` (GCC/Clang) to the simulation library target
- Replace `std::cos`/`std::sin` in hot paths with platform-stable approximations (or verify identical output with cross-compiler replay test)
- Add CI step: build with two different optimization levels, run same seed, compare hashes
- Document the deterministic math policy in a header (`deterministic_math.h`)

### 4. Add Free-List to GpuBulletSystem / Rewrite as Actual GPU Path

**Impact:** Makes the GPU bullet mode viable (currently unusable at scale)  
**Risk:** Low for free-list fix; High for true GPU compute  
**Complexity:** Free-list: 1 day. True GPU: 2-3 weeks  

**Approach (minimum viable):**
- Add `std::vector<uint32_t> freeList_` to `GpuBulletSystem`, mirror `ProjectileSystem` pattern
- Cache `activeCount_` instead of scanning
- For actual GPU: implement compute shader dispatch with SSBO for bullet state, atomic counters for allocation, and indirect draw for rendering. SDL2's renderer doesn't support compute — this requires OpenGL/Vulkan directly or waiting for SDL3.

### 5. Compact Active-List for Iteration

**Impact:** 2-4x improvement in update/render for typical bullet counts  
**Risk:** Low  
**Complexity:** ~2 days  

**Approach:** Maintain a packed `activeIndices_` array. On spawn, append index. On deactivate, swap-remove. Update/render iterate only `activeIndices_`. This converts both `update()` and `render()` from O(capacity) to O(active). For determinism, sort `activeIndices_` once after all spawns/removals if order matters.

### 6. Adopt a Real Test Framework + Property Tests

**Impact:** Catches regressions that manual tests miss, enables CI confidence  
**Risk:** None  
**Complexity:** ~2-3 days  

**Approach:**
- Integrate Catch2 or doctest (header-only, trivial to add via FetchContent)
- Convert existing tests to use `TEST_CASE`/`REQUIRE` macros
- Add property tests: "N ticks with seed S always produces hash H" (determinism)
- Add fuzz harness for JSON loading (malformed content must not crash)
- Add collision correctness tests (known bullet positions → expected hit/graze results)
- Target: >80% line coverage on `projectiles.cpp`, `pattern_graph.cpp`, `replay.cpp`

### 7. Eliminate Hot-Path Allocations

**Impact:** Required by spec; prevents frame spikes in shipping builds  
**Risk:** Low  
**Complexity:** ~1-2 days  

**Approach:**
- Replace `pendingSpawns_` `push_back` with a fixed-capacity ring buffer or pre-sized array with count. Assert on overflow in dev builds, silently cap in release.
- Remove `std::ostringstream` from `simTick()` — use `snprintf` into a stack buffer or defer logging to post-tick.
- Add a CI check: instrument `FrameAllocator` to assert no system allocator calls during `simTick()` in release builds.

### 8. Implement Basic Audio System

**Impact:** Essential for commercial bullet-hell (graze sounds, hit confirms, pattern warnings)  
**Risk:** Low — SDL_mixer or miniaudio are well-understood  
**Complexity:** ~3-4 days  

**Approach:**
- Add SDL_mixer or miniaudio as a dependency
- `AudioSystem` with deterministic event queue: `playSound(SoundId, Vec2 position, float volume)`
- Fire-and-forget for SFX; positional mixing for spatial awareness
- Wire graze events, collision events, pattern phase transitions, and boss warnings to sound triggers
- Sounds are non-deterministic (presentation-only) — do not feed into sim

### 9. Content Hot-Reload at Safe Boundaries

**Impact:** Dramatically accelerates designer iteration speed  
**Risk:** Medium — must not break determinism  
**Complexity:** ~2-3 days  

**Approach:**
- File watcher on content directories (or poll on editor focus)
- On change detected: validate new content, compile pattern graphs, swap at next tick boundary
- Already partially implemented (graph path hot-reload exists in `simTick`), but needs generalization to entities, traits, difficulty profiles
- Add "last-good" fallback: if new content fails validation, keep previous version and show error in editor console

### 10. Packaging / Release Scripts + Automated Installer

**Impact:** Needed to actually ship; eliminates manual error in release process  
**Risk:** Low  
**Complexity:** ~1-2 days  

**Approach:**
- `build_release.ps1` already exists but is minimal. Extend to: clean build → run tests → run benchmarks → pack content → stamp version → produce zip/installer
- Add NSIS or WiX script for Windows installer (already have `build_installer.ps1` stub)
- Add a `RELEASE_CHECKLIST.md` with manual verification steps
- CI pipeline: tag → build → test → package → upload artifact

---

## E) Suggested Roadmap (Next 3 Milestones)

### Milestone A: "Ship-Safe Architecture" (1-2 weeks)

Goal: Resolve structural risks that will compound if deferred.

- [ ] Decompose `Runtime` into 4-5 focused systems (#1)
- [ ] Fix collision for multi-target (#2)
- [ ] Harden floating-point determinism (#3)
- [ ] Eliminate hot-path allocations (#7)
- [ ] Adopt test framework, add property tests (#6)

**Exit criteria:** Replay hash stability verified across Debug/Release builds. Multi-target collision passes 10k stress test. Zero system allocator calls during `simTick()` in Release.

### Milestone B: "Production Performance" (1-2 weeks)

Goal: Hit spec performance targets with real-world content complexity.

- [ ] Compact active-list iteration (#5)
- [ ] Fix or redesign GpuBulletSystem (#4)
- [ ] Collision stress lab with multi-target scenarios (200 enemies + 10k bullets)
- [ ] Profile and optimize pattern graph VM (batch emissions, reduce per-spawn overhead)
- [ ] Render batching audit (minimize texture switches, verify instanced draw path)

**Exit criteria:** 10k bullets + 200 enemies + collision under 2ms budget. GPU bullet mode sustains 100k+ at 60fps. Benchmark suite thresholds updated and passing.

### Milestone C: "Author-Ready" (2-3 weeks)

Goal: A designer can build a complete boss encounter end-to-end in the editor.

- [ ] Audio system (#8)
- [ ] Generalized hot-reload (#9)
- [ ] Encounter/wave editor improvements (timeline drag, spawn clip preview)
- [ ] Pattern graph editor: preview viewport, click-to-explain bullet provenance
- [ ] Release packaging automation (#10)
- [ ] Documentation pass: Quickstart, Authoring Guide with worked examples

**Exit criteria:** Vertical slice — a 3-phase boss fight authored entirely through editor tools, with audio, deterministic replay, and packaged as an installable demo.

---

## F) Questions / Unknowns

1. **Fixed-point math:** The spec prefers fixed-point for cross-platform determinism. Is there a plan to introduce a `FixedFloat` type, or is the strategy to stay with IEEE 754 under strict compiler flags? This decision affects every math operation in the engine and should be locked before finalizing.

2. **Target hardware baseline:** PerfTargets reference "guard-rail thresholds" but no reference hardware spec. What's the minimum target? (e.g., "Intel UHD 620 integrated GPU, 4-core i5, 16GB RAM"). This determines whether the SDL2 software renderer fallback is a real shipping path or a debug convenience.

3. **SDL2 vs SDL3:** SDL2 is being used, but SDL3 is stable and offers GPU compute/render API improvements that would make the "GPU bullets" path far more viable. Is there a migration plan, or is SDL2 locked for 1.0?

4. **Multiplayer / networking:** The spec mentions "optional deterministic co-op/networking" post-1.0. The current input system uses `SDL_GetKeyboardState` directly. If networking is planned, the input abstraction needs to be designed now (command buffer → deterministic dispatch) even if network transport comes later. The current replay system is a good foundation but the input model needs formalization.

5. **Plugin ABI stability:** The public API headers (`api.h`, `plugins.h`) expose raw C++ interfaces. For binary plugin compatibility across compiler versions, a C ABI boundary is needed. Is the plugin system intended for source-level or binary-level extensibility?

## Audit Follow-up (2026-03-07): GameplaySession Responsibility Overload Refactor

Implemented architectural remediation for the previously identified overloaded `GameplaySession` responsibilities:
- Separated simulation/session orchestration state from player combat state.
- Isolated progression/upgrade UI runtime state from presentation event state.
- Isolated camera-shake + danger-field visual hooks in a dedicated presentation partition.
- Isolated debug/tool consumption state in a dedicated debug partition.
- Isolated encounter collision/danger-field-adjacent runtime scratch data in a dedicated encounter partition.

Result: ownership boundaries are now explicit and testable while preserving deterministic replay behavior.


## Update: Editor Tools Monolith Decomposition (2026-03-07)
- Addressed audit concern around monolithic editor tooling by decomposing `ControlCenterToolSuite` panel rendering into modular tool panel methods.
- Added explicit shared editor services for encounter asset assembly and panel-state seeding to reduce cross-panel coupling.
- Preserved existing functionality and editor menu topology while improving maintainability and extension readiness.

## Update 2026-03-07 — Asset Pipeline Remediation

The content pipeline now includes a real source-art import workflow for sprite/texture assets:

- Manifest-driven import (`assetManifestType: art-import`) with per-asset import settings.
- Validation for missing files, unsupported formats, and invalid setting values.
- Pack-integrated source/import registries plus deterministic import fingerprints.
- Reimport invalidation support via `--previous-pack` fingerprint comparison.
- Atlas-group planning output keyed by atlas group and color workflow, including grayscale/monochrome support metadata.

This addresses the prior audit gap where authored art handling was underdeveloped relative to pack/schema infrastructure.


## 2026-03-07 Pipeline Audit Update — Animation/Variant Import
- **Closed gap**: Source-art import now emits runtime-ready animation clip grouping and variant grouping metadata.
- **What was added**:
  - animation clip plan generation (`animationBuild`) with state/direction/frame timing output.
  - variant group plan generation (`variantBuild`) with weighted options and palette-template compatibility hints.
  - naming/grouping validation for malformed identifiers and duplicate/ambiguous grouping.
- **Operational impact**:
  - authored 2D animation is practical without manual per-frame runtime wiring.
  - themed/procedural visual variation is first-class in content packs.

## Audit follow-up update (Audio)
- **Status**: Addressed.
- Implemented production baseline audio runtime with deterministic event-queue integration and content-driven routing.
- Added bus volumes (master/music/sfx), music loop support, and gameplay/UI audio trigger hooks.
- Added content pipeline support for `audio` metadata in packs plus runtime parsing and fallback behavior.

## Audit Follow-up (2026-03-07): GPU Bullet Path Repositioned to CPU Mass Render
- Closed the naming/expectation gap by renaming the implementation to `CpuMassBulletRenderSystem` and explicit mode labels (`CpuCollisionDeterministic`, `CpuMassRender`).
- Clarified that this path is CPU-driven update + quad preparation with batched submission, not GPU compute projectile simulation.
- Improved scalability by switching update/render traversal to compact active-slot iteration (`activeSlots_`) while keeping O(1) free-list emission.
- Added explicit render-prep telemetry surface (`preparedQuadCount`) and refreshed benchmark/render-path documentation.

## Audit Follow-up (2026-03-07): External Documentation Productization
- **Status**: Addressed.
- Delivered a major external-user documentation pass covering:
  - getting started/onboarding
  - asset import workflow
  - pattern authoring
  - boss/encounter authoring
  - replay/debug workflow
  - plugin/mod extension overview
  - creator performance guidance
  - troubleshooting
- Updated governance logs/spec references (`MasterSpec`, `DecisionLog`, `ImplementationPlan`, `CHANGELOG`) to keep documentation-state traceability aligned with runtime workflows.
## Release Engineering Follow-up (2026-03-07)
Status: **Addressed** for packaging/release workflow closure.

Implemented closures:
- Build release script now performs end-to-end release checks (tests, benchmarks, content pack build, replay verify).
- Packaging script now validates portable output by running bundled binaries in-place.
- New orchestration script (`tools/release_validate.ps1`) enforces artifact presence and archive output.
- Runtime pack-version enforcement was centralized to remove duplicated compatibility constants.

Residual risk:
- Installer generation still depends on external NSIS tooling; workflow now degrades with explicit diagnostics when unavailable.

## 2026-03-08 Creator Documentation Audit Update
- External creator workflows are now documented as first-class guides for onboarding and day-to-day authoring.
- Internal-only assumptions were removed from primary onboarding pages and replaced with executable command loops aligned to current tooling.
- Follow-up recommendation: keep one owner per workflow doc and update during feature merges to avoid spec/doc drift.

> **2026-03-08 follow-up:** The prior `editor_tools.cpp` monolith risk has been mitigated by splitting editor tooling implementation into focused modules and shared services under `src/engine/editor/`. Functionality is preserved while improving maintainability and extension boundaries.

## Pattern panel modularization follow-up (2026-03-08)
- Status: Completed refinement pass after initial editor decomposition.
- Previous risk: Pattern tooling remained feature-dense inside one large draw function (generation, testing, graph editing, preview diagnostics).
- Mitigation: Split responsibilities into focused helper methods with clear boundaries while preserving a single coherent panel for usability.
- Extension points now documented and code-level explicit: generation controls, seed/testing controls, node palette, node inspector, preview asset builder, and preview/analysis renderer.
- Residual risk: UI is still one window by design; future overflow should prefer adding focused collapsible sections before adding new top-level panels.
- Render-path routing ownership was finalized by removing duplicate projectile path branches from `RenderPipeline::buildSceneOverlay` and enforcing single-source `ProjectileRenderPath` selection.
- Palette/grayscale template ownership was documented and clarified (`GrayscaleSpriteAtlas` template source, `paletteRamp_` GL LUT authority, `proceduralPaletteRamp_` procedural staging).

## Audit Addendum — 2026-03-08 (Test Infrastructure)
- Previous Windows test failures were traced to two build-system issues rather than engine runtime logic:
  1. Catch target-definition drift allowed some Catch executables to be defined without `Catch2::Catch2WithMain`, producing missing `main` link failures.
  2. Runtime DLL deployment for tests was not standardized, causing `0xc0000135` during Catch discovery when SDL2/SDL2_mixer DLLs were unavailable beside test binaries.
- The remediation introduces centralized CMake helpers for plain and Catch tests and a single reusable Windows DLL deployment helper (`TARGET_RUNTIME_DLLS` copy) applied to all test targets.
