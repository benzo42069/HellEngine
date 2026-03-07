# Boss and Encounter Authoring Workflow

This guide describes how to author boss phases and encounter pacing using current runtime data boundaries.

## 1) Core data ownership model

- `entities` define enemy/boss runtime templates and combat-facing fields.
- `encounters` define pacing/timeline/wave scheduling metadata.
- Pattern assets (`patterns`/`graphs`) define attack behavior that entities execute.

This separation keeps encounters composable and deterministic.

## 2) Boss authoring baseline

Boss entities are authored in `entities` with boss phase definitions (`boss.phases`) and pattern references.

Recommended per-phase structure:
- phase identity/name
- health/time trigger intent
- pattern or pattern sequence references
- cadence controls (e.g., interval/cooldown)
- telegraph lead behavior where supported by encounter schedule

## 3) Encounter schedule authoring

Encounters should define deterministic wave/schedule events and use explicit ownership boundaries:
- spawn events
- telegraph events
- hazard sync events
- phase gate events

Keep schedule IDs stable and avoid implicit ordering assumptions.

## 4) Authoring checklist (boss + encounter)
- Boss template has stable `guid` and references valid pattern IDs.
- Encounter entries reference existing entities.
- Telegraph/hazard nodes are authored when high-threat transitions occur.
- Difficulty scaling uses authored fields (not hard-coded runtime assumptions).
- All content repacks cleanly with zero validation errors.

## 5) Validation/run loop

```bash
./build/ContentPacker --input data --output content.pak
./build/EngineDemo --headless --ticks 600 --content-pack content.pak
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

## 6) Practical tuning guidance
- Keep early boss phases readable; increase complexity over time.
- Pair major pattern changes with clear telegraph windows.
- Use deterministic cadence values so replay and balancing sessions stay comparable.
- Treat encounter schedules as pacing assets, not just spawn lists.
