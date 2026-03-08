# Asset Import Workflow

This guide describes the creator-facing source-art import flow used by `ContentPacker`.

## 1) Manifest format
Source art manifests are JSON files with:
- `assetManifestType: "art-import"`
- `assets: []`

Each asset defines `guid`, source path, and import settings.

## 2) Supported source formats
- `.png`
- `.jpg` / `.jpeg`
- `.bmp`
- `.tga`

## 3) Per-asset settings
Common settings:
- `kind`: `sprite` or `texture`
- `colorWorkflow`: `full-color`, `grayscale`, `monochrome`
- `pivotX`, `pivotY` in `[0,1]`
- `collisionBoundsPolicy`: `none`, `tight`, `rect`
- `atlasGroup`
- `filter`: `nearest`, `linear`
- `mipPreference`: `none`, `generate`

Animation/variant settings:
- `animationSet`, `animationState`, `animationDirection`, `animationFrame`, `animationFps`
- `animationSequenceFromFilename`, optional `animationNamingRegex`
- `variantGroup`, `variantName`, `variantWeight`, `paletteTemplate`
- optional `variantNamingRegex`

## 4) Build workflow
```bash
./build/ContentPacker --input data --output content.pak
```

`ContentPacker` validates manifests, normalizes import settings, and emits import metadata into the pack.

## 5) Reimport invalidation workflow
To compare against previous import fingerprints:

```bash
./build/ContentPacker --input data --output content.pak --previous-pack old_content.pak
```

Import status per asset:
- `up-to-date`
- `reimported`
- `new`

Invalidations are emitted in `importInvalidations`.

## 6) Pack outputs creators should understand
Generated pack metadata includes:
- `sourceAssetRegistry`
- `importRegistry`
- `importInvalidations`
- `atlasBuild`
- `animationBuild`
- `variantBuild`

## 7) Related workflow docs
- Palette/grayscale shader usage: `docs/PaletteAndGrayscaleWorkflow.md`
- Troubleshooting import failures: `docs/Troubleshooting.md`
