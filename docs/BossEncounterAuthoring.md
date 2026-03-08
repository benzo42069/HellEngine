# Encounter and Boss Authoring

This guide describes practical authoring patterns for encounters and boss phase flow.

## 1) Authoring ownership boundaries
- `patterns` / `graphs`: attack behavior.
- `entities`: enemy and boss templates, including phase behavior.
- `encounters`: pacing, scheduling, and progression orchestration.

Keep these boundaries clean to reduce merge conflicts and improve validation quality.

## 2) Boss authoring checklist
For each boss entity:
- Stable `guid`.
- `boss.phases` authored explicitly.
- Per-phase pattern binding (`attackPatternGuid` or sequence fields where used).
- Cadence/interval fields authored for deterministic pacing.

Use readable phase escalation: teach -> pressure -> climax.

## 3) Encounter scheduling checklist
For each encounter:
- Explicit spawn/schedule events.
- Deterministic timing values (avoid implicit ordering assumptions).
- Telegraph and hazard sync events where threat level jumps.
- References to valid authored entity GUIDs.

## 4) Validation loop
```bash
./build/ContentPacker --input data --output content.pak
./build/EngineDemo --headless --ticks 600 --content-pack content.pak
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

## 5) Common creator mistakes
- Name-only references when GUID references are available.
- Boss phases that spike difficulty without telegraph windows.
- Overlong high-density phases with no recovery pacing.
- Editing entities/patterns/encounters together without replay verification after each increment.
