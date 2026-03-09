# Palette and Grayscale Shader Workflow

This guide explains how creators author palette templates and connect them to grayscale/monochrome asset workflows.

## 1) Where palette data lives
> Editor note: use **Workflow Shortcuts → Palette/FX Pass** in Control Center to focus palette + preview panels before editing templates.

- Palette templates: `data/palettes/palette_fx_templates.json`
- Asset import manifests (grayscale/monochrome flags): `data/art_import_manifest.json` (or your own manifest file)

## 2) Palette authoring basics
Each template in `paletteTemplates` should define stable naming and color intent.

Practical guidance:
- Keep template names stable once referenced by entities/content.
- Use gradients when you want animated ramps.
- Use threshold-based banding when you want strict highlight/core/glow segmentation.

## 3) Runtime shader ramp mapping
At runtime, templates are converted into rows in the palette ramp texture (`64 x N`):
- Row `0` is fallback.
- Template rows start at `1` in registry order.
- Templates with gradients use gradient LUT generation.
- Templates without gradients use threshold-driven band generation.

## 4) Connect palettes to authored content
- Projectile visuals resolve `projectilePalette` template names to runtime palette indices.
- Archetypes/entities that reference palette templates are resolved during content load/spawn.

## 5) Grayscale/monochrome import workflow


Validation expectations now enforced by the importer:
- `assetManifestType` must be `art-import`.
- `atlasGroup` must be a non-empty identifier.
- Duplicate `guid` or duplicate `source` entries in the same manifest fail validation.
- When `paletteTemplate` is authored, the import registry records a `paletteTemplate:<name>` dependency so reimport invalidations show color-ramp coupling explicitly.

In art manifests (`assetManifestType: "art-import"`), set per-asset:
- `colorWorkflow`: `full-color`, `grayscale`, or `monochrome`
- `atlasGroup`: keep similarly shaded assets grouped

Creator recommendation:
- Use `grayscale`/`monochrome` for assets intended for palette-driven recolor.
- Use `full-color` only when art should bypass palette recolor.

## 6) Validation loop
```bash
./build/ContentPacker --input data --output content.pak
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
```

If palette references or manifest settings are invalid, fix authoring JSON and repack.
