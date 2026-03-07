# Sprite + Texture Asset Import Workflow

This guide is for creators importing source art into runtime packs.

## Quick workflow
1. Author/update an `art-import` manifest (for example `data/art_import_manifest.json`).
2. Run `ContentPacker` to validate + compile pack metadata.
3. Fix any import validation errors and repack.
4. Run the runtime with the generated pack.

```bash
./build/ContentPacker --input data --output content.pak
./build/EngineDemo --content-pack content.pak
```


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


## Animation naming/grouping rules
Animation clips are grouped by `(animationSet, animationState, animationDirection)`.

Authoring options:
- explicit fields: `animationSet`, `animationState`, `animationDirection`, `animationFrame`
- or filename extraction with `animationSequenceFromFilename: true` and `animationNamingRegex`

Default filename fallback pattern (when extraction is enabled but regex is omitted):
- `^([A-Za-z0-9_-]+)_([A-Za-z0-9_-]+)_([A-Za-z0-9_-]+)_([0-9]+)$`
- capture groups map to: set, state, direction, frame

Validation:
- frame indexes must be non-negative and unique per clip
- clips should use consistent FPS
- malformed group/state/direction identifiers are rejected

## Variant grouping rules
Variant groups are authored via `variantGroup` and `variantName` (or filename-derived through `variantNamingRegex`).

Rules:
- `variantWeight` must be > 0
- variant names must be unique within a group
- `paletteTemplate` is optional metadata used for grayscale/palette-template compatibility

Runtime usage expectations:
- runtime selects variants deterministically using group weights
- runtime/editor can consume `animationBuild` and `variantBuild` metadata directly from pack JSON
