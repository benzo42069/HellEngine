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

## 6) Plugin lifecycle boundary

Expected host lifecycle:
1. Construct plugin instances.
2. Register via `register*Plugin(...)`.
3. On shutdown/unload, unregister by plugin id (`unregister*Plugin(id)`) or call `clearRegisteredPlugins()`.

Registration validates:
- non-null plugin pointer
- non-empty metadata id
- unique plugin id and unique plugin instance
- API compatibility (`targetApiVersion` vs runtime public API version)

This boundary keeps plugin management in the public layer while keeping registry storage and ordering internal/unstable.


## 7) Compatibility + diagnostics expectations

- Host tools can call `isPluginTargetCompatible(metadata)` for registration preflight before attempting registration.
- Failed registration results should be surfaced via `pluginRegistrationErrorMessage(error)` to keep diagnostics stable for end users.
- Plugin instances remain host-owned and must be unregistered before plugin/module unload.
- Later pack assets override earlier assets by GUID.
- Conflicts are logged for diagnostics.

## 5) Creator-safe extension workflow
1. Build against public headers only.
2. Run smoke + replay verify with your pack/plugin.
3. Pin your integration to a compatible runtime public API version.
4. Avoid internal include dependencies to reduce breakage risk.
