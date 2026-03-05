# Troubleshooting

## Build issues

### `build/` missing or stale

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 4
```

### SDL/audio dependency warnings

On Linux containers, optional backends (ALSA/Pulse/JACK/Wayland) may be unavailable.
This is usually non-fatal for headless and test runs.

---

## Runtime/content issues

### Engine logs `error_report=...content_load_failed...`

Cause: malformed or incompatible JSON pack.

Fix:
1. Validate JSON syntax.
2. Ensure metadata fields exist (`schemaVersion`, `packVersion`, compatibility).
3. Repack:

```bash
./build/ContentPacker --input data --output content.pak
```

Runtime should fall back to safe/default content instead of hard-crashing.

### Hot reload failed

You will see on-screen `HotReloadError: ...` and structured log entry.
Runtime keeps last-good content; fix the file and reload again.

---

## Replay verify failures

Run:

```bash
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

If mismatch occurs, logs include first divergence tick + subsystem.
Check recent content/script changes affecting deterministic order.

---

## Crash reports

In Release builds, fatal crashes write reports to `crashes/`:
- `crash_report_*.txt` (build stamp + log tail + structured crash error)
- Windows also: `crash_dump_*.dmp`

---

## “Out of the box” sanity check

```bash
./build/ContentPacker --input examples/content_packs --output content.pak --pack-id starter
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
```

If these pass, your environment + content pipeline is operational.

