# Build and Release Pipeline (Windows)

This repository supports a productized release flow with validation gates for build reproducibility, content pack compatibility, and portable distribution integrity.

## Prerequisites
- Windows 10/11
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.28+
- PowerShell 7+ (or Windows PowerShell 5.1)
- Git (required for git-hash build stamping)
- Optional: NSIS (`makensis`) for installer generation

## Canonical release flow

### 1) Full release validation (recommended)
```powershell
./tools/release_validate.ps1 -Clean -Deterministic
```
This performs:
- configure + release build
- full test suite
- benchmark threshold validation
- default and sample content pack generation
- replay verification against generated pack
- portable bundle assembly
- portable self-validation (ContentPacker + EngineDemo smoke run + sample-pack replay verify)
- archive generation and artifact presence checks

### 2) Build release only
```powershell
./tools/build_release.ps1 -Clean -Deterministic
```
This now includes test, benchmark, content pack, sample pack, and replay validation by default.

### 3) Package distribution
```powershell
./tools/package_dist.ps1 -BuildDir build-release
```
Outputs:
- Portable folder: `dist/portable`
- Archive: `dist/EngineDemo-portable.zip`

### 4) Optional installer
```powershell
./build_installer.ps1
```
If NSIS is unavailable, a placeholder note is generated instead of failing the entire release.

## Packaging assumptions and bundled outputs
Portable bundles intentionally include:
- runtime binaries: `EngineDemo`, `ContentPacker`
- generated packs: `content.pak`, `sample-content.pak`
- source/content examples: `data/`, `assets/`, `examples/`
- configuration and docs: `engine_config.json`, `BuildAndRun.md`, `BuildAndRelease.md`, `VERSION.txt`
- runtime DLL dependencies auto-discovered from build output (`*.dll` in `build-release/Release` and `build-release`)
- release manifest: `RELEASE_MANIFEST.txt` with deterministic format/version header and stable sorted full file inventory, sizes, and SHA-256 hashes

## Content pack compatibility enforcement
- Runtime pack compatibility version is now centralized as `kRuntimePackVersion = 4`.
- ContentPacker emits:
  - `schemaVersion = 2`
  - `packVersion = 4`
  - compatibility range: `minRuntimePackVersion = 2`, `maxRuntimePackVersion = 4`
- Runtime loading rejects packs outside compatibility bounds for patterns/entities.

## Reproducible release behavior
Use `-Deterministic` for release builds to enable deterministic compile/link behavior and fetch-content disconnect mode:
- `ENGINE_DETERMINISTIC_BUILD=ON`
- `FETCHCONTENT_UPDATES_DISCONNECTED=ON`

## Failure diagnostics
Release scripts emit step-scoped diagnostics:
- `[build_release]` for configure/build/test/benchmark/pack/replay failures
- `[package_dist]` for bundle and portable self-validation failures
- `[release_validate]` for final artifact verification failures

## Version stamping
Version metadata comes from `version/VERSION.txt`.
At configure time, CMake captures:
- semantic version from `version/VERSION.txt`
- git hash from `git rev-parse --short=12 HEAD`

Executable build stamp:
- `<version>+<git_hash>`

Runtime behavior:
- Build stamp logged on startup
- Window title includes build stamp

## Release validation expectations
- `tools/build_release.ps1` now hard-fails when required content input directories are missing (`data/`, sample pack input dir).
- Release build flow replay-verifies both packs: generated default `content.pak` and generated `sample-content.pak`.
- `tools/release_validate.ps1` requires `RELEASE_MANIFEST.txt` and asserts required artifact entries are listed.

## Troubleshooting-first artifacts
When triaging distribution issues, start with `dist/portable/RELEASE_MANIFEST.txt`:
- Verify target file exists in inventory and size/hash align with expected build outputs.
- Compare hashes between local build and distributed archive to isolate corruption or stale artifacts.
- Confirm runtime DLL inventory includes platform dependencies expected by your deployment target.
