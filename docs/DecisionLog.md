- 2026-03-09: Decision: test executable type is determined by ownership of `main()`, not by Catch include heuristics. Context: repeated unresolved `main`/LNK1120 failures (`pattern_graph_perf_tests`, `gpu_bullets_tests`, `diagnostics_tests`, `benchmark_suite_tests`, `renderer_smoke_tests`) proved include-based detection plus force-lists was non-systemic and fragile. Decision: in shared helper, if source defines `main()` treat as standalone; otherwise always link `Catch2::Catch2WithMain` and use Catch discovery. Consequences: consistent entrypoint provisioning for Catch tests, fewer outlier patches, preserved target names/discovery/runtime DLL deployment; requires clean rebuild to flush stale generated build rules.
- 2026-03-09: Decision — keep the missing-main remediation minimal by extending existing helper classification instead of introducing a per-target one-off block. Added `pattern_graph_perf_tests` to `_engine_force_catch_main_targets` so Catch main linkage is guaranteed through the common path (`engine_add_test` -> `engine_add_test_target` -> `engine_link_catch_main_if_needed`). This preserves test naming/discovery/runtime-DLL behavior and avoids duplicating target wiring. A clean rebuild is required to apply regenerated linker metadata.
- 2026-03-09: Decision: apply a targeted explicit-link guard for `editor_tools_tests` instead of broad helper refactors. Rationale: unblock unresolved `main`/`LNK1120` immediately with the smallest safe change and no behavior churn to other tests. Implementation: use `engine_add_test_target`, add explicit `target_link_libraries(editor_tools_tests PRIVATE Catch2::Catch2WithMain)`, force Catch discovery property, then call `engine_register_test`. Operational note: perform a full clean rebuild after reconfigure.
## 2026-03-09 — Decision: Explicit Catch2 main link for boss_phase_tests
- **Context**: `boss_phase_tests.exe` remained the last missing-main linker failure on Windows (`unresolved external symbol main`, `LNK1120`), indicating the executable could still be produced without guaranteed Catch entrypoint ownership.
- **Decision**: Apply a minimal target-local CMake fix by wiring `boss_phase_tests` through `engine_add_test_target(...)`, then explicitly linking `Catch2::Catch2WithMain`, forcing Catch discovery property, and registering via `engine_register_test(...)`.
- **Consequence**: Target name and discovery semantics stay unchanged, runtime DLL deployment remains via helper path, and the test now always receives Catch main ownership for clean-link stability. A clean rebuild is required to refresh generated link/discovery commands.

## 2026-03-09 — Build hotfix: Catch main linkage + guarded DLL copy command
- **Context:** Remaining Windows link failures reported unresolved `main` for `content_packer_tests`, `editor_tools_tests`, `boss_phase_tests`, and `entity_tests`; `ContentPacker` post-build also failed when `copy_if_different` received only a destination path.
- **Decision:** Keep helpers and target names intact; add a minimal forced Catch classification list for the four failing test targets in `engine_link_catch_main_if_needed(...)`, and keep existing `NOT main(...)` gate before linking `Catch2::Catch2WithMain`. In `engine_deploy_runtime_dlls(...)`, emit copy only when `$<TARGET_RUNTIME_DLLS:target>` is non-empty via generator-expression command guarding.
- **Consequences:** Test discovery behavior stays unchanged; valid DLL deployment still occurs; empty-runtime-DLL cases no longer fail post-build. Requires full clean rebuild to validate regenerated link and post-build steps.

## 2026-03-09 — Decision: keep subsystem callback contract, fix call-site lambdas
- **Context:** Post-refactor `GameplaySession` passed lambdas returning `TraitSystem::rollChoices()` into orchestration APIs that accept `const std::function<bool()>&`.
- **Root cause:** Return-type mismatch (`std::array<Trait, 3>` vs `bool`) at call sites, not in subsystem API definitions.
- **Decision:** Preserve `SessionOrchestrationSubsystem` API and apply a surgical call-site fix only.
- **Implementation:** Added explicit `-> bool` lambdas in `GameplaySession::updateGameplay(...)`; each lambda calls `rollChoices()` for its side effects and returns `true` to satisfy the contract.
- **Consequence:** Compile failure resolved with no architecture/API changes and no broadened gameplay-system refactor scope.

## 2026-03-09 — Final documentation/productization presentation framing
- **Context**: Creator workflows were largely implemented but product-facing entry points remained fragmented (no repository README and overlap between index docs), reducing external usability despite mature engine/package tooling.
- **Decision**: Establish a clear documentation information architecture and treat the repository root README as the commercial-facing front door, with `GettingStarted` as execution-first onboarding and `AuthoringGuide` as the persistent creator index.
- **Implementation**: Added `README.md`; refreshed `docs/AuthoringGuide.md` and `docs/GettingStarted.md` to align around concrete build/pack/run/replay/testing/release steps; logged final state updates in architecture/spec/changelog/audit docs.
- **Consequence**: External developers now have a predictable entry path with reduced stale-internal assumptions while preserving existing technical behavior and script/tool contracts.
## 2026-03-09 — Vertical slice encounter polish remains content-authored
- **Decision:** Final showcase tuning was implemented by updating authored sample content (`data/entities.json`, `data/encounters.json`) rather than adding runtime special-case logic.
- **Rationale:** The sample must validate intended engine workflows (content pipeline -> runtime hydration -> replay/package checks) and remain representative for creators.
- **Applied changes:** Added `Vanguard Lancer` authored enemy profile, elevated Stage 01 elite composition to mixed pressure, tightened event lane downtime, and switched Stage 01 boss flow ordering to include `Spread Lattice` with explicit `boss_phase_shift` audio transition cue.
- **Consequence:** The polished slice better demonstrates combat readability and phase variety while preserving determinism and avoiding architecture bypasses.
## 2026-03-09 — Renderer subsystem ownership closure
- **Context**: Multiple renderer-related modules (`render2d`, `render_pipeline`, `modern_renderer`, `gl_bullet_renderer`, `gpu_bullets`) were functionally correct but still easy to misread as overlapping owners.
- **Decision**: Freeze ownership boundaries as documentation contract: `render_pipeline` is sole path/orchestration owner; `gl_bullet_renderer` is GL projectile draw backend only; `render2d` is shared 2D primitive layer; `modern_renderer` is post-FX/compositing only; `gpu_bullets` (`CpuMassBulletRenderSystem`) is presentation-only for `CpuMassRender`.
- **Rationale**: Removes residual ambiguity without redesign, preserving replay/determinism and current rendering behavior.
- **Consequence**: Future GPU/compute simulation work must arrive as an explicitly new authoritative simulation mode, not by repurposing existing presentation subsystems.

## 2026-03-09 — Immediate Catch2 main-link closure for content_packer/entity/boss tests


## 2026-03-09 — Content pipeline production-closure validation pass
- **Context**: Creator-facing import flow still allowed ambiguous manifests (missing/incorrect manifest type, duplicate asset identity entries, loose atlas naming), and pack invalidation metadata did not surface palette-template dependencies clearly.
- **Decision**: Tighten validation at source-manifest parse time and minimally extend dependency metadata without changing runtime data model or pack layering behavior.
- **Implementation**: `parseSourceArtManifest` now requires `assetManifestType: "art-import"`, trims/validates atlas and grouping identifiers, and rejects duplicate asset GUID/source-path entries within a manifest; import records now include `paletteTemplate:<name>` dependency tags when applicable.
- **Consequence**: Import failures are clearer and earlier, reimport invalidation diagnostics are more actionable, and source-vs-runtime responsibilities are better delineated for content creators.


## 2026-03-09 — Build/release reproducibility and DLL deployment closure
- **Context**: Final release hardening still had two reliability gaps: top-level binaries (`EngineDemo`, `ContentPacker`) did not use the standardized runtime-DLL deployment helper, and release manifest generation included timestamp/path entropy that prevented deterministic package metadata across identical builds.
- **Decision**: Keep changes minimal and production-focused by reusing existing CMake/runtime packaging infrastructure instead of introducing new tooling layers.
- **Implementation**: Added `engine_deploy_runtime_dlls(EngineDemo)` and `engine_deploy_runtime_dlls(ContentPacker)` in top-level `CMakeLists.txt`; updated `tools/package_dist.ps1` to sort discovered DLL copies by name and generate `RELEASE_MANIFEST.txt` from a stable relative-path sorted inventory with fixed `FormatVersion=1` header.
- **Consequence**: Clean and incremental Windows builds now deploy runtime DLLs consistently for runtime binaries/tests/tools, and portable release metadata is deterministic/reproducible when input artifacts are unchanged.

- **Context**: Build Engineering reported unresolved `main` / `LNK1120` for `content_packer_tests`, `entity_tests`, and `boss_phase_tests` in clean Windows test builds.
- **Decision**: Keep the fix minimal and local to existing shared helper logic by extending the known-target safety override set in `engine_link_catch_main_if_needed(...)` to include `boss_phase_tests` (while retaining the `NOT main(...)` guard).
- **Implementation**: Updated CMake override condition from two targets to three (`content_packer_tests`, `entity_tests`, `boss_phase_tests`) so Catch classification fallback and `Catch2::Catch2WithMain` linkage are consistent for this failure class.
- **Consequence**: All three affected targets now share the same missing-main safety path without altering target names, CTest registration style, or DLL deployment behavior; clean rebuild required to clear stale link artifacts.

## 2026-03-08 — Missing-main closure for content_packer_tests and entity_tests
- **Context**: Build Engineering reported remaining unresolved `main` / `LNK1120` failures for `content_packer_tests.exe` and `entity_tests.exe`, indicating those targets could still bypass Catch2-main linkage under helper classification drift.
- **Decision**: Keep the fix minimal in shared CMake logic by adding a narrow target-name safety override inside `engine_link_catch_main_if_needed(...)` for `content_packer_tests` and `entity_tests`.
- **Implementation**: If either target does not define a local `main(...)`, force Catch classification and link `Catch2::Catch2WithMain`; all other targets remain governed by existing source-include + main detection.
- **Consequence**: The two failing targets now align with the Catch missing-main policy without changing target names, test registration structure, or discovery behavior.

## 2026-03-08 — Catch2 missing-main completion across all test target paths
- **Context**: The earlier Catch2 fix covered most helper-based targets, but manual test target creation (`content_packer_tests`) still bypassed shared Catch-main linkage logic, leaving room for unresolved-main link failures when a Catch source is introduced on a non-helper path.
- **Decision**: Centralize Catch-main eligibility in reusable CMake helpers by combining Catch-include detection with explicit `main(...)` detection, and reuse that helper for both `engine_add_test(...)` and manually declared test executables.
- **Implementation**: Added `engine_source_defines_main(...)` and `engine_link_catch_main_if_needed(...)`; updated `engine_add_test(...)` and the `content_packer_tests` target to call the same linkage helper.
- **Consequence**: Every test target creation path now enforces the same rule: Catch sources without their own `main` link `Catch2::Catch2WithMain`; non-Catch or self-main tests remain unchanged.

## 2026-03-08 — Audio authoring/runtime boundary hardening
- **Context**: Runtime playback still loaded hardcoded WAV paths while content pipeline already parsed authored audio records; event schema and runtime dispatch were partially disconnected.
- **Decision**: Route runtime playback through authored `AudioContentDatabase` records (clip registry + event bindings + buses) and make `Runtime` dispatch authored `AudioEventId` values instead of hardcoded `SoundId` members.
- **Implementation**:
  - `AudioSystem` now owns clip registry, event binding table, per-bus volume controls, listener position, and event dispatch (`dispatchEvent`).
  - `Runtime` now parses pack JSON audio sections via `parseAudioContentDatabase`, configures `AudioSystem`, and maps gameplay presentation events to authored audio event ids.
  - Content parser now recognizes `boss_phase_shift`, `defensive_special`, and `run_clear` event names.
- **Consequence**: Content creators can iterate audio event bindings/bus assignments in authored data without touching runtime code, while deterministic sim boundaries remain unchanged (events still emitted in sim, playback still dispatched post-tick).

# Decision Log

## 2026-03-09 — Keep one editor window but split gameplay tooling module
- **Decision:** Keep the existing single Control Center window workflow while extracting gameplay-focused panels into a dedicated translation unit (`editor_tools_gameplay_panel.cpp`).
- **Rationale:** The previous pattern panel file still contained projectile/encounter/trait workflows, increasing cognitive load and maintenance friction. Separating these responsibilities improves maintainability without changing user-facing behavior.
- **Result:** Clear module boundaries: workspace shell/content flow, pattern authoring, gameplay authoring, palette/FX + standards, and services/validation.

## 2026-03-09 — Add explicit workflow shortcuts instead of adding more top-level panels
- **Decision:** Add in-workspace workflow shortcuts (Content Pass, Pattern Pass, Palette/FX Pass, Diagnostics Pass) that toggle panel visibility defaults.
- **Rationale:** This improves clarity for creators and reduces panel hunting while avoiding arbitrary visual churn or new persistent UI complexity.
- **Result:** Faster task-oriented navigation with stable existing panel behavior and state.
## 2026-03-09 — Unified test target model and registration contract
- **Context**: Test wiring had accumulated mixed helper/manual paths and target-specific Catch safety overrides, creating long-term drift risk for missing-main and discovery behavior.
- **Decision**: Use one shared model for all test targets: build/link/deploy in `engine_add_test_target(...)`, registration in `engine_register_test(...)`, Catch classification by source + local `main(...)` detection, no per-target overrides.
- **Rationale**: Keeps target names stable while making Catch-vs-standalone behavior explicit, auditable, and consistent across Windows discovery and regular CTest execution.
- **Status**: Implemented.

## 2026-03-08 — Catch2 missing-main class fix via unified test helper
- **Context**: Catch-based tests were still split across multiple registration paths, and any Catch source wired through the plain-test path could miss `Catch2::Catch2WithMain`, causing unresolved `main`/`LNK1120` on Windows.
- **Decision**: Replace the split plain/catch registration API with one `engine_add_test(...)` helper that inspects test source content for Catch includes and applies Catch linking/discovery automatically when needed.
- **Implementation**: Added `engine_source_uses_catch(...)` + `engine_add_test(...)` in `CMakeLists.txt`; migrated all helper-call test targets to this unified path so Catch target class behavior is consistent.
- **Consequence**: Catch targets (including `render2d_tests`, `pattern_tests`, and any future Catch-authored tests) consistently receive `Catch2::Catch2WithMain`, while plain `main()` tests remain unchanged.

## 2026-03-08 — Catch2 test target and runtime DLL deployment standardization
- **Context**: Windows test builds were failing in two modes: some Catch-based targets were linked without a Catch2-provided test main (`unresolved external symbol main`/`LNK1120`), and some Catch targets crashed during discovery with `0xc0000135` because SDL runtime DLLs were not present beside the test executable.
- **Decision**: Centralize test wiring in CMake helper functions: one helper for plain `main()` tests and one helper for Catch tests that always links `Catch2::Catch2WithMain` and registers discovery consistently.
- **Decision**: Add a shared Windows runtime deployment helper (`engine_deploy_runtime_dlls`) that copies `TARGET_RUNTIME_DLLS` to each test executable output directory so discovery-time execution has required dependencies.
- **Consequence**: Catch and non-Catch tests now share one maintainable setup path, and Windows Catch discovery is resilient without one-off per-target DLL copy logic.

## 2026-03-08 — Windows min/max macro collision build fix
- **Context**: Windows headers (`<windows.h>`) can define function-like `min`/`max` macros that corrupt `std::min`/`std::max` parsing (`C4003`, illegal token after `::`) and trigger cascading failures in engine compilation units.
- **Decision**: Apply global Windows macro hygiene at the build-system layer by defining `NOMINMAX` (and `WIN32_LEAN_AND_MEAN`) for all targets when `WIN32` is true, instead of ad hoc file-by-file workarounds.
- **Implementation**: Added `add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)` in top-level `CMakeLists.txt` under `if(WIN32)`, ensuring `engine_core` and all dependent targets compile with consistent preprocessor definitions.
- **Consequence**: Existing `std::min`/`std::max` call sites compile correctly under MSVC without broad source edits; developers should perform a clean rebuild/reconfigure so all objects are rebuilt with the updated definitions.

## 2026-03-08 — Public API/plugin/mod boundary finalization
- Decision: keep runtime/plugin registry internals private while adding small host-facing helper APIs for compatibility checks and registration error diagnostics.
- Rationale: host applications and plugin managers need predictable lifecycle/status reporting without exposing mutable registry internals as stable contract.
- Outcome: added `isPluginTargetCompatible(...)` and `pluginRegistrationErrorMessage(...)` to `engine/public/plugins.h`; retained existing registration/unregistration behavior.
- Compatibility expectation: plugin targets must match runtime major and not exceed runtime minor; patch versions are non-breaking.
## 2026-03-08 — Decision: Creator docs are workflow-first and split by task
- **Context:** Existing docs had useful detail but mixed internal notes, stale examples, and overlapping pages that increased onboarding friction for external creators.
- **Decision:** Keep a short index-style `AuthoringGuide.md` and maintain focused workflow guides (build/run, import, palette/grayscale, patterns, encounter/boss, replay/debug, audio, sample usage, troubleshooting, plugins/mods, performance).
- **Rationale:** External creators need dependable, executable workflows more than architecture prose during first contact with the engine.
- **Consequence:** Lower doc drift risk (single owner page per workflow) and clearer upgrade path for future creator-facing changes.
## 2026-03-08 — Release packaging reliability + sample bundle validation hardening
- **Context**: Release scripts packaged expected binaries/content but had weak dependency discovery (`SDL2.dll` only), no signed inventory/trace artifact, and no explicit sample-pack execution check from the produced portable folder.
- **Decision**: Extend packaging to (1) discover/copy all runtime DLLs from release build output, (2) generate `RELEASE_MANIFEST.txt` with file inventory + SHA-256 hashes, and (3) run replay verification using `sample-content.pak` from within the packaged bundle.
- **Decision**: Extend release validation to require the manifest and verify required entry fragments for core release artifacts.
- **Rationale**: Improves practical distribution readiness and troubleshooting confidence while preserving existing successful build/package behavior.
## 2026-03-08 — Run-structure driven by authored encounter zones
- **Context**: Vertical-slice validation required encounter pacing (combat/elite/event/boss) to run through authored content-pack data, but runtime stage flow was still sourced from hardcoded defaults.
- **Decision**: Add a content-pack boot path that reads `encounters[].zones[]` and hydrates `RunStructure` stages from authored encounter definitions; keep default stage initialization as fallback when packs are missing/invalid.
- **Rationale**: Validates intended workflow end-to-end (author JSON -> ContentPacker -> Runtime) and avoids bespoke demo-only runtime paths.
## 2026-03-08 — Pattern panel responsibility split inside editor tooling
- **Context**: After the first editor decomposition, the pattern panel still mixed generation tuning, seed/testing actions, graph editing, and preview diagnostics in one dense function body.
- **Decision**: Keep a single Pattern Graph Editor window for workflow continuity, but split implementation into focused helper modules for generation controls, seed/testing, graph editing, preview-asset construction, and simulation/analysis rendering.
- **Rationale**: Preserves user flow and existing behavior while reducing coupling, clarifying ownership, and creating extension points for future tooling features.
## 2026-03-08 — ContentPacker runtime-audio dependency trim
- **Context**: `ContentPacker` previously linked SDL2 + SDL2_mixer due to older transitive coupling from content pipeline headers into runtime audio APIs.
- **Decision**: Keep audio-content parsing in the content pipeline (`audio_content.h` / `parseAudioContentDatabase`) but remove direct SDL2 and SDL2_mixer linkage from the `ContentPacker` target.
- **Rationale**: The packer compiles JSON/schema parsing utilities only and no longer includes/runtime-calls SDL audio or mixer APIs, so runtime playback libraries are unnecessary for this tool target.
## 2026-03-08 — Rebuild reliability hardening and validation
- **Context**: Prior audits flagged potential divergence between source edits and incremental build state, especially around generated/versioned configuration artifacts.
- **Decision**: Add `CMAKE_CONFIGURE_DEPENDS` for `version/VERSION.txt` and explicitly validate both clean and incremental rebuild paths in the same environment.
- **Rationale**: Automatic reconfigure on version input changes removes a subtle stale-config class while preserving current target/link structure.
- **Status**: Accepted.

## 2026-03-08 — ControlCenterToolSuite modular editor decomposition
- **Context**: Audit findings identified `editor_tools.cpp` as an overly broad tooling monolith spanning workspace shell, content browser, pattern graph authoring, palette/FX controls, diagnostics, encounter authoring, and shared utility logic.
- **Decision**: Keep the public `ControlCenterToolSuite` API stable, but split implementation into focused modules: core lifecycle/orchestration, workspace+content browser panels, pattern/encounter/trait/projectile panels, and shared editor services.
- **Rationale**: Reduces coupling between UI domains and shared runtime-facing logic, improves maintainability, and creates explicit extension points for future panels/services while preserving existing behavior.
- **Status**: Accepted.


## 2026-03-08 — Content pipeline/audio runtime boundary cleanup
- **Context**: `content_pipeline.h` included `audio_system.h`, creating an inverted dependency where generic content interfaces pulled in runtime SDL_mixer headers and forced tooling targets to link/runtime-audio dependencies.
- **Decision**: Split audio content schema definitions (`AudioBus`, `AudioEventId`, `AudioClipRecord`, `AudioEventBinding`, `AudioContentDatabase`) into `audio_content.h`; update `content_pipeline.h` and `audio_system.h` to depend on that shared lightweight header.
- **Rationale**: Restores architectural layering: content pipeline types stay data-focused while runtime playback/system concerns remain in `audio_system`.
## 2026-03-08 — Consolidated FetchContent dependency registration
- **Context**: Dependency setup in top-level CMake remained operational but fragmented across repeated declaration blocks, making wiring audits harder and inviting accidental duplication during future additions.
- **Decision**: Introduce a single dependency registration helper (`engine_register_dependency`) that appends each third-party package to one canonical list, then materialize once via `FetchContent_MakeAvailable(${ENGINE_FETCHCONTENT_DEPENDENCIES})`.
- **Rationale**: Keeps behavior unchanged while making third-party setup auditable, reducing redundancy risk, and clarifying target availability order for downstream linking.
- **Status**: Accepted.


## 2026-03-08 — RenderPipeline projectile routing de-duplication
- **Context**: `RenderPipeline::buildSceneOverlay` still contained duplicate projectile routing checks, which obscured ownership boundaries and risked divergent path behavior.
- **Decision**: Keep a single `ProjectileRenderPath` resolution per frame-overlay build and remove the duplicated fallback branch. Keep particle/debug overlays keyed from the same resolved path.
- **Decision**: Rename the secondary palette-ramp member to `proceduralPaletteRamp_` to distinguish procedural sprite-generation staging from GL shader-authoritative `paletteRamp_`.
- **Rationale**: Preserves behavior while making renderer ownership boundaries auditable and less error-prone.
- **Status**: Accepted.

## 2026-03-08 — Clarify renderer stack ownership boundaries
- **Context**: Rendering ownership had overlapping language across `gpu_bullets`, `gl_bullet_renderer`, `modern_renderer`, `render_pipeline`, and `render2d`, with projectile route checks duplicated inside `RenderPipeline`.
- **Decision**: Make `RenderPipeline` the explicit owner of projectile render-path selection via `ProjectileRenderPath` (`Disabled`, `ProceduralSpriteBatch`, `GlInstanced`) and rename `useModernRenderer_` to `modernPipelineEnabled_` to clarify that modern mode is a compositing backend toggle.
- **Decision**: Document subsystem roles in headers and specs: `gl_bullet_renderer` = projectile GL draw backend, `render2d` = generic 2D primitives, `modern_renderer` = post-fx/compositing, `gpu_bullets` = CPU mass-render alternate mode.
- **Rationale**: Reduces path ambiguity without changing gameplay or render behavior, and keeps authoritative projectile ownership in simulation systems.
- **Status**: Accepted.


## 2026-03-08 — Product validation via authored vertical slice
- **Context**: Audit follow-up required proving HellEngine as a coherent product loop (combat → encounter flow → boss phases → replay/package viability), not just subsystem completeness.
- **Decision**: Treat the vertical slice as an authored content-pack workflow artifact: expand encounter metadata, wire slice run/replay instructions, and validate through existing build/test/package scripts rather than bespoke runtime toggles.
- **Rationale**: Keeps the validation representative of shipping workflows and avoids architectural bypasses/hardcoded demo paths.
- **Status**: Accepted.


## 2026-03-07 — Build hygiene reliability guardrails
- **Context**: Audit found stale-state risk factors in the build graph (duplicate dependency materialization calls, potential in-source cache pollution, and generated-header path assumptions).
- **Decision**: Enforce out-of-source builds, create generated include directory explicitly before `configure_file`, and consolidate third-party dependency setup into a single `FetchContent_MakeAvailable(...)` pass.
- **Rationale**: Reduces clean/incremental divergence risk and keeps generated/header dependency state deterministic across rebuilds.
- **Status**: Accepted.

## 2026-03-07 — ContentPacker SDL_mixer dependency propagation fix
- **Context**: Visual Studio/CMake/Ninja builds failed in `ContentPacker` with `fatal error C1083: Cannot open include file: 'SDL_mixer.h'` when compiling content pipeline sources that include audio system headers.
- **Decision**: Keep `engine_core` dependencies unchanged and explicitly link `ContentPacker` to `${ENGINEDEMO_SDL_TARGET}` and `SDL2_mixer::SDL2_mixer`.
- **Rationale**: `ContentPacker` compiles `content_pipeline.cpp` transitively requiring SDL audio include/link usage; explicit target linkage is the smallest safe fix.
- **Status**: Accepted.


## 2026-03-07 — Projectile hot-path allocation cleanup
- **Context**: The legacy `ProjectileSystem::update()` compatibility wrapper and procedural fallback renderer still had allocation risks in hot paths.
- **Decision**: Keep `update()` for test compatibility but preallocate `legacyCollisionEvents_` to capacity during `initialize()`, and precompute all procedural bullet texture IDs (`64 palettes × 6 shapes`) once during initialization for indexed lookup in `renderProcedural()`.
- **Rationale**: Removes per-tick/per-frame heap churn while preserving compatibility and deterministic behavior across legacy and fallback execution paths.
- **Status**: Accepted.

## 2026-03-07 — Persistence schema and fallback contract
- **Context**: Save/profile/settings persistence existed in isolated paths but lacked a single migration strategy and corrupted-data fallback baseline.
- **Decision**: Add a dedicated persistence module with explicit schema versions (`v2` current), hard rejection of unknown future schemas, and explicit v1→v2 migrators for settings/profiles.
- **Rationale**: Keeps deterministic simulation decoupled from storage concerns while making future product-grade progression saves extensible and safe.
- **Status**: Accepted.

## 2026-03-07 — Camera shake vocabulary
- **Context**: A single sine-wave shake mode could not convey distinct gameplay feedback for hits, grazes, boss transitions, specials, and explosions.
- **Decision**: Introduce a six-profile camera shake vocabulary (`Impact`, `BossRumble`, `GrazeTremor`, `SpecialPulse`, `Explosion`, `Ambient`) implemented with additive blending, max 4 simultaneous active shakes, and ±20px clamp.
- **Rationale**: Improves feedback clarity while keeping shake logic presentation-only, frame-delta-driven, and decoupled from deterministic simulation state.
- **Status**: Accepted.

## 2026-03-07 — Pattern signature textures
- **Context**: Graph-authored patterns needed a compact visual fingerprint for editor/library browsing and optional per-pattern projectile identity.
- **Decision**: Generate a 64×64 signature texture per compiled pattern graph at load time by running a short throwaway VM simulation and rasterizing emitted projectile trajectories into a blurred radial density map.
- **Rationale**: Provides deterministic, low-memory visual identity per pattern without affecting live simulation state.
- **Status**: Accepted.

## 2026-03-06 — Danger field overlay
- **Context**: Players need lightweight subconscious guidance about bullet density without changing deterministic simulation behavior.
- **Decision**: Add a presentation-only, low-resolution danger field overlay generated from projectile collision-grid occupancy, smoothed with a box blur, gradient-mapped (blue→yellow→red), and additively blended over the scene.
- **Rationale**: Reuses existing broadphase data for cheap per-frame density estimation, improving readability while keeping sim/render boundaries clean.
- **Status**: Accepted.

## 2026-03-06 — Parallax background layering
- **Context**: The runtime renderer used a flat clear color with no depth cues behind gameplay actors.
- **Decision**: Add `BackgroundSystem` with three startup-generated procedural layers (hash-noise deep space, low-alpha grid, and sparse particle dust) using bounded parallax tiling.
- **Rationale**: Improves spatial readability and scene depth while keeping rendering presentation-only and deterministic-sim independent.
- **Status**: Accepted.

## 2026-03-04 — Phase 1 Scope Enforcement
- **Context**: Phase requires documentation ingestion only.
- **Decision**: Restrict outputs to `docs/MasterSpec.md`, `docs/DecisionLog.md`, and `docs/ImplementationPlan.md`.
- **Rationale**: Explicit user instruction for this phase.
- **Status**: Accepted.

## 2026-03-04 — Layered Architecture Lock-In
- **Context**: Requirements define a runtime+tooling layered architecture.
- **Decision**: Adopt canonical layers: CoreRuntime, Simulation, GameplaySystems, PatternRuntime, Render, AssetPipeline, EditorAPI/Tooling.
- **Rationale**: Maintains deterministic boundaries and clean dependency flow.
- **Status**: Accepted.

## 2026-03-04 — Deterministic Math Policy
- **Context**: Design permits fixed-point or deterministic float.
- **Decision**: Prefer fixed-point-capable implementation, retain deterministic-float compatibility mode under strict validation.
- **Rationale**: Strong parity for replay/network while preserving practical adoption path.
- **Status**: Accepted.

## 2026-03-04 — Bullet Motion Composition Order
- **Context**: Multiple motion operators (steering, integration, offsets, constraints) can produce divergent results if ordered inconsistently.
- **Decision**: Canonical per-tick order is Steering -> Velocity Integration -> Parametric Offsets -> Constraints -> Final Position.
- **Rationale**: Ensures reproducibility and consistent designer expectations.
- **Status**: Accepted.

## 2026-03-04 — Hybrid ECS + Bullet World
- **Context**: Pure ECS flexibility conflicts with 10k+ projectile throughput requirements.
- **Decision**: Use general ECS for broad gameplay entities and specialized Bullet World for bullets.
- **Rationale**: Balances flexibility and deterministic high-throughput performance.
- **Status**: Accepted.

## 2026-03-04 — Unified Pattern IR from Graph/Timeline/DSL
- **Context**: Three authoring modalities are required.
- **Decision**: Compile all modalities to a single Pattern IR/bytecode representation.
- **Rationale**: Avoids behavior mismatch and centralizes deterministic runtime semantics.
- **Status**: Accepted.

## 2026-03-04 — Indexed RNG Discipline
- **Context**: Traditional sequential RNG calls can diverge under branch differences.
- **Decision**: Use stream/substream derivation and prefer indexed hash-based randomness for pattern variation.
- **Rationale**: Minimizes call-order drift and hardens replay consistency.
- **Status**: Accepted.

## 2026-03-04 — Deterministic Trigger/Collision Tie-Breaks
- **Context**: Simultaneous triggers/collisions require deterministic resolution.
- **Decision**: Enforce stable tie-break keys (tick/order key, stable IDs, then deterministic sort).
- **Rationale**: Prevents non-deterministic branch outcomes in phase transitions and hit resolution.
- **Status**: Accepted.

## 2026-03-04 — CPU-First Sim, GPU-First Rendering
- **Context**: GPU sim is possible but increases determinism/debugging complexity.
- **Decision**: Keep simulation CPU-first; use GPU heavily for rendering/visualization.
- **Rationale**: Improves tooling introspection and reduces implementation risk.
- **Status**: Accepted.

## 2026-03-04 — Allocation and Degradation Policy
- **Context**: Overload scenarios can threaten frame stability.
- **Decision**: No per-tick dynamic allocations in shipping sim; degrade visuals/effects before sim correctness.
- **Rationale**: Maintains fairness and deterministic integrity under pressure.
- **Status**: Accepted.

## 2026-03-04 — Optional Advanced Features Phasing
- **Context**: Networking/plugins/mod runtime expansion is high value but non-foundational for core deterministic runtime.
- **Decision**: Deliver optional advanced features in later phases behind feature flags.
- **Rationale**: Keeps early phases focused on deterministic kernel and buildability.
- **Status**: Accepted.


## 2026-03-04 — Pattern Graph Dual-Flow Contract
- **Context**: Pattern authoring requires both sequencing and typed parameter computation.
- **Decision**: Standardize Pattern Graph as dual-flow (Exec + Data) with explicit type conversions and deterministic merge ordering for parallel branches.
- **Rationale**: Improves designer clarity while preserving deterministic compilation/runtime behavior.
- **Status**: Accepted.

## 2026-03-04 — Modifier Stack Canonicalization
- **Context**: Modifier order materially changes output and can create inconsistent authored behavior.
- **Decision**: Enforce explicit modifier stack ordering (Parameter -> Angle -> Position -> Post-spawn bindings) with editor warnings on conflicts.
- **Rationale**: Stabilizes visual results and replay parity across edits and refactors.
- **Status**: Accepted.

## 2026-03-04 — Pattern Graph Compilation Requirements
- **Context**: Visual graphs need commercial usability without runtime overhead.
- **Decision**: Require validation/lowering/optimization/codegen pipeline producing cached Pattern IR plus node-to-IR debug mapping.
- **Rationale**: Ensures performance, deterministic execution, and high-quality tooling introspection.
- **Status**: Accepted.

## 2026-03-04 — Pattern Graph Debuggability Baseline
- **Context**: Designers require fast diagnosis of unfair or expensive patterns.
- **Decision**: Make provenance tracing, node execution highlights, breakpoints/watches, and heatmap overlays mandatory Pattern Graph tooling features.
- **Rationale**: Reduces iteration cost and improves fairness/performance debugging quality.
- **Status**: Accepted.


## 2026-03-04 — PBAGS Generation Boundary
- **Context**: Procedural generation can drift into opaque emergent behavior.
- **Decision**: PBAGS may only compose vetted authored modules and parameterize them; it must not synthesize unvetted runtime logic.
- **Rationale**: Maintains trust, debuggability, and deterministic predictability.
- **Status**: Accepted.

## 2026-03-04 — Deterministic Selection and Synthesis Policy for PBAGS
- **Context**: Weighted random generation and parameter tuning can break replay parity without strict discipline.
- **Decision**: Use deterministic weighted sampling with stream discipline and indexed parameter synthesis, followed by deterministic bounded constraint adjustments.
- **Rationale**: Preserves replay/network consistency while still enabling procedural variety.
- **Status**: Accepted.

## 2026-03-04 — Budget-First Difficulty Governance
- **Context**: Difficulty must be controlled across density/speed/entropy/aim pressure, not a single scalar.
- **Decision**: Govern PBAGS by explicit multi-dimensional budgets with deterministic fallback order (reduce secondary layers, then density, speed, aim aggression, then module swap).
- **Rationale**: Keeps generated attacks fair, readable, and performance-safe.
- **Status**: Accepted.

## 2026-03-04 — Lock/Regenerate Authoring Workflow
- **Context**: Designers need partial control over generated outputs.
- **Decision**: Require lock/regenerate granularity at segment/motif/parameter levels with explainable per-segment reports.
- **Rationale**: Enables procedural speed without sacrificing authored intent.
- **Status**: Accepted.

## 2026-03-04 — Deterministic Adaptive Variant Rule
- **Context**: Adaptive difficulty can invalidate replays if generated online.
- **Decision**: Allow adaptation only through pre-generated variants selected deterministically from recorded performance bands.
- **Rationale**: Retains adaptability while preserving strict replay determinism.
- **Status**: Accepted.


## 2026-03-04 — AI Copilot Boundary (Pattern Generation)
- **Context**: AI-assisted tooling can become opaque and undermine deterministic trust.
- **Decision**: AI may propose blueprint structure only; final outputs must be concretized through approved deterministic graph templates and constraint solvers.
- **Rationale**: Preserves predictability, auditability, and runtime safety.
- **Status**: Accepted.

## 2026-03-04 — Tiered Generator Strategy
- **Context**: Product roadmap includes rule-based and model-assisted options.
- **Decision**: Tier 1 (rule/template) is required shipping baseline; Tier 2+ (LLM suggestion/ranking models) are optional enhancements behind safety boundaries.
- **Rationale**: Ensures production viability without ML dependency while allowing future expansion.
- **Status**: Accepted.

## 2026-03-04 — Explainability as a Hard Requirement
- **Context**: Designer trust requires understanding why a generated pattern exists.
- **Decision**: Generation reports, node annotations, constraint satisfaction summaries, and selective lock/regenerate controls are mandatory.
- **Rationale**: Supports rapid iteration and safer tuning under constraints.
- **Status**: Accepted.

## 2026-03-04 — Difficulty Target Fitting Policy
- **Context**: Difficulty goals require both fast estimates and deeper simulation.
- **Decision**: Use immediate analytical metrics plus optional deterministic bot simulation, then bounded deterministic tuning toward explicit target bands.
- **Rationale**: Balances responsiveness with accuracy and replay consistency.
- **Status**: Accepted.


## 2026-03-04 — Physics Scope Lock (Overlap-Only)
- **Context**: Bullet-hell physics needs throughput and deterministic predictability over realism.
- **Decision**: Baseline physics is overlap-based (no rigidbody solver/impulse stack/continuous-by-default pipeline).
- **Rationale**: Minimizes complexity and preserves deterministic performance at high bullet counts.
- **Status**: Accepted.

## 2026-03-04 — Uniform Grid Broadphase Standard
- **Context**: Candidate structures include uniform grid, spatial hash, quadtree.
- **Decision**: Use uniform grid (or deterministic hash-grid variant) as baseline broadphase, with optional multi-grid extension for mixed collider scales.
- **Rationale**: Best fit for dense bounded bullet fields and allocation-free deterministic implementation.
- **Status**: Accepted.

## 2026-03-04 — Fixed-Capacity Buckets and Overflow Policy
- **Context**: Unbounded per-cell lists violate allocation/predictability constraints.
- **Decision**: Use fixed-capacity cell buckets plus deterministic overflow handling and diagnostics.
- **Rationale**: Enforces no-allocation sim loop while preserving deterministic fallback behavior.
- **Status**: Accepted.

## 2026-03-04 — Collision Event Precedence Contract
- **Context**: Simultaneous shield/hit/graze interactions can produce ambiguous outcomes.
- **Decision**: Enforce explicit precedence (Shield -> PlayerHit -> Graze -> EnemyHit -> Hazard) with stable event-sort keys before resolution.
- **Rationale**: Guarantees replay-identical outcomes and reduces gameplay ambiguity.
- **Status**: Accepted.

## 2026-03-04 — CPU-First Gameplay Collision
- **Context**: GPU collision acceleration increases debug and determinism risk.
- **Decision**: Keep gameplay collision CPU-first; reserve GPU for rendering and optional analysis overlays.
- **Rationale**: Preserves deterministic debugging and tool transparency.
- **Status**: Accepted.

## 2026-03-04 — Graze Tracking Data Policy
- **Context**: Graze logic can create hidden allocation/set-tracking overhead.
- **Decision**: Graze eligibility must be stored per bullet via flags/ticks only (no dynamic sets/maps).
- **Rationale**: Keeps hot-path memory predictable and deterministic.
- **Status**: Accepted.


## 2026-03-04 — Deterministic World-Run Structure
- **Context**: Roguelite runs require procedural variation without replay drift.
- **Decision**: Run progression is deterministic by seed across four generated worlds plus final boss, with reproducible node maps and encounter selections.
- **Rationale**: Enables fair debugging, daily challenge parity, and reliable replay verification.
- **Status**: Accepted.

## 2026-03-04 — World Graph Topology Rule
- **Context**: Procedural maps can become unreadable or unbalanced without structural guardrails.
- **Decision**: World layouts use constrained DAG generation with mandatory node-distribution rules (elite + mutation presence, mid-boss before boss, balanced spacing).
- **Rationale**: Preserves strategic pathing and progression quality while retaining variation.
- **Status**: Accepted.

## 2026-03-04 — Budget-Driven Encounter Synthesis
- **Context**: Encounter randomness can cause unfair spikes.
- **Decision**: Generate encounters from explicit threat budgets allocating pressure across roles, waves, density, and hazards.
- **Rationale**: Keeps combat variation within deterministic fairness/performance envelopes.
- **Status**: Accepted.

## 2026-03-04 — Event/Hazard Integration Policy
- **Context**: Non-combat systems and hazards can destabilize pacing.
- **Decision**: Events and hazards are world-aware generation layers with deterministic selection, progressive introduction, and budget coupling.
- **Rationale**: Maintains pacing coherence and prevents runaway complexity.
- **Status**: Accepted.

## 2026-03-04 — Designer Governance Requirement for World Gen
- **Context**: Fully autonomous procedural generation can erode authored intent.
- **Decision**: Require explicit designer controls for weights, budget ranges, pacing constraints, and lock/override capabilities.
- **Rationale**: Ensures procedural outputs remain art-directed and tunable.
- **Status**: Accepted.


## 2026-03-04 — Product Scope Boundary (1.0)
- **Context**: Broad engine ambitions can dilute delivery and destabilize milestones.
- **Decision**: 1.0 scope centers on deterministic high-throughput 2D bullet-hell runtime, tools, and content pipeline; general-purpose 3D physics and open-world streaming remain post-1.0.
- **Rationale**: Preserves focus on core market value and execution reliability.
- **Status**: Accepted.

## 2026-03-04 — Tooling Co-Evolution Mandate
- **Context**: Late tooling development causes unusable runtime systems and slow iteration.
- **Decision**: Tooling must co-evolve with runtime from early phases, with preview/debug/profiler capabilities introduced alongside core systems.
- **Rationale**: Ensures workflow validation and accelerates content iteration.
- **Status**: Accepted.

## 2026-03-04 — Milestone-Gated Delivery Policy
- **Context**: Large roadmap requires objective readiness checks.
- **Decision**: Enforce milestone gates from architecture lock through release candidate, including two mandatory vertical slices (10k deterministic stress and full tool-authored boss fight).
- **Rationale**: Reduces schedule risk and prevents premature scope expansion.
- **Status**: Accepted.

## 2026-03-04 — CI Replay Verification Requirement
- **Context**: Determinism regressions are high risk and hard to detect manually.
- **Decision**: Replay hash verification and divergence diagnostics are mandatory CI checks for curated replay sets.
- **Rationale**: Provides continuous determinism guarantees during development.
- **Status**: Accepted.


## 2026-03-04 — Editor Layout Canonical Model
- **Context**: Tool sprawl and inconsistent paneling hurt authoring speed.
- **Decision**: Standardize a dockable five-region editor model (top toolbar, left content/outline, center document tabs, right inspector/params/docs, bottom diagnostics).
- **Rationale**: Provides predictable workflows and lowers cognitive overhead across tools.
- **Status**: Accepted.

## 2026-03-04 — Preview-Centric Authoring Principle
- **Context**: Bullet-hell content iteration depends on immediate sim feedback.
- **Decision**: Keep preview controls (play/pause/step/scrub, difficulty, bot, seed lock/reset, overlays, determinism badge) globally accessible in primary authoring workflows.
- **Rationale**: Shortens design-debug loops and improves determinism visibility.
- **Status**: Accepted.

## 2026-03-04 — Explainability UX Requirement
- **Context**: Complex generated/authored patterns are hard to tune without provenance.
- **Decision**: Require click-to-explain bullet provenance, node annotations, contextual docs, and profiler-to-asset deep links across pattern/projectile/boss/wave tools.
- **Rationale**: Increases trust and tuning velocity while reducing debugging ambiguity.
- **Status**: Accepted.

## 2026-03-04 — Non-Intrusive Validation UX Policy
- **Context**: Aggressive warning UX can interrupt authoring flow.
- **Decision**: Validation issues appear as subtle badges/panels with severity, explanation, and quick-fix navigation rather than blocking modal interruptions.
- **Rationale**: Preserves flow while still enforcing quality/determinism constraints.
- **Status**: Accepted.


## 2026-03-04 — Phase 3 Runtime Service Baseline
- **Context**: Phase 3 requires foundational runtime services and headless simulation execution.
- **Decision**: Implement baseline runtime modules (`logging`, `config`, `timing`, `job_system`, `deterministic_rng`, `memory`, `runtime`) and wire headless mode into `EngineDemo`.
- **Rationale**: Establishes deterministic core loop and testable runtime primitives before higher-level gameplay systems.
- **Status**: Implemented.

## 2026-03-04 — Testing Baseline for Phase 3
- **Context**: User requested unit tests for timing and configuration.
- **Decision**: Add `timing_tests` and `config_tests` as CTest targets, and include headless simulation command validation in build docs.
- **Rationale**: Provides immediate regression coverage for deterministic timing and runtime configuration parsing.
- **Status**: Implemented.

## 2026-03-05 — Runtime Decomposition (Phase 1)
- **Decision**: Decomposed runtime responsibilities into `InputSystem`, `RenderPipeline`, and `GameplaySession`.
- **Rationale**: Reduce god-object coupling and establish clearer ownership boundaries while retaining deterministic fixed-step orchestration.
- **Status**: Accepted.

## 2026-03-05 — SimSnapshot Contract
- **Decision**: Introduced `SimSnapshot` as a simulation-to-render read contract consumed by `RenderPipeline`.
- **Rationale**: Prevent render-side back-channel mutation and make frame composition boundaries explicit.
- **Status**: Accepted.

## 2026-03-05 — Collision Pipeline Stage Split (Phase 2)
- **Decision**: Split projectile collision pipeline into `updateMotion()`, `buildGrid()`, and `resolveCollisions()` stages.
- **Rationale**: Separate deterministic motion integration from broadphase construction and narrowphase resolution to support multi-target collision queries.
- **Status**: Accepted.

## 2026-03-05 — Deterministic Collision Event Ordering
- **Decision**: Sort `CollisionEvent` output by `(targetId, bulletIndex)` before processing.
- **Rationale**: Enforces deterministic event ordering independent of grid bucket traversal order.
- **Status**: Accepted.

## 2026-03-05 — Grid-query Collision Replaces Inline Single-target Checks
- **Decision**: Use overlapping-grid-cell queries against `CollisionTarget` AABBs in `resolveCollisions()` instead of inline player-only distance checks inside projectile update.
- **Rationale**: Enables multi-target collision support with existing spatial partitioning data.
- **Status**: Accepted.

## 2026-03-05 — Golden Replay Hash Note
- **Decision**: No golden replay hash updates were applied in this phase.
- **Rationale**: Collision ordering was made deterministic and validated in targeted tests; golden replay target was not modified here.
- **Status**: Recorded.

## 2026-03-05 — FP Determinism Hardened
- **Decision**: Added deterministic floating-point compilation policy (`/fp:strict` on MSVC, `-ffp-contract=off -fno-fast-math` on GCC/Clang) for `engine_core`, introduced `engine::dmath` wrappers, and added cross-config determinism test coverage.
- **Rationale**: Centralizes simulation transcendental math through a single choke-point and reduces build-configuration drift risks for replay/hash determinism.
- **Status**: Accepted.

## 2026-03-05 — GpuBulletSystem O(1) Slot Management
- **Decision**: Replaced O(N) linear-scan slot allocation/counting in `GpuBulletSystem` with free-list allocation and cached `activeCount_`.
- **Rationale**: Stabilizes hybrid mass-bullet mode under very high emit rates and removes per-call full-buffer scans from hot paths.
- **Status**: Accepted.

## 2026-03-05 — Compact Active-List Iteration for ProjectileSystem
- **Decision**: Replaced O(capacity) loops with skip-branches by O(active) iteration using `activeIndices_` and cached `activeCount_`.
- **Rationale**: Reduces hot-path iteration cost in update/render/debug/graze/hash paths while preserving deterministic order via sort-on-removal at tick end.
- **Status**: Accepted.


## 2026-03-06 — Per-bullet palette colorization
- **Context**: Projectile rendering used allegiance hardcoded colors while palette templates already define richer projectile colors.
- **Decision**: Add visual-only `paletteIndex` projectile SoA data, route per-shot palette assignment through emission paths, and introduce `BulletPaletteTable` built from `PaletteFxTemplateRegistry`/`deriveProjectileFillFromCore`.
- **Rationale**: Preserves deterministic simulation/replay hashes while enabling authored palette-driven projectile colors.
- **Status**: Accepted.

## 2026-03-06 — Procedural Bullet Sprites
- **Context**: Bullet rendering previously relied on an external texture asset and single-shape tinting.
- **Decision**: Procedural bullet sprites are generated at render startup via SDFs from palette-derived colors, with six built-in shapes (circle, rice, star, diamond, ring, beam) and no required external bullet art.
- **Rationale**: Eliminates sprite-sheet dependency while preserving palette identity and shape readability.
- **Status**: Accepted.

## 2026-03-06 — Projectile Trail System
- **Context**: Fast-moving projectile archetypes benefit from motion readability, while deterministic simulation state must remain unchanged.
- **Decision**: Add an opt-in visual-only projectile trail system using a fixed per-bullet ring buffer of 4 past positions and faded afterimage rendering before the main sprite.
- **Rationale**: Improves legibility at low memory overhead (~32 bytes per bullet) without affecting replay hashes.
- **Status**: Accepted.

## 2026-03-06 — Impact particles visual-only boundary
- **Context**: Projectile despawn/hit feedback needs readability without affecting deterministic simulation.
- **Decision**: Add `ParticleFxSystem` as a render-layer-only subsystem fed by projectile despawn events, updated on frame delta and excluded from collision/state hash/replay determinism.
- **Rationale**: Preserves simulation determinism while improving visual clarity for bullet hit/expire events.
- **Status**: Accepted.

## 2026-03-07 — Gradient animation phase-wave mapping
- **Context**: Bullet palettes include gradient + animation settings but runtime projectile rendering only used static palette colors.
- **Decision**: Add `GradientAnimator` with precomputed LUT sampling and per-bullet phase offset (`instanceIndex * perInstanceOffset`) applied at render time, then route animated palettes through this sampler in projectile rendering.
- **Rationale**: Produces the signature rotating wave look across emitted rings while keeping simulation deterministic and unchanged.
- **Status**: Accepted.

## 2026-03-07 — Procedural level tiles: cellular automata + zone-typed rules, seed-deterministic, 256px tileable
- **Context**: Stage/zone transitions needed stronger visual identity without authored texture dependencies.
- **Decision**: Add `LevelTileGenerator` to create deterministic, tileable 256x256 background textures at zone transitions using wrapped cellular automata/value-noise and zone-type-to-rule mapping.
- **Rationale**: Preserves deterministic replay behavior while giving each stage/zone distinct palette-driven presentation from run seed alone.
- **Status**: Accepted.

## 2026-03-07 — OpenGL 3.3 Core hybrid context alongside SDL_Renderer
- **Context**: SDL_Renderer fixed-function path cannot run custom bullet shaders.
- **Decision**: Add an OpenGL 3.3 Core context plus GLAD loader at render initialization while retaining SDL_Renderer for ImGui + debug/UI drawing.
- **Rationale**: Enables custom shader pipeline and generated sprite-atlas textures with graceful fallback to SDL_Renderer-only when GL context/loader init fails.
- **Status**: Accepted.

## 2026-03-07 — OpenGL bullet renderer: single draw call, grayscale SDF + palette ramp shader
- **Context**: CPU deterministic projectile simulation was rendering bullets through per-sprite `SpriteBatch` geometry, increasing draw overhead at high bullet counts.
- **Decision**: Add `GlBulletRenderer` that rebuilds a preallocated dynamic CPU->GPU vertex/index buffer each frame from projectile SoA data and renders all bullets in one `glDrawElements` call using grayscale atlas + palette ramp shading.
- **Rationale**: Preserves deterministic CPU authority while achieving GPU-efficient batching for stress targets (10k bullets) and keeping SDL sprite rendering as fallback when OpenGL is unavailable.
## 2026-03-07 — Palette ramp texture: 64×64 GL_TEXTURE_2D, Band3 and GradientRamp modes, per-row hot-reload
- **Context**: Bullet shaders need stable palette sampling with both authored gradients and legacy 3-band palette behavior.
- **Decision**: Add `PaletteRampTexture` that builds a 64px-wide RGBA ramp per palette row from the palette template registry, supporting Band3 and GradientRamp generation plus `glTexSubImage2D` single-row hot-reload.
- **Rationale**: Keeps palette lookup deterministic, tiny in memory footprint (~16KB), and fast to patch when JSON palette definitions change.
- **Status**: Accepted.

## 2026-03-07 — Real post-processing shaders: Kawase bloom, vignette, tone mapping, chromatic aberration, film grain, scanlines
- **Context**: Existing post-FX path used fake SDL overlays, so editor controls could not drive physically plausible bloom, tone mapping, or camera-style post passes.
- **Decision**: Replace fake post overlays with shader-driven passes in `RendererModernPipeline`: half-resolution Kawase bloom (threshold + 4 blur iterations + additive composite), vignette pass, and final composite pass including tone mapping, exposure/contrast/saturation, chromatic aberration, film grain, and scanlines.
- **Rationale**: Preserves presentation-only coupling while upgrading visual quality and making PostFx controls map directly to shader uniforms and palette FX presets.
- **Status**: Accepted.

## 2026-03-07 — GameplaySession responsibility split into runtime state partitions
- **Context**: `GameplaySession` was carrying simulation orchestration, player combat state, progression UI flow, presentation effects, debug/tool wiring, and encounter collision scratch buffers in one undifferentiated object.
- **Decision**: Introduce explicit state partitions (`SessionSimulationState`, `PlayerCombatState`, `ProgressionState`, `PresentationState`, `DebugToolState`, `EncounterRuntimeState`) as owned sub-objects inside `GameplaySession`, and route runtime/render integration through those boundaries.
- **Rationale**: Preserves deterministic behavior while reducing responsibility overlap and making ownership and integration points testable.
- **Status**: Accepted.


## 2026-03-07 — Modularized ControlCenter tool panels
**Context:** `src/engine/editor_tools.cpp` had grown into a high-coupling editor monolith where menu shelling, authoring state initialization, validation, and all panel UI were interleaved in one function.

**Decision:** Split editor rendering into dedicated panel methods (`drawWorkspaceShell`, `drawPatternGraphEditorPanel`, `drawEncounterWaveEditorPanel`, `drawPaletteFxEditorPanel`, `drawValidationDiagnosticsPanel`, etc.) and extracted shared editor service helpers (`buildEncounterAsset`, panel state seeding helpers).

**Consequences:**
- Reduced edit blast radius for tool changes.
- Clearer ownership boundaries between panel UI and shared editor services.
- Preserved user-visible behavior while preparing plugin and panel expansion.
- Runtime/editor split is cleaner because panel methods consume immutable runtime snapshots and mutate only editor-scoped state.

## 2026-03-07 — Decision: integrate source-art import into ContentPacker

- Implemented source-art import as a first-class stage in `ContentPacker` rather than standalone scripts.
- Chosen data model: manifest-driven `art-import` JSON documents with explicit per-asset import settings.
- Added import fingerprinting and previous-pack comparison (`--previous-pack`) to support deterministic reimport/dependency invalidation.
- Atlas integration is represented as emitted `atlasBuild` plans keyed by `(atlasGroup, colorWorkflow)` so runtime/editor can consume build grouping deterministically.
- Grayscale and monochrome workflows are validated/imported directly through `colorWorkflow` and represented in pack metadata to support palette/shader workflows.


## 2026-03-07 — Data-driven animation/variant grouping in art import
- **Context**: Source-art import had basic registry/fingerprints but no production-ready animation clip or variant grouping output.
- **Decision**: Introduce explicit settings for animation/variant identity plus optional regex-based filename extraction so naming conventions remain configurable rather than hardcoded.
- **Decision**: Emit `animationBuild` (set/state/direction/fps/frame GUID list) and `variantBuild` (group/options/weights/palette template) into packs as canonical runtime/editor metadata.
- **Decision**: Reject malformed/ambiguous groups (invalid identifiers, duplicate variant names, duplicate frame indices, inconsistent clip FPS).
- **Consequences**:
  - Authors can choose explicit metadata or configured naming conventions.
  - Runtime/content packs now expose deterministic animation and variant group plans for procedural/themed selection.
  - Grayscale/palette workflow remains compatible via per-variant `paletteTemplate` annotation.

## DL-0020 — Core Audio Runtime Uses Event-Queued Presentation Model
- **Date**: 2026-03-07
- **Decision**: Added `AudioSystem` as a runtime presentation service fed by deterministic gameplay events, with bus-based volume (`master/music/sfx`) and pack-authored clip/event routing.
- **Rationale**: Preserves deterministic simulation boundaries while enabling commercial-grade feedback hooks for combat, boss telegraphs, and UI interactions.
- **Consequences**:
  - Audio authoring remains in content data (`audio` pack section), not hardcoded gameplay classes.
  - Runtime can be expanded to ducking/mix snapshots/event middleware without altering simulation contracts.

## DL-0021 — Enemy/Boss Runtime Eventization and Phase-Orchestrated Patterns
- **Date**: 2026-03-07
- **Context**: Enemy/boss runtime behavior mixed authoring concerns, state mutation, and presentation trigger logic in a shallow prototype structure.
- **Decision**:
  - Added boss phase multi-pattern authoring (`patternSequence`) and per-phase cadence (`patternCadenceSeconds`).
  - Added deterministic runtime event stream from `EntitySystem` (`BossIntroStarted`, `BossPhaseStarted`, `BossPhaseCompleted`, `BossDefeated`, `Telegraph`, `HazardSync`).
  - Moved presentation reactions to gameplay-session ownership by consuming runtime events there.
  - Extended encounter compiler node model with `telegraph`, `hazardSync`, and `phaseGate`, and annotated scheduled events with owner domains.
- **Consequences**:
  - Cleaner ownership boundary between simulation behavior and combat presentation.
  - Better future tooling integration: timeline/encounter editors can target explicit phase and hazard sync hooks.
  - Determinism preserved via deterministic sequence cursoring and per-tick event flush.


## 2026-03-07 — Reposition GPU Bullet Path as CPU Mass Render
- **Decision**: Repositioned the prior `GpuBulletSystem` terminology to `CpuMassBulletRenderSystem` and renamed mode labels to `CpuCollisionDeterministic`/`CpuMassRender`.
- **Rationale**: Audit required removal of naming ambiguity; current path is CPU-driven sim/prep with batched SDL geometry submission, not GPU compute simulation.
- **Status**: Accepted.

## 2026-03-07 — Active-slot iteration for CPU Mass Bullet Render
- **Decision**: Added compact `activeSlots_` + `slotToActiveIndex_` bookkeeping so update/render loops iterate only live bullets.
- **Rationale**: Improves scaling behavior and profiling clarity at high bullet counts while preserving deterministic update order within active set transitions.
- **Status**: Accepted.

## 2026-03-07 — Generalized content hot-reload
- **Context**: Runtime hot-reload covered only generated pattern graph handoff and did not provide a unified JSON content refresh path.
- **Decision**: Introduce `ContentWatcher` and deterministic tick-boundary polling for patterns, entities, traits, difficulty profiles, and palette templates; each reload validates into temporary state and swaps only on success.
- **Rationale**: Keeps replay determinism and protects runtime state by preserving previous content when parse/validation fails.
- **Status**: Accepted.


## 2026-03-07 — Audio system fallback boundary
- **Context**: Runtime feedback needed basic SFX without introducing deterministic simulation coupling or hard failures when audio assets/devices are missing.
- **Decision**: Use SDL_mixer-based `AudioSystem` owned by `Runtime`, feed it from a presentation-only `GameplaySession` audio-event queue, and dispatch sounds outside `simTick()`.
- **Rationale**: Keeps replay hashes and simulation state independent from audio timing/device state while allowing graceful silence when init/load fails.
- **Status**: Accepted.
## 2026-03-07 — Catch2 v3.5.2 test framework adoption and targeted migration
- **Context**: Test infrastructure mixed many standalone `main()` executables with manual assertions, limiting discovery/tag filtering and making property/fuzz style tests harder to scale.
- **Decision**: Adopt Catch2 v3.5.2 via CMake `FetchContent`, migrate five core test binaries (`projectile`, `projectile_behavior`, `pattern_graph`, `replay`, `collision_correctness`) to `TEST_CASE`/`REQUIRE`, and add determinism property and content fuzz test targets.
- **Rationale**: Improves test ergonomics and selective execution (`[determinism]`, `[fuzz]`) while preserving existing logic and leaving non-migrated tests unchanged.
- **Status**: Accepted.
## 2026-03-07 — Camera shake vocabulary: 6 profiles, additive blending, max 4 simultaneous
- **Context**: A single shake mode made combat feedback feel uniform and forced simulation code to hand-tune one-off shake parameters.
- **Decision**: Standardize camera shake around a six-profile vocabulary (`Impact`, `BossRumble`, `GrazeTremor`, `SpecialPulse`, `Explosion`, `Ambient`) implemented by `CameraShakeSystem`, with additive blending, a max of four active shakes, and total per-axis clamp to ±20 px.
- **Rationale**: Keeps shake presentation-only and deterministic per frame while giving gameplay hooks consistent, reusable semantics across hit, graze, boss, special, and ambience events.
- **Status**: Accepted.
## 2026-03-07 — GlBulletRenderer single-draw-call OpenGL bullet rendering with SpriteBatch fallback
- **Context**: Shader cache, grayscale sprite atlas, and palette ramp texture pipeline were available but runtime bullets still defaulted to CPU SpriteBatch quads.
- **Decision**: Wire `GlBulletRenderer` into `RenderPipeline` to rebuild one CPU vertex/index stream each frame and submit bullets + trails in a single OpenGL draw call when GL is available; keep existing `renderProcedural` fallback for non-GL/runtime-failure paths.
- **Rationale**: Preserves deterministic simulation ownership and replay/hash boundaries while unlocking GPU-side presentation throughput and reducing draw-call overhead.
- **Status**: Accepted.
## 2026-03-07 — External-facing creator documentation baseline
- **Context**: Documentation quality was strong for internal engineering, but external onboarding and creator workflow coverage was fragmented.
- **Decision**: Establish a dedicated external documentation set covering getting started, asset import, pattern authoring, boss/encounter authoring, replay/debug, plugin/mod extension overview, creator performance guidance, and troubleshooting.
- **Rationale**: Improves product readiness for creators/integrators while keeping documentation aligned to implemented runtime and content-pipeline behavior.
- **Status**: Accepted.
## 2026-03-07 — Release gating moved into build/package scripts
- **Decision**: promote release scripts from convenience wrappers to required gates.
- **Rationale**: audit identified productization gaps where tests/content/replay checks could be skipped by default.
- **Implementation**:
  - `tools/build_release.ps1` now runs tests, benchmarks, content pack build (default + sample), and replay verification by default.
  - `tools/package_dist.ps1` now performs portable self-validation before zip creation.
  - `tools/release_validate.ps1` added as end-to-end release gate.
- **Consequence**: slower release build command but substantially stronger confidence and failure diagnostics.

## 2026-03-07 — Centralized runtime pack compatibility version
- **Decision**: define runtime content compatibility as shared constant `engine::kRuntimePackVersion = 4`.
- **Rationale**: avoid drift from duplicated hardcoded values in entity/pattern loaders and packer outputs.
- **Consequence**: pack-version enforcement becomes explicit and aligned with generated pack metadata.

## 2026-03-07 — Palette ramp compile hotfix scope
- **Context**: `engine_core` build was blocked by duplicated `PaletteRampTexture` declarations and a renderer-side unresolved projectile allegiance symbol.
- **Decision**: Apply a surgical fix: deduplicate `palette_ramp.h/.cpp`, keep one canonical `texture_` + member set, expose `animationFor` publicly, and include `projectiles.h` in `gl_bullet_renderer.cpp`.
- **Rationale**: Restores compile integrity without redesigning palette or projectile rendering architecture.
- **Status**: Accepted.

## 2026-03-08 — Public API / Plugin Boundary Hardening
- **Decision:** Keep the external/public surface constrained to `engine/public` headers and avoid promoting internal runtime classes into the plugin contract.
- **Decision:** Introduce additive plugin metadata (`PluginMetadata`) plus compatibility validation against runtime public API version.
- **Decision:** Add explicit plugin lifecycle controls (`unregister*`, `clearRegisteredPlugins`) so hosts/tools can tear down extension state cleanly.
- **Rationale:** Commercial plugin ecosystems need predictable registration failure reasons and lifecycle boundaries while preserving ABI/API stability.
- **Compatibility:** Additive-only change (MINOR-safe): existing registration flow semantics are preserved for valid plugins; invalid inputs now produce typed rejection instead of silent ignore.

## 2026-03-08 — GameplaySession ownership facet split
- **Context**: `GameplaySession` still mixed session orchestration, player combat runtime transitions, progression navigation logic, and presentation event shaping in one update path.
- **Decision**: Introduce explicit subsystem interfaces in `gameplay_session_subsystems` (`PlayerCombatSubsystem`, `ProgressionSubsystem`, `PresentationSubsystem`) and route `GameplaySession::updateGameplay()` / `onUpgradeNavigation()` through those boundaries.
- **Rationale**: Keeps deterministic runtime behavior intact while isolating concern-specific logic into testable units with smaller APIs.
- **Migration Notes**: Existing `GameplaySession` state fields remain available, but gameplay mutation entry points now flow through subsystem interfaces.
- **Status**: Accepted.

## 2026-03-08 — GameplaySession encounter-runtime ownership split
- **Context**: After the initial subsystem split, `GameplaySession::updateGameplay()` still directly owned collision resolution flow, despawn visual reactions, zone transition/ambient presentation emission, and entity runtime-event fanout.
- **Decision**: Introduce `EncounterSimulationSubsystem` and route encounter-specific simulation/presentation coordination through it (`emitDespawnParticles`, `processRuntimeEvents`, `resolveCpuDeterministicCollisions`, `emitZoneTransitionFeedback`, `emitAmbientZoneFeedback`).
- **Rationale**: Reduces overloaded session orchestration code, establishes explicit encounter boundary ownership, and improves testability of encounter/presentation glue without altering deterministic behavior.
- **Migration Notes**: `GameplaySession` remains top-level deterministic coordinator; internal encounter responsibilities are delegated to subsystem methods while preserving public API and replay integration.
- **Status**: Accepted.

## 2026-03-08 — RenderPipeline compile hotfix for duplicated `buildSceneOverlay` signature
- **Context**: `render_pipeline.cpp` failed to compile with C2601/C1075 due to an unmatched opening brace near the `buildSceneOverlay(...)` region.
- **Decision**: Apply a surgical syntax fix by removing the stray duplicated `buildSceneOverlay(const SimSnapshot&, const double frameDelta)` opening block and retaining the existing `buildSceneOverlay(const SimSnapshot&, const double /*frameDelta*/)` definition.
- **Rationale**: Restores correct function/block structure so `buildSceneOverlay` and `renderFrame` are parsed as class member definitions without changing rendering behavior.
- **Status**: Accepted.

## 2026-03-08 — modern_renderer_tests syntax/build fix
- **Context**: MSVC test build failed in `tests/modern_renderer_tests.cpp` with parser errors near line 10 (`syntax error: 'const'`, missing `;`, missing function header).
- **Decision**: Apply minimal, local syntax-safe rename of the helper identifier to avoid preprocessor macro collision.
- **Implementation**: Renamed `near(...)` helper to `nearlyEqual(...)` and updated all local call sites in the same test file.
- **Consequence**: Test compiles cleanly without changing renderer behavior or test intent.

## 2026-03-08 — Catch2 test target helper consistency for missing-main link failures
- **Context**: Windows linking reported unresolved `main` for `render2d_tests.exe` and `pattern_tests.exe` while other Catch targets (for example `projectile_tests`) linked correctly.
- **Decision**: Route `render2d_tests` and `pattern_tests` through `engine_add_catch_test(...)` instead of `engine_add_plain_test(...)` to inherit `Catch2::Catch2WithMain` from the shared helper.
- **Rationale**: Keeps Catch entrypoint ownership centralized in one helper and prevents per-target drift that can reintroduce missing-main linker failures.
- **Status**: Accepted.

## 2026-03-09 — Finalize public/plugin/mod extension boundary
- **Context**: Extensibility behavior existed, but final v1 packaging needed an explicit commercial-facing contract for public API scope, plugin lifecycle, and mod/content extension structure.
- **Decision**: Freeze the supported boundary to `include/engine/public/*`; keep internal registry/layout/storage details non-contract; formalize plugin host lifecycle expectations (metadata/compatibility checks, registration diagnostics, unregister-or-clear teardown, host ownership); and position content-pack layering as the primary supported mod extension path.
- **Consequences**: Integrators get a clear, supportable extension story with minimal ABI risk; engine internals can keep evolving without accidental external coupling; plugin/mod guidance is now consistent across API, architecture, and overview docs.


## 2026-03-09 — Audio authoring validation is strict; runtime remains graceful
- Decision: treat malformed authored audio metadata (duplicate clip ids, duplicate event bindings, empty paths, unknown clip references) as parse-time validation failures in `parseAudioContentDatabase`.
- Rationale: fail fast for creators during content packaging/loading instead of silently degrading routing maps.
- Decision: keep runtime mixer behavior resilient (missing files still warn/no-op) while ensuring reconfiguration frees previous chunks and auto-starts configured loop music.
- Boundary note: deterministic simulation remains unchanged because audio dispatch/update still execute in presentation path after `simTick()`.
## 2026-03-09 — GameplaySession final orchestration-policy extraction
- **Context**: After prior subsystem decomposition, `GameplaySession::updateGameplay()` still directly owned two policy-heavy concerns: content hot-reload poll/fanout and upgrade cadence/debug mutation policy.
- **Decision**: Introduce `SessionOrchestrationSubsystem` in `gameplay_session_subsystems` and route both concerns through explicit subsystem interfaces.
- **Rationale**: Completes runtime ownership partitioning so `GameplaySession` acts as deterministic phase coordinator rather than policy + orchestration + runtime mutator in one unit.
- **Determinism note**: Preserved tick-gated cadence (`kHotReloadPollTicks`, 300-tick upgrade roll cadence) and existing side-effect ordering.
- **Migration Notes**: Public API unchanged (`GameplaySession::updateGameplay()`, `onUpgradeNavigation()`); behavior remains stable while internals are delegated.

## 2026-03-09 — Decision: resolve remaining missing-main failures by full Catch standardization
- Context: `content_packer_tests.exe` and `entity_tests.exe` were the final linker failures (`main` unresolved) blocking green builds.
- Options considered:
  1. Keep standalone mains and harden ad-hoc entrypoint handling.
  2. Convert to standard Catch tests and centralize entrypoint ownership through `Catch2::Catch2WithMain`.
- Decision: Option (2).
- Rationale: aligns with the working suite pattern, removes per-target entrypoint drift, and is the fastest low-risk path to green.
- Consequences:
  - `content_packer_tests` now validates generated pack output by probing expected generated pak locations instead of consuming custom process argv.
  - CMake `engine_register_test(...)` now forwards `DEPENDS` metadata when using `catch_discover_tests(...)`, preserving generation-before-validation ordering for discovered Catch tests.
  - Clean rebuild is mandatory after deleting the build directory.
