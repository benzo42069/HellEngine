# HellEngine — Graphics System Improvement Prompts for Claude Code

## How to Use

Same workflow as the architecture prompts:
1. Ensure `CLAUDE.md` is at repo root (for hard constraints).
2. Open a fresh Claude Code session per phase.
3. Copy-paste one prompt at a time.
4. Review diff, verify build + tests, commit.

These 10 graphics phases are **independent of the 10 architecture phases** but assume Phase 1 (Runtime decomposition) is complete. If it isn't, the prompts reference `runtime.cpp` / `render_pipeline.cpp` and you may need to adjust.

Recommended order for maximum visual impact: GFX-1 → GFX-2 → GFX-3 → GFX-4 → GFX-8 → GFX-5 → rest.

---

## GFX-1 — Per-Bullet Palette Colorization

```
You are a Principal Engine Architect working on a C++20 bullet-hell engine.

CONTEXT — Read these files in full before writing any code:
- include/engine/projectiles.h
- src/engine/projectiles.cpp (focus on spawn(), render(), and the SoA arrays)
- include/engine/palette_fx_templates.h (PaletteMaterialParams, PaletteFillResult, deriveProjectileFillFromCore)
- src/engine/palette_fx_templates.cpp (deriveProjectileFillFromCore implementation)
- include/engine/render2d.h (Color struct, SpriteDrawCmd)
- src/engine/runtime.cpp or src/engine/gameplay_session.cpp (where patterns emit projectiles)
- data/palettes/palette_fx_templates.json (existing palette data)

PROBLEM:
Every bullet currently renders with one of two hardcoded colors based on allegiance:
  Enemy → Color{255, 220, 120, 220}
  Player → Color{120, 220, 255, 220}

The palette system has rich per-archetype color data (core/highlight/glow/trail/impact) but none of it reaches the bullet renderer.

TASK:

1. Add a `std::uint8_t paletteIndex` to the ProjectileSystem SoA arrays (alongside posX_, posY_, etc.).
   - Also add it to ProjectileSpawn so emitters can specify which palette a bullet uses.
   - Default value 0.

2. Create a `BulletPaletteTable` class in `include/engine/bullet_palette.h` and `src/engine/bullet_palette.cpp`:
   ```cpp
   class BulletPaletteTable {
   public:
       static constexpr std::uint8_t kMaxPalettes = 64;
       
       struct Entry {
           Color core;
           Color highlight;
           Color glow;
           Color trail;
       };
       
       void setEntry(std::uint8_t index, const PaletteFillResult& fill);
       void buildFromRegistry(const PaletteFxTemplateRegistry& registry);
       [[nodiscard]] const Entry& get(std::uint8_t index) const;
       
   private:
       std::array<Entry, kMaxPalettes> entries_;
   };
   ```

3. In `ProjectileSystem::render()`, look up `paletteIndex_[i]` from the palette table and use the `core` color instead of the hardcoded colors. Keep allegiance-based coloring as a fallback when paletteIndex is 0.

4. Wire it up: When patterns emit projectiles, set `spawn.paletteIndex` based on the active archetype's palette or the enemy's assigned palette from entity data.

5. Add `paletteIndex` field to the ProjectileSpawn in existing pattern emission code paths:
   - PatternPlayer emission lambda in runtime.cpp / gameplay_session.cpp
   - PatternGraphVm emission callback
   - Entity system attack emissions

CONSTRAINTS:
- paletteIndex is NOT part of simulation state — it's visual-only. Do NOT include it in debugStateHash(). Replay hashes must be unchanged.
- The SoA array for paletteIndex should be initialized in initialize() and set in spawn(), same pattern as allegiance_.
- Match existing code style exactly.

DOCUMENTATION:
- docs/DecisionLog.md — "Per-bullet palette colorization: paletteIndex SoA array, BulletPaletteTable, visual-only (not in state hash)"
- docs/Architecture.md — Add BulletPaletteTable to the rendering section.

VERIFY:
- Build compiles
- All tests pass
- Replay hashes unchanged (paletteIndex is visual-only)
- Bullets now render with palette-derived colors instead of hardcoded values

OUTPUT: File tree of changes, git commit command.
```

---

## GFX-2 — Procedural Bullet Sprite Generation

```
You are a Principal Engine Architect working on a C++20 bullet-hell engine.

CONTEXT — Read these files in full before writing any code:
- include/engine/render2d.h (TextureStore, TextureAtlas, Color)
- src/engine/render2d.cpp (createSolidTexture, loadTexture — understand the texture creation pattern)
- include/engine/palette_fx_templates.h (PaletteColor, PaletteFillResult, deriveProjectileFillFromCore)
- src/engine/palette_fx_templates.cpp (deriveProjectileFillFromCore, hsvToRgb, rgbToHsv)
- include/engine/projectiles.h (ProjectileSpawn, ProjectileBehavior)

TASK:

Create a procedural bullet sprite generator that produces distinct bullet textures from palette data at startup. No sprite sheet files needed.

1. Create `include/engine/bullet_sprite_gen.h` and `src/engine/bullet_sprite_gen.cpp`:

   ```cpp
   enum class BulletShape : std::uint8_t {
       Circle,     // standard round bullet
       Rice,       // elongated capsule / needle
       Star,       // polar SDF star (petal-like)
       Diamond,    // rotated square
       Ring,       // hollow circle
       Beam,       // wide horizontal stripe for beam segments
   };
   
   struct BulletSpriteParams {
       BulletShape shape {BulletShape::Circle};
       PaletteColor core;
       PaletteColor highlight;
       PaletteColor glow;
       int size {16};              // texture size in pixels (square)
       float glowFalloff {0.6f};   // how quickly glow fades at edges
       float emissiveBoost {1.0f};
   };
   
   class BulletSpriteGenerator {
   public:
       // Generate a single bullet texture and register it in the TextureStore.
       // Returns the texture ID string.
       std::string generate(
           SDL_Renderer* renderer,
           TextureStore& store,
           const std::string& idPrefix,
           const BulletSpriteParams& params
       );
       
       // Generate a full set of shapes for a given palette, returns atlas of IDs.
       void generatePaletteSet(
           SDL_Renderer* renderer,
           TextureStore& store,
           const std::string& paletteName,
           const PaletteFillResult& fill,
           int size = 16
       );
   };
   ```

2. Implement SDF-based generation for each shape. For each pixel in the texture:
   - Compute signed distance from pixel center to the shape boundary
   - Map distance to color: inside core radius → core color, transition zone → highlight, outer zone → glow with alpha falloff
   - Use SDL_CreateTexture with SDL_TEXTUREACCESS_STATIC, write pixel data via SDL_UpdateTexture

   Shape SDFs (all in normalized [-1,1] space):
   - Circle: `d = length(p) - 0.45`
   - Rice: `d = capsuleSDF(p, vec2(0,-0.4), vec2(0,0.4), 0.15)`
   - Star: `d = length(p) - 0.4 + 0.15 * cos(5 * atan2(p.y, p.x))` (5-pointed)
   - Diamond: `d = abs(p.x) + abs(p.y) - 0.45` (L1 distance)
   - Ring: `d = abs(length(p) - 0.35) - 0.06`
   - Beam: `d = max(abs(p.x) - 0.45, abs(p.y) - 0.08)`

3. On engine startup (in initializeRenderContext or equivalent), call `generatePaletteSet()` for each palette template in the registry. This creates textures like `"bullet_CyanStrike_circle"`, `"bullet_CyanStrike_rice"`, etc.

4. In `ProjectileSpawn`, add a `BulletShape shape` field (default Circle). The render path should look up the appropriate texture by combining palette name + shape.

CONSTRAINTS:
- BulletShape is visual-only, not included in state hash.
- Texture generation runs ONCE at startup, not per-frame.
- Generated textures are small (8-32px). Keep memory bounded — max 64 palettes × 6 shapes × 32×32×4 bytes = ~1.5MB.
- No external image files required. Engine must run and look good with zero sprite assets.

DOCUMENTATION:
- docs/DecisionLog.md — "Procedural bullet sprites: SDF-based generation from palette data, 6 shapes, no external art required"
- docs/AuthoringGuide.md — Add section explaining how to author new bullet visuals by defining palettes in JSON.

VERIFY:
- Build compiles, all tests pass
- Engine renders bullets with procedurally generated sprites (distinct shapes and colors per palette)
- No external sprite files are required for bullet rendering

OUTPUT: File tree, git commit command.
```

---

## GFX-3 — Bullet Trail System

```
You are a Principal Engine Architect working on a C++20 bullet-hell engine.

CONTEXT — Read these files in full before writing any code:
- include/engine/projectiles.h (SoA arrays, render method)
- src/engine/projectiles.cpp (render(), update loop)
- include/engine/render2d.h (SpriteBatch, SpriteDrawCmd, Color)
- include/engine/palette_fx_templates.h (PaletteColor — paletteTrail field)
- include/engine/bullet_palette.h (if GFX-1 is done, the BulletPaletteTable)

TASK:

Add a lightweight trail / afterimage system for bullets.

1. Add trail position history to the ProjectileSystem SoA layout. Use a fixed ring buffer per bullet:

   ```cpp
   static constexpr std::uint8_t kTrailLength = 4;
   // Store last N positions as interleaved X,Y per bullet
   std::vector<float> trailX_;  // capacity_ * kTrailLength
   std::vector<float> trailY_;  // capacity_ * kTrailLength
   std::vector<std::uint8_t> trailHead_;  // ring buffer write index per bullet
   ```

2. In `updateMotion()` (or the motion section of `update()`), BEFORE updating position, record the current position into the trail ring buffer:
   ```cpp
   const std::uint32_t trailBase = i * kTrailLength;
   trailX_[trailBase + trailHead_[i]] = posX_[i];
   trailY_[trailBase + trailHead_[i]] = posY_[i];
   trailHead_[i] = (trailHead_[i] + 1) % kTrailLength;
   ```

3. In `render()`, BEFORE drawing the main bullet sprite, draw the trail positions as faded copies:
   ```cpp
   for (std::uint8_t t = 0; t < kTrailLength; ++t) {
       // Read from oldest to newest
       const std::uint8_t idx = (trailHead_[i] + t) % kTrailLength;
       const float tx = trailX_[trailBase + idx];
       const float ty = trailY_[trailBase + idx];
       if (tx == 0.0f && ty == 0.0f) continue;  // uninitialized
       
       const float alpha = 0.15f + 0.15f * static_cast<float>(t);  // 0.15 → 0.60
       const float scale = 0.6f + 0.1f * static_cast<float>(t);    // shrinking trail
       
       Color trailColor = bulletColor;  // or palette trail color
       trailColor.a = static_cast<Uint8>(alpha * 255.0f);
       
       batch.draw(SpriteDrawCmd { ... with trailColor and scaled size });
   }
   ```

4. Add a `bool enableTrails` flag to ProjectileSpawn / ProjectileBehavior so designers can opt specific archetypes into trails (fast bullets, homing bullets, spirals benefit most; slow linear bullets may not need trails).

5. Initialize trail arrays in `initialize()`, clear in `clear()`, zero out on `spawn()`.

CONSTRAINTS:
- Trail positions are NOT part of state hash. Visual-only.
- Trail rendering adds draw calls but uses the same texture batch — minimize flushes.
- Memory: kTrailLength=4 adds 4*2*4 = 32 bytes per bullet. At 20k capacity = 640KB. Acceptable.
- Only draw trails for bullets where enableTrails is set (default: false for enemy, true for player).

DOCUMENTATION:
- docs/DecisionLog.md — "Trail system: ring buffer of 4 past positions per bullet, visual-only, opt-in per archetype"

VERIFY:
- Build compiles, all tests pass
- Replay hashes unchanged
- Bullets with trails enabled show fading afterimages

OUTPUT: File tree, git commit command.
```

---

## GFX-4 — Impact / Despawn Particles

```
You are a Principal Engine Architect working on a C++20 bullet-hell engine.

CONTEXT — Read these files in full before writing any code:
- include/engine/projectiles.h
- src/engine/projectiles.cpp (deactivation paths — search for "active_[i] = 0")
- include/engine/render2d.h (SpriteBatch, Color)
- include/engine/palette_fx_templates.h (paletteImpact color)

TASK:

Add a lightweight visual-only particle burst when bullets hit or expire.

1. Create `include/engine/particle_fx.h` and `src/engine/particle_fx.cpp`:

   ```cpp
   class ParticleFxSystem {
   public:
       void initialize(std::uint32_t capacity);  // e.g. 4096 particles
       
       // Spawn a burst of N particles at position with given color
       void burst(Vec2 pos, Color color, std::uint8_t count = 4, float speed = 80.0f);
       
       // Update all particles (call once per frame, NOT per sim tick)
       void update(float frameDelta);
       
       // Render all particles
       void render(SpriteBatch& batch, const std::string& textureId) const;
       
       void clear();
       
   private:
       struct Particle {
           float x, y;
           float vx, vy;
           float life;       // remaining life in seconds
           float maxLife;
           Color color;
       };
       
       std::vector<Particle> particles_;
       std::uint32_t activeCount_ {0};
       std::uint32_t capacity_ {0};
   };
   ```

2. Implementation details:
   - `burst()`: spawn N particles with random-ish directions (use a simple deterministic spread: `angle = i * (2π/N) + small_offset`). Velocity is `speed * (cos(angle), sin(angle))`. Life = 0.12-0.18 seconds.
   - `update()`: for each active particle, integrate position, decrement life, alpha-fade based on `life/maxLife`. Swap-remove dead particles.
   - `render()`: draw each particle as a small quad (2-4px) with current alpha.

3. Wire into the bullet system: When a bullet is deactivated (in updateMotion or resolveCollisions), emit a burst event. Since ParticleFxSystem is visual-only, the cleanest approach is:
   - Add a callback or event buffer to ProjectileSystem that records despawn positions + colors this tick
   - The render pipeline reads the buffer and calls `particleFx_.burst()` for each
   - Alternatively, ProjectileSystem stores a `std::vector<DespawnEvent>` (pos, paletteIndex) cleared each tick

4. Use the palette's `impact` color for the particle color. Fall back to the bullet's core color if no palette is set.

CONSTRAINTS:
- ParticleFxSystem is ENTIRELY presentation-only. It runs on frame time, not sim time.
- It does NOT participate in collision, state hashing, or determinism.
- No heap allocations after initialize() — fixed capacity with swap-remove.
- Keep particle count bounded: max 4096 particles alive, oldest are dropped if exceeded.

DOCUMENTATION:
- docs/DecisionLog.md — "Impact particles: visual-only ParticleFxSystem, frame-time update, no sim coupling"
- docs/Architecture.md — Add ParticleFxSystem to the rendering layer section.

VERIFY:
- Build compiles, all tests pass
- Replay hashes unchanged
- Bullets produce small particle bursts on hit/expire

OUTPUT: File tree, git commit command.
```

---

## GFX-5 — Parallax Background Layer System

```
You are a Principal Engine Architect working on a C++20 bullet-hell engine.

CONTEXT — Read these files in full before writing any code:
- include/engine/render2d.h (Camera2D, TextureStore, SpriteBatch)
- src/engine/render2d.cpp (worldToScreen, texture creation)
- include/engine/standards.h (playfieldWidth, playfieldHeight)
- src/engine/runtime.cpp or src/engine/render_pipeline.cpp (where renderFrame draws the scene)

TASK:

Add a layered parallax background system with procedurally generated textures.

1. Create `include/engine/background_layers.h` and `src/engine/background_layers.cpp`:

   ```cpp
   struct BackgroundLayer {
       std::string textureId;
       float parallaxFactor;   // 0.0 = static, 1.0 = moves with camera
       float scrollSpeedX;     // autonomous scroll per second
       float scrollSpeedY;
       float opacity;          // 0.0-1.0
       Color tint;
   };
   
   class BackgroundSystem {
   public:
       void initialize(SDL_Renderer* renderer, TextureStore& store);
       void addLayer(const BackgroundLayer& layer);
       void update(float frameDelta);
       void render(SpriteBatch& batch, const Camera2D& camera) const;
       void clear();
       
   private:
       std::vector<BackgroundLayer> layers_;
       float scrollOffset_ {0.0f};
   };
   ```

2. Procedural texture generation for backgrounds. In `initialize()`, generate 3 default layers:

   **Layer 0 — Deep space (parallax 0.1):** Dark noise field. Generate a 128×128 texture where each pixel is `base_color + noise * variation`. Use a simple hash-based noise: `hash(x * 73856093 ^ y * 19349663) % 256` scaled to small brightness variation. Tile seamlessly by wrapping coordinates.

   **Layer 1 — Grid lines (parallax 0.3):** A 256×256 texture with subtle grid lines every 32px, low alpha. This gives spatial reference during gameplay.

   **Layer 2 — Particle dust (parallax 0.6):** A 128×128 texture with scattered bright dots (probability ~1% per pixel, brightness 0.4-0.8). Simulates floating particles.

3. `render()`: For each layer, calculate the tiled UV offset based on camera position × parallaxFactor + scrollOffset, then draw enough tiles to cover the viewport. Use SpriteBatch with the layer's tint and opacity.

4. Call `backgroundSystem_.render(batch, camera)` BEFORE bullet/entity rendering in the render pipeline so backgrounds are behind everything.

5. Data-driven layers: Allow layers to be defined in JSON (optional — the procedural defaults should look good out of the box).

CONSTRAINTS:
- Background is presentation-only, no sim coupling.
- Texture generation runs once at startup.
- Maximum 6 layers to keep draw call count bounded.
- Background must tile seamlessly — no visible seams when camera scrolls.

DOCUMENTATION:
- docs/DecisionLog.md — "Parallax background: 3-layer procedural generation, hash-noise + grid + particle dust"
- docs/Architecture.md — Add BackgroundSystem to rendering section.

VERIFY:
- Build compiles, all tests pass
- Engine shows layered scrolling background instead of solid color
- Camera pan shows parallax depth effect

OUTPUT: File tree, git commit command.
```

---

## GFX-6 — Danger Field Heatmap Overlay

```
You are a Principal Engine Architect working on a C++20 bullet-hell engine.

CONTEXT — Read these files in full before writing any code:
- include/engine/projectiles.h (gridHead_, gridNext_, gridX_, gridY_, SoA arrays)
- src/engine/projectiles.cpp (gridIndexFor, buildGrid / update grid-build section)
- include/engine/render2d.h (TextureStore, SpriteBatch, Camera2D)
- include/engine/palette_fx_templates.h (GradientDefinition, generateGradientLut)
- include/engine/modern_renderer.h (RendererModernPipeline — understand the render target approach)

TASK:

Create a low-resolution SDF-based danger heatmap overlay that gives players subconscious awareness of bullet density.

1. Create `include/engine/danger_field.h` and `src/engine/danger_field.cpp`:

   ```cpp
   class DangerFieldOverlay {
   public:
       void initialize(SDL_Renderer* renderer, int fieldWidth, int fieldHeight);
       // fieldWidth/Height should be low-res, e.g. 160×90 for a 1280×720 viewport
       
       void shutdown();
       
       // Build the density field from the projectile grid
       void buildFromGrid(
           const std::vector<int>& gridHead,
           const std::vector<int>& gridNext,
           const std::vector<float>& posX,
           const std::vector<float>& posY,
           const std::vector<std::uint8_t>& active,
           std::uint32_t gridX, std::uint32_t gridY,
           float worldHalfExtent
       );
       
       void render(SDL_Renderer* renderer, const Camera2D& camera, float opacity = 0.25f);
       
       void setGradient(const std::vector<PaletteColor>& lut);  // danger gradient (blue→yellow→red)
       
   private:
       SDL_Texture* fieldTexture_ {nullptr};
       int fieldW_ {160};
       int fieldH_ {90};
       std::vector<float> density_;        // fieldW_ * fieldH_
       std::vector<std::uint32_t> pixels_; // RGBA pixel buffer
       std::vector<PaletteColor> gradientLut_;
   };
   ```

2. `buildFromGrid()` implementation:
   - For each cell in the density field, map to the corresponding region of the collision grid
   - Count bullets in nearby grid cells (simple box filter, radius 1-2 cells)
   - Normalize count to [0,1] range (0 bullets = 0.0, threshold like 8+ bullets = 1.0)
   - Apply a simple 3×3 box blur pass on the density array for smoothness

3. `render()` implementation:
   - Map density values through the gradient LUT to RGBA pixels
   - Density 0.0 → transparent (alpha=0)
   - Density > 0.1 → gradient color with alpha scaled by density × opacity
   - Upload pixel buffer to fieldTexture_ via SDL_UpdateTexture
   - Draw the texture stretched to cover the viewport with additive blending

4. Default gradient: deep blue (safe) → yellow (moderate) → red (danger). Use the existing gradient LUT system from palette_fx_templates.

5. Wire into render pipeline: call `dangerField_.buildFromGrid(...)` after the projectile grid is built, call `dangerField_.render(...)` as a screen overlay before UI.

6. Make it toggleable via a keyboard shortcut (e.g., 'V' for "vision") and an editor tools checkbox.

CONSTRAINTS:
- Presentation-only. No sim coupling.
- The density texture is LOW resolution — do not make it 1:1 with viewport.
- SDL_UpdateTexture per frame at 160×90 = ~57KB upload — this is fine.
- Blur must be simple (box filter, not gaussian) to stay fast.

DOCUMENTATION:
- docs/DecisionLog.md — "Danger field overlay: low-res density heatmap from collision grid, gradient-mapped, additive blend"

VERIFY:
- Build compiles, all tests pass
- Toggle overlay with 'V' — areas with many bullets glow warm/red
- Performance: density build + upload < 0.5ms per frame

OUTPUT: File tree, git commit command.
```

---

## GFX-7 — Pattern Signature Textures

```
You are a Principal Engine Architect working on a C++20 bullet-hell engine.

CONTEXT — Read these files in full before writing any code:
- include/engine/pattern_graph.h (CompiledPatternGraph, PatternGraphVm, PatternOp)
- src/engine/pattern_graph.cpp (VM execute — understand how patterns emit)
- include/engine/render2d.h (TextureStore)
- include/engine/projectiles.h (ProjectileSpawn)

TASK:

Generate a unique visual "fingerprint" texture for each pattern by running a quick offline simulation and rendering the bullet positions into a small radial texture.

1. Create `include/engine/pattern_signature.h` and `src/engine/pattern_signature.cpp`:

   ```cpp
   class PatternSignatureGenerator {
   public:
       // Run a short simulation of the pattern and render positions into a texture
       std::string generate(
           SDL_Renderer* renderer,
           TextureStore& store,
           const CompiledPatternGraph& graph,
           std::uint64_t seed,
           int textureSize = 64,   // 64×64 signature texture
           int simTicks = 80       // ~1.3 seconds of sim at 60fps
       );
   };
   ```

2. Implementation:
   - Create a temporary float buffer (textureSize × textureSize, initialized to 0).
   - Instantiate a PatternGraphVm::RuntimeState, reset with the given seed.
   - Run the VM for simTicks iterations at dt=1/60. Collect all emitted projectile positions.
   - For each emitted position, integrate it forward for a few ticks (simple linear: pos += vel * dt * remaining_ticks).
   - Map each final bullet position to a pixel in the texture (center of texture = origin, scale to fit).
   - Accumulate brightness at that pixel (additive, clamped to 1.0).
   - Apply a small blur pass for smoothness.
   - Convert the float buffer to RGBA using the pattern's palette (or a default white→cyan gradient).
   - Create an SDL_Texture and register in the TextureStore with id like `"sig_[graphId]"`.

3. Generate signatures for all compiled pattern graphs at content load time.

4. These signature textures can be used for:
   - Pattern library browser thumbnails in the editor
   - As the actual bullet sprite for that pattern (each pattern's bullets have a unique micro-texture)
   - Debug overlay showing which pattern spawned which bullets

CONSTRAINTS:
- Generation runs at load time, not per-frame.
- The simulation is a throwaway — uses temporary VM state, does not affect the real sim.
- Max texture size 64×64 per pattern. Even 100 patterns = 100 × 64 × 64 × 4 = ~1.6MB.

DOCUMENTATION:
- docs/DecisionLog.md — "Pattern signature textures: offline mini-sim renders pattern fingerprint for library thumbnails and visual identity"
- docs/AuthoringGuide.md — Mention that pattern signatures are auto-generated.

VERIFY:
- Build compiles, all tests pass
- Each compiled pattern graph generates a unique signature texture
- Signatures are visible in the editor pattern browser

OUTPUT: File tree, git commit command.
```

---

## GFX-8 — Palette-Animated Gradient Mapping

```
You are a Principal Engine Architect working on a C++20 bullet-hell engine.

CONTEXT — Read these files in full before writing any code:
- include/engine/palette_fx_templates.h (GradientDefinition, GradientStop, PaletteAnimationSettings, PaletteAnimationMode, generateGradientLut)
- src/engine/palette_fx_templates.cpp (generateGradientLut implementation, animation mode parsing)
- include/engine/projectiles.h (render method, SoA arrays)
- src/engine/projectiles.cpp (render — current sprite draw loop)
- include/engine/render2d.h (TextureStore, TextureAtlas)
- data/palettes/palette_fx_templates.json (gradient definitions, animation settings per palette)

TASK:

Implement palette-animated gradient mapping: bullets rendered as grayscale shapes are colored by sampling a gradient LUT that scrolls over time, creating shimmering color waves across bullet rings.

1. Create `include/engine/gradient_animator.h` and `src/engine/gradient_animator.cpp`:

   ```cpp
   class GradientAnimator {
   public:
       // Pre-render N frames of a gradient into an atlas row.
       // Each frame is a 1×height strip showing the gradient at a different phase offset.
       void initialize(SDL_Renderer* renderer, TextureStore& store);
       
       // Register a gradient+animation pair. Returns an animation ID.
       std::uint8_t addGradient(
           const GradientDefinition& gradient,
           const PaletteAnimationSettings& anim,
           int lutWidth = 64   // pixels per frame
       );
       
       // Get the current color for a bullet given its animation ID, life time, and instance index
       Color sample(
           std::uint8_t animId,
           float time,              // global sim clock
           float bulletAge,         // bullet's life_ value
           std::uint32_t instanceIndex  // bullet slot for per-instance phase offset
       ) const;
       
   private:
       struct AnimatedGradient {
           std::vector<PaletteColor> lut;  // the gradient LUT
           PaletteAnimationSettings anim;
       };
       std::vector<AnimatedGradient> gradients_;
   };
   ```

2. `sample()` implementation:
   - Compute phase: `phase = time * anim.speed + instanceIndex * anim.perInstanceOffset + anim.phaseOffset`
   - For HueShift mode: sample the gradient at position `fmod(bulletAge * 0.5 + phase, 1.0)`, then hue-shift the result
   - For GradientCycle mode: sample at `fmod(phase, 1.0)` — the entire gradient cycles over time
   - For PulseBrightness mode: sample normally but modulate brightness by `0.7 + 0.3 * sin(phase * 2π)`
   - For PhaseScroll mode: each bullet in a ring gets a slightly different gradient position, creating a wave effect
   - Map the sampled PaletteColor to a Color (Uint8 RGBA)

3. Wire into the bullet render loop:
   - If a bullet's palette has an animation mode != None, use `gradientAnimator_.sample()` instead of the static palette color.
   - Pass `simClock_` as the time parameter, `life_[i]` as bulletAge, and `i` as instanceIndex.

4. The key visual effect: a ring of 16 bullets emitted simultaneously will each have a slightly different gradient phase (due to perInstanceOffset × index), creating a sequential color wave that rotates around the ring as time progresses. This is the signature look of high-end bullet-hell games.

CONSTRAINTS:
- Gradient sampling is per-bullet per-frame — must be fast. The LUT is a small array lookup + lerp, no trig per sample (precompute sine tables if needed for PulseBrightness).
- Visual-only, no sim coupling, no state hash impact.
- Precompute gradient LUTs at initialization time, not per-frame.

DOCUMENTATION:
- docs/DecisionLog.md — "Gradient animation: per-bullet phase-offset sampling creates color wave effect across emission rings"
- docs/AuthoringGuide.md — Explain animation modes and how perInstanceOffset creates wave effects.

VERIFY:
- Build compiles, all tests pass
- Replay hashes unchanged
- Bullet rings with GradientCycle or PhaseScroll animation show visible color wave

OUTPUT: File tree, git commit command.
```

---

## GFX-9 — Screen-Shake Vocabulary

```
You are a Principal Engine Architect working on a C++20 bullet-hell engine.

CONTEXT — Read these files in full before writing any code:
- include/engine/render2d.h (Camera2D — setShake, update, shakeAmplitude_, shakeTimeRemaining_)
- src/engine/render2d.cpp (Camera2D::update — current shake implementation)
- include/engine/defensive_special.h (for defensive special activation events)
- src/engine/runtime.cpp or gameplay_session.cpp (where camera.setShake is called)

TASK:

Replace the single shake mode with a vocabulary of parameterized shake profiles.

1. Expand `Camera2D` or create a companion `CameraShakeSystem`:

   ```cpp
   enum class ShakeProfile : std::uint8_t {
       Impact,         // short directional kick away from hit point
       BossRumble,     // heavy low-frequency roll for boss phase transitions
       GrazeTremor,    // rapid micro-shake for near-miss graze
       SpecialPulse,   // smooth radial pulse for defensive special
       Explosion,      // sharp spike with fast decay
       Ambient,        // very subtle continuous drift
   };
   
   struct ShakeParams {
       ShakeProfile profile {ShakeProfile::Impact};
       float amplitude {2.0f};
       float duration {0.15f};
       Vec2 direction {0.0f, 0.0f};  // for directional shakes (Impact, Explosion)
       float frequency {30.0f};       // oscillation Hz
       float damping {8.0f};          // exponential decay rate
   };
   
   class CameraShakeSystem {
   public:
       void trigger(const ShakeParams& params);
       void update(float dt);
       [[nodiscard]] Vec2 offset() const;  // current shake displacement
       void clear();
       
   private:
       // Support multiple simultaneous shakes (max 4, oldest replaced)
       static constexpr int kMaxActive = 4;
       struct ActiveShake {
           ShakeParams params;
           float elapsed {0.0f};
           bool active {false};
       };
       std::array<ActiveShake, kMaxActive> shakes_;
   };
   ```

2. Shake profile implementations in `update()`:
   - **Impact**: `offset = direction * amplitude * sin(elapsed * frequency) * exp(-damping * elapsed)`. Short, punchy, one direction.
   - **BossRumble**: `offset = (sin(elapsed*8), cos(elapsed*6)) * amplitude * (1 - elapsed/duration)`. Slow, heavy, omnidirectional.
   - **GrazeTremor**: `offset = (hash_noise(elapsed*120), hash_noise(elapsed*120+99)) * amplitude * exp(-damping*elapsed)`. Rapid, random, tiny.
   - **SpecialPulse**: `offset = (sin(elapsed*12), cos(elapsed*12)) * amplitude * sin(π * elapsed/duration)`. Smooth bell curve.
   - **Explosion**: `offset = direction * amplitude * exp(-damping*2 * elapsed) + random_micro`. Sharp initial spike.
   - **Ambient**: `offset = (sin(elapsed*2.3), sin(elapsed*1.7)) * amplitude`. Continuous gentle drift.

3. Multiple shakes blend additively: `totalOffset = sum of all active shake offsets`.

4. Wire to game events:
   - Player hit by bullet → `Impact` toward hit direction
   - Graze → `GrazeTremor` (very subtle)
   - Boss phase transition → `BossRumble`
   - Defensive special activation → `SpecialPulse`
   - Bullet explosion (explodeShards) → `Explosion` toward explosion center
   - Default ambient during combat → `Ambient` at very low amplitude

5. Replace the existing `Camera2D::setShake()` call sites with `cameraShake_.trigger(...)`.

CONSTRAINTS:
- Presentation-only. No sim coupling.
- All shake math uses frame delta time, not sim delta time.
- Keep total shake offset clamped to a reasonable maximum (±20px) to prevent disorienting camera movement.

DOCUMENTATION:
- docs/DecisionLog.md — "Camera shake vocabulary: 6 profiles, additive blending, max 4 simultaneous"

VERIFY:
- Build compiles, all tests pass
- Different game events produce visually distinct shake responses
- Replay hashes unchanged

OUTPUT: File tree, git commit command.
```

---

## GFX-10 — Procedural Level Tile Generation

```
You are a Principal Engine Architect working on a C++20 bullet-hell engine.

CONTEXT — Read these files in full before writing any code:
- include/engine/run_structure.h (RunStructure, StageDefinition, ZoneDefinition, ZoneType)
- src/engine/run_structure.cpp (stage/zone progression)
- include/engine/render2d.h (TextureStore)
- include/engine/palette_fx_templates.h (PaletteColor, deriveBackgroundFillFromAccent)
- include/engine/background_layers.h (if GFX-5 is done, the BackgroundSystem)

TASK:

Generate unique level background tiles procedurally using cellular automata and the zone/stage palette, so each stage in a run has a distinct visual identity from seed alone.

1. Create `include/engine/level_tile_gen.h` and `src/engine/level_tile_gen.cpp`:

   ```cpp
   enum class TileGenRule : std::uint8_t {
       CaveFormation,      // classic B678/S345678 — organic blobs
       CrystalGrowth,      // B3/S1234 — angular crystalline structures
       MazeCorridors,      // B3/S12345 — maze-like passages
       OrganicNoise,       // Perlin/value noise field — smooth gradients
       CircuitBoard,       // Grid-aligned right-angle channels
       VoidScatter,        // Sparse bright dots on dark field (deep space)
   };
   
   struct TileGenParams {
       TileGenRule rule {TileGenRule::OrganicNoise};
       std::uint64_t seed {0};
       int gridWidth {64};       // cellular automata grid
       int gridHeight {64};
       int iterations {4};       // CA simulation steps
       float initialDensity {0.45f};  // probability of alive cell at init
       PaletteColor primary;     // wall/structure color
       PaletteColor secondary;   // fill/background color
       PaletteColor accent;      // highlight/glow spots
       int textureSize {256};    // output texture resolution
   };
   
   class LevelTileGenerator {
   public:
       // Generate a tileable background texture for a stage
       std::string generate(
           SDL_Renderer* renderer,
           TextureStore& store,
           const std::string& idPrefix,
           const TileGenParams& params
       );
       
       // Auto-generate for a stage based on zone type and run seed
       std::string generateForZone(
           SDL_Renderer* renderer,
           TextureStore& store,
           ZoneType zone,
           std::uint32_t stageIndex,
           std::uint64_t runSeed
       );
   };
   ```

2. Cellular automata implementation:
   - Initialize grid: each cell alive with probability `initialDensity`, seeded by `hash(seed, x, y)`
   - Run N iterations of the specified rule (B/S notation — count alive neighbors, apply birth/survival rules)
   - For tileable results: wrap grid edges (cell at x=-1 reads from x=gridWidth-1)

3. Texture rendering from CA grid:
   - For each pixel in the output texture, sample the CA grid (bilinear interpolation for smooth look)
   - Alive cells → primary color, dead cells → secondary color
   - Add accent highlights at cell boundaries (edge detection: alive cell next to dead cell)
   - Apply slight brightness noise for texture (hash-based, ±5%)

4. Zone type → rule mapping:
   - Combat → CaveFormation (organic, alien)
   - Elite → CrystalGrowth (angular, sharp)
   - Event → OrganicNoise (calm, smooth)
   - Boss → CircuitBoard (structured, ominous)
   - Derive colors from zone palette using deriveBackgroundFillFromAccent()

5. `generateForZone()` derives deterministic params from `hash(runSeed, stageIndex, zoneType)` so the same run seed always produces the same level backgrounds.

6. Wire into BackgroundSystem (GFX-5) or the render pipeline: when the run structure transitions to a new zone, generate the tile and swap the background layer texture.

CONSTRAINTS:
- Generation is deterministic from seed — same seed = same background always.
- Generation runs at zone transition, not per-frame. Target: < 50ms for a 256×256 texture.
- The generated texture must tile seamlessly (wrap-around CA + edge-matched noise).
- Presentation-only, no sim coupling.

DOCUMENTATION:
- docs/DecisionLog.md — "Procedural level tiles: cellular automata + zone-typed rules, seed-deterministic, 256px tileable"
- docs/AuthoringGuide.md — Explain how run seed + zone type drives visual generation. Mention that designers can override with custom tile rules per stage.

VERIFY:
- Build compiles, all tests pass
- Each zone type generates visually distinct background tiles
- Same seed produces identical tiles across runs
- Tiles are seamlessly tileable (no visible seams when scrolling)

OUTPUT: File tree, git commit command.
```
