# Plugin and Mod Extension Overview

This document describes the supported external extension boundary for HellEngine.

## 1) Public API boundary

External integrations should include only public headers:
- `include/engine/public/engine.h`
- `include/engine/public/api.h`
- `include/engine/public/plugins.h`
- `include/engine/public/versioning.h`

Do not depend on `include/engine/internal/*` or `src/engine/*` internals for external plugins/mod tooling.

## 2) Supported plugin types

### Content pack plugins
Implement `IContentPackPlugin` and register with `registerContentPackPlugin(plugin)`.

Purpose:
- Provide additional pack paths at runtime.
- Extend/override content through pack load order.

### Shader pack plugins
Implement `IShaderPackPlugin` and register with `registerShaderPackPlugin(plugin)`.

Purpose:
- Surface custom shader packs for runtime diagnostics/HUD and renderer integration.

### Tool panel plugins
Implement `IToolPanelPlugin` and register with `registerToolPanelPlugin(plugin)`.

Purpose:
- Add custom control-center/editor panels without editing core editor modules.

## 3) Mod pack layering behavior

Runtime supports multiple packs via `--content-pack` separated by `;` or `,`.

Example:

```bash
./build/EngineDemo --content-pack "mods/base.pak;mods/balance_patch.pak"
```

Merge rules:
- Left-to-right load order.
- Later pack entries override earlier ones by `guid`.
- Conflicts are logged.

## 4) Versioning and compatibility

Public API policy:
- PATCH: bugfixes only.
- MINOR: additive API changes.
- MAJOR: breaking API changes.

Deprecations are marked and retained for at least one MINOR before MAJOR removal.

## 5) Recommended extension workflow
1. Build a content pack or plugin against current public headers.
2. Validate with headless run + replay verify.
3. Pin against a compatible runtime version.
4. Avoid internal include reliance to reduce breakage risk.
