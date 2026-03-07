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

Pattern signatures are auto-generated at load time for compiled graph patterns as `sig_<graphId>` textures. These are derived from an offline mini-simulation and can be used by tooling as pattern thumbnails or unique micro-sprites.

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


---

## Procedural bullet visuals from palette JSON

Bullet visuals are generated procedurally at startup from palette template data in `data/palettes/palette_fx_templates.json`.

- No sprite-sheet image is required for bullets.
- Each palette produces six shape variants: `circle`, `rice`, `star`, `diamond`, `ring`, `beam`.
- The generator derives fill bands (core/highlight/glow) from the palette's Core layer through the existing palette fill pipeline.

Authoring flow:
1. Add or edit a palette template in `paletteTemplates`.
2. Define a `Core` layer color (and optional supporting layers for material use).
3. Run the engine; bullet textures are generated automatically at startup.

Texture IDs are emitted in the form `bullet_<paletteName>_<shape>` (sanitized palette name) and index-compatible aliases (`bullet_<index>_<shape>`) for runtime lookup.


### Palette animation modes for projectile gradients

`data/palettes/palette_fx_templates.json` supports per-palette animation under `animation`:

- `None`: static palette color behavior.
- `HueShift`: samples gradient with age+phase and shifts hue over time.
- `GradientCycle`: cycles through gradient as a whole over time.
- `PulseBrightness`: keeps gradient color but modulates brightness with a pulse curve.
- `PhaseScroll`: scrolls gradient phase per bullet instance (using `perInstanceOffset`) to create wave motion across ring emissions.

Wave authoring tip:
- For a 16-bullet ring, set `perInstanceOffset` to a small positive value (for example `0.03` to `0.08`).
- Each bullet receives `phase += instanceIndex * perInstanceOffset`, so adjacent bullets are color-shifted and the wave appears to rotate as `speed` advances phase over time.

---

## Procedural level tile generation (run-seed + zone driven)

Background base tiles can now be generated procedurally at runtime from run seed + stage index + zone type.

- Determinism: identical `simulationSeed` + progression order yields identical background tiles.
- Rule mapping:
  - Combat -> CaveFormation (organic blobs)
  - Elite -> CrystalGrowth (sharper structures)
  - Event -> OrganicNoise (smoother fill)
  - Boss -> CircuitBoard (grid/channel look)
- Color derivation uses a deterministic accent color and the existing `deriveBackgroundFillFromAccent()` palette transform.

Generation runs when run structure transitions to a new zone (not per frame), then the background system swaps the primary tile layer texture.

Designer override path: stage definitions/tools can opt to provide explicit tile generation parameters or custom authored textures per stage/zone to replace the default zone-type mapping.

### Palette JSON to shader ramp row mapping

`PaletteRampTexture` builds a `64 x N` palette ramp texture from `data/palettes/palette_fx_templates.json` in registry order (row 0 is the default fallback row; template rows begin at index 1).

- If a palette template has no `gradient` value, the row is generated in **Band3** mode using `thresholdA`/`thresholdB` from material params:
  - glow band: `[0, thresholdA * 64)`
  - highlight band: `[thresholdA * 64, thresholdB * 64)`
  - core band: `[thresholdB * 64, 64)`
  - each boundary gets a 4-pixel blend strip.
- If a palette template has a valid named `gradient`, the row is generated in **GradientRamp** mode using `generateGradientLut()`.

For content authors:
- `projectilePalette` on entity JSON can reference a palette template by name, and runtime resolves it to the internal `paletteIndex` used by projectile SoA.
- Archetypes also resolve their configured palette template name to runtime `paletteIndex` at spawn time.


---

## Entity authoring (enemy, boss, resource definitions)

Entity templates are authored in `data/entities.json` under `entities`.

Recommended coverage per authored pack:
- **Enemy** definitions: movement/attack cadence/collision radii/hp/contact damage.
- **Boss** definitions: phased pattern sequences (`boss.phases`, `patternSequence`, cadence fields).
- **Resource** definitions: collectible/harvestable nodes and reward descriptors used by progression/economy systems.

Authoring rule: entity references to patterns should use GUIDs where possible for stable cross-pack merges.

---

## Palette authoring (template -> shader colorization)

Palette templates are authored in `data/palettes/palette_fx_templates.json` and compiled to runtime palette rows.

Pipeline:
1. Author template layers/gradient/animation in palette JSON.
2. Runtime builds palette LUT rows (`PaletteRampTexture`).
3. `GlBulletRenderer` shader samples grayscale atlas + palette row and colorizes projectiles in GPU.

This keeps gameplay bullets sprite-light while preserving rich per-palette visual identity.

---

## Hot-reload workflow

Current workflow for iterative authoring:
1. Edit content JSON (`patterns`, `graphs`, `entities`, `traits`, palettes, etc.).
2. Repack with `ContentPacker` (optionally with `--previous-pack` for reimport invalidation tracking).
3. Reload in runtime/editor at safe boundaries (pattern/palette/shader reload paths are integrated).
4. Re-run deterministic/replay checks when gameplay-affecting data changes.

Hot-reload latency targets are documented in `docs/PerfTargets.md`.

---

## FX presets authoring

Post-processing FX presets are authored as data and resolved into runtime `PostFxSettings`.

Authoring expectations:
- Define named presets for zone/archetype-driven looks.
- Keep bloom/vignette/composite settings within readability/perf constraints.
- Validate in both modern renderer path and fallback path.

Palette FX templates and post-FX presets should be reviewed together so projectile palette motion and scene grading remain visually coherent.
