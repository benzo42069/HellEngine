# Troubleshooting

## Build/configuration failures

### CMake configure fails or dependencies do not resolve
- Delete build directory and reconfigure.
- Ensure CMake version/toolchain are installed.

```bash
cmake -E remove_directory build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

### Runtime binary not found
Build again and verify expected outputs (`EngineDemo`, `ContentPacker`) under `build/` (or `build/Debug` on VS).

## Content pack failures

### `ContentPacker` reports validation errors
- Fix malformed JSON first.
- Verify required arrays and required fields exist.
- Re-run pack build after each fix.

```bash
./build/ContentPacker --input data --output content.pak
```

### Import manifest issues
Common causes:
- unsupported image extension
- missing source file
- invalid pivots or settings
- duplicate/invalid animation/variant metadata

See `docs/AssetImportWorkflow.md`.

## Runtime content load failures
If logs show `content_load_failed` or `error_report=...`:
1. Rebuild pack from known-good data.
2. Run a short headless smoke.
3. Re-run replay verify.

```bash
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

## Replay mismatch failures
- Keep seed/tick count/pack fixed during reproduction.
- Isolate recent content edits in patterns/entities/encounters/palettes.
- Reintroduce incrementally until divergence appears.

## Audio issues
- Confirm `data/audio.json` is included in pack input.
- Use CLI gain overrides to verify mixer path:

```bash
./build/EngineDemo --content-pack content.pak --audio-master 1.0 --audio-music 1.0 --audio-sfx 1.0
```

See `docs/AudioWorkflow.md`.

## Packaging/release script issues
Use script-specific logs:
- `tools/build_release.ps1`
- `tools/package_dist.ps1`
- `tools/release_validate.ps1`
If this passes, environment + core content pipeline are operational.


## Release packaging issues

### Missing runtime dependency in portable build
Cause: dependency DLL absent from build output, so auto-discovery cannot bundle it.

Fix:
- Confirm dependency DLL exists in `build-release/Release` or `build-release`.
- Re-run release packaging:

```powershell
./tools/package_dist.ps1 -BuildDir build-release
```

### Manifest validation failure (`RELEASE_MANIFEST.txt`)
Cause: packaging incomplete or post-package files modified/deleted.

Fix:
1. Re-run full gate:

```powershell
./tools/release_validate.ps1 -Clean -Deterministic
```

2. Inspect manifest for expected entries (`EngineDemo`, `ContentPacker`, `content.pak`, `sample-content.pak`, `VERSION.txt`).

### Sample pack replay verify fails during packaging
Cause: sample content drift introduced deterministic/runtime incompatibility.

Fix:
- Rebuild sample pack locally and replay-verify directly:

```powershell
./tools/build_release.ps1 -BuildDir build-release -SkipTests -SkipBenchmarks
```

- Check runtime logs for divergence tick and update sample content under `examples/content_packs`.
