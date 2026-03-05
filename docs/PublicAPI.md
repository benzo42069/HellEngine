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

## Versioning + deprecation policy

See `engine/public/versioning.h`:

- PATCH = bugfixes only, no API break.
- MINOR = additive API changes.
- MAJOR = breaking API changes.
- Deprecations are marked with `ENGINE_DEPRECATED(...)` for at least one MINOR before MAJOR removal.

## Migration guidance

- Legacy integrations using internal headers should be migrated to `engine/public` includes.
- Treat all `engine/internal` and non-public headers as unstable implementation details.

