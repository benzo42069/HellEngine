# Changelog

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
