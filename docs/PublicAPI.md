# Public API + Extensibility

This project now defines a stable external integration surface under:

- `include/engine/public/engine.h`
- `include/engine/public/api.h`
- `include/engine/public/plugins.h`
- `include/engine/public/versioning.h`

Internal implementation details are intentionally separated under:

- `include/engine/internal/*`
- `src/engine/*`

External modules should only include headers from `engine/public`.

## Public surface closure (v1)

Supported/stable surface for external integrations:

- Engine embedding and launch: `EngineHost`, `EngineLaunchOptions`, `publicApiVersion()`
- Versioning helpers: `ApiVersion`, `isApiCompatible(...)`, `ENGINE_DEPRECATED(...)`
- Plugin contracts: `IPlugin`, `IContentPackPlugin`, `IShaderPackPlugin`, `IToolPanelPlugin`
- Plugin lifecycle helpers: `register*Plugin(...)`, `unregister*Plugin(...)`, `clearRegisteredPlugins()`
- Compatibility/diagnostics helpers: `isPluginTargetCompatible(...)`, `pluginRegistrationErrorMessage(...)`

Not part of the public contract (intentionally unstable):

- Any file under `include/engine/internal/*`
- Any file under `src/engine/*`
- Internal container/layout details for plugin registries and load-order storage

## Stable boundary

- `engine::public_api::EngineHost` (PIMPL-backed) provides a small host entry point for embedding/running the engine without including heavy runtime internals.
- Public plugin interfaces allow extension without modifying core engine source.

## Plugin interfaces

### Content pack plugins

Implement `IContentPackPlugin` and call `registerContentPackPlugin(plugin)`.

At runtime, engine appends plugin-provided pack paths to the pack search list.

### Shader pack plugins

Implement `IShaderPackPlugin` and register via `registerShaderPackPlugin(plugin)`.

Current runtime exposes registration and diagnostics/HUD visibility; renderer-side shader-pack application can evolve without API break.

### Tool panel plugins

Implement `IToolPanelPlugin` and register via `registerToolPanelPlugin(plugin)`.

Registered panels are drawn by the control center each frame, enabling custom tools without editing core `editor_tools.cpp` logic.

## Plugin lifecycle contract (host expectations)

Expected integration flow:

1. Construct plugin instance(s) in host/plugin module.
2. Populate `PluginMetadata` with stable plugin id and target API version.
3. Optional preflight: call `isPluginTargetCompatible(metadata)`.
4. Register via `register*Plugin(...)`.
5. Surface failures using `pluginRegistrationErrorMessage(...)`.
6. Before module unload/shutdown, unregister by id (or call `clearRegisteredPlugins()`).

Ownership model:

- Plugin objects are host-owned.
- Registry stores non-owning pointers.
- Destroying plugin instances before unregistering is unsupported and unsafe.

## Versioning + deprecation policy

See `engine/public/versioning.h`:

- PATCH = bugfixes only, no API break.
- MINOR = additive API changes.
- MAJOR = breaking API changes.
- Deprecations are marked with `ENGINE_DEPRECATED(...)` for at least one MINOR before MAJOR removal.

## Migration guidance

- Legacy integrations using internal headers should be migrated to `engine/public` includes.
- Treat all `engine/internal` and non-public headers as unstable implementation details.


## Plugin lifecycle + compatibility

Plugins provide `PluginMetadata` (id, display name, target API version) via `IPlugin::metadata()`.

Registration returns `PluginRegistrationResult` and may reject with:
- `NullPlugin`
- `MissingId`
- `IncompatibleApiVersion`
- `DuplicateId`
- `DuplicateInstance`

Lifecycle controls:
- Register with `register*Plugin(...)`
- Unregister with `unregister*Plugin(pluginId)`
- Clear all registries using `clearRegisteredPlugins()` (recommended for host shutdown/tests)

Compatibility check uses `isApiCompatible(pluginTarget, publicApiVersion())`:
- Major must match exactly.
- Plugin minor must be <= runtime minor.

This keeps extensibility stable while avoiding exposure of runtime internals.


## Host lifecycle expectations

- Plugin instances are host-owned; registry stores non-owning pointers only.
- Unregister plugin ids before destroying instances or call `clearRegisteredPlugins()` during host shutdown.
- Use `isPluginTargetCompatible(metadata)` prior to registration for preflight checks.
- Use `pluginRegistrationErrorMessage(result.error)` for stable user-facing diagnostics.

## Mod/content boundary expectations

- Mod authors should prefer content-pack extension (`--content-pack` layering) over runtime/internal symbol coupling.
- Public plugin hooks are intentionally narrow: content paths, shader-pack descriptors, tool panel draw callbacks.
- Internal load-order storage and registry structure are not API contract and may evolve between versions.

## Mod/content extension structure (supported)

- Base game and mods are layered as content packs via `--content-pack`.
- Multiple pack paths are separated with `;` or `,`.
- Merge order is left-to-right; later packs override earlier entries by `guid`.
- Conflict overrides are logged for diagnostics.
- Plugin-provided content pack paths are appended after CLI/base pack paths and follow the same merge semantics.

This is the supported commercial-v1 extension path for gameplay/content mods.
