# Paste This Into Claude Code

Below is the complete prompt. Copy everything between the `---START---` and `---END---` markers. Paste it into Claude Code when you are `cd`'d into your HellEngine repo root.

Before pasting, make sure:
1. `docs/AuditReport.md` exists (the audit report file)
2. `CLAUDE.md` exists at repo root (the phased implementation plan)

---START---

Read the following files in this exact order, in full, before doing anything else. Do not summarize — internalize them as your working context:

1. `CLAUDE.md` — your phased implementation plan (10 phases, hard constraints, verification steps)
2. `docs/AuditReport.md` — the architecture audit that produced the plan
3. `docs/MasterSpec.md` — the authoritative engine specification
4. `docs/Architecture.md` — current module map
5. `docs/DecisionLog.md` — locked architectural decisions
6. `docs/ImplementationPlan.md` — phase history and milestone definitions
7. `docs/PerfTargets.md` — frame budgets and benchmark thresholds
8. `docs/BuildAndRun.md` — exact build/test/run commands

Now execute all 10 phases from `CLAUDE.md` sequentially. For each phase, follow this protocol:

## Per-Phase Protocol

### Before coding:
- Read every source file listed in that phase's instructions (`include/engine/...`, `src/engine/...`, `tests/...`).
- Identify what already exists that partially satisfies the phase. Do not duplicate or fight existing code.
- If the existing code already handles something the phase asks for, skip that sub-step and note it.

### While coding:
- Match existing code style exactly: `snake_case` filenames, `camelCase` methods, `camelCase_` private members, `PascalCase` types, `namespace engine { }` wrapping.
- Every new `.cpp` file must be added to the `engine_core` STATIC library in `CMakeLists.txt`.
- Every new test file must get an `add_executable` + `add_test` (or `catch_discover_tests` after Phase 7) entry in `CMakeLists.txt`.
- No `TODO`, no `FIXME`, no placeholder stubs. Every function must have a real implementation.
- No new third-party dependencies except Catch2 (Phase 7) and SDL_mixer (Phase 9).

### After coding each phase:
- Verify the build: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 && cmake --build build --config Release`
- Verify tests: `ctest --test-dir build -C Release --output-on-failure`
- Verify headless determinism: `.\build\Release\EngineDemo.exe --headless --ticks 600 --seed 42`
- If any verification fails, fix it before moving to the next phase.

### Documentation updates (MANDATORY for every phase):

After completing each phase's code changes, update ALL of the following docs to reflect what changed:

**`docs/DecisionLog.md`** — Add a dated entry for every architectural decision made during the phase. Use the existing format:
```
## YYYY-MM-DD — [Decision Title]
- **Context**: [why this decision was needed]
- **Decision**: [what was decided]
- **Rationale**: [why this choice over alternatives]
- **Status**: Accepted.
```
Examples of decisions worth logging:
- "Collision pipeline split into motion/build/resolve stages"
- "Fixed-capacity pending spawns array (4096) chosen over ring buffer"
- "dmath:: wrappers introduced as choke-point for future fixed-point migration"
- "Catch2 v3.5.2 adopted as test framework"

**`docs/Architecture.md`** — Update the module map and data flow sections to reflect new files and changed responsibilities. After Phase 1, the "Simulation core" section should list the new extracted systems (InputSystem, RenderPipeline, GameplaySession) instead of just "runtime.cpp". After Phase 2, add the collision pipeline stages. After Phase 9, add the audio system.

**`docs/MasterSpec.md`** — Update ONLY the sections that are materially affected by implementation decisions:
- Section 23 (Collision System): After Phase 2, update to reflect the actual collision pipeline stages, the `CollisionTarget`/`CollisionEvent` types, and the grid-query approach implemented.
- Section 14 (Memory and Performance): After Phase 6, document the fixed-capacity pending spawn limit (4096) and the zero-allocation contract enforcement approach.
- Section 4 (Deterministic Simulation Contract): After Phase 3, document the `/fp:strict` policy and the `dmath::` wrapper approach.
- Section 15 (Rendering Strategy): After Phase 4, update GPU bullet section to reflect the free-list fix and current capabilities.
- Section 27 (Implemented Runtime Baseline): After EVERY phase, update this section to reflect what is now implemented. This section is the ground-truth snapshot of what exists in the repo.
Do NOT rewrite sections that weren't affected. Append or amend, never delete spec content.

**`docs/ImplementationPlan.md`** — After each phase, add a completion entry following the existing format:
```
## Phase N — [Title] (Complete)
### What changed
- [bullet list of changes]

### Files added/edited
- [file list]

### Exact build/run commands (Windows)
[commands]
```

**`docs/PerfTargets.md`** — If any benchmark thresholds change (Phase 5's active-list optimization may improve numbers), update the target values and add a note explaining why they changed.

**`docs/BuildAndRun.md`** — If new test commands, run modes, or dependencies are added (Catch2 in Phase 7, SDL_mixer in Phase 9), add them to the appropriate sections.

### Git commit (MANDATORY after each phase):

After all code and doc updates for a phase are complete and verified, output the exact git commands:
```
git add -A
git commit -m "Phase N: [descriptive title] (Audit #X)

- [key change 1]
- [key change 2]
- [key change 3]

Docs updated: DecisionLog, Architecture, MasterSpec §X, ImplementationPlan"
```

## Phase Execution Order

Execute phases in this order (matches dependency chain from CLAUDE.md):

1. **Phase 1** — Decompose Runtime god object
2. **Phase 2** — Fix collision architecture for multi-target
3. **Phase 3** — Harden floating-point determinism
4. **Phase 4** — Fix GpuBulletSystem free-list
5. **Phase 5** — Compact active-list iteration
6. **Phase 6** — Eliminate hot-path allocations
7. **Phase 7** — Adopt Catch2 test framework
8. **Phase 8** — (CONDITIONAL) Deterministic math wrappers — only if Phase 3's cross-config test fails
9. **Phase 9** — Basic audio system
10. **Phase 10** — Content hot-reload

## Final Verification

After all phases are complete, run the full completion checklist from CLAUDE.md and report results:

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\EngineDemo.exe --headless --ticks 600 --seed 42
.\build\Release\benchmark_suite_tests.exe --assert-thresholds
```

Then confirm each checklist item:
- [ ] `runtime.cpp` is under 400 LOC
- [ ] Collision uses spatial grid queries with multi-target support
- [ ] `/fp:strict` set on MSVC for `engine_core`
- [ ] `dmath::` wrappers used for all trig in simulation code
- [ ] `GpuBulletSystem::emit()` is O(1)
- [ ] `ProjectileSystem` iterates only active bullets
- [ ] Zero heap allocations during `simTick()` in Release
- [ ] Catch2 integrated with property tests and fuzz tests
- [ ] Audio system exists and is presentation-only
- [ ] Hot-reload works for patterns, entities, traits, difficulty
- [ ] All tests pass
- [ ] Headless determinism test passes
- [ ] Benchmark thresholds pass
- [ ] DecisionLog, Architecture, MasterSpec, ImplementationPlan all updated

Begin with Phase 1 now.

---END---
