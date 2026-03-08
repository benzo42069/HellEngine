## Unreleased
### Changed
- Hardened CMake build hygiene: reject in-source builds, explicitly create generated include output directory before configured-header emission, and remove duplicate `FetchContent_MakeAvailable` dependency materialization to improve clean/incremental rebuild consistency.
- Fixed Visual Studio/CMake/Ninja `ContentPacker` build failure (`SDL_mixer.h` not found) by explicitly linking `ContentPacker` with `${ENGINEDEMO_SDL_TARGET}` and `SDL2_mixer::SDL2_mixer` while preserving `engine_core` dependency setup.
- Major external-facing documentation pass for creators/integrators, including new onboarding, pattern authoring, boss/encounter authoring, replay/debug, plugin/mod overview, and creator performance guides.
- Updated `Quickstart`, `AssetImportWorkflow`, and `Troubleshooting` to align external instructions with current `EngineDemo` + `ContentPacker` workflows.
- Synchronized documentation governance across `MasterSpec`, `DecisionLog`, `ImplementationPlan`, and audit report references for external doc quality tracking.
- Repositioned the old “GPU bullet” path as `CpuMassBulletRenderSystem` to reflect actual architecture (CPU-driven simulation/prep with batched geometry submission).
- Renamed simulation modes to `CpuCollisionDeterministic` and `CpuMassRender` to remove expectation ambiguity.
- Improved CPU mass bullet scaling via compact active-slot iteration and added prepared-quad profiling counter support.
- Updated bullet-path tests and documentation to reflect the new ownership boundaries and scaling expectations.

# Changelog


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
