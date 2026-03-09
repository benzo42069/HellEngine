## 2026-03-09 - Shared test helper: systemic Catch main rule
- Fixed unresolved `main` / `LNK1120` failures across remaining test executables by updating shared CMake test creation logic.
- Root cause: Catch classification relied on include matching and per-target force-lists, allowing Catch-based targets to miss `Catch2::Catch2WithMain`.
- New rule: tests without an explicit `main()` always link `Catch2::Catch2WithMain`; only explicit standalone tests with a real `main()` skip Catch main.
- Exact CMake changes: removed `engine_source_uses_catch(...)` and target force-list behavior; simplified `engine_link_catch_main_if_needed(...)` to gate only on `engine_source_defines_main(...)`; restored `boss_phase_tests` and `editor_tools_tests` to `engine_add_test(...)` shared path.
- Note: perform a full clean rebuild (delete build directory) so regenerated link/discovery commands take effect.

## Unreleased
### Fixed
- Fixed remaining Windows missing-main linker failure for `pattern_graph_perf_tests` by guaranteeing `Catch2::Catch2WithMain` linkage via `_engine_force_catch_main_targets` in `engine_link_catch_main_if_needed(...)`.
- Noted clean-rebuild requirement (delete build directory, reconfigure, rebuild) so regenerated linker/discovery rules are applied.

- Fixed final Windows test linker blocker for `editor_tools_tests` (`unresolved external symbol main`, `LNK1120`) by explicitly linking `Catch2::Catch2WithMain` in target setup while preserving existing discovery and runtime DLL deployment behavior.
- Operational follow-up: run a clean rebuild from a deleted build directory so regenerated linker/discovery commands take effect.
## Unreleased
### Fixed
- Fixed final Windows missing-main linker failure for `boss_phase_tests` by explicitly linking `Catch2::Catch2WithMain` in target setup while preserving Catch discovery and runtime DLL deployment behavior; requires a full clean rebuild so regenerated link commands are used.

# Changelog

## Unreleased
### Fixed
- Fixed remaining Windows test link failures for `content_packer_tests`, `editor_tools_tests`, `boss_phase_tests`, and `entity_tests` by ensuring shared CMake helper links `Catch2::Catch2WithMain` whenever those targets are Catch-based and do not define `main(...)`.
- Fixed `engine_deploy_runtime_dlls(...)` empty-runtime case by guarding `cmake -E copy_if_different` so the copy command is emitted only when runtime DLL sources exist (prevents invalid destination-only invocation, notably on `ContentPacker`).
- Note: perform a full clean rebuild (delete build directory, reconfigure, rebuild) to fully validate regenerated link and post-build commands.

### Fixed
- Resolved `GameplaySession` compile failure caused by upgrade callback type mismatch: lambdas passed to `SessionOrchestrationSubsystem::updateUpgradeCadence(...)` and `applyUpgradeDebugOptions(...)` now explicitly match `std::function<bool()>` by returning `bool` after invoking `TraitSystem::rollChoices()`.
### Changed
- Completed final external-facing documentation/polish pass: added repository `README.md`, refreshed creator index + onboarding flow, and aligned product-facing workflow guidance across build/run, authoring, validation, and release packaging docs.
- Documented final creator documentation architecture in `docs/Architecture.md` and recorded closure state in spec/plan/decision/audit docs to reduce stale guidance drift for package consumers.
## 2026-03-09
- Polished the shipped vertical-slice encounter data for stronger showcase pacing: Stage 01 now uses a mixed elite enemy set (`Basic Enemy Shooter` + `Vanguard Lancer`) with retuned intro/elite/event durations.
- Added `Vanguard Lancer` as an authored enemy profile (Spread Lattice pattern + Scarlet Wave palette) so the sample demonstrates multi-enemy pressure and content pipeline extensibility without runtime hacks.
- Updated Stage 01 boss flow authoring to run `Wave Weave` -> `Spread Lattice` -> `Composed Helix` and trigger `boss_phase_shift`, improving phase readability and authored audio transition validation.
- Refreshed sample vertical-slice documentation/runbook to reflect the finalized encounter composition and validation scope.

## Unreleased
- Public API/extensibility closure pass: finalized supported external scope (`include/engine/public/*`), documented plugin lifecycle/ownership/compatibility expectations, and formalized content-pack layering as the primary mod extension contract for commercial v1.

### Changed
- Finalized content import validation for production authoring: art manifests now enforce `assetManifestType: "art-import"`, reject duplicate asset GUID/source path entries, and require non-empty identifier-style `atlasGroup` values.
- Import fingerprint dependency metadata now includes explicit palette-template tags (`paletteTemplate:<name>`) so reimport invalidation reasons better reflect color-workflow dependencies.

### Fixed
- Finalized renderer stack ownership contract across code/docs: `render_pipeline` orchestrates path selection, `render2d` provides shared SDL 2D primitives, `modern_renderer` handles post-FX composition, `gl_bullet_renderer` handles GL projectile submission, and `gpu_bullets` remains presentation-only (`CpuMassRender`) with no deterministic gameplay authority.
- Added explicit `ProjectileRenderPath` ownership comments in `render_pipeline` to keep backend selection centralized and reduce future overlap drift.
- Finalized CMake test target consistency by introducing a two-step helper flow (`engine_add_test_target` + `engine_register_test`), removing target-name Catch overrides, and keeping Catch/non-Catch registration behavior uniform across all test executables.
- Preserved and codified runtime DLL deployment for test executables so Windows execution and Catch discovery continue to resolve SDL/other runtime dependencies reliably.
- Fixed remaining missing-main linker failures (`unresolved external symbol main`, `LNK1120`) for `content_packer_tests`, `entity_tests`, and `boss_phase_tests` by extending the shared CMake Catch safety override to include `boss_phase_tests`, ensuring `Catch2::Catch2WithMain` is linked whenever these targets lack a local `main(...)`.
- Noted operational requirement: perform a clean rebuild from a deleted build directory so regenerated link state fully picks up the CMake test-target fix.
- Added a minimal shared CMake safety override in `engine_link_catch_main_if_needed(...)` so `content_packer_tests` and `entity_tests` are forced to link `Catch2::Catch2WithMain` when they do not define their own `main(...)`, resolving the remaining unresolved-`main` / `LNK1120` follow-up failures.
- Fixed remaining Windows Catch test link failures caused by inconsistent test-helper routing: Catch-authored targets registered through plain wiring could miss `Catch2::Catch2WithMain` and fail with unresolved `main`/`LNK1120`.
- Unified helper-path test registration via `engine_add_test(...)` + source-based Catch detection so Catch targets consistently link `Catch2::Catch2WithMain` and use `catch_discover_tests` (covering targets such as `render2d_tests`, `pattern_tests`, and `entity_tests` class checks during audit).
- Closed the remaining manual target gap by applying shared Catch/main detection to `content_packer_tests` too, ensuring any Catch-based test executable without a local `main(...)` links `Catch2::Catch2WithMain` regardless of whether it is created via helper or direct `add_executable` path.

# Changelog

## 2026-03-09 — Editor tooling structure closure + UX polish
- Added `src/engine/editor/editor_tools_gameplay_panel.cpp` and moved gameplay authoring responsibilities out of the pattern panel (projectile debug, encounter/wave editor, trait/upgrade preview, and encounter asset helpers).
- Updated CMake source registration to compile the new gameplay editor module.
- Improved workspace UX with explicit workflow shortcuts (Content, Pattern, Palette/FX, Diagnostics), dynamic content browser scanning from `data/`, and clearer empty states/rescan guidance.
- Preserved existing runtime/editor behavior while reducing panel responsibility density and improving workflow discoverability.

## Unreleased
- Finalized audio workflow resilience: audio content parsing now rejects duplicate clip/event bindings, empty clip paths, and unknown music/event clip references so authoring errors fail fast before runtime.
- Audio runtime now clears/reloads clip memory safely during reconfiguration and auto-starts configured loop music after content load, preserving the deterministic simulation boundary while improving creator-facing reliability.
### Fixed
- Standardized Catch2 target setup through shared CMake helpers so all Catch-based test executables are linked with `Catch2::Catch2WithMain`, eliminating Windows `unresolved external symbol main`/`LNK1120` failures caused by per-target drift.
- Added Windows runtime DLL deployment for every test executable by copying each target's `TARGET_RUNTIME_DLLS` into the test binary output directory at build time, preventing `0xc0000135` failures during `catch_discover_tests` discovery when SDL2/SDL2_mixer DLLs were missing.

## Unreleased
### Fixed
- Fixed Windows/MSVC build failures caused by `<windows.h>` `min`/`max` macro collisions by defining `NOMINMAX` and `WIN32_LEAN_AND_MEAN` globally for `WIN32` CMake builds (covers `engine_core` and dependent targets).
- Eliminated resulting cascading parse errors affecting `std::min`/`std::max` use in headers/sources (including renderer/public API/palette-related units) without broad refactors.
- Documented that a clean rebuild/reconfigure is required after applying the compile-definition change so all translation units see consistent macro state.

### Changed
- Completed final creator-facing documentation pass with workflow-first onboarding and refreshed guidance for build/run, content authoring, replay/debug, troubleshooting, plugins/mods, and performance.
- Reworked `docs/AuthoringGuide.md` into a maintained index to reduce stale duplicated instructions.

### Added
- Added dedicated creator guides: `docs/PaletteAndGrayscaleWorkflow.md`, `docs/AudioWorkflow.md`, and `docs/SampleProjectUsage.md`.
## [Unreleased]
### Added
- Public plugin metadata contract (`PluginMetadata`, `IPlugin`) with explicit per-plugin target API version declaration.
- Plugin lifecycle endpoints: `unregisterShaderPackPlugin`, `unregisterContentPackPlugin`, `unregisterToolPanelPlugin`, and `clearRegisteredPlugins`.
- Typed plugin registration result (`PluginRegistrationResult`) with boundary-safe rejection causes.

### Changed
- Hardened authored audio workflow end-to-end: `Runtime` now loads and validates pack `audio` metadata (`clips`, `events`, `music`) and configures `AudioSystem` from authored records instead of hardcoded clip paths.
- Added explicit runtime audio event routing (`AudioEventType` -> `AudioEventId`) with bus-aware gain composition (`master/music/sfx`), listener-position updates, and loop-aware music handling in `AudioSystem`.
- Expanded supported authored event bindings with `boss_phase_shift`, `defensive_special`, and `run_clear` to align content/event schema with current audio authoring data.
- Hardened top-level CMake reconfigure reliability by tracking `version/VERSION.txt` via `CMAKE_CONFIGURE_DEPENDS`, ensuring generated version header updates are not missed on incremental runs.
- Verified build reliability with a full deleted-build configure/build and representative incremental rebuild probes (header touch + cpp touch) using Ninja, confirming dependency propagation and link refresh behavior.
- Plugin registration now enforces compatibility/version checks plus duplicate-id and duplicate-instance protection.
- Public API versioning helpers now include `isApiCompatible(...)` for host/plugin compatibility gating.

## Unreleased
### Changed
- Standardized Windows runtime DLL deployment by applying `engine_deploy_runtime_dlls(...)` to `EngineDemo` and `ContentPacker`, aligning runtime executables/tools/tests with one post-build dependency copy policy.
- Made portable release manifest generation deterministic in `tools/package_dist.ps1` by removing timestamp/path entropy and sorting inventory by relative path.
- Updated build/run release docs to reflect active Catch2 usage and reproducible packaging expectations.
- Clarified public plugin boundary with explicit host ownership contract comments and added host-facing helpers `isPluginTargetCompatible(...)` + `pluginRegistrationErrorMessage(...)` for stable compatibility/error diagnostics.
- Documented final public/plugin/mod compatibility expectations and lifecycle responsibilities across architecture/spec/plan/decision docs.
- Refactored the pattern editor panel internals into focused helpers for generation controls, seed/testing controls, graph editing, and preview/analysis while preserving the single-window workflow and existing behavior.
- Added explicit pattern-panel extension seams (`buildPatternPreviewAsset`, `drawPatternPreviewAndAnalysis`, and dedicated control/graph draw helpers) to support future tooling features with less coupling.
- Added a polished product-validation vertical slice definition via authored encounter/audio content and a dedicated sample runbook covering combat readability, enemy/boss flow, replay, UI, audio, and packaging checks.
- Runtime now hydrates `RunStructure` stage/zone sequencing from authored pack encounters (`encounters[].zones[]`), so vertical-slice progression is driven by content pipeline data with fallback defaults when encounter data is unavailable.
- Consolidated CMake third-party dependency setup under a single registration/materialization flow (`engine_register_dependency` + `FetchContent_MakeAvailable(${ENGINE_FETCHCONTENT_DEPENDENCIES})`) while preserving all existing target wiring and dependency availability.
- Hardened CMake build hygiene: reject in-source builds, explicitly create generated include output directory before configured-header emission, and remove duplicate `FetchContent_MakeAvailable` dependency materialization to improve clean/incremental rebuild consistency.
- Fixed Visual Studio/CMake/Ninja `ContentPacker` build failure (`SDL_mixer.h` not found) by explicitly linking `ContentPacker` with `${ENGINEDEMO_SDL_TARGET}` and `SDL2_mixer::SDL2_mixer` while preserving `engine_core` dependency setup.
- Major external-facing documentation pass for creators/integrators, including new onboarding, pattern authoring, boss/encounter authoring, replay/debug, plugin/mod overview, and creator performance guides.
- Updated `Quickstart`, `AssetImportWorkflow`, and `Troubleshooting` to align external instructions with current `EngineDemo` + `ContentPacker` workflows.
- Synchronized documentation governance across `MasterSpec`, `DecisionLog`, `ImplementationPlan`, and audit report references for external doc quality tracking.
- Repositioned the old “GPU bullet” path as `CpuMassBulletRenderSystem` to reflect actual architecture (CPU-driven simulation/prep with batched geometry submission).
- Renamed simulation modes to `CpuCollisionDeterministic` and `CpuMassRender` to remove expectation ambiguity.
- Improved CPU mass bullet scaling via compact active-slot iteration and added prepared-quad profiling counter support.
- Updated bullet-path tests and documentation to reflect the new ownership boundaries and scaling expectations.
- Clarified renderer-stack ownership: `render_pipeline` now centrally resolves projectile render path (`Disabled`/`ProceduralSpriteBatch`/`GlInstanced`) and applies it consistently for buffer build + draw submission.
- Renamed `RenderPipeline` modern mode flag to `modernPipelineEnabled_` to distinguish compositing mode from projectile backend decisions.
- Added explicit ownership comments in renderer headers (`gpu_bullets`, `gl_bullet_renderer`, `modern_renderer`) to reduce subsystem overlap ambiguity.
- Removed duplicated projectile render-path branching in `RenderPipeline::buildSceneOverlay` so a single route decision governs GL prep/procedural fallback/overlay follow-up each frame.
- Renamed internal procedural palette-ramp staging member to `proceduralPaletteRamp_` to clarify palette ramp ownership (`paletteRamp_` = GL projectile shader LUT).

## Unreleased
### Changed
- Decomposed `ControlCenterToolSuite` implementation into modular editor translation units:
  - `src/engine/editor/editor_tools_core.cpp`
  - `src/engine/editor/editor_tools_workspace_panel.cpp`
  - `src/engine/editor/editor_tools_pattern_panel.cpp`
  - `src/engine/editor/editor_tools_services.cpp`
- Updated CMake engine source registration to compile editor modules from `src/engine/editor/` while preserving existing editor functionality and public API.
- Strengthened editor smoke coverage by extending `editor_tools_tests` to validate that generated demo content is scannable by the validator service.



## 2026-03-07 — Persistence foundations (settings + profiles + migration)
- Added persistence module for user settings and profile/save-slot baselines with explicit schema versioning (`schemaVersion: 2`) and migration from legacy v1 JSON layouts.
- Added corrupted-save fallback behavior that returns load status + fallback/migration flags instead of crashing on malformed files.
- Added runtime hooks to load settings/profiles on boot and persist settings + active-profile progression at shutdown.
- Extended meta progression with snapshot import/export hooks so future progression systems can persist independently of deterministic simulation internals.
- Added `persistence_tests` coverage for roundtrip save/load, v1→v2 migration, and corrupted JSON fallback behavior.

## 2026-03-07 — Animation clip import + sprite variant workflow
- Added data-driven animation clip grouping in the content pipeline with explicit metadata (`animationSet`, `animationState`, `animationDirection`, `animationFrame`) and optional filename parsing via configurable regex (`animationNamingRegex`).
- Added sprite variant grouping metadata (`variantName`, `variantWeight`, `paletteTemplate`) with optional regex-based group/variant extraction (`variantNamingRegex`).
- Added importer/build validation for malformed animation/variant group identifiers, invalid frame/fps/weights, duplicate frames, duplicate variant names, and inconsistent clip fps.
- Added runtime pack build outputs: `animationBuild` and `variantBuild` in addition to existing `sourceAssetRegistry`, `importRegistry`, and `atlasBuild`.
- Updated content packer and tests to validate animation/variant plan metadata generation and registry shape.

## 2026-03-07
- Refactored `GameplaySession` into explicit state partitions: simulation, player combat, progression, presentation, debug/tool, and encounter runtime state.
- Updated runtime integration to use partitioned tool/tick ownership without changing replay determinism behavior.
- Added `gameplay_session_state_tests` to validate boundary wiring and getter/ownership consistency.
- Updated architecture documentation (`MasterSpec`, `DecisionLog`, `ImplementationPlan`, `HellEngine_Audit_Report`).


## Unreleased
### Changed
- Removed generic content pipeline header coupling to runtime audio playback headers by introducing `include/engine/audio_content.h` for shared audio pack schema types; this drops transitive `SDL_mixer.h` requirements from content pipeline includes while preserving audio content parsing behavior.
- Removed unnecessary `ContentPacker` linkage to SDL2/SDL2_mixer after confirming content pipeline/audio parsing paths no longer depend on runtime mixer APIs; tool now keeps a smaller dependency surface while preserving audio content packing behavior.
- Refactored `ControlCenterToolSuite` editor tooling from a single monolithic draw routine into modular domain panel methods (workspace/content browser, pattern graph editor, encounter/wave editor, palette/FX editor, trait/projectile tooling, validator diagnostics).
- Added shared editor service helpers for encounter asset composition and deterministic panel-state seeding to reduce coupling and improve maintainability without changing editor user behavior.
- Updated architecture/plan/decision documentation for the new editor module structure and extension guidance.

### Added
- Production-style source-art import pipeline integrated into `ContentPacker` for sprite/texture assets.
- New art import manifest support (`assetManifestType: "art-import"`) with per-asset settings validation.
- Import fingerprinting, optional `--previous-pack` reimport comparison, and dependency invalidation records.
- Atlas build planning metadata emitted into generated packs (`atlasBuild`) grouped by atlas group + color workflow.
- New content import pipeline test executable (`content_import_pipeline_tests`).
- Sample authored source-art manifest and source-art placeholder files under `data/art*` (text placeholders with texture extensions to avoid binary blobs).

### Changed
- `content_pipeline` now exposes reusable parsing/import/fingerprint helpers for asset-import tooling.
- `content_packer_tests` now validates generated pack import/atlas metadata shape.

## Unreleased
### Added
- First core audio subsystem (`AudioSystem`) with SFX/music playback, event routing, and bus/category mix controls.
- Event-driven gameplay hooks for hit, graze, player damage, enemy death, boss warning, UI click, and UI confirm.
- Content integration for audio assets/events via pack `audio` section and `data/audio.json`.
- Runtime volume config fields and CLI overrides for master/music/sfx.

### Changed
- `ContentPacker` now forwards `audio` blocks into built packs.
- Runtime now initializes SDL audio and flushes queued audio events once per deterministic sim tick.

## Unreleased
### Added
- Enemy/boss runtime event channel (`EntityRuntimeEvent`) with phase lifecycle + telegraph/hazard sync event types.
- Boss phase authoring support for multi-pattern sequencing (`patternSequence`) and phase cadence (`patternCadenceSeconds`).
- Encounter graph node support for `telegraph`, `hazardSync`, and `phaseGate` with owner-domain schedule metadata.
- Gameplay-session hooks that consume runtime combat events for camera/audio presentation sync.

### Changed
- Refactored `EntitySystem` runtime state to better separate immutable authoring data from mutable behavior execution state.
- Boss phase transitions now emit explicit runtime events and counters for tool/runtime observability.
- Sample boss entries in `data/entities.json` now demonstrate sequence/cadence phase authoring flow.
- Encounter compiler now emits duration and owner data per schedule event to support hazard/telegraph orchestration.
## Unreleased
### Added
- Added `tools/release_validate.ps1` to run a full release gate (build, test, benchmark, content pack generation, replay verify, packaging, artifact checks).

### Changed
- `tools/package_dist.ps1` now auto-bundles runtime DLL dependencies discovered in build output and writes `RELEASE_MANIFEST.txt` with SHA-256 file hashes for troubleshooting/distribution traceability.
- `tools/package_dist.ps1` now validates `sample-content.pak` via portable replay verification in addition to smoke/content-packer checks.
- `tools/release_validate.ps1` now enforces release-manifest presence and required artifact entry coverage.
- `tools/build_release.ps1` now fails early on missing content input directories and replay-verifies the generated sample pack during release pack generation.
- Hardened `tools/build_release.ps1` to run release validation gates by default instead of only configuring/building.
- Hardened `tools/package_dist.ps1` to bundle generated content packs and run portable self-validation before creating archives.
- Improved `build_installer.ps1` diagnostics and output handling around NSIS availability and path resolution.
- Centralized runtime content compatibility version (`kRuntimePackVersion = 4`) and aligned loader enforcement.
- Extended `content_packer_tests` with pack metadata compatibility assertions.

## Unreleased
### Fixed
- Corrected `PaletteRampTexture` declaration/definition duplication causing compile failures (`textureId`, `rowV`, `shutdown`, and `texture_` duplicates removed).
- Restored `GlBulletRenderer` access to `PaletteRampTexture::animationFor` through the class public interface.
- Fixed `ProjectileAllegiance::Enemy` symbol resolution in `gl_bullet_renderer.cpp` by including the canonical projectile allegiance definition.

## Unreleased
### Changed
- Refactored `GameplaySession` responsibility flow by extracting player combat, progression navigation, and presentation event emission into explicit subsystem interfaces (`gameplay_session_subsystems`).
- Updated `GameplaySession` orchestration to delegate to these subsystem boundaries while preserving deterministic update behavior.
- Added subsystem implementation unit to `engine_core` build sources.

## Unreleased
### Changed
- Continued `GameplaySession` architecture cleanup by introducing `EncounterSimulationSubsystem` for encounter-runtime ownership boundaries.
- Moved deterministic CPU collision flow, despawn presentation emission, and zone-feedback emission out of inline `GameplaySession` logic into encounter subsystem methods.
- Routed entity runtime-event presentation fanout through encounter subsystem coordination.
- Extended `gameplay_session_state_tests` with coverage for encounter zone-feedback emission behavior.

## Unreleased
### Fixed
- Fixed a brace/function-structure mismatch in `src/engine/render_pipeline.cpp` where a duplicated `buildSceneOverlay(...)` signature introduced an unmatched `{` and caused subsequent member functions (including `renderFrame`) to parse as illegal local definitions.

## Unreleased
### Fixed
- Fixed MSVC syntax build failure in `tests/modern_renderer_tests.cpp` by renaming helper `near(...)` to `nearlyEqual(...)` to avoid Windows macro collision that produced parser errors around `const` near file top.

## Unreleased
### Fixed
- Fixed Windows test-link missing-main failures for `render2d_tests` and `pattern_tests` by switching both targets to the shared Catch2 helper path (`engine_add_catch_test`) so `Catch2::Catch2WithMain` is linked consistently.
- Converted `tests/render2d_tests.cpp` and `tests/pattern_tests.cpp` to Catch `TEST_CASE` style so test entrypoint ownership remains centralized in Catch2 main.

## 2026-03-09
- Finalized GameplaySession runtime ownership cleanup by adding `SessionOrchestrationSubsystem` for session-level hot-reload cadence and upgrade cadence/debug policy.
- Delegated remaining orchestration-policy blocks out of `GameplaySession::updateGameplay()` while preserving deterministic tick order and replay-sensitive behavior.
- Extended `gameplay_session_state_tests` with coverage for orchestration subsystem upgrade cadence and debug-option application paths.

## Unreleased
### Fixed
- Fixed remaining Windows missing-main linker failures for `content_packer_tests` and `entity_tests` by converting both test sources to Catch `TEST_CASE` style and relying on `Catch2::Catch2WithMain` for executable entrypoint ownership.
- Updated CMake test registration so Catch-discovered tests can still receive dependency metadata; `content_packer_tests` now preserves dependency ordering on `content_packer_generate` without standalone main argv handling.
- Required a deleted-build-directory clean rebuild to fully apply test target/linkage changes.
