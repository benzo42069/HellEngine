# Pattern Authoring Guide

This guide is for content creators authoring bullet patterns that run in deterministic simulation.

## Choose an authoring path

HellEngine currently supports two practical paths:

1. **Layered patterns** (`patterns` array)
   - Best for straightforward ring/spiral/spread attacks.
   - Easier to read for small teams and rapid iteration.
2. **Compiled graph patterns** (`graphs` array)
   - Best for reusable authored behavior with explicit flow (`emit`, `wait`, `loop`, modifiers).
   - Better for scalable libraries and tooling integration.

## A) Layered pattern workflow (`patterns`)

Minimal example:

```json
{
  "guid": "pattern-my-ring",
  "name": "My Ring",
  "seedOffset": 10,
  "loopSequence": true,
  "layers": [
    {
      "name": "ring",
      "type": "radial",
      "bulletCount": 24,
      "bulletSpeed": 160.0,
      "projectileRadius": 3.0,
      "cooldownSeconds": 0.15
    }
  ],
  "sequence": [
    { "durationSeconds": 1.0, "activeLayers": [0] }
  ]
}
```

Supported emitter types in this path:
- `radial`
- `spiral`
- `spread`
- `wave`
- `aimed`

Authoring tips:
- Keep a stable `guid`; this is what other assets should reference.
- Use `seedOffset` for controlled variation while keeping deterministic replay.
- Use short sequence windows while iterating, then extend cadence after behavior is validated.

## B) Graph pattern workflow (`graphs`)

Minimal loop graph:

```json
{
  "id": "graph-basic-ring",
  "nodes": [
    { "id": "010-emit", "type": "emit_ring", "params": {"count": 12, "speed": 150, "radius": 3, "angle": 0} },
    { "id": "020-wait", "type": "wait", "params": {"seconds": 0.12} },
    { "id": "030-loop", "type": "loop", "params": {"count": 16}, "targetNode": "010-emit" }
  ]
}
```

Common node types:
- Emit: `emit_ring`, `emit_spread`, `emit_spiral`, `emit_wave`, `emit_aimed`
- Flow: `wait`, `loop`
- Modifiers/utilities: `modifier_rotate`, `modifier_phase`, `random_range`, `difficulty_scalar`

Validation notes:
- Loop count is compiler-bounded (`<= 64`).
- Invalid loop targets fail validation and should be fixed in authored graph JSON.

## C) Binding patterns to enemies/bosses

Entity templates should reference patterns by GUID where possible (`attackPatternGuid`).
Use name fallback (`attackPatternName`) only for legacy content.

## D) Recommended authoring loop

```bash
./build/ContentPacker --input data --output content.pak
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

## E) Common pattern authoring mistakes
- Reusing GUIDs accidentally across unrelated patterns.
- Large loop counts without enough `wait` spacing (visual overload, perf spikes).
- Depending on file ordering instead of explicit references.
- Treating render-only effects as simulation behavior.

## F) Performance-safe creator defaults
- Start new patterns at low bullet counts; scale gradually.
- Prefer composable short loops over giant one-shot bursts.
- Profile with `F10` perf HUD and headless smoke runs before release.
