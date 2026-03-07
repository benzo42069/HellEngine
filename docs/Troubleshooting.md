# Troubleshooting

## Build issues

### `build/` missing or stale

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 4
```

### Optional SDL/audio backend warnings
On Linux containers, optional ALSA/Pulse/JACK/Wayland backends may be unavailable.
This is often non-fatal for headless and test runs.

---

## Content pipeline issues

### `error_report=...content_load_failed...`
Cause: malformed or incompatible JSON/pack metadata.

Fix:
1. Validate JSON syntax and required metadata (`schemaVersion`, `packVersion`, compatibility).
2. Repack:

```bash
./build/ContentPacker --input data --output content.pak
```

Runtime should fall back to last-good/default content rather than hard-crashing.

### Art import validation failures
Cause: missing source files, unsupported source formats, or invalid import settings.

Fix:
- Confirm manifest points to valid files.
- Ensure source extension is supported (`.png`, `.jpg/.jpeg`, `.bmp`, `.tga`).
- Rebuild pack after manifest correction.

---

## Replay and determinism issues

### Replay verify mismatch

```bash
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

If mismatch occurs, logs include divergence tick + subsystem.
Check recent edits to patterns/entities/encounters and reintroduce changes incrementally.

### Hot reload failed
Runtime displays `HotReloadError: ...` and logs details.
Fix the authored file and reload; runtime retains last-good content while failing asset is corrected.

---

## Extension issues (plugins/mods)

### Plugin compiles but breaks after engine update
Likely cause: dependency on internal headers.

Fix:
- Restrict integration to `include/engine/public/*` APIs.
- Rebuild against current runtime version and check deprecation notices.

### Unexpected content override behavior with multiple packs
Likely cause: pack load-order assumption.

Fix:
- Remember merge is left-to-right; later pack wins by GUID.
- Validate your `--content-pack` ordering and GUID uniqueness.

---

## Crash reports
In Release builds, fatal crashes write reports to `crashes/`:
- `crash_report_*.txt`
- Windows may also include `crash_dump_*.dmp`

Attach crash report + seed + pack version when filing issues.

---

## Out-of-box sanity check

```bash
./build/ContentPacker --input examples/content_packs --output content.pak --pack-id starter
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
```

If this passes, environment + core content pipeline are operational.
