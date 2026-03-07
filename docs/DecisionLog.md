# Decision Log

## 2026-03-07 — Camera shake vocabulary
- **Context**: A single sine-wave shake mode could not convey distinct gameplay feedback for hits, grazes, boss transitions, specials, and explosions.
- **Decision**: Introduce a six-profile camera shake vocabulary (`Impact`, `BossRumble`, `GrazeTremor`, `SpecialPulse`, `Explosion`, `Ambient`) implemented with additive blending, max 4 simultaneous active shakes, and ±20px clamp.
- **Rationale**: Improves feedback clarity while keeping shake logic presentation-only, frame-delta-driven, and decoupled from deterministic simulation state.
- **Status**: Accepted.

## 2026-03-07 — Pattern signature textures
- **Context**: Graph-authored patterns needed a compact visual fingerprint for editor/library browsing and optional per-pattern projectile identity.
- **Decision**: Generate a 64×64 signature texture per compiled pattern graph at load time by running a short throwaway VM simulation and rasterizing emitted projectile trajectories into a blurred radial density map.
- **Rationale**: Provides deterministic, low-memory visual identity per pattern without affecting live simulation state.
- **Status**: Accepted.

## 2026-03-06 — Danger field overlay
- **Context**: Players need lightweight subconscious guidance about bullet density without changing deterministic simulation behavior.
- **Decision**: Add a presentation-only, low-resolution danger field overlay generated from projectile collision-grid occupancy, smoothed with a box blur, gradient-mapped (blue→yellow→red), and additively blended over the scene.
- **Rationale**: Reuses existing broadphase data for cheap per-frame density estimation, improving readability while keeping sim/render boundaries clean.
- **Status**: Accepted.

## 2026-03-06 — Parallax background layering
- **Context**: The runtime renderer used a flat clear color with no depth cues behind gameplay actors.
- **Decision**: Add `BackgroundSystem` with three startup-generated procedural layers (hash-noise deep space, low-alpha grid, and sparse particle dust) using bounded parallax tiling.
- **Rationale**: Improves spatial readability and scene depth while keeping rendering presentation-only and deterministic-sim independent.
- **Status**: Accepted.

## 2026-03-04 — Phase 1 Scope Enforcement
- **Context**: Phase requires documentation ingestion only.
- **Decision**: Restrict outputs to `docs/MasterSpec.md`, `docs/DecisionLog.md`, and `docs/ImplementationPlan.md`.
- **Rationale**: Explicit user instruction for this phase.
- **Status**: Accepted.

## 2026-03-04 — Layered Architecture Lock-In
- **Context**: Requirements define a runtime+tooling layered architecture.
- **Decision**: Adopt canonical layers: CoreRuntime, Simulation, GameplaySystems, PatternRuntime, Render, AssetPipeline, EditorAPI/Tooling.
- **Rationale**: Maintains deterministic boundaries and clean dependency flow.
- **Status**: Accepted.

## 2026-03-04 — Deterministic Math Policy
- **Context**: Design permits fixed-point or deterministic float.
- **Decision**: Prefer fixed-point-capable implementation, retain deterministic-float compatibility mode under strict validation.
- **Rationale**: Strong parity for replay/network while preserving practical adoption path.
- **Status**: Accepted.

## 2026-03-04 — Bullet Motion Composition Order
- **Context**: Multiple motion operators (steering, integration, offsets, constraints) can produce divergent results if ordered inconsistently.
- **Decision**: Canonical per-tick order is Steering -> Velocity Integration -> Parametric Offsets -> Constraints -> Final Position.
- **Rationale**: Ensures reproducibility and consistent designer expectations.
- **Status**: Accepted.

## 2026-03-04 — Hybrid ECS + Bullet World
- **Context**: Pure ECS flexibility conflicts with 10k+ projectile throughput requirements.
- **Decision**: Use general ECS for broad gameplay entities and specialized Bullet World for bullets.
- **Rationale**: Balances flexibility and deterministic high-throughput performance.
- **Status**: Accepted.

## 2026-03-04 — Unified Pattern IR from Graph/Timeline/DSL
- **Context**: Three authoring modalities are required.
- **Decision**: Compile all modalities to a single Pattern IR/bytecode representation.
- **Rationale**: Avoids behavior mismatch and centralizes deterministic runtime semantics.
- **Status**: Accepted.

## 2026-03-04 — Indexed RNG Discipline
- **Context**: Traditional sequential RNG calls can diverge under branch differences.
- **Decision**: Use stream/substream derivation and prefer indexed hash-based randomness for pattern variation.
- **Rationale**: Minimizes call-order drift and hardens replay consistency.
- **Status**: Accepted.

## 2026-03-04 — Deterministic Trigger/Collision Tie-Breaks
- **Context**: Simultaneous triggers/collisions require deterministic resolution.
- **Decision**: Enforce stable tie-break keys (tick/order key, stable IDs, then deterministic sort).
- **Rationale**: Prevents non-deterministic branch outcomes in phase transitions and hit resolution.
- **Status**: Accepted.

## 2026-03-04 — CPU-First Sim, GPU-First Rendering
- **Context**: GPU sim is possible but increases determinism/debugging complexity.
- **Decision**: Keep simulation CPU-first; use GPU heavily for rendering/visualization.
- **Rationale**: Improves tooling introspection and reduces implementation risk.
- **Status**: Accepted.

## 2026-03-04 — Allocation and Degradation Policy
- **Context**: Overload scenarios can threaten frame stability.
- **Decision**: No per-tick dynamic allocations in shipping sim; degrade visuals/effects before sim correctness.
- **Rationale**: Maintains fairness and deterministic integrity under pressure.
- **Status**: Accepted.

## 2026-03-04 — Optional Advanced Features Phasing
- **Context**: Networking/plugins/mod runtime expansion is high value but non-foundational for core deterministic runtime.
- **Decision**: Deliver optional advanced features in later phases behind feature flags.
- **Rationale**: Keeps early phases focused on deterministic kernel and buildability.
- **Status**: Accepted.


## 2026-03-04 — Pattern Graph Dual-Flow Contract
- **Context**: Pattern authoring requires both sequencing and typed parameter computation.
- **Decision**: Standardize Pattern Graph as dual-flow (Exec + Data) with explicit type conversions and deterministic merge ordering for parallel branches.
- **Rationale**: Improves designer clarity while preserving deterministic compilation/runtime behavior.
- **Status**: Accepted.

## 2026-03-04 — Modifier Stack Canonicalization
- **Context**: Modifier order materially changes output and can create inconsistent authored behavior.
- **Decision**: Enforce explicit modifier stack ordering (Parameter -> Angle -> Position -> Post-spawn bindings) with editor warnings on conflicts.
- **Rationale**: Stabilizes visual results and replay parity across edits and refactors.
- **Status**: Accepted.

## 2026-03-04 — Pattern Graph Compilation Requirements
- **Context**: Visual graphs need commercial usability without runtime overhead.
- **Decision**: Require validation/lowering/optimization/codegen pipeline producing cached Pattern IR plus node-to-IR debug mapping.
- **Rationale**: Ensures performance, deterministic execution, and high-quality tooling introspection.
- **Status**: Accepted.

## 2026-03-04 — Pattern Graph Debuggability Baseline
- **Context**: Designers require fast diagnosis of unfair or expensive patterns.
- **Decision**: Make provenance tracing, node execution highlights, breakpoints/watches, and heatmap overlays mandatory Pattern Graph tooling features.
- **Rationale**: Reduces iteration cost and improves fairness/performance debugging quality.
- **Status**: Accepted.


## 2026-03-04 — PBAGS Generation Boundary
- **Context**: Procedural generation can drift into opaque emergent behavior.
- **Decision**: PBAGS may only compose vetted authored modules and parameterize them; it must not synthesize unvetted runtime logic.
- **Rationale**: Maintains trust, debuggability, and deterministic predictability.
- **Status**: Accepted.

## 2026-03-04 — Deterministic Selection and Synthesis Policy for PBAGS
- **Context**: Weighted random generation and parameter tuning can break replay parity without strict discipline.
- **Decision**: Use deterministic weighted sampling with stream discipline and indexed parameter synthesis, followed by deterministic bounded constraint adjustments.
- **Rationale**: Preserves replay/network consistency while still enabling procedural variety.
- **Status**: Accepted.

## 2026-03-04 — Budget-First Difficulty Governance
- **Context**: Difficulty must be controlled across density/speed/entropy/aim pressure, not a single scalar.
- **Decision**: Govern PBAGS by explicit multi-dimensional budgets with deterministic fallback order (reduce secondary layers, then density, speed, aim aggression, then module swap).
- **Rationale**: Keeps generated attacks fair, readable, and performance-safe.
- **Status**: Accepted.

## 2026-03-04 — Lock/Regenerate Authoring Workflow
- **Context**: Designers need partial control over generated outputs.
- **Decision**: Require lock/regenerate granularity at segment/motif/parameter levels with explainable per-segment reports.
- **Rationale**: Enables procedural speed without sacrificing authored intent.
- **Status**: Accepted.

## 2026-03-04 — Deterministic Adaptive Variant Rule
- **Context**: Adaptive difficulty can invalidate replays if generated online.
- **Decision**: Allow adaptation only through pre-generated variants selected deterministically from recorded performance bands.
- **Rationale**: Retains adaptability while preserving strict replay determinism.
- **Status**: Accepted.


## 2026-03-04 — AI Copilot Boundary (Pattern Generation)
- **Context**: AI-assisted tooling can become opaque and undermine deterministic trust.
- **Decision**: AI may propose blueprint structure only; final outputs must be concretized through approved deterministic graph templates and constraint solvers.
- **Rationale**: Preserves predictability, auditability, and runtime safety.
- **Status**: Accepted.

## 2026-03-04 — Tiered Generator Strategy
- **Context**: Product roadmap includes rule-based and model-assisted options.
- **Decision**: Tier 1 (rule/template) is required shipping baseline; Tier 2+ (LLM suggestion/ranking models) are optional enhancements behind safety boundaries.
- **Rationale**: Ensures production viability without ML dependency while allowing future expansion.
- **Status**: Accepted.

## 2026-03-04 — Explainability as a Hard Requirement
- **Context**: Designer trust requires understanding why a generated pattern exists.
- **Decision**: Generation reports, node annotations, constraint satisfaction summaries, and selective lock/regenerate controls are mandatory.
- **Rationale**: Supports rapid iteration and safer tuning under constraints.
- **Status**: Accepted.

## 2026-03-04 — Difficulty Target Fitting Policy
- **Context**: Difficulty goals require both fast estimates and deeper simulation.
- **Decision**: Use immediate analytical metrics plus optional deterministic bot simulation, then bounded deterministic tuning toward explicit target bands.
- **Rationale**: Balances responsiveness with accuracy and replay consistency.
- **Status**: Accepted.


## 2026-03-04 — Physics Scope Lock (Overlap-Only)
- **Context**: Bullet-hell physics needs throughput and deterministic predictability over realism.
- **Decision**: Baseline physics is overlap-based (no rigidbody solver/impulse stack/continuous-by-default pipeline).
- **Rationale**: Minimizes complexity and preserves deterministic performance at high bullet counts.
- **Status**: Accepted.

## 2026-03-04 — Uniform Grid Broadphase Standard
- **Context**: Candidate structures include uniform grid, spatial hash, quadtree.
- **Decision**: Use uniform grid (or deterministic hash-grid variant) as baseline broadphase, with optional multi-grid extension for mixed collider scales.
- **Rationale**: Best fit for dense bounded bullet fields and allocation-free deterministic implementation.
- **Status**: Accepted.

## 2026-03-04 — Fixed-Capacity Buckets and Overflow Policy
- **Context**: Unbounded per-cell lists violate allocation/predictability constraints.
- **Decision**: Use fixed-capacity cell buckets plus deterministic overflow handling and diagnostics.
- **Rationale**: Enforces no-allocation sim loop while preserving deterministic fallback behavior.
- **Status**: Accepted.

## 2026-03-04 — Collision Event Precedence Contract
- **Context**: Simultaneous shield/hit/graze interactions can produce ambiguous outcomes.
- **Decision**: Enforce explicit precedence (Shield -> PlayerHit -> Graze -> EnemyHit -> Hazard) with stable event-sort keys before resolution.
- **Rationale**: Guarantees replay-identical outcomes and reduces gameplay ambiguity.
- **Status**: Accepted.

## 2026-03-04 — CPU-First Gameplay Collision
- **Context**: GPU collision acceleration increases debug and determinism risk.
- **Decision**: Keep gameplay collision CPU-first; reserve GPU for rendering and optional analysis overlays.
- **Rationale**: Preserves deterministic debugging and tool transparency.
- **Status**: Accepted.

## 2026-03-04 — Graze Tracking Data Policy
- **Context**: Graze logic can create hidden allocation/set-tracking overhead.
- **Decision**: Graze eligibility must be stored per bullet via flags/ticks only (no dynamic sets/maps).
- **Rationale**: Keeps hot-path memory predictable and deterministic.
- **Status**: Accepted.


## 2026-03-04 — Deterministic World-Run Structure
- **Context**: Roguelite runs require procedural variation without replay drift.
- **Decision**: Run progression is deterministic by seed across four generated worlds plus final boss, with reproducible node maps and encounter selections.
- **Rationale**: Enables fair debugging, daily challenge parity, and reliable replay verification.
- **Status**: Accepted.

## 2026-03-04 — World Graph Topology Rule
- **Context**: Procedural maps can become unreadable or unbalanced without structural guardrails.
- **Decision**: World layouts use constrained DAG generation with mandatory node-distribution rules (elite + mutation presence, mid-boss before boss, balanced spacing).
- **Rationale**: Preserves strategic pathing and progression quality while retaining variation.
- **Status**: Accepted.

## 2026-03-04 — Budget-Driven Encounter Synthesis
- **Context**: Encounter randomness can cause unfair spikes.
- **Decision**: Generate encounters from explicit threat budgets allocating pressure across roles, waves, density, and hazards.
- **Rationale**: Keeps combat variation within deterministic fairness/performance envelopes.
- **Status**: Accepted.

## 2026-03-04 — Event/Hazard Integration Policy
- **Context**: Non-combat systems and hazards can destabilize pacing.
- **Decision**: Events and hazards are world-aware generation layers with deterministic selection, progressive introduction, and budget coupling.
- **Rationale**: Maintains pacing coherence and prevents runaway complexity.
- **Status**: Accepted.

## 2026-03-04 — Designer Governance Requirement for World Gen
- **Context**: Fully autonomous procedural generation can erode authored intent.
- **Decision**: Require explicit designer controls for weights, budget ranges, pacing constraints, and lock/override capabilities.
- **Rationale**: Ensures procedural outputs remain art-directed and tunable.
- **Status**: Accepted.


## 2026-03-04 — Product Scope Boundary (1.0)
- **Context**: Broad engine ambitions can dilute delivery and destabilize milestones.
- **Decision**: 1.0 scope centers on deterministic high-throughput 2D bullet-hell runtime, tools, and content pipeline; general-purpose 3D physics and open-world streaming remain post-1.0.
- **Rationale**: Preserves focus on core market value and execution reliability.
- **Status**: Accepted.

## 2026-03-04 — Tooling Co-Evolution Mandate
- **Context**: Late tooling development causes unusable runtime systems and slow iteration.
- **Decision**: Tooling must co-evolve with runtime from early phases, with preview/debug/profiler capabilities introduced alongside core systems.
- **Rationale**: Ensures workflow validation and accelerates content iteration.
- **Status**: Accepted.

## 2026-03-04 — Milestone-Gated Delivery Policy
- **Context**: Large roadmap requires objective readiness checks.
- **Decision**: Enforce milestone gates from architecture lock through release candidate, including two mandatory vertical slices (10k deterministic stress and full tool-authored boss fight).
- **Rationale**: Reduces schedule risk and prevents premature scope expansion.
- **Status**: Accepted.

## 2026-03-04 — CI Replay Verification Requirement
- **Context**: Determinism regressions are high risk and hard to detect manually.
- **Decision**: Replay hash verification and divergence diagnostics are mandatory CI checks for curated replay sets.
- **Rationale**: Provides continuous determinism guarantees during development.
- **Status**: Accepted.


## 2026-03-04 — Editor Layout Canonical Model
- **Context**: Tool sprawl and inconsistent paneling hurt authoring speed.
- **Decision**: Standardize a dockable five-region editor model (top toolbar, left content/outline, center document tabs, right inspector/params/docs, bottom diagnostics).
- **Rationale**: Provides predictable workflows and lowers cognitive overhead across tools.
- **Status**: Accepted.

## 2026-03-04 — Preview-Centric Authoring Principle
- **Context**: Bullet-hell content iteration depends on immediate sim feedback.
- **Decision**: Keep preview controls (play/pause/step/scrub, difficulty, bot, seed lock/reset, overlays, determinism badge) globally accessible in primary authoring workflows.
- **Rationale**: Shortens design-debug loops and improves determinism visibility.
- **Status**: Accepted.

## 2026-03-04 — Explainability UX Requirement
- **Context**: Complex generated/authored patterns are hard to tune without provenance.
- **Decision**: Require click-to-explain bullet provenance, node annotations, contextual docs, and profiler-to-asset deep links across pattern/projectile/boss/wave tools.
- **Rationale**: Increases trust and tuning velocity while reducing debugging ambiguity.
- **Status**: Accepted.

## 2026-03-04 — Non-Intrusive Validation UX Policy
- **Context**: Aggressive warning UX can interrupt authoring flow.
- **Decision**: Validation issues appear as subtle badges/panels with severity, explanation, and quick-fix navigation rather than blocking modal interruptions.
- **Rationale**: Preserves flow while still enforcing quality/determinism constraints.
- **Status**: Accepted.


## 2026-03-04 — Phase 3 Runtime Service Baseline
- **Context**: Phase 3 requires foundational runtime services and headless simulation execution.
- **Decision**: Implement baseline runtime modules (`logging`, `config`, `timing`, `job_system`, `deterministic_rng`, `memory`, `runtime`) and wire headless mode into `EngineDemo`.
- **Rationale**: Establishes deterministic core loop and testable runtime primitives before higher-level gameplay systems.
- **Status**: Implemented.

## 2026-03-04 — Testing Baseline for Phase 3
- **Context**: User requested unit tests for timing and configuration.
- **Decision**: Add `timing_tests` and `config_tests` as CTest targets, and include headless simulation command validation in build docs.
- **Rationale**: Provides immediate regression coverage for deterministic timing and runtime configuration parsing.
- **Status**: Implemented.

## 2026-03-05 — Runtime Decomposition (Phase 1)
- **Decision**: Decomposed runtime responsibilities into `InputSystem`, `RenderPipeline`, and `GameplaySession`.
- **Rationale**: Reduce god-object coupling and establish clearer ownership boundaries while retaining deterministic fixed-step orchestration.
- **Status**: Accepted.

## 2026-03-05 — SimSnapshot Contract
- **Decision**: Introduced `SimSnapshot` as a simulation-to-render read contract consumed by `RenderPipeline`.
- **Rationale**: Prevent render-side back-channel mutation and make frame composition boundaries explicit.
- **Status**: Accepted.

## 2026-03-05 — Collision Pipeline Stage Split (Phase 2)
- **Decision**: Split projectile collision pipeline into `updateMotion()`, `buildGrid()`, and `resolveCollisions()` stages.
- **Rationale**: Separate deterministic motion integration from broadphase construction and narrowphase resolution to support multi-target collision queries.
- **Status**: Accepted.

## 2026-03-05 — Deterministic Collision Event Ordering
- **Decision**: Sort `CollisionEvent` output by `(targetId, bulletIndex)` before processing.
- **Rationale**: Enforces deterministic event ordering independent of grid bucket traversal order.
- **Status**: Accepted.

## 2026-03-05 — Grid-query Collision Replaces Inline Single-target Checks
- **Decision**: Use overlapping-grid-cell queries against `CollisionTarget` AABBs in `resolveCollisions()` instead of inline player-only distance checks inside projectile update.
- **Rationale**: Enables multi-target collision support with existing spatial partitioning data.
- **Status**: Accepted.

## 2026-03-05 — Golden Replay Hash Note
- **Decision**: No golden replay hash updates were applied in this phase.
- **Rationale**: Collision ordering was made deterministic and validated in targeted tests; golden replay target was not modified here.
- **Status**: Recorded.

## 2026-03-05 — FP Determinism Hardened
- **Decision**: Added deterministic floating-point compilation policy (`/fp:strict` on MSVC, `-ffp-contract=off -fno-fast-math` on GCC/Clang) for `engine_core`, introduced `engine::dmath` wrappers, and added cross-config determinism test coverage.
- **Rationale**: Centralizes simulation transcendental math through a single choke-point and reduces build-configuration drift risks for replay/hash determinism.
- **Status**: Accepted.

## 2026-03-05 — GpuBulletSystem O(1) Slot Management
- **Decision**: Replaced O(N) linear-scan slot allocation/counting in `GpuBulletSystem` with free-list allocation and cached `activeCount_`.
- **Rationale**: Stabilizes hybrid mass-bullet mode under very high emit rates and removes per-call full-buffer scans from hot paths.
- **Status**: Accepted.

## 2026-03-05 — Compact Active-List Iteration for ProjectileSystem
- **Decision**: Replaced O(capacity) loops with skip-branches by O(active) iteration using `activeIndices_` and cached `activeCount_`.
- **Rationale**: Reduces hot-path iteration cost in update/render/debug/graze/hash paths while preserving deterministic order via sort-on-removal at tick end.
- **Status**: Accepted.


## 2026-03-06 — Per-bullet palette colorization
- **Context**: Projectile rendering used allegiance hardcoded colors while palette templates already define richer projectile colors.
- **Decision**: Add visual-only `paletteIndex` projectile SoA data, route per-shot palette assignment through emission paths, and introduce `BulletPaletteTable` built from `PaletteFxTemplateRegistry`/`deriveProjectileFillFromCore`.
- **Rationale**: Preserves deterministic simulation/replay hashes while enabling authored palette-driven projectile colors.
- **Status**: Accepted.

## 2026-03-06 — Procedural Bullet Sprites
- **Context**: Bullet rendering previously relied on an external texture asset and single-shape tinting.
- **Decision**: Procedural bullet sprites are generated at render startup via SDFs from palette-derived colors, with six built-in shapes (circle, rice, star, diamond, ring, beam) and no required external bullet art.
- **Rationale**: Eliminates sprite-sheet dependency while preserving palette identity and shape readability.
- **Status**: Accepted.

## 2026-03-06 — Projectile Trail System
- **Context**: Fast-moving projectile archetypes benefit from motion readability, while deterministic simulation state must remain unchanged.
- **Decision**: Add an opt-in visual-only projectile trail system using a fixed per-bullet ring buffer of 4 past positions and faded afterimage rendering before the main sprite.
- **Rationale**: Improves legibility at low memory overhead (~32 bytes per bullet) without affecting replay hashes.
- **Status**: Accepted.

## 2026-03-06 — Impact particles visual-only boundary
- **Context**: Projectile despawn/hit feedback needs readability without affecting deterministic simulation.
- **Decision**: Add `ParticleFxSystem` as a render-layer-only subsystem fed by projectile despawn events, updated on frame delta and excluded from collision/state hash/replay determinism.
- **Rationale**: Preserves simulation determinism while improving visual clarity for bullet hit/expire events.
- **Status**: Accepted.

## 2026-03-07 — Gradient animation phase-wave mapping
- **Context**: Bullet palettes include gradient + animation settings but runtime projectile rendering only used static palette colors.
- **Decision**: Add `GradientAnimator` with precomputed LUT sampling and per-bullet phase offset (`instanceIndex * perInstanceOffset`) applied at render time, then route animated palettes through this sampler in projectile rendering.
- **Rationale**: Produces the signature rotating wave look across emitted rings while keeping simulation deterministic and unchanged.
- **Status**: Accepted.

## 2026-03-07 — Procedural level tiles: cellular automata + zone-typed rules, seed-deterministic, 256px tileable
- **Context**: Stage/zone transitions needed stronger visual identity without authored texture dependencies.
- **Decision**: Add `LevelTileGenerator` to create deterministic, tileable 256x256 background textures at zone transitions using wrapped cellular automata/value-noise and zone-type-to-rule mapping.
- **Rationale**: Preserves deterministic replay behavior while giving each stage/zone distinct palette-driven presentation from run seed alone.
- **Status**: Accepted.

## 2026-03-07 — OpenGL 3.3 Core hybrid context alongside SDL_Renderer
- **Context**: SDL_Renderer fixed-function path cannot run custom bullet shaders.
- **Decision**: Add an OpenGL 3.3 Core context plus GLAD loader at render initialization while retaining SDL_Renderer for ImGui + debug/UI drawing.
- **Rationale**: Enables custom shader pipeline and generated sprite-atlas textures with graceful fallback to SDL_Renderer-only when GL context/loader init fails.
- **Status**: Accepted.

## 2026-03-07 — OpenGL bullet renderer: single draw call, grayscale SDF + palette ramp shader
- **Context**: CPU deterministic projectile simulation was rendering bullets through per-sprite `SpriteBatch` geometry, increasing draw overhead at high bullet counts.
- **Decision**: Add `GlBulletRenderer` that rebuilds a preallocated dynamic CPU->GPU vertex/index buffer each frame from projectile SoA data and renders all bullets in one `glDrawElements` call using grayscale atlas + palette ramp shading.
- **Rationale**: Preserves deterministic CPU authority while achieving GPU-efficient batching for stress targets (10k bullets) and keeping SDL sprite rendering as fallback when OpenGL is unavailable.
## 2026-03-07 — Palette ramp texture: 64×64 GL_TEXTURE_2D, Band3 and GradientRamp modes, per-row hot-reload
- **Context**: Bullet shaders need stable palette sampling with both authored gradients and legacy 3-band palette behavior.
- **Decision**: Add `PaletteRampTexture` that builds a 64px-wide RGBA ramp per palette row from the palette template registry, supporting Band3 and GradientRamp generation plus `glTexSubImage2D` single-row hot-reload.
- **Rationale**: Keeps palette lookup deterministic, tiny in memory footprint (~16KB), and fast to patch when JSON palette definitions change.
- **Status**: Accepted.

## 2026-03-07 — Real post-processing shaders: Kawase bloom, vignette, tone mapping, chromatic aberration, film grain, scanlines
- **Context**: Existing post-FX path used fake SDL overlays, so editor controls could not drive physically plausible bloom, tone mapping, or camera-style post passes.
- **Decision**: Replace fake post overlays with shader-driven passes in `RendererModernPipeline`: half-resolution Kawase bloom (threshold + 4 blur iterations + additive composite), vignette pass, and final composite pass including tone mapping, exposure/contrast/saturation, chromatic aberration, film grain, and scanlines.
- **Rationale**: Preserves presentation-only coupling while upgrading visual quality and making PostFx controls map directly to shader uniforms and palette FX presets.
- **Status**: Accepted.

## 2026-03-07 — GameplaySession responsibility split into runtime state partitions
- **Context**: `GameplaySession` was carrying simulation orchestration, player combat state, progression UI flow, presentation effects, debug/tool wiring, and encounter collision scratch buffers in one undifferentiated object.
- **Decision**: Introduce explicit state partitions (`SessionSimulationState`, `PlayerCombatState`, `ProgressionState`, `PresentationState`, `DebugToolState`, `EncounterRuntimeState`) as owned sub-objects inside `GameplaySession`, and route runtime/render integration through those boundaries.
- **Rationale**: Preserves deterministic behavior while reducing responsibility overlap and making ownership and integration points testable.
- **Status**: Accepted.


## 2026-03-07 — Modularized ControlCenter tool panels
**Context:** `src/engine/editor_tools.cpp` had grown into a high-coupling editor monolith where menu shelling, authoring state initialization, validation, and all panel UI were interleaved in one function.

**Decision:** Split editor rendering into dedicated panel methods (`drawWorkspaceShell`, `drawPatternGraphEditorPanel`, `drawEncounterWaveEditorPanel`, `drawPaletteFxEditorPanel`, `drawValidationDiagnosticsPanel`, etc.) and extracted shared editor service helpers (`buildEncounterAsset`, panel state seeding helpers).

**Consequences:**
- Reduced edit blast radius for tool changes.
- Clearer ownership boundaries between panel UI and shared editor services.
- Preserved user-visible behavior while preparing plugin and panel expansion.
- Runtime/editor split is cleaner because panel methods consume immutable runtime snapshots and mutate only editor-scoped state.

## 2026-03-07 — Decision: integrate source-art import into ContentPacker

- Implemented source-art import as a first-class stage in `ContentPacker` rather than standalone scripts.
- Chosen data model: manifest-driven `art-import` JSON documents with explicit per-asset import settings.
- Added import fingerprinting and previous-pack comparison (`--previous-pack`) to support deterministic reimport/dependency invalidation.
- Atlas integration is represented as emitted `atlasBuild` plans keyed by `(atlasGroup, colorWorkflow)` so runtime/editor can consume build grouping deterministically.
- Grayscale and monochrome workflows are validated/imported directly through `colorWorkflow` and represented in pack metadata to support palette/shader workflows.


## 2026-03-07 — Data-driven animation/variant grouping in art import
- **Context**: Source-art import had basic registry/fingerprints but no production-ready animation clip or variant grouping output.
- **Decision**: Introduce explicit settings for animation/variant identity plus optional regex-based filename extraction so naming conventions remain configurable rather than hardcoded.
- **Decision**: Emit `animationBuild` (set/state/direction/fps/frame GUID list) and `variantBuild` (group/options/weights/palette template) into packs as canonical runtime/editor metadata.
- **Decision**: Reject malformed/ambiguous groups (invalid identifiers, duplicate variant names, duplicate frame indices, inconsistent clip FPS).
- **Consequences**:
  - Authors can choose explicit metadata or configured naming conventions.
  - Runtime/content packs now expose deterministic animation and variant group plans for procedural/themed selection.
  - Grayscale/palette workflow remains compatible via per-variant `paletteTemplate` annotation.
