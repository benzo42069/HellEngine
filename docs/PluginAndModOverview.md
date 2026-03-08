# Plugin and Mod Overview

This guide defines the supported external extension boundary for creators and integrators.

## 1) Public API boundary (stable contract)
Only include public headers:
- `include/engine/public/engine.h`
- `include/engine/public/api.h`
- `include/engine/public/plugins.h`
- `include/engine/public/versioning.h`

Do **not** depend on `include/engine/internal/*` or `src/engine/*` internals for shipped plugins/mod tooling.

## 2) Plugin categories

### Content pack plugins (`IContentPackPlugin`)
- Register with `registerContentPackPlugin(...)`.
- Provide pack paths or pack providers to extend content load order.

### Shader pack plugins (`IShaderPackPlugin`)
- Register with `registerShaderPackPlugin(...)`.
- Provide custom shader-pack metadata for runtime renderer diagnostics/integration.

### Tool panel plugins (`IToolPanelPlugin`)
- Register with `registerToolPanelPlugin(...)`.
- Add editor/control-center panels without modifying engine core editor code.

## 3) Registration and lifecycle expectations
Registration rejects:
- null plugin pointers
- empty metadata ids
- duplicate ids or duplicate instances
- incompatible target API version

Lifecycle:
1. Construct plugin instance.
2. Register plugin.
3. Unregister by id when unloading, or call `clearRegisteredPlugins()` during host teardown.

## 4) Mod pack layering behavior
Runtime accepts multiple packs via `--content-pack`, separated by `;` or `,`.

```bash
./build/EngineDemo --content-pack "mods/base.pak;mods/balance_patch.pak"
```

Merge rules:
- Left-to-right load order.
- Later pack assets override earlier assets by GUID.
- Conflicts are logged for diagnostics.

## 5) Creator-safe extension workflow
1. Build against public headers only.
2. Run smoke + replay verify with your pack/plugin.
3. Pin your integration to a compatible runtime public API version.
4. Avoid internal include dependencies to reduce breakage risk.
