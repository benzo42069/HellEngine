# Authoring Guide

This guide covers authoring for:
- patterns (legacy layered)
- graph patterns (compiled VM)
- entities/projectiles
- encounters
- traits
- archetypes

Use `examples/content_packs/` as reference templates.

---

## Pack metadata (required)

Each content file should include:

```json
{
  "schemaVersion": 2,
  "packVersion": 3,
  "packId": "my-pack",
  "compatibility": {
    "minRuntimePackVersion": 2,
    "maxRuntimePackVersion": 3
  }
}
```

`ContentPacker` will merge arrays by GUID and generate missing GUIDs when needed.

---

## Patterns (legacy layered)

Top-level array: `patterns`.

Minimal pattern:

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

Types commonly used: `radial`, `spiral`, `spread`, `wave`, `aimed`.

---

## Graph patterns (compiled)

Top-level array: `graphs`.

Minimal looping graph:

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

Supported authoring node types in current runtime include:
- `emit_ring`, `emit_spread`, `emit_spiral`, `emit_wave`, `emit_aimed`
- `wait`, `loop`
- `modifier_rotate`, `modifier_phase`, `random_range`, `difficulty_scalar`

Loop count is bounded (`<=64`) by compiler validation.

---

## Projectiles and entities

Projectile archetypes live in `projectiles` (data-only presets) while runtime combat behavior is primarily driven by `entities` + pattern emission.

Entity essentials:

```json
{
  "guid": "enemy-sample",
  "name": "Sample Enemy",
  "type": "enemy",
  "movement": "sine",
  "spawnPosition": [0, -140],
  "baseVelocity": [0, 30],
  "maxHealth": 30,
  "contactDamage": 4,
  "interactionRadius": 14,
  "attacksEnabled": true,
  "attackPatternGuid": "pattern-my-ring",
  "attackIntervalSeconds": 0.5
}
```

Boss entities can include phased pattern swaps via `boss.phases`.

---

## Encounters

Top-level array: `encounters`.

Use encounters to define pacing/waves/spawn timing metadata consumed by runtime systems and tools.

---

## Traits

Top-level array: `traits`.

Traits provide run modifiers (pattern cooldown, projectile speed/radius, etc.) and are presented in upgrade choices.

Example:

```json
{
  "guid": "trait-fast-casting",
  "id": "fast_casting",
  "name": "Fast Casting",
  "rarity": "rare",
  "modifiers": {
    "patternCooldownScale": 0.9,
    "projectileSpeedMul": 1.1
  }
}
```

---

## Archetypes

Top-level array: `archetypes`.

Archetypes define base stat profile + flavor labels (weapon/active ability) and unlock tier.

---

## Validate + package workflow

```bash
./build/ContentPacker --input examples/content_packs --output content.pak --pack-id starter
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
```

If JSON is malformed, runtime logs structured errors and falls back rather than crashing.


## Live hot-reload authoring flow
During runtime/editor sessions, content file edits are detected by `ContentWatcher`. Valid edits are swapped at deterministic tick boundaries; invalid edits are rejected with error logging and the previous content remains active. Supported live-reload domains:
- Patterns
- Entities
- Traits
- Difficulty profiles
