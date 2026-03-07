# Sprite + Texture Asset Import Workflow

## Manifest format
Author source-art manifests as JSON files with:

- `assetManifestType: "art-import"`
- `assets: []` records

Each asset record supports:

- `kind`: `sprite` or `texture`
- `source`: source image path (`.png`, `.jpg/.jpeg`, `.bmp`, `.tga`)
- `name`/`guid` (optional, deterministic GUID generated when omitted)
- `settings`:
  - `colorWorkflow`: `full-color`, `grayscale`, `monochrome`
  - `pivotX`, `pivotY` in `[0,1]`
  - `collisionBoundsPolicy`: `none`, `tight`, `rect`
  - `atlasGroup`
  - `filter`: `nearest`, `linear`
  - `mipPreference`: `none`, `generate`
  - `variantGroup`
  - `animationGroup`
  - `animationFps`
  - `animationSequenceFromFilename`

Example: `data/art_import_manifest.json` (checked-in `data/art/*.png` are text placeholders to avoid storing binary blobs in this repository).

## Import and validation stages
`ContentPacker` now performs the art import workflow as part of normal pack generation:

1. Scan all JSON inputs.
2. Parse gameplay/schema content as before.
3. Parse `art-import` manifests.
4. Validate source files and import settings.
5. Build import fingerprints for each art asset from:
   - source path + file size + write timestamp
   - normalized import settings
6. Group imported assets into atlas build plans by `(atlasGroup, colorWorkflow)`.
7. Emit import metadata into the runtime pack.

## Reimport + dependency invalidation
Use:

```bash
./build/ContentPacker --input data --output content.pak --previous-pack old_content.pak
```

When `--previous-pack` is provided, importer compares previous and current `importFingerprint` values by GUID:

- unchanged => `status: "up-to-date"`
- changed => `status: "reimported"` and entry in `importInvalidations`
- missing history => `status: "new"`

`importInvalidations` records old/new fingerprint and dependency set (source file + manifest).

## Runtime pack integration
Generated packs now include:

- `sourceAssetRegistry`
- `importRegistry`
- `importInvalidations`
- `atlasBuild`

And imported art assets are appended into the global `assetRegistry`.
