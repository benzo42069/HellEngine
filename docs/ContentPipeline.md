# Content Pipeline

## Goals
- Stable GUID-based identity for authored assets.
- Pack metadata with versioning, compatibility, and content hash.
- Schema migration guardrails.
- Multi-pack load order with conflict reporting.

## Stable GUIDs
All content assets should carry a `guid`:
- patterns
- projectiles
- traits
- archetypes
- enemies/entities
- encounters

If a file omits `guid`, tooling/runtime migrates schema v1->v2 by generating deterministic GUIDs from logical keys (`id` or `name`).

## Pack metadata
Generated packs include:
- `schemaVersion`
- `packVersion`
- `packId`
- `contentHash` (FNV-1a over pack payload)
- `compatibility.minRuntimePackVersion`
- `compatibility.maxRuntimePackVersion`

Runtime load fails when compatibility rules are violated.

## Migration framework
`migratePackJson` applies basic schema migrations:
- v1 -> v2: adds missing GUID fields to known asset arrays.
- unsupported future schema versions fail with migration instructions.

## Multi-pack loading
Runtime accepts multiple pack paths in `--content-pack` using `;` or `,` separators.
Example:
```powershell
.\build\Debug\EngineDemo.exe --content-pack "mods/base.pak;mods/balance_patch.pak"
```

Merge behavior:
- load order is left-to-right
- later packs override earlier entries by GUID
- conflicts are logged (`GUID override detected`)

## Reference stability
Entity->pattern references can use `attackPatternGuid` (preferred) or `attackPatternName` (legacy fallback).
Because references resolve by GUID-or-name, moving or renaming source JSON files does not break references.
