# Authoring and Content Pipeline

## Overview
Phase 8/9 introduces an offline content compilation tool:

- `ContentPacker.exe`

The packer reads authored JSON from `/data`, validates schema shape, then emits a runtime pack file:

- `content.pak`

Runtime consumes both pattern and entity content from the pack.

## Source Content Layout
Recommended authoring structure:

- `data/patterns.json` (pattern composition content)
- `data/entities.json` (entity templates and spawn rules)

Both files may be split into additional JSON files under `data/**`.

### Pattern JSON
- root `patterns` array
- each pattern contains composition/layer fields

### Entity JSON
- root `entities` array
- entity types: `player`, `enemy`, `boss`, `resource`, `hazard`
- movement: `static`, `linear`, `sine`, `chase`
- optional attack settings (`attacksEnabled`, `attackPatternName`, `attackIntervalSeconds`)
- spawn rules (`spawnRule.enabled`, `initialDelaySeconds`, `intervalSeconds`, `maxAlive`)
- resource-node interaction fields:
  - `interactionRadius`
  - `resourceYield.upgradeCurrency`
  - `resourceYield.healthRecovery`
  - `resourceYield.buffDurationSeconds`

## Build the Tool
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --target ContentPacker
```

## Compile Content
```powershell
.\build\Debug\ContentPacker.exe --input data --output content.pak
```

## Run the Engine with Pack
```powershell
.\build\Debug\EngineDemo.exe --content-pack content.pak
```

## Validation/Failure Behavior
- Packer fails with schema errors and does not write output if validation fails.
- Runtime attempts to load pack first, then falls back to sandbox/data defaults.
