# HellEngine — Claude Code Implementation Prompt

## YOUR IDENTITY

You are a Principal Engine Architect implementing targeted fixes to a C++20 bullet-hell engine. You have been given an architecture audit (see `docs/AuditReport.md` in this repo) that identified 8 red flags and 10 prioritized improvements. Your job is to implement them.

## REFERENCE DOCUMENTS — READ THESE FIRST

Before writing any code, read these files in full to understand the architecture, spec contracts, and existing patterns:

```
docs/MasterSpec.md          — authoritative engine specification (27 sections)
docs/Architecture.md        — module map and data flow
docs/PerfTargets.md         — frame budgets, benchmark thresholds
docs/DecisionLog.md         — locked architectural decisions
docs/AuditReport.md         — the audit you are implementing fixes for
include/engine/runtime.h    — current Runtime class layout (the god object)
include/engine/projectiles.h — SoA bullet world interface
src/engine/runtime.cpp      — main simulation + render loop
src/engine/projectiles.cpp  — bullet update, collision, render
src/engine/gpu_bullets.cpp  — GPU bullet system (broken)
src/engine/pattern_graph.cpp — pattern compiler + VM
CMakeLists.txt              — build system
```

## HARD CONSTRAINTS

1. **Additive evolution only.** Never rename existing public headers or exported symbols. New files are fine; deleting modules is not.
2. **Keep building after every phase.** Each phase ends with a compilable, testable codebase. Do not leave half-refactored code.
3. **Determinism is sacred.** Any change to simulation logic must preserve identical replay hashes for the same seed + content. If you change tick ordering, update the golden replay expected hashes and document why.
4. **C++20 standard, no exceptions in hot paths.** Match the existing code style: `snake_case` for files, `camelCase` for methods, `camelCase_` for members, `PascalCase` for types.
5. **No new third-party dependencies** except where explicitly specified (Catch2 for tests, SDL_mixer for audio).
6. **Windows-primary.** All new code must compile with MSVC (C++20) and with the existing CMakeLists.txt FetchContent workflow.

## EXECUTION MODEL

Work in **sequential phases**. Complete each phase fully before starting the next. After each phase:
- Verify the build compiles: `cmake --build build --config Release`
- Run existing tests: `ctest --test-dir build -C Release`
- Commit with a descriptive message referencing the audit item number

If a phase is too large for a single pass, break it into sub-steps internally but still deliver the full phase as one coherent change.

---

## PHASE 1: Decompose Runtime (Audit #1 — RED FLAG 1)

**Goal:** Break `runtime.cpp` (1,234 LOC god object) into focused subsystems without changing any behavior.

### Step 1.1 — Extract InputSystem

Create `include/engine/input_system.h` and `src/engine/input_system.cpp`.

Move from `Runtime`:
- `currentInputMask_` and all `SDL_SCANCODE` / `SDL_KEYDOWN` input polling logic
- The `handleUpgradeNavigation()` method
- Replay input injection (`replayPlayer_.inputForTick()`)
- Replay input recording (`replayRecorder_.recordTickInput()`)

New interface:
```cpp
class InputSystem {
public:
    void pollInput(bool headless);                        // call once per frame
    void processEvent(const SDL_Event& event);            // forward SDL events
    [[nodiscard]] std::uint32_t inputMask() const;        // current tick's mask
    void setReplayPlayback(ReplayPlayer* player);         // replay injection
    void recordTo(ReplayRecorder* recorder);              // replay recording
    // Upgrade navigation delegates
    using UpgradeNavCallback = std::function<void(UpgradeNavAction)>;
    void setUpgradeNavCallback(UpgradeNavCallback cb);
};
```

`Runtime` stores an `InputSystem` member and calls it instead of inlining the logic.

### Step 1.2 — Extract RenderPipeline

Create `include/engine/render_pipeline.h` and `src/engine/render_pipeline.cpp`.

Move from `Runtime`:
- `renderFrame()` method body
- `buildSceneOverlay()` method body
- All render-context management: `initializeRenderContext()`, `destroyRenderContext()`, `recreateRenderContext()`
- `refreshDisplayMetrics()`
- Members: `window_`, `renderer_`, `textures_`, `modernRenderer_`, `spriteBatch_`, `debugDraw_`, `debugText_`, `camera_`, DPI fields, `renderContextReady_`

New interface:
```cpp
class RenderPipeline {
public:
    bool initialize(SDL_Window* window, const EngineConfig& config);
    void shutdown();
    void renderFrame(const SimSnapshot& snapshot, double frameDelta);
    Camera2D& camera();
    // ... render-context recovery, fullscreen toggle
};
```

Define a `SimSnapshot` struct that contains everything the renderer needs from the sim (projectile system ref, entity system ref, player pos, debug flags, perf snapshot, etc.). The renderer reads from this — no back-channel into sim.

### Step 1.3 — Extract GameplaySession

Create `include/engine/gameplay_session.h` and `src/engine/gameplay_session.cpp`.

Move from `Runtime`:
- `playerPos_`, `playerRadius_`, `playerHealth_`, `aimTarget_`
- Trait/archetype/meta-progression/run-structure/difficulty orchestration logic from `simTick()`
- Upgrade screen state (`upgradeScreenOpen_`, `focusedUpgradeIndex_`, `cardAnim_`)
- `drawUpgradeSelectionUi()`, `buildCurrentViewStats()`, `buildProjectedViewStats()`, `hasSynergyWithActive()`
- Zone/stage memo tracking

`Runtime::simTick()` should shrink to roughly:
```cpp
void Runtime::simTick(double dt) {
    frameAllocator_.reset();
    profiler_.beginFrame();
    input_.pollInput(config_.headless);
    session_.updateGameplay(dt, input_.inputMask());
    projectiles_.update(dt, session_.playerPos(), session_.playerRadius(), ...);
    session_.updatePostCollision(projectiles_.stats());
    replayVerify(dt);
    ++tickIndex_;
}
```

### Step 1.4 — Update CMakeLists.txt

Add the new `.cpp` files to `engine_core` STATIC library sources.

### Step 1.5 — Verify

- Build compiles
- All 32 existing tests pass
- Headless run produces identical output: `EngineDemo --headless --ticks 600 --seed 42`
- `runtime.cpp` is now under 400 LOC

---

## PHASE 2: Fix Collision Architecture (Audit #2 — RED FLAG 2)

**Goal:** Make the spatial grid actually used for collision queries and support multiple collision targets.

### Step 2.1 — Introduce CollisionTarget Abstraction

In `include/engine/projectiles.h`, add:
```cpp
struct CollisionTarget {
    Vec2 pos;
    float radius;
    std::uint32_t id;           // stable ID for deterministic ordering
    std::uint8_t team;          // 0=player, 1=enemy
};

struct CollisionEvent {
    std::uint32_t bulletIndex;
    std::uint32_t targetId;
    bool graze;                 // true = graze-only, false = hit
};
```

### Step 2.2 — Separate Motion from Collision

Split `ProjectileSystem::update()` into three methods:
```cpp
void updateMotion(float dt, float enemyTimeScale, float playerTimeScale);
void buildGrid();
void resolveCollisions(
    std::span<const CollisionTarget> targets,
    std::vector<CollisionEvent>& outEvents   // preallocated, cleared each tick
);
```

`updateMotion()` does steps 1-6 from the current update (accel, curved, homing, split, move, bounce) but NOT collision.

`buildGrid()` rebuilds `gridHead_`/`gridNext_`.

`resolveCollisions()`:
- For each target, compute overlapping grid cells (target AABB → cell range)
- Walk linked list in each cell, run narrowphase circle-circle
- Emit `CollisionEvent` into preallocated buffer
- Sort events by `(targetId, bulletIndex)` for deterministic processing
- Apply one-hit-per-bullet policy

### Step 2.3 — Wire Into Runtime

In `Runtime::simTick()`:
```cpp
projectiles_.updateMotion(dt, dilation.enemyProjectiles, dilation.playerProjectiles);
projectiles_.buildGrid();

// Build target list
collisionTargets_.clear();
collisionTargets_.push_back({session_.playerPos(), session_.playerRadius(), 0, 0});
entitySystem_.appendCollisionTargets(collisionTargets_);  // adds enemy hurtboxes

collisionEvents_.clear();
projectiles_.resolveCollisions(collisionTargets_, collisionEvents_);

// Process events
session_.processCollisionEvents(collisionEvents_);
entitySystem_.processCollisionEvents(collisionEvents_);
```

### Step 2.4 — Add Collision Tests

Create `tests/collision_correctness_tests.cpp`:
- Test: single bullet at known position, single target → hit detected
- Test: bullet in graze band → graze event, not hit
- Test: bullet with `allegiance=Player` does not hit player target
- Test: 1000 bullets, 50 targets → results match brute-force reference
- Test: deterministic ordering — same inputs → same event order

### Step 2.5 — Verify

- Build compiles
- All tests pass including new collision tests
- Headless replay hash is stable (update golden hashes if collision ordering changed, document in `docs/DecisionLog.md`)
- Benchmark: 10k bullets + 200 targets under 2ms per tick

---

## PHASE 3: Harden Floating-Point Determinism (Audit #3 — RED FLAG 4)

**Goal:** Ensure identical simulation output across Debug/Release and across rebuilds.

### Step 3.1 — CMake Compiler Flags

In `CMakeLists.txt`, add to `engine_core` target ONLY (not tests or tools):
```cmake
if(MSVC)
    target_compile_options(engine_core PRIVATE /fp:strict)
else()
    target_compile_options(engine_core PRIVATE -ffp-contract=off -fno-fast-math)
endif()
```

### Step 3.2 — Create `include/engine/deterministic_math.h`

Wrap trig functions used in hot paths:
```cpp
#pragma once
#include <cmath>

namespace engine::dmath {
    // Platform-stable wrappers. Currently forward to std:: under /fp:strict.
    // If cross-platform divergence is detected, replace bodies with
    // fixed-precision polynomial approximations.
    inline float sin(float x) { return std::sin(x); }
    inline float cos(float x) { return std::cos(x); }
    inline float atan2(float y, float x) { return std::atan2(y, x); }
    inline float sqrt(float x) { return std::sqrt(x); }
}
```

Replace all `std::cos`, `std::sin`, `std::atan2`, `std::sqrt` calls in:
- `src/engine/projectiles.cpp`
- `src/engine/pattern_graph.cpp`
- `src/engine/entities.cpp`
- `src/engine/patterns.cpp`
- `src/engine/gpu_bullets.cpp`

with `dmath::cos`, `dmath::sin`, etc. This creates a single choke-point for future fixed-point migration.

### Step 3.3 — Add Cross-Config Determinism Test

Create `tests/determinism_cross_config_test.cpp`:
- Run 300 ticks headless with seed 42
- Compute final state hash
- Hard-code expected hash value
- This test runs in CI for both Debug and Release — if hashes differ, the test fails

### Step 3.4 — Update `docs/DecisionLog.md`

Add entry: "Floating-Point Policy Hardened — added `/fp:strict`, `dmath::` wrappers, cross-config CI test."

### Step 3.5 — Verify

- Build compiles in both Debug and Release
- Headless run with seed 42 produces identical hash in both configs
- All existing tests pass

---

## PHASE 4: Fix GpuBulletSystem (Audit #4 — RED FLAG 3)

**Goal:** Fix O(N) linear scan in `emit()` and `activeCount()`.

### Step 4.1 — Add Free-List and Cached Count

In `include/engine/gpu_bullets.h`, add members:
```cpp
std::vector<std::uint32_t> freeList_;
std::uint32_t activeCount_ {0};
```

### Step 4.2 — Rewrite `emit()`, `update()`, `activeCount()`

`initialize()`: populate `freeList_` (same pattern as `ProjectileSystem`), set `activeCount_ = 0`.

`emit()`: pop from `freeList_`, increment `activeCount_`. O(1).

`update()`: when deactivating a bullet, push to `freeList_`, decrement `activeCount_`.

`activeCount()`: return `activeCount_`. O(1).

`clear()`: repopulate `freeList_`, set `activeCount_ = 0`.

### Step 4.3 — Verify

- `gpu_bullets_tests` passes
- Emit 100,000 bullets, verify `activeCount()` == 100,000
- Update until all expire, verify `activeCount()` == 0 and `freeList_.size()` == capacity

---

## PHASE 5: Compact Active-List Iteration (Audit #5 — RED FLAG 7)

**Goal:** `update()`, `render()`, `debugDraw()`, `collectGrazePoints()`, and `debugStateHash()` iterate only active bullets.

### Step 5.1 — Add Active Index Array

In `include/engine/projectiles.h`, add:
```cpp
std::vector<std::uint32_t> activeIndices_;
std::uint32_t activeCount_ {0};   // replaces stats_.activeCount for source-of-truth
```

### Step 5.2 — Maintain on Spawn/Deactivate

`spawn()`: append slot index to `activeIndices_[activeCount_++]`.

On deactivation (in `updateMotion` or collision processing): swap with last element, decrement `activeCount_`.

After all spawns and deactivations in a tick, the first `activeCount_` entries of `activeIndices_` are the live set.

For **determinism**: sort `activeIndices_[0..activeCount_)` by index at the end of the tick if any removals happened. This ensures iteration order is stable. (Alternatively, never swap — use a "mark and compact" pass at tick end.)

### Step 5.3 — Convert All Loops

Every loop in `ProjectileSystem` that currently does:
```cpp
for (uint32_t i = 0; i < capacity_; ++i) {
    if (!active_[i]) continue;
```
becomes:
```cpp
for (uint32_t j = 0; j < activeCount_; ++j) {
    const uint32_t i = activeIndices_[j];
```

Apply to: `updateMotion()`, `buildGrid()`, `resolveCollisions()`, `render()`, `debugDraw()`, `collectGrazePoints()`, `debugStateHash()`.

### Step 5.4 — Verify

- All tests pass
- Replay hash unchanged (deterministic ordering preserved)
- Benchmark: measurable speedup at 5k active / 20k capacity

---

## PHASE 6: Eliminate Hot-Path Allocations (Audit #7 — RED FLAG 6)

**Goal:** Zero heap allocations during `simTick()` in Release builds.

### Step 6.1 — Fixed-Capacity Pending Spawns

In `ProjectileSystem`, replace:
```cpp
std::vector<ProjectileSpawn> pendingSpawns_;
```
with a fixed-capacity array:
```cpp
static constexpr std::uint32_t kMaxPendingSpawns = 4096;
std::array<ProjectileSpawn, kMaxPendingSpawns> pendingSpawns_;
std::uint32_t pendingSpawnCount_ {0};
```

In `updateMotion()`, replace `pendingSpawns_.push_back(...)` with:
```cpp
if (pendingSpawnCount_ < kMaxPendingSpawns) {
    pendingSpawns_[pendingSpawnCount_++] = spawn;
} else {
    // Dev assert in debug, silent drop in release
    assert(false && "pendingSpawns overflow");
}
```

### Step 6.2 — Remove ostringstream from simTick

In `runtime.cpp`, the tick-periodic logging currently uses `std::ostringstream`. Replace with `snprintf`:
```cpp
if (tickIndex_ % 240 == 0) {
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "Sim tick=%llu activeProjectiles=%u collisionsTotal=%u",
        static_cast<unsigned long long>(tickIndex_),
        projectiles_.stats().activeCount,
        projectiles_.stats().totalCollisions);
    logInfo(std::string(buf));
}
```

### Step 6.3 — Fixed-Capacity Collision Events

The `collisionEvents_` vector introduced in Phase 2 should also be fixed-capacity:
```cpp
static constexpr std::uint32_t kMaxCollisionEvents = 8192;
std::array<CollisionEvent, kMaxCollisionEvents> collisionEvents_;
std::uint32_t collisionEventCount_ {0};
```

### Step 6.4 — Verify

- Build compiles
- All tests pass
- Add a temporary debug check: override global `operator new` in a test build to assert it's never called between `simTick()` entry and exit. Remove after verifying.

---

## PHASE 7: Adopt Catch2 Test Framework (Audit #6 — RED FLAG 5)

**Goal:** Migrate to Catch2, add property tests and collision correctness tests.

### Step 7.1 — Add Catch2 via FetchContent

In `CMakeLists.txt`:
```cmake
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
```

### Step 7.2 — Migrate Key Test Files

Convert these tests first (highest value):
- `tests/projectile_tests.cpp`
- `tests/projectile_behavior_tests.cpp`
- `tests/pattern_graph_tests.cpp`
- `tests/replay_tests.cpp`
- `tests/collision_correctness_tests.cpp` (created in Phase 2)

Replace raw `main()` + `assert()` with `TEST_CASE` / `SECTION` / `REQUIRE` macros. Link to `Catch2::Catch2WithMain`.

### Step 7.3 — Add New Property Tests

In `tests/determinism_property_tests.cpp`:
```cpp
TEST_CASE("Determinism: same seed produces same hash", "[determinism]") {
    for (int trial = 0; trial < 5; ++trial) {
        auto hash1 = runHeadlessSim(/*seed=*/42, /*ticks=*/300);
        auto hash2 = runHeadlessSim(/*seed=*/42, /*ticks=*/300);
        REQUIRE(hash1 == hash2);
    }
}

TEST_CASE("Determinism: different seeds produce different hashes", "[determinism]") {
    auto h1 = runHeadlessSim(42, 300);
    auto h2 = runHeadlessSim(43, 300);
    REQUIRE(h1 != h2);
}
```

### Step 7.4 — Add JSON Fuzz Test

In `tests/content_fuzz_tests.cpp`:
```cpp
TEST_CASE("Malformed JSON does not crash pattern loader", "[fuzz]") {
    const std::vector<std::string> inputs = {
        "", "{}", "null", "{\"patterns\": null}", "{\"patterns\": [{}]}",
        "{\"patterns\": [{\"layers\": \"not_array\"}]}",
        "{\"patterns\": [{\"layers\": [{\"bulletCount\": -1}]}]}",
        std::string(1000000, 'x'),  // 1MB garbage
    };
    for (const auto& input : inputs) {
        engine::PatternBank bank;
        // Must not crash — return value doesn't matter
        REQUIRE_NOTHROW(bank.loadFromString(input));
    }
}
```

This requires adding a `loadFromString()` method to `PatternBank` (or writing input to a temp file).

### Step 7.5 — Update CMakeLists.txt

For migrated tests, change `target_link_libraries` to include `Catch2::Catch2WithMain` and use `catch_discover_tests()` instead of `add_test()`.

### Step 7.6 — Verify

- All migrated tests discovered and pass
- New property tests pass
- New fuzz test passes

---

## PHASE 8: Floating-Point Deterministic Math Wrappers (Audit #3 — continued)

**Goal:** If Phase 3 revealed cross-config hash divergence, replace `dmath::` bodies with polynomial approximations.

### Conditional Execution

Only do this phase if the `determinism_cross_config_test` FAILS after Phase 3. If it passes, skip to Phase 9.

### Step 8.1 — Implement Minimax Polynomial Approximations

In `src/engine/deterministic_math.cpp`:
- `dmath::sin()` — 7th-order minimax on [-π, π], max error < 1e-6
- `dmath::cos()` — via `sin(x + π/2)`
- `dmath::atan2()` — CORDIC or polynomial, matching platform output to ±1 ULP
- `dmath::sqrt()` — Newton-Raphson with fixed iteration count (3 iterations)

These are well-documented algorithms. Use the implementations from the public domain (e.g., Cephes library, or handwritten minimax fits).

### Step 8.2 — Verify

- Cross-config test now passes
- Replay hashes stable
- Benchmark: trig functions not more than 2x slower than `std::` versions

---

## PHASE 9: Basic Audio System (Audit #8 — RED FLAG 8)

**Goal:** Add fire-and-forget sound effects for graze, hit, pattern phase, and boss warnings.

### Step 9.1 — Add SDL_mixer via FetchContent

In `CMakeLists.txt`:
```cmake
FetchContent_Declare(
    SDL2_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG release-2.8.0
)
FetchContent_MakeAvailable(SDL2_mixer)
```

### Step 9.2 — Create AudioSystem

Create `include/engine/audio_system.h` and `src/engine/audio_system.cpp`.

```cpp
class AudioSystem {
public:
    bool initialize();
    void shutdown();

    using SoundId = std::uint16_t;
    SoundId loadSound(const std::string& path);
    void playSound(SoundId id, float volume = 1.0f);
    void playSoundPositional(SoundId id, Vec2 position, Vec2 listener, float volume = 1.0f);

    // Call once per frame to process pending plays
    void update();

private:
    // SDL_mixer handles, sound bank, etc.
};
```

Key rule: **Audio is presentation-only.** It must NEVER feed back into simulation state. No `AudioSystem` calls inside `simTick()` except to queue events. Actual mixing happens in the render/present phase.

### Step 9.3 — Wire to Game Events

In the render/present phase (not sim):
- On collision event → `audio.playSound(hitSoundId)`
- On graze event → `audio.playSound(grazeSoundId)`
- On boss phase transition → `audio.playSound(phaseWarnSoundId)`
- On defensive special activation → `audio.playSound(specialSoundId)`

### Step 9.4 — Verify

- Build compiles
- Engine starts and runs without audio files (graceful fallback)
- No simulation hash changes (audio is presentation-only)

---

## PHASE 10: Content Hot-Reload (Audit #9)

**Goal:** Generalize the existing pattern graph hot-reload to all content types.

### Step 10.1 — Create ContentWatcher

Create `include/engine/content_watcher.h` and `src/engine/content_watcher.cpp`.

```cpp
class ContentWatcher {
public:
    void addWatchPath(const std::string& path, ContentType type);
    // Returns list of changed files since last check
    std::vector<ContentChange> pollChanges();

private:
    struct WatchEntry {
        std::string path;
        ContentType type;
        std::filesystem::file_time_type lastModified;
    };
    std::vector<WatchEntry> entries_;
};
```

### Step 10.2 — Generalize Reload Logic

In `Runtime` (or `GameplaySession` after Phase 1), replace the pattern-only hot-reload with:
```cpp
for (const auto& change : contentWatcher_.pollChanges()) {
    switch (change.type) {
        case ContentType::Patterns:
            reloadPatterns(change.path);  // existing logic
            break;
        case ContentType::Entities:
            reloadEntities(change.path);  // new
            break;
        case ContentType::Traits:
            reloadTraits(change.path);    // new
            break;
        case ContentType::Difficulty:
            reloadDifficulty(change.path); // new
            break;
    }
}
```

Each reload function: validate → swap at tick boundary → fallback on failure → log to editor console.

### Step 10.3 — Verify

- Modify a JSON file while engine is running → changes appear within 1 second
- Invalid JSON → error in console, previous content preserved
- Replay hash unaffected by reload timing (content is reloaded at deterministic boundaries)

---

## COMPLETION CHECKLIST

After all phases, verify:

- [ ] `runtime.cpp` is under 400 LOC
- [ ] Collision uses spatial grid queries with multi-target support
- [ ] `/fp:strict` is set on MSVC for `engine_core`
- [ ] `dmath::` wrappers used for all trig in simulation code
- [ ] `GpuBulletSystem::emit()` is O(1)
- [ ] `ProjectileSystem` iterates only active bullets (compact active list)
- [ ] Zero heap allocations during `simTick()` in Release
- [ ] Catch2 is integrated, property tests and fuzz tests exist
- [ ] Audio system exists and is presentation-only
- [ ] Hot-reload works for patterns, entities, traits, and difficulty
- [ ] All existing tests pass
- [ ] Headless determinism test passes across Debug/Release
- [ ] Benchmark suite thresholds still pass

Run the full verification:
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
build/Release/EngineDemo.exe --headless --ticks 600 --seed 42
build/Release/benchmark_suite_tests.exe --assert-thresholds
```
