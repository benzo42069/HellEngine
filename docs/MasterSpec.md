# MasterSpec

## 1. Scope and Authority
This is the merged authoritative specification for a standalone Windows bullet-hell engine. All provided design documents are source-of-truth and must be integrated additively.

## 2. Non-Negotiable Constraints
- Target: Windows 10/11 x64.
- Keep repository buildable after every phase.
- Additive evolution only; never rename created modules.
- Determinism, replay parity, and high-throughput projectile simulation are first-class constraints.

## 3. Layered Engine Architecture
### 3.1 Core Runtime (Platform + Kernel)
Responsibilities:
- Platform abstraction (window, input, audio, threads, files).
- Job system + synchronization primitives.
- Memory subsystem (arenas, pools, allocators, telemetry).
- Time/clocking and frame pacing.
- Logging, asserts, crash capture.
- Deterministic utilities (fixed RNG, fixed math helpers).

Rule: no gameplay dependencies.

### 3.2 Simulation Layer (Deterministic Sim)
Responsibilities:
- Fixed-step world simulation.
- Deterministic entity scheduling and updates.
- Specialized Bullet World.
- Collision and queries.
- Deterministic event queue.
- Snapshot/hash/checkpoint generation.

Rule: pure state output only (no render/platform calls).

### 3.3 Gameplay Systems Layer
Responsibilities:
- Player deterministic intent processing.
- Enemy AI (HFSM default; optional BT).
- Boss phase/timeline orchestration.
- Upgrades/mutations/scoring/economy hooks.

Rule: stable IDs and data-driven references.

### 3.4 Pattern Layer (Authoring IR + Runtime VM)
Responsibilities:
- Graph/timeline/DSL authoring inputs.
- Compilation to unified Pattern IR + bytecode.
- Deterministic VM execution and binding.

Rule: authoring flexibility; runtime predictability.

### 3.5 Rendering Layer
Responsibilities:
- Instanced/batched sprite/mesh rendering.
- Materials/atlases.
- Debug overlays.

Rule: render consumes sim snapshots; no sim feedback.

### 3.6 Asset/Content Pipeline
Responsibilities:
- Import authored content.
- Compile immutable runtime packs.
- Validation/lint/schema versioning/migrations.
- Optional mod pack production.

### 3.7 Editor/Tooling Layer
Responsibilities:
- Graph/timeline editors and inspectors.
- In-editor deterministic preview.
- Replay debugger + profiler.
- Hot reload with safe swap boundaries.

Rule: tooling integrates through stable Editor API.

## 4. Deterministic Simulation Contract

### 4.x Floating-Point Determinism Policy (Phase 3)
- `engine_core` is compiled with strict FP determinism flags:
  - MSVC: `/fp:strict`
  - GCC/Clang: `-ffp-contract=off -fno-fast-math`
- Simulation trig/root math routes through `engine::dmath` wrappers (`deterministic_math.h`) instead of direct `std::sin/cos/atan2/sqrt` calls in hot simulation code.
- Wrapper indirection exists as a deterministic choke-point for future fixed-precision or polynomial replacement without broad call-site churn.
- A cross-config determinism test (`determinism_cross_config_test`) runs 300 deterministic ticks and verifies a hard-coded hash to detect configuration drift.

### 4.1 Time and Tick Model
- 2D simulation with fixed timestep `Δt` (typical `1/60` or `1/120`).
- Integer tick index `n`; simulation time is `t = n·Δt`.
- Math mode:
  - Preferred fixed-point for strongest cross-platform parity.
  - Deterministic-float mode allowed with strict platform/toolchain restrictions and validation.
- Accumulator stepping: while accumulator >= Δt, run `SimTick()`.

### 4.2 Canonical Tick Order
1. Input sampling to deterministic command buffer.
2. PreSim flush (spawn/despawn/upgrades/difficulty scalars).
3. AI/scheduling (enemy AI, boss transitions, pattern VM).
4. Motion integration.
5. Collision broadphase+narrowphase and deterministic collision queue.
6. PostSim apply damage/deaths/score/events.
7. Emit snapshot + state hash.

### 4.3 Determinism Guarantees
- No variable-delta sim.
- Stable iteration/sort order.
- Deterministic RNG stream discipline.
- Deterministic math policy enforcement.
- Per-tick state hashing for replay/desync/debug.

## 5. Bullet Motion Mathematics
### 5.1 Core Kinematics
- Position `p(t)=(x(t),y(t))`, velocity `v(t)` in 2D.
- Linear motion:
  - Continuous: `p(t)=p0 + v·t`
  - Discrete: `p[n+1]=p[n] + v[n]·Δt`
- Constant acceleration:
  - `v[n+1]=v[n] + a·Δt`
  - `p[n+1]=p[n] + v[n+1]·Δt` (semi-implicit Euler).

### 5.2 Angular/Curvilinear Motion
- Heading form `v = s·(cos θ, sin θ)`.
- Angular velocity update: `θ[n+1]=θ[n] + ω·Δt`.
- Position: `p[n+1]=p[n] + s·(cos θ[n+1], sin θ[n+1])·Δt`.

### 5.3 Sinusoidal Models
- Lateral offset model around forward axis:
  - `p_base(t)=p0 + f·s·t`
  - `offset(t)=u·A·sin(2πft + φ)`
  - `p(t)=p_base(t)+offset(t)`
- Heading perturbation model:
  - `θ(t)=θ0 + A·sin(2πft + φ)`.

### 5.4 Orbit Motion
- Around center `c(t)` with radius `R` and phase `α`:
  - `p(t)=c(t)+R·(cos α, sin α)`
  - `α[n+1]=α[n] + ω·Δt`
- Supports drifting centers (`c(t)=c0 + vc·t`).

### 5.5 Homing with Turn Limits
- Desired heading from target direction.
- Per-tick angular clamp:
  - `Δθ = clamp(angleDiff(θd,θ), -ωmax·Δt, +ωmax·Δt)`.
- Update heading, then velocity from heading.

### 5.6 Bézier Paths
- Quadratic and cubic Bézier path support for authored trajectories.
- Lifetime-normalized parameter `u = clamp(age/L, 0..1)`.
- Position set by `B(u)` each tick; derivative `B'(u)` used when velocity is required.

### 5.7 Composable Motion Stack (Canonical Order)
1. Steering/turning (homing/turn modules).
2. Velocity integration (linear/accel/angular).
3. Parametric offsets (sin/orbit/path overlays).
4. Constraints (speed clamps, boundaries).
5. Final position commit.

## 6. Emission Pattern Algorithms
### 6.1 Canonical Emission Event
Each spawn event captures:
- Spawn tick.
- Origin.
- Angle/direction.
- Speed.
- Archetype and init params.
- RNG substream reference.

Emitter parameters include `N`, `θ0`, spread `Δ`, rotation rate `Ω`, and interval `I`.

### 6.2 Standard Emitters
- Radial burst: `θi = θ0 + 2π·i/N`.
- Arc burst: `θi = θ0 - Δ/2 + Δ·i/(N-1)`.
- Rotating rings: `θ0(n)=θbase + Ω·n·Δt`; spawn when `n mod I = 0`.
- Aimed shots: `θ0 = atan2(player.y-origin.y, player.x-origin.x)`.
- Shotgun variants: even or center-weighted spreads.
- Wave emitters: time-modulated angle/rate.
- Lattice/grid emitters for curtains and synchronized fields.

### 6.3 Spiral Families
- Burst spiral via rotating emitter (`θ0(k)=θstart + k·Δθ`).
- Parametric spiral loci (`r=a+b·t`, `θ=θstart+ω·t`) with spawn-on-curve or on-rail behavior.
- Spiral acceleration supported via cumulative/analytic integration of `Ω(t)`.

## 7. Parametric Pattern System
### 7.1 Exposed Parameters
Typical designer parameters: `N, Δ, Ω, I, S, A, L, a, ωturn, phase` and variant-specific controls.

### 7.2 Deterministic Curve/Binding Model
- Parameters can be time/phase functions sampled deterministically each tick.
- Deterministic bindings permitted from:
  - Difficulty tier.
  - Boss HP%/phase.
  - Player distance (if sampled deterministically).
  - Deterministic noise fields.

### 7.3 Useful Derived Metrics
- Burst throughput: bullets/sec ≈ `N / (I·Δt)`.
- Ring spacing at radius `r`: ≈ `(2πr)/N`.
- Arc spacing: ≈ `Δ/(N-1)`.

## 8. Procedural Pattern Modifiers
- Rotation and oscillation modifiers can alter angle/origin over time.
- Split modifiers (timer/hit/death) enqueue deterministic child-spawn commands at tick boundaries.
- Delayed telegraph spawning separates warning marker from delayed spawn using stored seeds/params.
- Layer phase offsets support interference/weave patterns.
- Constraint-based modifier (advanced) solves bounded params (`Δ,N,Ω` etc.) via deterministic iterations for fairness goals.

## 9. Pattern Composition
- Nested patterns (emitters spawn emitters/subpatterns).
- Seed derivation for child instances via deterministic hashing.
- Layered concurrent emitters with shared phase clock and independent params.
- Attack timelines as clip sequences with deterministic trigger order/tie-breaking by stable IDs.

## 10. Targeting Algorithms
- Direct aiming via `atan2`.
- Predictive lead by solving intercept quadratic for smallest positive time.
- Homing with bounded angular velocity and optional proportional steering.
- Difficulty-scaled adaptive tracking blends direct/predictive/lagged strategies deterministically.

## 11. Deterministic RNG Discipline
- Pattern-instance stream seeds derive from run seed + asset/instance IDs.
- Emitter and burst substreams derived from parent stream IDs.
- Prefer indexed randomness (hash-derived values) to avoid call-order divergence.
- Replay stores run seed + input stream + content hash/version (+ optional periodic hashes).

## 12. Entity and Bullet Runtime Model
### 12.1 Hybrid ECS + Bullet World
- ECS handles general entities.
- Bullet World handles projectile-heavy workloads.

### 12.2 Bullet Data Structures
- Immutable `BulletArchetype` runtime descriptors.
- SoA bullet pools with fixed-capacity arrays.
- Preallocated command buffers and free-list reuse.
- No per-bullet dynamic allocations in hot path.

### 12.3 Collision and Spatial Optimization
- Uniform grid/spatial hash broadphase.
- Narrowphase checks for circle/capsule/OBB; stable deterministic resolution ordering.
- Collision layers and cheap deterministic culling rules (bounds/lifetime).
- Batch updates by archetype/program/layer for locality and branch reduction.

## 13. Data-Driven Content and Asset Pipeline
- Author-time assets (graph/timeline/configs) compile to runtime packs.
- Runtime pack includes asset DB, string table, dependencies, schema version, content hash.
- Hot reload recompiles changed assets and swaps at safe deterministic points.
- Optional JSON export for modding/diff workflows.

## 14. Memory and Performance Contract
- No GC-dependent behavior in simulation runtime.
- Frame arenas, pools, chunk allocators, ring buffers.
- Telemetry: high-water marks, exhaustion, per-tick allocation violations.
- Pre-size pools by budgets; growth allowed in editor/loading only.
- Degradation strategy preserves sim correctness first (reduce visuals/effects before sim fidelity).

## 15. Rendering Strategy
- OpenGL bullet renderer (`gl_bullet_renderer`) is the primary high-throughput path for gameplay-authoritative projectiles.
- CPU collision/determinism path (`CpuCollisionDeterministic`) remains replay-authoritative.
- CPU mass bullet render path (`CpuMassBulletRenderSystem`) is a presentation-oriented mass-quad path, not GPU compute simulation.
- Render remains presentation-only.

### 15.x CPU Mass Bullet Render Runtime Note (Phase 5)
- `CpuMassBulletRenderSystem` replaces ambiguous naming previously implying GPU simulation.
- Runtime uses O(1) free-list emission and compact active-slot iteration for update/render traversal.
- Prepared quad count is tracked explicitly (`preparedQuadCount`) for profiling clarity.
- Mode naming is explicit: `CpuCollisionDeterministic` vs `CpuMassRender`.

## 16. Replay, Debugging, and Introspection
- Replay record/playback uses authoritative per-tick input stream and deterministic settings.
- Determinism checks compare recorded/current state hashes and capture first divergence tick.
- Debug Stream ring buffer keyed by tick + IDs captures spawn/kill/collision/state transitions/markers.
- Tools include bullet provenance, path history, AI trace, collision ordering/grid overlays, and timeline scrubbing.

## 17. Boss Pattern Design System
- Phases define enter conditions, active timelines, exit conditions, and transition rules.
- HP/timers sampled at tick boundaries (integer tick counters, no variable wall time).
- Escalation via controlled ramps (`N`, `I`, `S`, targeting sophistication, layered modifiers).
- Transitions require readability controls (telegraph windows, optional cancel/conversion behavior).
- Roguelite variation via deterministic pattern genomes/mutations with fairness constraints.

## 18. Extensibility, Modding, and Safety
- Plugin extension points for runtime systems, editor panels, importers, and pattern compiler modules.
- Mod packs can add/override data content with dependency/schema checks.
- Safety rules:
  - No wall-clock access in sim.
  - Deterministic RNG/tick-phase declarations.
  - No per-tick dynamic allocations in shipping builds.
  - Versioned schemas + migrations.

## 19. Commercial Tooling and UX Requirements
- Pattern/projectile/enemy/boss/wave editors with deterministic preview.
- Replay debugger, fairness tools, and bullet-domain profiler.
- Validation/linting for telegraphing, density, lane guarantees, and dependencies.
- Dockable pro editor UI and built-in docs/templates/benchmarks.
- CI gates for determinism/performance/fairness.
- Optional analytics must be explicit opt-in and separated from core sim.


## 20. Pattern Graph System Specification
### 20.1 Pattern Graph Purpose and Model
- Pattern Graph is a domain-specific visual scripting system specialized for bullet-hell authoring.
- It combines dual-flow semantics:
  - Execution flow: deterministic sequencing and scheduling of actions.
  - Data flow: typed parameter/value computation for emissions and modifiers.
- Authoring goals: fast iteration, deterministic replays, compile-time safety, and high runtime performance.
- Runtime model: all graphs compile to deterministic Pattern IR/bytecode executed by Pattern VM.

### 20.2 Graph Documents and Entry Context
Supported graph document types:
- Pattern Graph (primary runtime authoring document).
- Timeline Graph (embedded or referenced scheduling/keyframe tracks).
- Macro/Subgraph documents (reusable typed components).

Entry nodes include:
- `OnPatternStart`
- `OnPhaseEnter(PhaseName)`
- `OnMarker(MarkerName)`
- `OnEvent(EventType)`

Entry context pins include owner/target refs, difficulty tier, seed handle, tick index, and local phase time.

### 20.3 Exec/Data Flow and Deterministic Merge
- Execution links define deterministic control flow; parallel branches are compiled with stable merge rules (node GUID deterministic sort).
- Data links are strongly typed (`Scalar`, `Int`, `Bool`, `Angle`, `Vector2`, `Curve`, `RNGStream`, refs).
- Type conversion is explicit via conversion nodes only.

### 20.4 Pattern Outputs and Emission Semantics
Pattern graph outputs are deterministic side effects:
- Spawn commands (`SpawnBullet`, `SpawnLaser`, `SpawnHazard`).
- Deterministic events/markers.
- Parameter/state set requests.
- Non-sim FX requests.

Emission nodes execute only when Exec input fires, compute deterministic bullet batches, and write to preallocated spawn command buffers.

### 20.5 Core Node Families
#### Emission Nodes
Common pins: `ExecIn/Out`, `Origin`, `Aim`, `Projectile`, `Speed`, `Count`, optional `Seed`, `Tags`.
Required primitives:
- Radial Burst.
- Arc Spread.
- Spiral Emitter.
- Aimed Shot.
- Wave Emitter.
- Grid/Lattice Emitter.

#### Timing Nodes
- `Wait/Delay` (tick-based with explicit seconds->tick rounding policy).
- `IntervalLoop` (bounded/unbounded).
- `Timeline`.
- `PhaseTrigger`.

#### Transform/Targeting/Utility Nodes
- Transform: rotate, position offset, count scale (deterministic rounding), speed modify.
- Targeting: direct aim, predictive aim with fallback policy, deterministic random direction, nearest-target with stable tie-break.
- Utility: deterministic random node (indexed mode), parameter expose node, difficulty scaling, seed split.

### 20.6 Modifier Stack Contract
- Emission nodes expose `ModifiersIn` stack and optional per-bullet modifier pin.
- Modifier stack supports reorder/toggle with deterministic compile order.
- Required modifiers:
  - Spiral Rotation.
  - Oscillating Angle.
  - Split Bullet.
  - Delayed Spawn.
  - Speed Ramp.
- Canonical modifier execution order:
  1. Parameter modifiers.
  2. Angle modifiers.
  3. Position modifiers.
  4. Post-spawn behavior bindings.

### 20.7 Timeline Integration
- Embedded timeline strip supports playhead/scrub/zoom and tracks for exec clips, parameter curves, and markers.
- Clip types: burst, loop, ramp, gate.
- Curve/keyframe types: linear, smooth, stepped.
- Deterministic sampling via fixed-step lookup or deterministic interpolation.
- Timeline emits marker/phase start/end events with fixed-order trigger resolution.

### 20.8 Composition and Reuse
- Subpattern node instantiates child pattern instances with origin/aim/param overrides/seed.
- Library system requirements: tags, search, favorites, thumbnails, metadata, dependency view.
- Macro graphs use typed I/O and compile-time expansion (no runtime overhead).
- Layering: named subgraph layers compile to deterministic parallel streams merged by stable ordering.

### 20.9 Parameter and Inspector Model
- Parameter types: scalar, int, bool, angle, vector2, curve ref, enum.
- Metadata: name/description/range/step/grouping/per-difficulty overrides/runtime lock.
- Inspector panels must provide:
  - Exposed parameter editing.
  - Live resolved values during preview.
  - Validation warnings for budget/range risks.
- Binding sources: constants, curves, difficulty scalers, deterministic context vars, deterministic noise.

### 20.10 Visual Debugging and Preview UX
- Node execution highlighting, loop iteration counters, wait remaining time, timeline firing indicators.
- Bullet provenance drill-down: source node, applied modifier stack, resolved parameters at spawn tick.
- Playback controls: play/pause/step/scrub, slow-mo, seed reset/lock, bot profile selection.
- Heatmaps: parameter influence and spatial bullet density; optional safe-lane overlays.
- Breakpoints and watches on node/pin execution with short trace window.

### 20.11 Graph Compilation Pipeline
Stages:
1. Validation (types/cycles/forbidden nondeterminism/budget estimates).
2. Lowering (exec to schedule blocks; timeline to WAIT/LOOP + curves; data to constants/registers).
3. Optimization (constant fold, dead-node elimination, batch grouping).
4. Codegen (Pattern IR + tables + debug mapping node GUID to IR ranges).

Runtime requirements:
- IR cached by content hash.
- Shared immutable IR across instances.
- Small per-instance state (`PC`, locals, RNG counters, time cursor).
- No runtime dynamic allocations in VM hot path.
- If multithreaded, use thread-local spawn buffers merged by stable keys.

### 20.12 Designer Workflow Requirements
- New pattern flow: create graph, wire loop+emitters+modifiers, expose params, save with thumbnail.
- Instant preview: difficulty selection, overlays, tick stepping, provenance click-through, hot reload at safe boundaries.
- Reuse flow: drag from library into boss phases, override per phase, extract/reuse macros with versioned propagation.

### 20.13 Advanced/Innovative Graph Features
- AI-assisted constrained pattern generation with multi-candidate outputs and cost estimates.
- Deterministic mutation tools for roguelite variants.
- Auto-difficulty scaling over designated parameters with generated reports.
- Pre-sim density estimation and spike prediction overlays.
- Dodge-path/fairness simulation overlays linked to timeline timestamps and source nodes.
- 2D parameter surfaces for coherent multi-parameter tuning.


## 21. Procedural Boss Attack Generation System (PBAGS)
### 21.1 Definition and Goals
- PBAGS generates boss phases automatically from vetted modular pattern components under explicit constraints.
- Generated artifacts include:
  - Phase timelines (attack/recovery/transition sequencing).
  - Pattern instances (module selections + parameters + modifiers).
  - Transition logic (HP/time/event/performance-gated variants).
  - Deterministic variants across difficulty tiers and roguelite seeds.
- Outputs are editable assets: designers can lock, regenerate subsets, and hand-tune.

### 21.2 High-Level Generation Flow
1. Designer selects style profile, difficulty bounds, module pool, and constraints.
2. Generator creates phase plan (count, durations, escalation curve).
3. Each phase is synthesized into timeline segments from weighted attack pools.
4. Fast analyzers and optional deterministic bot simulation validate output.
5. System emits Boss Phase Asset + explainable generation report.

### 21.3 Runtime-Facing Data Model
Required engine representation:
- `BossAttackPlan`
  - `seed`, `difficultyTier`, `styleProfileId`
  - `phases[]`
- `PhasePlan`
  - `phaseId`, `hpRange`, `timeCap`, `escalationLevel`
  - `segments[]`, `transitionRules[]`
- `AttackSegment`
  - `segmentType` (`Primary`, `Secondary`, `Transition`, `Recovery`)
  - `startTick`, `durationTicks`
  - `patternInstances[]` (`moduleId`, params, modifiers, anchors)
  - `constraintsSnapshot`

### 21.4 Phase/Segment Architecture
- Boss fight is composed of phases.
- Each phase has:
  - Entry (telegraph/reposition/rule-change signal).
  - Active timeline loop (primary + optional secondary/hazard layering).
  - Exit (cleanup/cancel/convert/reposition).
- Attack segments contain:
  - Duration window.
  - One primary attack.
  - Optional secondary pressure layer.
  - Recovery/breather windows.

### 21.5 Deterministic Timing Structures
- Fixed-tick timelines only.
- Segment durations align to a beat grid (e.g., 0.25s quantization).
- Pulse windows define burst microstructure (0.2s, 1.0s cycles, etc.).
- Breather quota policy enforced: every X seconds provide at least Y seconds reduced pressure.

### 21.6 Pattern Building Blocks and Module Taxonomy
Modules are pre-authored, compile to Pattern Graph/IR templates, and expose parameters/modifier slots.

Baseline module families:
- Radial: ring, arc, rotating ring, petal/rose.
- Spiral: constant spiral, accelerating spiral, double helix, spiral+wave.
- Targeted: direct, predictive, turn-limited homing, delayed punish targeting.
- Barrage/Curtain: line sweeps, curtain walls, lattice grids, explicit lane gaps.
- Environmental hazards: persistent zones, moving hazards, arena constraints, bullet interactions.
- Transition modules: cancel/convert, telegraphed reposition, low-density interlude.

Standard module interface:
- Inputs: origin anchors, target refs, seed stream, difficulty scalar.
- Parameters: count, speed, spread, interval, rotation, lifetime, aim variance.
- Modifier slots: oscillation, split, delay, speed ramp, phase shift.
- Outputs: density estimate, safe-lane hints, performance estimate.

### 21.7 Combination and Fairness Rules
- Primary module defines core dodge problem.
- Secondary/hazard layering is allowed only if overlap constraints pass:
  - Must not remove all escape routes.
  - Must not exceed density/speed/perf budgets.
- Hazard presence constrains legal bullet pattern combinations.

### 21.8 Selection Algorithms and Weighted Pools
Attack pools by role/intensity:
- `Primary.Low/Med/High`
- `Secondary.Support`
- `Hazards`
- `Transitions`

Each candidate includes style weights, prerequisites, fatigue metadata, and synergy tags.

Deterministic selection process:
1. Filter by constraints and prerequisites.
2. Score candidates with `style * phase * novelty * synergy * feasibility` weighting.
3. Deterministically sample using seeded stream.
4. Apply repetition cooldowns by module/tag.

### 21.9 Deterministic Parameter Synthesis
- Parameters are selected within module/style bounds.
- Phase escalation curves map to parameter scaling.
- Constraint pass enforces global budgets (spawn/alive/speed/reaction/telegraph).
- Prefer indexed randomness: `param = lerp(min,max, hash(seed,moduleId,param,segmentIndex))`.
- Clamp/adjust deterministically if constraints are violated.

### 21.10 Sequencing and Escalation
- Sequence generated from pacing templates (e.g., Primary -> Recovery -> Layered Primary+Secondary -> Transition).
- Durations chosen from bounded beat-grid-aligned sets.
- Escalation levers:
  - Add secondary layers.
  - Increase rotation/amplitude.
  - Reduce interval.
  - Introduce modifiers/hazards in later phases.
- Avoid stacking many escalation levers simultaneously except late-phase climax.

### 21.11 Difficulty Scaling Model
Difficulty is multi-dimensional:
- Density, speed, entropy, aim aggression, hazard pressure, reaction time.

Budget-driven constraints per phase:
- `B_alive_max`, `B_spawn_rate_max`, `B_collision_checks_max`.
- `S_avg_max`, `S_peak_max`.
- `MinTelegraphTicks`, `MinBreatherRatio`.

If over budget, deterministic reduction priority:
1. Reduce secondary layers.
2. Reduce density.
3. Reduce speed.
4. Reduce aim aggression.
5. Last resort: change module selection.

Optional adaptive mode:
- Runtime selects among pre-generated variants (A/B/C) using deterministic performance bands.

### 21.12 Style Profiles and Motifs
Style profile is a declarative generation config:
- Category/module weights.
- Parameter ranges/signature motifs.
- Pacing templates.
- Modifier allowances/probabilities.
- Theming hooks.

Required support for motifs (“phrases”) to preserve identity across procedural output.

### 21.13 Designer Control and Regeneration Workflow
Designer controls:
- Module allow/deny lists.
- Category/module weights.
- Parameter caps and hard constraints.
- Pacing templates.
- Mandatory signature attacks.
- Seed/variation policy.

Lock/regenerate capabilities:
- Lock segment/motif.
- Regenerate unlocked sections only.
- Regenerate parameters only while preserving module picks.

Constraint authoring includes:
- Max bullets alive/spawn rate.
- Min safe-lane frequency.
- Min telegraph duration.
- Max homing turn rate.
- Optional strict “no unavoidable hits” mode.

### 21.14 Transition Generation
Transition segments are synthesized mini-sequences:
- Bullet clear/cancel/convert.
- Repositioning.
- Rule-change telegraphing.
- Low-density reset pattern.

Trigger types:
- HP thresholds (tick boundary evaluation).
- Time caps.
- Optional deterministic performance triggers selecting precomputed variants.

### 21.15 Playtest Simulation and Validation
Deterministic bot profiles:
- Cautious, grazer, aggressive, limited-skill.

Validation metrics:
- Survival distributions.
- Hit timestamps/locations.
- Required reaction-time estimates.
- Density heatmaps and safe-region estimates.
- Spike detection and segment difficulty scores.

Feedback loop:
- Hard validation failure for violated constraints.
- Optional bounded deterministic tuning pass with fixed iteration budget.
- Output diff report describing parameter changes and reasons.

### 21.16 Generation Reports and Explainability
Each generated segment report must include:
- Selected module and weighted-selection rationale.
- Final parameters + applied constraints/clamps.
- Predicted difficulty/performance metrics.
- Bot-simulation summary vs target bands.
- Any substitutions and deterministic causes.

### 21.17 Reference Example (Normative)
Engine/docs should support end-to-end generation equivalent in structure to a 3-phase “spiral-heavy with aimed punctuation” boss profile featuring:
- Establishment phase with motifs and recovery windows.
- Mid-phase layered entropy increases and transition hazards.
- Climax phase with constrained split crescendos and fairness-preserving telegraphs.

This example is normative for capabilities (not fixed content values).


## 22. AI-Assisted Pattern Generator (In-Editor Co-Pilot)
### 22.1 Definition and Scope
- The AI-assisted generator is an editor co-pilot that outputs engine-native Pattern Graph assets (or IR templates), not opaque runtime scripts.
- It proposes candidate structures, explains decisions, supports deterministic refinement, and enforces performance/fairness constraints.
- It is explicitly **not** a black-box runtime behavior controller.

### 22.2 Primary Goals
- Speed: idea to playable pattern in minutes.
- Quality: readability, fairness, style consistency.
- Control: hard constraints and deterministic reproducibility.
- Integration: outputs normal pattern assets with preview/profiling/report metadata.

### 22.3 Hybrid Generation Philosophy
Generation pipeline combines:
- AI-assisted structure proposals (optional tier).
- Rule/template-based parameterization and constraint solving.
- Deterministic simulation/analyzers for validation and bounded tuning.

### 22.4 Input Model
Supported inputs:
- Natural-language intent prompts.
- Hard-constraint sliders/targets (alive cap, spawn-rate cap, speed bands, aim aggression, telegraph strictness, entropy, complexity).
- Style presets.
- Existing-pattern reference/remix modes.
- Optional advanced inputs (safe-lane policy, arena constraints, boss socket map).

Prompt parsing produces a structured Intent Spec containing motifs, pacing, constraints, and style tags.

### 22.5 Three-Stage Generation Pipeline
1. Intent -> Blueprint
   - Generate timeline skeleton, layer plan, and weighted module palette.
2. Blueprint -> Graph Drafts
   - Assemble graph/timeline candidates from approved template library.
   - Produce multiple deterministic candidates (typical 3–6).
3. Parameter Solve + Validation
   - Deterministic constrained sampling with indexed randomness.
   - Run fast analyzers and optional deterministic bot simulation.
   - Apply bounded deterministic tuning iterations.

### 22.6 AI Tiers and Safety Boundary
- Tier 1: deterministic rule/template generator (shipping baseline).
- Tier 2: LLM-assisted blueprint suggestions (optional).
- Tier 3: learned candidate ranking model (optional).

Safety rule:
- AI/LLM may suggest composition only.
- Final runtime behavior must be concretized via approved templates, deterministic compilation, and constraints.

### 22.7 Engine-Compatible Output Contract
Generated output includes:
- Standard Pattern Graph asset.
- Timeline tracks/markers/keyframes.
- Exposed parameters (`Count`, `Speed`, `Interval`, `RotationSpeed`, `Spread`, `TelegraphDelay`, etc.).
- Embedded generation report metadata:
  - intent summary
  - constraints used
  - rationale for nodes/layers
  - predicted metrics

### 22.8 Style Library Integration
Style presets define:
- Module weights.
- Parameter ranges.
- Rhythm templates.
- Allowed modifiers.
- Readability rules.
- Optional visual theming hints.

Styles influence blueprint shape, module probabilities, parameter initialization, constraint priorities, and motif injection.

### 22.9 Refinement Workflow
Core loop: Generate -> Edit -> Selective Regenerate.

Required operations:
- Parameter editing with grouped controls and hot-reload preview.
- Node replacement while preserving compatible inputs.
- Timeline retiming and keyframe editing.
- Modifier stack tuning (toggle/reorder/intensity).
- Lock Structure / Lock Parameters modes per segment.

Explainability requirements:
- Node annotations for generated intent.
- Side panel summarizing blueprint and constraint satisfaction.

### 22.10 Preview and Debug UX
Always-on deterministic sandbox preview:
- play/pause/step/scrub
- speed scale, difficulty selector
- seed lock/reset

Required overlays:
- density heatmaps
- flow fields
- danger spike timeline
- bot reachable-region/dodge guidance

Graph coupling:
- node execution highlights
- click bullet to show spawn provenance and resolved spawn parameters
- timeline scrubbing reflects graph highlights.

### 22.11 Difficulty Estimation Model
Fast analytical metrics:
- bullets/sec
- peak alive estimate
- avg/peak speed
- aim aggression index
- entropy index
- telegraph compliance

Spatial/temporal metrics:
- density gradients
- gap-width estimates
- reaction-time approximation

Optional slower bot metrics:
- survivability curves across seeds/variants
- normalized score by tier

Supports explicit difficulty targets and deterministic tuning toward target bands.

### 22.12 Mutation System
Mutation modes:
- Parameter perturbation.
- Structural remix.
- Motif injection.

Controls:
- mutation intensity
- constraints always enforced
- lock list for protected nodes/segments

Mutation determinism:
- seeded mutation IDs (e.g., hash over run seed + pattern ID + mutation index).

### 22.13 Pattern Library Requirements
Generated patterns are stored as standard assets with metadata:
- style/motif tags
- estimated and verified difficulty
- budget metrics
- provenance snapshot (prompt + constraints)
- versioning

Library capabilities:
- tag/numeric filters
- similarity search via fingerprints
- favorites/collections
- phase-level reuse and override workflows

### 22.14 Future Extensions (Optional)
- ML models for structure proposal/ranking/difficulty prediction.
- Tight PBAGS integration for full-boss generation.
- Deterministic runtime selection among pre-generated adaptation variants.
- Community marketplace import/export with validation.
- “Explain and Teach” onboarding mode with before/after guidance.

### 22.15 Non-Code Enforcement Requirements
- Output must remain engine-native graph assets.
- Determinism via fixed ticks, seed discipline, indexed randomness, stable IR compilation.
- Performance via template whitelist + budget-aware solving + analyzer/bot validation.
- Designer trust via explainable reports, selective regeneration, locking, and integrated debugging.


## 23. Specialized Physics and Collision System
### 23.1 Purpose and Scope
Physics responsibilities are strictly deterministic, overlap-centric, and throughput-oriented:
- Determine overlaps per tick.
- Generate deterministic interaction events (`hit`, `graze`, `shield`, `pierce`, `cancel`).
- Sustain 10,000+ projectiles with predictable frame cost and zero per-tick allocations.

### 23.2 Boundary vs Traditional Physics
This engine intentionally excludes general rigidbody simulation:
- No impulse solver.
- No friction/constraint stacks.
- No general-purpose dynamic broadphase by default.

Focus area:
- Simple kinematics + overlap tests + deterministic special interaction rules.

### 23.3 Collision Shape Set
Minimal primitive set:
- Circle (default bullets/player/hurtboxes).
- Capsule (beams/elongated hurtboxes).
- OBB (optional rare hazards/enemies).
- Segment+radius (laser represented as capsule-like primitive).

Default gameplay mapping:
- Bullet hitbox: circle.
- Player hurtbox: small circle.
- Player graze zone: larger circle.
- Enemy hurtboxes: circle/capsule.

### 23.4 Hitbox/Hurtbox/Meta-Zone Model
Collision interactions are defined between:
- Bullet hitbox and target hurtbox.

Meta-zones include:
- Graze zone.
- Shield zone.
- Damage/hazard zone.

Each collider carries:
- shape type, layer/mask, team, policy, priority.

Policies include:
- destroy, pierce N, reflect, trigger-only, graze-only.

### 23.5 Optional Continuous Checks
Default is discrete overlap at fixed ticks.
Optional per-archetype swept checks for fast bullets only:
- Swept circle-vs-circle (prev to current segment test).
- Enabled selectively to preserve predictability.

### 23.6 Spatial Partitioning Strategy
Recommended broadphase:
- Uniform grid (or hash-grid variant for large/scrolling arenas).

Reasons:
- Predictable O(N)-like insertion behavior.
- Deterministic iteration.
- Dense-field suitability.
- Allocation-free fixed-array implementations.

Grid design requirements:
- Deterministic cell mapping via floor transform.
- Tunable cell size (~2–4× typical bullet radius).
- Bounds clamp/despawn policy for out-of-range bullets.

### 23.7 Bucket Capacity and Overflow Handling
Per-cell fixed-capacity buffers:
- `cellCounts[]`
- `cellItems[]`

Overflow policy must be deterministic:
- Prefer conservative capacities + fixed-size global overflow buffer + diagnostics.
- Dev builds must surface overflow hotspots.

Optional multi-grid mode:
- Fine grid for bullets.
- Coarse grid for large hazards/colliders.

### 23.8 Per-Tick Collision Pipeline
Stage 0: integrate motion (store previous positions when swept enabled).

Stage 1: build broadphase grid(s).

Stage 2: generate candidate queries per target by overlapping cell bounds.

Stage 3: narrowphase exact tests (circle-circle, circle-capsule, minimal extra pairs).

Stage 4: emit collision/graze/shield events into preallocated buffers.

Stage 5: deterministic resolution apply pass with stable ordering and one-hit-per-bullet rules as configured.

### 23.9 Deterministic Event and Resolution Rules
Event buffers are preallocated ring arrays (`hitEvents`, `grazeEvents`, etc.).

Stable ordering key is explicit (e.g., targetId + bulletPoolId + bulletIndex, or bullet-first equivalent) and must remain fixed per game ruleset.

Required precedence order:
1. Shield
2. Player hit (if vulnerable)
3. Graze
4. Enemy hit
5. Hazard interactions

### 23.10 Memory and Data Layout
Bullets use SoA hot-path arrays:
- positions, previous positions (optional), velocity, radius, flags, team mask, owner, lifetime, behavior id.

Targets/hazards may remain ECS/AoS, but collision uses compact deterministic proxy buffers built allocation-free each tick.

No per-tick dynamic allocation rules apply to:
- grid buffers
- event buffers
- temporary query scratch

### 23.11 Deterministic Multithreading Plan
Parallelization allowed with deterministic merge:
- thread-local preallocated buffers.
- stable merge keys at synchronization point.

Preferred partitioning:
- by target sets (player/enemy hurtboxes) for simpler deterministic ownership.

Parallel grid-build options:
- two-pass deterministic build (`bulletCellId` + stable radix/counting arrangement), preferred over atomics for reproducibility.

### 23.12 GPU Role (Optional)
Gameplay collision remains CPU-first for determinism/debuggability.
GPU is suitable for:
- rendering
- density heatmaps
- debug/analysis visualization

GPU collision for gameplay is optional/advanced and not baseline.

### 23.13 Graze Detection Model
Given player hit radius `R_hit`, graze radius `R_graze`, bullet radius `r`:
- Hit if `d^2 <= (R_hit + r)^2`
- Graze if `(R_hit + r)^2 < d^2 <= (R_graze + r)^2`

Graze throttling is allocation-free via per-bullet fields:
- once-per-life flags or
- `lastGrazeTick` cooldown checks.

### 23.14 Physics Debugging Tooling Requirements
Must provide overlays and introspection for:
- hitbox/hurtbox/graze bands
- collision markers
- grid occupancy + overflow cells
- per-tick resolution order keys
- first divergent replay tick event diffs

### 23.15 Collision Performance Counters
Per tick counters:
- bullets inserted
- cells touched per target
- narrowphase checks
- hits/grazes emitted
- stage timings
- overflow incidents

### 23.16 Collision Stress Lab and Acceptance Targets
Required benchmark scene/tool: deterministic Collision Stress Lab with fixed seeds.

Scenarios include:
- uniform dense fields
- clumped worst-case around player
- many-target scenes
- fast swept bullets
- large-hazard overlap

Reporting includes:
- per-stage ms
- checks/tick
- overflow incidents
- hash stability across runs

Example acceptance target (configurable):
- 10k bullets + player + ~200 enemies with collision stage under target frame budget, zero tick allocations, and controlled/diagnosed overflow behavior.


## 24. Procedural World and Encounter Generation System
### 24.1 Run Structure and Determinism
A run is a deterministic seeded sequence:
- World 1: introduction layer.
- World 2: systemic pressure layer.
- World 3: high intensity layer.
- World 4: endgame layer.
- Final boss encounter.

Each world is generated as a node map and must remain reproducible from run seed for:
- replay/debug parity,
- deterministic validation,
- daily challenge consistency.

### 24.2 World Composition Requirements
Per-world structure:
- 6–10 nodes.
- Exactly one mid-boss and one boss encounter.
- Branching paths with meaningful content diversity.

Node types:
- standard combat,
- elite combat,
- event,
- mutation/reward,
- recovery,
- optional shop.

Each world introduces additional mechanics/archetypes to increase systemic complexity.

### 24.3 World Generation Pipeline
Required generation stages:
1. Seed initialization.
2. World theme/style selection.
3. Node DAG generation.
4. Encounter pool selection.
5. Hazard assignment.
6. Reward distribution.
7. Difficulty-budget validation.

Pipeline outputs must include diagnostics suitable for offline balancing and replay-audit.

### 24.4 World Node Graph Model
World maps are directed acyclic graphs (DAGs) with branching/rejoin structures.

Node distribution constraints:
- at least one elite node,
- at least one mutation node,
- mid-boss appears before boss,
- balanced spacing for combat, event, recovery pressure.

Path bias classes are supported:
- combat-biased,
- event-biased,
- mutation-biased.

### 24.5 Encounter Composition and Threat Budgets
Encounters are generated from bounded threat budgets per world and per encounter.

Threat allocation dimensions:
- enemy role mix,
- wave count/duration,
- bullet density,
- hazard pressure.

Wave structure baseline:
- 2–4 waves per standard encounter,
- pacing includes pressure/recovery alternation,
- elite encounters include mutator layers and elevated rewards.

### 24.6 Enemy Role Taxonomy for Generation
Generation pools classify enemies by functional role:
- Swarmers,
- Snipers,
- Emitters,
- Bruisers.

Wave synthesis must combine roles with synergy-aware composition to avoid monotony and maintain dodge readability.

### 24.7 Event and Non-Combat Systems
Event subsystem supports deterministic randomization of:
- mutation chambers (choice sets + skip option),
- parasite nests (risk/reward combat events),
- dormant tissue zones (healing/lore/latent modifiers),
- temporary surge events (buff/debuff windows).

Event outcomes must be serialized in run state for replay consistency.

### 24.8 Environmental Hazard Integration
Hazard families include:
- blood currents,
- antiseptic bursts,
- slow zones,
- shrinking safe zones.

Hazards are introduced progressively by world and integrated into encounter generation budgets.

Hazard interactions must obey determinism contracts already defined for collision/simulation.

### 24.9 Difficulty Curve and Adaptive Control
Difficulty is multi-dimensional across:
- enemy durability,
- bullet density,
- projectile speed,
- hazard complexity,
- encounter duration.

World-over-world scaling increases complexity gradually, with optional deterministic adaptive adjustments based on performance metrics (damage taken, clear times, build strength).

Adaptive logic must be subtle and deterministic; no runtime nondeterministic tuning.

### 24.10 Reward and Mutation Economy
Mutation categories:
- offense,
- defense,
- mobility,
- utility.

Rarity tiers:
- common, rare, epic, legendary.

Reward flow emphasizes player agency via choice-based presentation (e.g., choose 1 of 3), with deterministic pool sampling and provenance logging.

### 24.11 Variation Systems
Run-to-run variation mechanisms:
- world-specific encounter pools,
- enemy modifiers,
- hazard combinations,
- mutation pool rotation.

Variation remains bounded by world budgets, pacing rules, and fairness constraints.

### 24.12 Designer Control Surface
Designer-authorable controls include:
- per-content weights (encounters/enemy roles/events/hazards),
- world/encounter threat budget ranges,
- pacing constraints (elite spacing, mutation frequency, hazard intro timing),
- progression and gating rules.

Generator must support lock/override mechanisms for manual curation where needed.

### 24.13 Validation and Tooling Requirements
World-generation validation tooling must support:
- large-run simulation sweeps,
- difficulty-distribution analysis,
- encounter frequency and imbalance detection,
- hazard/reward pacing audits,
- deterministic seed replay of generated world plans.

Outputs must include explainable generation reports for each world and encounter.

### 24.14 Summary Contract
Procedural world generation is a deterministic, designer-governed system combining:
- DAG map generation,
- budget-driven encounter synthesis,
- controlled event/hazard variation,
- player-choice rewards.

The resulting runs must differ meaningfully while preserving fairness, pacing quality, and reproducible debugging.


## 25. Product Vision, Delivery Roadmap, and Release Criteria
### 25.1 Product Vision and Scope
The product is a commercial-grade bullet-hell engine prioritizing:
- 10,000+ projectiles with predictable performance.
- Deterministic simulation and replay reproducibility.
- Data-driven content authoring with minimal code requirements.
- Professional editor workflows (graph/timeline/debug/profiling).
- Fast iteration via hot reload and live preview.

In-scope pillars:
- Runtime (simulation, rendering, runtime asset consumption).
- Editor tooling (pattern/boss/wave/debug/profiler).
- Content pipeline (author data -> validated runtime packs).
- Sample projects/templates and documentation.

Explicit initial out-of-scope items:
- general-purpose 3D physics,
- AAA cinematic/film pipelines,
- large-scale open-world streaming.

### 25.2 Target Users
Primary users:
- indie and small studios building bullet-hell roguelites,
- technical designers authoring patterns without programming,
- engine/tools programmers extending via plugin interfaces.

### 25.3 Core Principles (Cross-Cutting)
- Determinism-first.
- Performance-first.
- Tools-first.
- Data-first.
- Explainability-first (cost/difficulty attribution to authored assets).

### 25.4 Program Phase Model (Execution Intent)
Program intent aligns to the following sequence:
- Foundation pre-production (architecture lock and budgets).
- Core runtime.
- Bullet world throughput.
- Collision/graze.
- Pattern VM/IR.
- Editor productionization.
- Enemy/boss/wave gameplay tools.
- Author->build->runtime content pipeline.
- Optimization/scalability pass.
- Polish/docs/templates/release readiness.

ImplementationPlan maps this intent into additive buildable phases and exact validation commands.

### 25.5 Milestone Contracts
Required milestone progression:
- M0 Architecture lock.
- M1 First playable deterministic simulation sandbox.
- M2 1,000-bullet no-allocation stress validation.
- M3 Collision+graze prototype.
- M4 10,000-bullet sustained stress with stable hashes.
- M5 Pattern VM v1.
- M6 Pattern editor prototype.
- M7 Replay system v1 with divergence reporting.
- M8 Boss-fight vertical slice built through tools.
- M9 Full roguelite-loop demo.
- M10 1.0 release candidate readiness.

### 25.6 Tooling Co-Evolution Requirement
Tooling must co-evolve with runtime from early phases:
- early sandbox + inspector,
- debug overlays + profiler skeleton,
- pattern graph + timeline editing,
- replay debugger and divergence detector,
- boss/wave/enemy editors,
- validation/lint/build pipeline UI.

Tooling is a parallel product pillar, not a late-stage add-on.

### 25.7 Performance Targets
Core 1.0 targets:
- 10,000 projectiles at target framerate on defined reference hardware.
- zero allocations in simulation ticks (shipping builds).
- deterministic replay hash stability for identical input/seed/content.
- bounded worst-case collision stage cost.
- hot-reload preview responsiveness suitable for interactive authoring.

Secondary targets:
- performance mode support for higher projectile counts,
- deterministic multithreaded collision,
- profiler attribution down to authored-pattern granularity.

### 25.8 Testing and Verification Strategy
Continuous automation requirements:
- unit tests (RNG, clocks, collision primitives, grid operations),
- property tests (determinism invariants, numeric safety),
- golden tests for Pattern IR compilation output,
- replay verification suites in CI (hash checks + first divergence tick diagnostics),
- stress/soak suites (1k/10k/20k bullets, collision clumps, tool library scale),
- memory and overflow policy validation.

### 25.9 Documentation Program
Documentation begins early and evolves with releases:
- getting started,
- designer guides,
- engineering references,
- performance guides,
- troubleshooting playbooks.

In-editor docs (tooltips/examples/warnings) and templates are first-class documentation channels.

### 25.10 Community, Plugin, and Mod Baselines
1.0 plugin baseline:
- register pattern nodes, projectile behaviors, editor extensions,
- deterministic contract declarations enforced by validation.

Initial mod support:
- data-only packs,
- schema/dependency validation,
- runtime-pack format compatibility.

### 25.11 Post-1.0 Direction
Post-1.0 roadmap includes:
- advanced AI copilot enhancements,
- deeper procedural boss generation,
- optional deterministic co-op/networking,
- advanced mod workflows and publishing,
- GPU-assisted analysis,
- stricter cross-platform determinism hardening.

### 25.12 Vertical Slice Gating Rule
Two gating vertical slices are mandatory before expansion:
1. 10k bullets + collision + replay hash stability.
2. Designer-authored boss fight built end-to-end in tools.


## 26. Editor UX Architecture and Workflow Specification
### 26.1 Overall Layout and Workspace Model
Default authoring workspace includes:
- Top global menu/toolbar with command palette/search, preview controls, difficulty/bot selectors, determinism badge, and overlay toggles.
- Left dock for Content Browser and Outline hierarchy.
- Center document-tab work area (stage view, pattern graph, timeline, boss phases, projectile editor, etc.).
- Right dock for Inspector, live Parameters, and contextual Docs.
- Bottom dock for Console, Validation, Profiler, Replay, Tasks, and Build Output.

Workspace presets are first-class:
- save/load per-role layouts,
- panel docking/splitting/floating,
- per-workspace overlay presets,
- reset options (workspace or global).

### 26.2 Dashboard / Project Overview
Project-open landing dashboard must provide:
- quick navigation by asset type (stages/patterns/projectiles/enemies/bosses/hazards/replays/profiling/modding),
- recent assets + quick-open/run actions,
- quick-create actions/templates,
- project health panel (validation, determinism, perf budget, missing refs),
- last-run metrics (peak bullets, worst frame, hottest assets).

Primary goal: minimize time-to-action for both new and experienced users.

### 26.3 Pattern Graph Editor UX
Pattern authoring tool is hybrid Graph + Timeline + Preview:
- typed node palette,
- graph canvas with minimap,
- timeline scrub/clip area,
- integrated deterministic preview viewport,
- inspector/parameters/docs docks,
- validation/profiler micro-views.

Node families include:
- flow/scheduling,
- emitters,
- parameters/signals,
- spawn/projectile,
- transform/anchor utilities.

Connections:
- thick execution links for order,
- typed thin data links for values.

Supports layering (mute/solo/cost per layer), marker-driven triggers, and expose-parameter workflows.

### 26.4 Pattern Preview and Explainability UX
Pattern preview must support:
- play/pause/step/scrub,
- seed lock/reset,
- bot profile selection,
- overlay toggles,
- live hot-reload at safe boundaries.

Click-to-explain flow:
- selecting a bullet highlights origin node chain,
- displays resolved spawn parameters and modifier influences.

### 26.5 Projectile Editor UX
Projectile editor centers on reusable archetype validation:
- reorderable module stack with searchable add-module palette,
- dedicated motion/collision/visual/gameplay modules,
- collision shape editing overlays,
- split/spawn behavior modules,
- live preview with spawn modes and target dummies,
- collision test mode with event log.

Goal: ensure hitbox fairness, readability, deterministic behavior, and per-archetype performance transparency.

### 26.6 Enemy and Boss Authoring UX
Enemy editor (choreography-first) integrates:
- HFSM/BT behavior graph,
- movement timeline,
- attack slots with pattern refs and overrides,
- sandbox preview with bots,
- validation/event diagnostics.

Boss editor uses synchronized views:
- phase map for high-level progression/transitions,
- phase timeline for attack/hazard/movement/UI tracks,
- rehearsal controls (jump-to-phase/time with preset state),
- anchor/socket visualization with live pattern linkage.

### 26.7 Wave/Level Timeline Tool UX
Stage editor is timeline-driven with multi-tracks:
- enemy spawns,
- boss entries,
- hazards,
- audio/UI cues,
- difficulty curves.

Supports:
- marker events referenced by patterns/boss logic,
- spawn-clip visual metadata,
- predicted-vs-actual bullet load views,
- bot-assisted preview and quick marker jumps.

### 26.8 Debug and Overlay System UX
Overlay system is globally accessible and workspace-aware.

Overlay categories:
- collision (hit/graze/hurt boxes),
- bullet trajectories/lifetimes/ownership,
- density/flow/safe-zone analysis,
- AI state traces,
- performance HUD.

Requirements:
- preset sets (designer/QA/etc.),
- hotkey binding,
- contextual linkage back to source assets.

### 26.9 Profiler UI Requirements
Bullet-hell-specific profiler must provide:
- frame timeline lanes (CPU/GPU/Sim/Render),
- stage breakdown (motion/VM/broadphase/narrowphase/render),
- hotspot ranking by pattern/archetype/cell groups,
- allocation visibility (should remain zero in sim tick),
- click-through asset attribution and “open in editor” actions.

### 26.10 UX Core Behaviors
Mandatory UX fundamentals:
- robust shortcuts and command palette,
- fast search across assets/tools/commands,
- per-document undo/redo (optionally unified global stack),
- autosave, dirty-state indicators, crash recovery,
- non-intrusive smart warnings with actionable fixes and doc links,
- inline parameter docs with ranges/examples/perf notes.

### 26.11 Advanced UI Features (Roadmap-Tracked)
Editor must support roadmap hooks for:
- time-travel replay/pattern debugger with run-compare mode,
- UI-driven AI pattern generation panel with constraints/candidates,
- difficulty lab with bot curves and accessibility deltas,
- player path prediction and safe-corridor visualization,
- pre-sim trajectory concentration analysis,
- 2D macro parameter surfaces for rapid tuning,
- fairness HUD with drill-down reports.


## 27. Implemented Runtime Baseline (Phase 3 Delivery Snapshot)
Current repository implementation now includes:
- C++20 CMake engine skeleton (`EngineDemo`) with SDL2 + Dear ImGui integration.
- Fixed-step simulation loop with render-loop separation and optional headless execution.
- Core runtime services:
  - logging subsystem,
  - JSON-backed configuration loading + command-line overrides,
  - deterministic RNG stream manager,
  - thread-pool job system,
  - frame allocator and object-pool helpers.
- Automated unit tests for timing-step consumption and configuration parsing.

Headless mode contract:
- `EngineDemo --headless --ticks <N> --seed <S>` runs deterministic simulation ticks without opening a window.


### Runtime decomposition update (Phase 1 refactor)
- Runtime orchestration now delegates to three focused modules:
  - `InputSystem` (`input_system.h/.cpp`) for frame input polling, event processing, and replay input plumbing.
  - `GameplaySession` (`gameplay_session.h/.cpp`) for deterministic gameplay simulation state updates and upgrade-screen state.
  - `RenderPipeline` (`render_pipeline.h/.cpp`) for render-context lifecycle and frame rendering.
- `SimSnapshot` defines the explicit sim->render contract used by the render pipeline.


### 23.17 Implemented Collision Runtime Contract (Phase 2)
- `CollisionTarget` contains `{ pos, radius, id, team }` where `id` is a stable deterministic key and `team` filters friendly-fire.
- `CollisionEvent` contains `{ bulletIndex, targetId, graze }`; events are sorted by `(targetId, bulletIndex)` before processing.
- Runtime collision pipeline is now three-stage:
  1. `updateMotion(dt, enemyTimeScale, playerTimeScale)`
  2. `buildGrid()`
  3. `resolveCollisions(targets, outEvents)`
- `resolveCollisions()` performs target AABB -> grid-cell range query, traverses linked lists in overlapping cells, runs circle-circle narrowphase, and enforces one-hit-per-bullet per tick.

### Runtime Architecture Update — GameplaySession State Partitioning (2026-03-07)
- `GameplaySession` now owns explicit state partitions to enforce clearer runtime boundaries:
  - `SessionSimulationState`: deterministic tick-clock and scratch/frame allocation state.
  - `PlayerCombatState`: player runtime combat data (position, aim target, radius, health).
  - `ProgressionState`: upgrade/progression UI navigation state.
  - `PresentationState`: presentation-event sinks and visual runtime state (shake queue, particle effects, danger-field overlay data).
  - `DebugToolState`: debug/perf/tool integration state.
  - `EncounterRuntimeState`: deterministic encounter collision scratch buffers/counters.
- Deterministic simulation behavior is preserved; partitioning is architectural/ownership-focused and keeps replay/hash flow intact.


## Editor Tooling Modular Panel Architecture (Update)
- `ControlCenterToolSuite` now composes focused panel methods instead of a single monolithic draw body.
- Panel ownership is grouped into: workspace shell/content browser, pattern graph editor, encounter/wave editor, projectile+trait tooling, palette/FX editor, and validation diagnostics.
- Shared editor service helpers are explicit (`buildEncounterAsset`, `ensurePatternGraphSeeded`, `ensureEncounterNodesSeeded`) to keep deterministic runtime state separate from persistent UI authoring state.
- Runtime/editor boundaries remain telemetry-driven: panel rendering consumes `ToolRuntimeSnapshot` and mutation hand-off fields (`UpgradeDebugOptions`, `ProjectileDebugOptions`, `PatternGeneratorDebugState`) without embedding simulation execution into panel code.
- Extension point: future tools should add one panel method + one state/service surface instead of growing `drawControlCenter`.

## 13. Asset Import Pipeline Requirements (Implemented)
- Source-art import is manifest-driven and integrated directly into the pack build stage.
- Sprite + texture authored assets are supported with deterministic GUID assignment.
- Import settings include: classification, grayscale/monochrome workflow, pivot, collision policy, atlas group, filtering/mips, variant group, animation grouping.
- Reimport behavior is fingerprint-based and emits explicit dependency invalidation records.
- Atlas build grouping is deterministic and represented in pack metadata for downstream consumers.
- Runtime pack integration includes full source/import registries for tooling diagnostics and cache invalidation behavior.


## Content Pipeline Addendum — Animation and Variant Import Workflow
- Art import manifests support animation clip metadata and grouping:
  - `animationSet`, `animationState`, `animationDirection`, `animationFrame`, `animationFps`
  - optional filename-derived grouping using `animationSequenceFromFilename` + `animationNamingRegex`
- Art import manifests support variant grouping for biome/themed/procedural swaps:
  - `variantGroup`, `variantName`, `variantWeight`, `paletteTemplate`
  - optional filename-derived grouping using `variantNamingRegex`
- Build output includes:
  - `animationBuild`: deterministic clip plans with ordered frame GUIDs and frame timing.
  - `variantBuild`: deterministic variant groups/options with weights and palette-template compatibility metadata.
- Validation requirements:
  - detect malformed naming/group values, duplicate frame indices, duplicate variant names, invalid frame/fps/weight values, and inconsistent clip FPS within a clip.
- Runtime expectations:
  - runtime/editor consume `animationBuild` and `variantBuild` directly instead of requiring per-frame manual setup.
  - procedural selection uses variant `weight` under deterministic RNG, with optional palette-template application where authored.

## Audio Runtime and Event Routing
- `AudioSystem` is a presentation-only runtime service; simulation never depends on audio state.
- Gameplay emits deterministic audio events (`hit`, `graze`, `player_damage`, `enemy_death`, `boss_warning`, `ui_click`, `ui_confirm`) and runtime flushes them after each simulation tick.
- Audio content is data-driven via pack `audio` section (`clips`, `events`, `music`) and loaded at runtime from content pack JSON.
- Mix model supports buses (`master`, `music`, `sfx`) with independent volume controls (`audioMasterVolume`, `audioMusicVolume`, `audioSfxVolume`).
- Supported source asset type is WAV via SDL decode/conversion, with deterministic fallback tones when source files are unavailable.

## Enemy/Boss Runtime Ownership Model (DL-0021)
- Enemy authoring data (`EntityTemplate`, `BossPhase`) is immutable runtime input loaded from content packs.
- Behavior state is isolated in `EntitySystem::EntityInstance` (phase timers, telegraph lead timers, phase pattern cursor, cooldown state).
- Pattern firing/orchestration is split between:
  - generic `emitPatternFromTemplate` for normal entities.
  - boss-specific `emitBossPhasePattern` for phase sequence/cadence orchestration.
- Encounter ownership boundary is explicit:
  - `EntitySystem` owns enemy/boss lifecycle + deterministic runtime events.
  - `GameplaySession` owns presentation reactions (camera shake + audio) to runtime events.
- Combat presentation hooks are eventized via `EntityRuntimeEvent` (`Telegraph`, `HazardSync`, `BossPhaseStarted`, `BossDefeated`, etc.).
- Boss phases support multi-pattern sequence authoring with per-phase cadence (`patternSequence`, `patternCadenceSeconds`) while preserving deterministic update order.
- Encounter synchronization hooks:
  - authored encounter schedule now supports `telegraph`, `hazardSync`, and `phaseGate` node types.
  - compiled encounter events carry `owner` domain metadata (`encounter`, `boss`, `hazards`) for orchestration routing.


## Persistence Baseline (Settings, Profiles, Runtime Meta)
- Persistence is **outside** deterministic simulation ownership; simulation consumes resolved runtime config/snapshots and never performs direct file I/O during deterministic tick execution.
- Two baseline persisted products:
  - **User settings** (`user_settings.json`): audio/video/gameplay preferences with schema versioning.
  - **Profiles** (`profiles.json`): save-slot list, active profile pointer, and runtime/meta progression snapshot payload.
- Schema policy:
  - Each persisted document includes `schemaVersion`.
  - Current baseline schema is `2`.
  - Loader must run explicit migration for known legacy versions (v1→v2).
  - Unknown future versions must fail safely with fallback behavior (do not silently coerce unknown schemas).
- Corruption policy:
  - Invalid JSON or structurally invalid persisted files must return a fallback load status and default in-memory state, not crash runtime startup.
  - Runtime should log fallback reasons for diagnostics/QA triage.
- Save-slot baseline payload contains minimum product scaffolding:
  - Profile identity metadata (`id`, display label, last-play marker).
  - Meta progression snapshot (`progressionPoints`, purchased node IDs) for cross-run progression continuity.
  - Extension-ready counters for run-level lifetime stats (started/cleared) for future progression systems.

## Section 15 (Rendering Strategy) — Runtime baseline amendment
- Bullet presentation now supports a single-draw-call OpenGL path (`GlBulletRenderer`) using grayscale SDF atlas sampling plus palette ramp-compatible pipeline integration.
- CPU simulation remains authoritative; GL bullet buffers are rebuilt from projectile SoA each frame with no persistent GPU gameplay state.
- Non-GL platforms and GL init failures continue to use SpriteBatch procedural rendering fallback.

## Section 27 (Implemented Runtime Baseline) — Runtime feature amendment
- Integrated `GlBulletRenderer` into `RenderPipeline` for deterministic-sim-safe, presentation-only acceleration of bullets and trails.
- Added per-frame CPU vertex/index rebuild with preallocated GL buffers and one draw call for active projectiles.
