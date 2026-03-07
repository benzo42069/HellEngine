# Architecture

## Runtime layers

1. **Config/bootstrap** (`main.cpp`, `config.*`)
   - loads `engine_config.json`
   - applies CLI overrides
   - installs crash handlers (Release)

2. **Simulation core** (`runtime.cpp`, `gameplay_session.*`, `input_system.*`)
   - fixed-step deterministic loop orchestration in `Runtime`
   - gameplay simulation update and progression orchestration in `GameplaySession`
   - input polling/event processing + replay input injection/recording in `InputSystem`

3. **Render pipeline** (`render_pipeline.*`, `render2d.*`, `modern_renderer.*`, `bullet_palette.*`)
   - render-context lifecycle (init/recreate/shutdown)
   - `BulletPaletteTable` maps palette template registry entries to projectile render colors
   - scene overlay composition through `SimSnapshot` sim->render contract
   - `BackgroundSystem` generates startup procedural parallax layers (deep-space noise, grid lines, particle dust) and renders them before gameplay sprites
   - sprite/debug draw + tool HUD composition
   - `ParticleFxSystem` consumes projectile despawn events and renders short-lived impact bursts using frame-time updates (visual-only, no sim coupling)

4. **Diagnostics/replay** (`diagnostics.*`, `replay.*`, `logging.*`)
   - structured error reports (`code/context/stack`)
   - replay record/playback verification with subsystem mismatch reporting
   - logger tail buffering for crash reports

5. **Tooling/editor** (`editor_tools.*`)
   - in-engine control center
   - pattern graph editing/preview
   - validation status and debug controls

## Data flow

- Author JSON content (`patterns`, `entities`, `traits`, `archetypes`, `encounters`, optional `graphs`).
- Pack with `ContentPacker` -> `content.pak`.
- Runtime loads pack(s) and caches pattern/entity databases.
- Deterministic systems run from seed + fixed dt.
- Collision pipeline executes in three explicit stages: `updateMotion` -> `buildGrid` -> `resolveCollisions` (targets queried per overlapping grid cell, then deterministic event sort).

## Safety model

- Bad or missing content should not hard-crash gameplay startup.
- Load errors are logged as structured JSON error reports.
- Runtime falls back to last-good or built-in/default content where available.
- Release crashes emit crash reports (`crashes/`) and Windows minidumps.

## Key extension points

- Add new asset types in `content_pipeline` + packer merge rules.
- Add new pattern graph opcodes in `pattern_graph.h/.cpp`.
- Add authoring UI in `editor_tools.cpp`.
- Add deterministic verifier probes in `runtime.cpp` + `replay.cpp`.


## Rendering pipeline additions (OpenGL hybrid)

- `RenderPipeline` now attempts to initialize an OpenGL 3.3 Core context (`SDL_WINDOW_OPENGL`) and GLAD loader.
- `ShaderCache` compiles/links GLSL programs from `assets/shaders/` (disk-loaded for hot-reload workflows).
- `GrayscaleSpriteAtlas` generates a procedural grayscale SDF atlas texture for six bullet archetype shapes (circle/rice/star/diamond/ring/beam).
- Hybrid rendering model: OpenGL resources are initialized for shader-driven bullets while `SDL_Renderer` remains active for ImGui, debug draw, and existing 2D batching path.
- `PaletteRampTexture` builds a GPU palette ramp LUT (rows per palette) and stores per-row animation metadata used by the bullet shader uniforms.
- `GlBulletRenderer` consumes projectile SoA arrays (`pos/vel/radius/life/palette/active`) to generate rotated quads on CPU each frame, uploads preallocated dynamic buffers, then issues one draw call for all bullets.
- Runtime keeps a hard fallback: when GL context/shaders/textures are unavailable, projectiles continue through the existing `SpriteBatch::renderProcedural` path.
- If OpenGL init fails, renderer startup continues in SDL_Renderer-only mode.

## Post-processing pass chain

- `RendererModernPipeline` now supports a shader-based post-processing chain with explicit OpenGL render targets:
  - `sceneBuffer` (full resolution)
  - `bloomBuffer` (half resolution for performance)
  - `outputBuffer` (full resolution intermediate/final)
- Pass order at `endScene()`:
  1. Bloom pass (threshold + 4-iteration Kawase blur + additive composite)
  2. Vignette pass
  3. Composite pass (tone map + grading + optional chromatic aberration/film grain/scanlines)
- Existing SDL fake overlays are retained as fallback-only methods (`drawBloomLiteFallback`, `drawVignetteFallback`, `drawColorGradeFallback`) when OpenGL post resources are unavailable.
- PostFx data flow:
  - `FxPreset` values are mapped 1:1 into `PostFxSettings`.
  - `RenderPipeline` resolves archetype/zone-driven `autoFxPreset` names and applies the resolved settings each frame through `RendererModernPipeline::setPostFx`.

## Rendering Pipeline Update — GL Bullet Path
- `RenderPipeline::buildSceneOverlay` now routes projectile presentation through `GlBulletRenderer` when OpenGL is ready and the renderer is initialized.
- `GlBulletRenderer` consumes Projectile SoA data (including trails) each frame, resolves palette/gradient color on CPU, builds preallocated quad buffers, and submits one indexed draw.
- SpriteBatch procedural bullet rendering remains the fallback path if GL init fails or is unavailable.
