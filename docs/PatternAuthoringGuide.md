# Pattern Authoring Guide

This guide covers practical, deterministic pattern authoring for creators.

## 1) Choose an authoring path

> Editor note: use **Workflow Shortcuts → Pattern Pass** in Control Center to automatically focus on content browser + inspector + pattern editor + preview panels.

### Layered patterns (`patterns`)
Use for straightforward attacks and quick iteration.

### Graph patterns (`graphs`)
Use for reusable logic flows (`emit`, `wait`, `loop`, modifiers).

## 2) Layered pattern example
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

Supported layer emitter types:
- `radial`, `spiral`, `spread`, `wave`, `aimed`

## 3) Graph pattern example
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

Common graph node families:
- Emit: `emit_ring`, `emit_spread`, `emit_spiral`, `emit_wave`, `emit_aimed`
- Flow: `wait`, `loop`
- Modifiers/utilities: `modifier_rotate`, `modifier_phase`, `random_range`, `difficulty_scalar`

## 4) Authoring rules
- Keep GUIDs stable.
- Prefer GUID-based references from entities.
- Use `seedOffset`/deterministic parameters instead of runtime randomness assumptions.
- Keep loop counts practical; add waits to preserve readability.

## 5) Validation loop
```bash
./build/ContentPacker --input data --output content.pak
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```
