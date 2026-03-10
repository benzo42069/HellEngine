# HellEngine Exhaustive Audit (2026-03-10)

## Scope and method
- Reviewed all first-party source/build files via repository-wide static scans (`rg`, targeted Python scans) and manual code review of runtime, rendering, memory/concurrency, and build entrypoints.
- Third-party vendored libraries (`third_party/*`) were excluded from issue ownership unless integration misuse was found in engine code.

---

## 1) 🔴 Stack overflow & memory issues

### Findings
1. **Unchecked index from external array can cause OOB read/write in GL vertex build path** (**High**)
   - `GlBulletRenderer::buildVertexBuffer` trusts `activeIndices[j]` and directly indexes many SoA arrays (`active[i]`, `paletteIndex[i]`, `shape[i]`, etc.) with no bounds guard against `maxBullets_` or projectile capacity.
   - If upstream active-index bookkeeping is corrupted (or stale after resize), this becomes immediate memory corruption.
   - Evidence: `src/engine/gl_bullet_renderer.cpp` lines 161-207.
   - **Concrete fix**:
     - Add `if (i >= maxBullets_) { continue; }` hard guard in renderer.
     - Prefer passing explicit `capacity` and validating `i < capacity` before dereference.

2. **Raw pointer bundle API with ambiguous ownership/lifetime** (**Medium**)
   - `GlBulletRenderer::buildVertexBuffer(...)` accepts many raw pointers without lifetime/extent metadata, increasing risk of mismatch and UAF-style misuse in future call sites.
   - Evidence: `include/engine/gl_bullet_renderer.h` lines 27-51.
   - **Concrete fix**:
     - Replace pointer bundle with a typed immutable view struct (`ProjectileSoAView`) using spans (`std::span<const float>`, etc.) and explicit `count`.

3. **Thread-pool destructor race with enqueue (shutdown contract violation)** (**Medium**)
   - `JobSystem::~JobSystem` sets `stop_` and joins workers, but `enqueue` has no stop-state rejection. A concurrent producer can still push jobs after stop, causing silent dropped/unprocessed tasks at teardown.
   - Evidence: `include/engine/job_system.h` lines 23-37, `src/engine/job_system.cpp` lines 16-24.
   - **Concrete fix**:
     - Under mutex in `enqueue`, reject if `stop_` is true (throw or return failed future).
     - Introduce explicit `shutdown()` and document no enqueue after shutdown.

4. **No large stack allocations / VLAs / alloca found in first-party code** (**Info**)
   - Repository scan found no `alloca`, `_alloca`, or VLA usage in engine code.

---

## 2) 🟠 OpenGL correctness

### Findings
1. **No centralized GL error/debug callback instrumentation** (**Medium**)
   - No `glGetError`, `glDebugMessageCallback`, or KHR_debug usage in first-party rendering paths.
   - Failures are therefore mostly silent outside shader compile/link checks.
   - Evidence: repo scan and rendering files (`src/engine/render_pipeline.cpp`, `src/engine/modern_renderer.cpp`, `src/engine/gl_bullet_renderer.cpp`).
   - **Concrete fix**:
     - In debug builds, enable `GL_KHR_debug` callback once context is created.
     - Wrap critical resource creation/update calls with validation macro.

2. **Framebuffer status check does not restore previous FBO binding (state leak risk)** (**Low/Medium**)
   - `RendererModernPipeline::checkFramebuffer` binds the passed FBO and does not restore previous framebuffer binding.
   - Evidence: `src/engine/modern_renderer.cpp` lines 258-264.
   - **Concrete fix**:
     - Save old binding via `glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev)` and restore before return.

3. **Uniform location queried repeatedly in hot post-FX passes** (**Low performance correctness)**
   - `runBloomPass`, `runVignettePass`, `runCompositePass` call `glGetUniformLocation` every frame.
   - Evidence: `src/engine/modern_renderer.cpp` lines 473-548.
   - **Concrete fix**:
     - Cache uniform locations at program-link time.

4. **VAO/VBO/EBO binding/deletion discipline is otherwise correct** (**Info**)
   - `glGen*`/`glDelete*` pairing verified in `GlBulletRenderer`, `PaletteRampTexture`, `GrayscaleSpriteAtlas`, `ShaderCache`, and modern renderer target management.

5. **No deprecated fixed-function OpenGL found** (**Info**)
   - No `glBegin/glEnd`, `glMatrixMode`, etc. found in engine source.

---

## 3) 🟡 CMake build system audit

### Findings
1. **Global compile options used (not target-scoped)** (**Medium**)
   - `add_compile_options(/Brepro)` and `add_compile_options(-ffile-prefix-map=...)` are global.
   - Evidence: `CMakeLists.txt` lines 46-52.
   - **Concrete fix**:
     - Move reproducibility flags to target-level helper (`target_compile_options`/`target_link_options`) for engine targets only.

2. **Windows warning policy insufficient for engine targets** (**Medium**)
   - No explicit `/W4`, `/WX`, `/permissive-`, `/MP` target policy for MSVC.
   - Evidence: `CMakeLists.txt` (no such target flags).
   - **Concrete fix**:
     - Add per-target warning profiles (`engine_core`, `EngineDemo`, test targets).

3. **No install/export packaging rules** (**Low/Medium, release engineering risk)**
   - No `install()` directives for binaries/assets/runtime dependencies.
   - Evidence: `CMakeLists.txt` contains no `install(` entries.
   - **Concrete fix**:
     - Add install targets for executable, assets, and runtime DLLs; add CPack profile for Windows distribution.

4. **Dependency discovery strategy mixed but acceptable** (**Info**)
   - OpenGL via `find_package(OpenGL REQUIRED)` and key libs via `FetchContent`; this is valid though long configure times are expected.

5. **Top-of-file project configuration is correct** (**Info**)
   - `cmake_minimum_required` and `project(...)` are correctly placed and versioned.

---

## 4) 🟡 C++ correctness & UB

### Findings
1. **Potential integer narrowing when passing geometry counts to SDL** (**Low**)
   - `static_cast<int>(vertices_.size())`/`indices_.size()` in rendering helpers can theoretically overflow for very large buffers.
   - Evidence: `src/engine/render2d.cpp` line 191, `src/engine/gpu_bullets.cpp` line 148.
   - **Concrete fix**:
     - Clamp/check sizes before cast; fail fast if over `INT_MAX`.

2. **Global plugin registry is not thread-safe** (**Medium**)
   - Mutable singleton registry (`static PluginRegistry r`) has no mutex in register/unregister/read operations.
   - Evidence: `src/engine/plugin_registry.cpp` lines 8-11 and 86-130.
   - **Concrete fix**:
     - Add internal mutex around mutations + read snapshot accessors, or confine all operations to main thread and enforce contract.

3. **Double include minor hygiene issue** (**Low**)
   - `runtime.cpp` includes `<fstream>` twice.
   - Evidence: `src/engine/runtime.cpp` lines 13 and 15.
   - **Concrete fix**:
     - Remove duplicate include.

4. **No obvious direct recursion in first-party engine code** (**Info**)
   - Static scan did not find intended self-recursive gameplay/render paths.

---

## 5) 🟢 Engine architecture & design

### Findings
1. **`Runtime` remains a high-responsibility coordinator (near God object)** (**Medium**)
   - Handles bootstrapping, content loading, window+renderer lifecycle, input loop, simulation stepping, replay mode, save/profile persistence.
   - Evidence: `src/engine/runtime.cpp` lines 99-390.
   - **Concrete fix**:
     - Split into boot (`RuntimeBootstrap`), frame loop (`MainLoop`), persistence (`SessionPersistence`), and platform shell.

2. **Global mutable state through singletons** (**Medium**)
   - Logger singleton and plugin registry singleton reduce test isolation and explicit lifetime control.
   - Evidence: `src/engine/logging.cpp` lines 25-28, `src/engine/plugin_registry.cpp` lines 8-11.
   - **Concrete fix**:
     - Inject interfaces where possible (logger sink, plugin registry service).

3. **Game loop timing model is solid fixed-step accumulator** (**Info/Positive**)
   - Uses accumulator + fixed step and bounded step count (`consumeFixedSteps`).
   - Evidence: `src/engine/runtime.cpp` lines 317-349.

4. **Input is mixed event-driven + polling; edge handling partially guarded** (**Low**) 
   - Polling for movement, event for actions/UI; some keydown paths do not filter repeats except where explicitly checked.
   - Evidence: `src/engine/input_system.cpp` lines 5-46.
   - **Concrete fix**:
     - Add repeat filtering on menu navigation where desired determinism/UX requires edge-trigger only.

---

## 6) 🔵 Windows-specific issues

### Findings
1. **ANSI WinMain signature used; no wide-char path** (**Medium**)
   - Entry uses `int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)` and forwards `__argc/__argv`.
   - Evidence: `src/main.cpp` lines 106-108.
   - **Concrete fix**:
     - Prefer `wWinMain` + UTF-16 argument handling (or robust UTF-8 conversion) for Unicode-safe launch paths.

2. **No explicit DPI-awareness declaration in Win32 path** (**Medium**)
   - SDL high-DPI window flags are used, but no explicit Win32 DPI API call or manifest policy is visible in first-party entry.
   - Evidence: `src/main.cpp` and `src/engine/runtime.cpp` line 299.
   - **Concrete fix**:
     - Add manifest `PerMonitorV2` (preferred) and/or early DPI API enable call.

3. **No DLL export/import visibility macros for public API types** (**Low/Medium if packaged as DLL)**
   - Public API classes (`EngineHost`) do not use `__declspec(dllexport/dllimport)` abstraction.
   - Evidence: `include/engine/public/api.h` lines 18-31.
   - **Concrete fix**:
     - Introduce `ENGINE_PUBLIC_API` macro and apply to exported classes/functions.

---

## Summary table (issue count by category/severity)

| Category | Critical | High | Medium | Low | Info |
|---|---:|---:|---:|---:|---:|
| Stack/memory | 0 | 1 | 2 | 0 | 1 |
| OpenGL correctness | 0 | 0 | 1 | 2 | 2 |
| CMake | 0 | 0 | 2 | 1 | 2 |
| C++ / UB | 0 | 0 | 1 | 2 | 1 |
| Architecture | 0 | 0 | 2 | 1 | 1 |
| Windows-specific | 0 | 0 | 2 | 1 | 0 |
| **Total** | **0** | **1** | **10** | **7** | **7** |

---

## Prioritized fix order
1. **Guard GL bullet buffer indexing immediately** (`activeIndices` bounds) to prevent potential memory corruption/crash.
2. **Harden lifecycle/threading contracts** in `JobSystem` and plugin registry (shutdown-safe enqueue + registry synchronization/main-thread contract).
3. **Add GL debug/error instrumentation** (KHR_debug + critical-call validation) to reduce silent rendering failures.
4. **Establish MSVC warning policy** (`/W4 /WX /MP` and strict mode policy per target) and target-scoped deterministic flags.
5. **Windows productization fixes**: DPI awareness manifest and Unicode-safe entrypoint.
6. **Architectural decomposition** of `Runtime` into narrower services.
7. **Packaging hardening**: CMake `install()` + release export structure.

---

## Long-term architectural recommendations
- Introduce a **Platform Layer** (`platform/windows`, `platform/sdl`) to isolate entrypoint, DPI, argument normalization, and window creation policy.
- Formalize **render backend contracts** with typed frame data views (avoid raw pointer bundles).
- Replace singletons with explicit service composition in runtime bootstrap (logger, plugins, telemetry).
- Add a CI lane for **sanitizers/static analysis** (`ASan/UBSan` on Linux, `/analyze` on MSVC) and a strict warning gate for engine targets.
- Add render correctness tests that validate GL state transitions and resource lifetime in headless/mock contexts where possible.
