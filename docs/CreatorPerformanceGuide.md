# Performance Guidance for Content Creators

This guide helps creators ship readable, deterministic content without avoidable performance regressions.

## 1) Creator-first performance goals
- Preserve readability during high-density bullet moments.
- Keep deterministic replay verification stable.
- Avoid authored spikes (burst walls, overlayered effects, pathological loops).

## 2) Pattern authoring guidelines
- Start with low bullet counts; scale in measured steps.
- Prefer cadence + sequencing over single giant bursts.
- Use waits and phase spacing in graph loops.
- Keep loop counts practical even when compiler bounds allow more.

## 3) Encounter pacing guidelines
- Ramp density across encounter timeline rather than front-loading.
- Pair high-density waves with recovery windows.
- Reserve maximum-intensity combinations for short boss windows.

## 4) Asset guidance
- Use atlas grouping intentionally (`atlasGroup`) in art import manifests.
- Keep animation FPS consistent per clip.
- Validate variant weights and avoid excessive low-value variants.

## 5) Runtime checks creators should run

```bash
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

Use runtime HUD toggles during visual playtests:
- `F10` perf HUD
- `` ` `` debug HUD

## 6) Red flags before shipping a pack
- Replay verify intermittently fails with unchanged seed/content.
- Significant frame-time jumps tied to a single authored phase.
- Content pack emits repeated validation/fallback warnings.
- Boss phases combine maximum spawn rate + long duration without readability breaks.
