# Replay and Debug Workflow

This guide is for creators validating deterministic behavior and diagnosing content issues quickly.

## 1) Fast deterministic verification

```bash
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

What it does:
- Runs simulation twice (record/playback flow).
- Compares deterministic state hashes.
- Reports divergence tick + subsystem if mismatch is detected.

## 2) Smoke test before replay verify

```bash
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
```

Use this to catch immediate content/runtime load issues first.

## 3) In-runtime debug controls
- `` ` ``: debug HUD toggle
- `F10`: perf HUD toggle
- `F11`: compiled graph VM toggle
- `F12`: bullet simulation mode toggle (when enabled by runtime wiring)

## 4) Structured error workflow

If content load fails, check logs for:
- `error_report=...`
- `content_load_failed`

Fix malformed/incompatible JSON and rebuild pack:

```bash
./build/ContentPacker --input data --output content.pak
```

Runtime should keep last-good/default content instead of crashing.

## 5) Replay mismatch triage

When replay verification fails:
1. Capture first divergence tick/subsystem from logs.
2. Identify recent edits in pattern/entity/encounter assets.
3. Re-run with same seed and unchanged pack to confirm reproducibility.
4. Roll back suspect edits and reintroduce incrementally.

## 6) Crash diagnostics

Release crashes emit reports in `crashes/`:
- `crash_report_*.txt`
- Windows builds may also emit `crash_dump_*.dmp`

Always attach crash report + seed + pack version when filing issues.
