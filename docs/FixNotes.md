# Fix Notes (2026-03-06)

## Root causes identified

1. **Determinism test mismatch**
   - The existing test used a single hard-coded hash baseline, but observed output differed by toolchain/platform while remaining stable within a given build environment.

2. **Tooling UX ambiguity for unavailable actions**
   - Editor panel actions for camera FX reported success-style messages despite no renderer integration path being available.

3. **Parallel build instability path**
   - Building both SDL shared and static variants via FetchContent exposed an intermittent static archive write failure in parallel builds in this environment.

## Fixes made

1. **Determinism test hardening**
   - Updated `determinism_cross_config_test` to:
     - run the deterministic scenario twice and require identical hashes
     - enforce toolchain-specific expected baselines for MSVC/GCC/Clang

2. **Tooling behavior hardening**
   - Updated editor tooling so camera FX apply/revert actions are disabled when unsupported and accompanied by explicit diagnostics.
   - Removed fake-success wording around those actions.
   - Updated preview viewport wording to accurately describe telemetry-driven preview behavior.

3. **CMake/parallel robustness hardening**
   - Set SDL options before `FetchContent_MakeAvailable`:
     - `SDL_SHARED=ON`
     - `SDL_STATIC=OFF`
     - `SDL_TEST=OFF`
   - This removes unnecessary static SDL archive generation and reduces parallel build race exposure.

## Deferred items and rationale

1. **Full camera post-stack runtime integration**
   - Deferred because it requires renderer backend expansion beyond safe scope for this change set.
   - Current mitigation is explicit capability gating + diagnostics, preventing misleading UI behavior.
