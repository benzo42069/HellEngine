# Code Audit & MasterSpec Update Snapshot (2026-03-06)

## Status summary

### Resolved in this cycle

1. **Determinism cross-config gate stabilized for supported toolchains.**
   - `determinism_cross_config_test` now validates two properties:
     - same-build repeatability (scenario run twice, hashes must match)
     - toolchain baseline hash match (MSVC/GCC/Clang baselines)
   - This keeps drift detection active while avoiding false failures from single-baseline assumptions.

2. **Editor tooling now uses honest capability gating for unavailable renderer paths.**
   - Palette/FX camera actions are no longer represented as fake-success actions.
   - Actions are disabled when the runtime capability is unavailable and a visible diagnostic is shown.
   - Preview text is updated to describe the real telemetry-driven behavior.

3. **Parallel-build robustness was improved in CMake dependency configuration.**
   - SDL FetchContent configuration is now constrained to shared-only (`SDL_SHARED=ON`, `SDL_STATIC=OFF`) and test targets are disabled (`SDL_TEST=OFF`).
   - This removes the static archive production path that previously exhibited intermittent parallel archive-write failures in this environment.

### Remaining follow-up (safe defer)

1. **Renderer-integrated camera post-stack application remains backend-dependent.**
   - The current SDL2 renderer backend does not expose the required post-stack hook for direct camera preset application.
   - UI now reports this explicitly and safely disables unavailable actions.

## MasterSpec alignment (current)

1. **Deterministic floating-point policy remains enforced in build flags.**
   - `engine_core` compiles with `/fp:strict` on MSVC and `-ffp-contract=off -fno-fast-math` on GCC/Clang.

2. **Deterministic math wrappers remain in active simulation modules.**
   - Simulation-oriented modules continue using `deterministic_math.h` wrappers.

3. **Determinism test wiring is in normal CTest execution.**
   - `determinism_cross_config_test` is compiled and registered in default test runs.

4. **Layered module boundaries and broad test coverage remain intact.**
   - Runtime/simulation/gameplay/render/tooling modules and broad system test coverage are preserved.
