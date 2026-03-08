# Replay and Debug Workflow

Use this workflow to confirm deterministic behavior and triage creator content issues.

## 1) Fast smoke then parity check
```bash
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

## 2) Record/playback workflow
```bash
./build/EngineDemo --replay-record runs/sample.rpl --headless --ticks 1200 --seed 1337 --content-pack content.pak
./build/EngineDemo --replay-playback runs/sample.rpl --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

## 3) In-runtime debug toggles
- `` ` ``: debug HUD
- `F10`: performance HUD
- `F11`: compiled graph VM HUD
- `F12`: bullet simulation mode toggle (when available in build)

## 4) Content load failure handling
If logs include `content_load_failed` or `error_report=...`:
1. Fix invalid authored JSON.
2. Rebuild pack.
3. Re-run smoke + replay verify.

Runtime is designed to avoid hard crashes and keep fallback content where possible.

## 5) Replay mismatch triage
When verify fails:
1. Capture first divergence tick/subsystem.
2. Re-run with same seed and same pack.
3. Isolate recent authoring edits (patterns/entities/encounters/palettes).
4. Reintroduce changes incrementally until mismatch reproduces.

## 6) Crash diagnostics
Check `crashes/` for:
- `crash_report_*.txt`
- optional `crash_dump_*.dmp` on Windows

Attach crash report + seed + pack version in issue reports.
