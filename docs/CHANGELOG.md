# Changelog

## Unreleased
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
