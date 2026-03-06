# How to Use This Prompt with Claude Code

## Setup (One-Time)

1. **Place the audit report in your repo:**
   ```bash
   cp HellEngine_Audit_Report.md  your-repo/docs/AuditReport.md
   git add docs/AuditReport.md && git commit -m "Add architecture audit report"
   ```

2. **Place the prompt file at your repo root:**
   ```bash
   cp HellEngine_ClaudeCode_Prompt.md  your-repo/CLAUDE.md
   ```
   Claude Code automatically reads `CLAUDE.md` from the repo root as its
   project instructions. This is the single most important step — it gives
   Claude Code persistent context across every command you run.

## Running Phase by Phase

The prompt is designed for **one phase at a time**. This is deliberate — large
monolithic refactors fail because Claude loses context. Phase-by-phase keeps
each change reviewable and verifiable.

### Recommended workflow:

```bash
# Phase 1: Decompose the god object
claude "Execute Phase 1 from CLAUDE.md. Read the referenced files first, then implement Steps 1.1 through 1.5. Verify compilation and tests at the end."

# Review the diff, test manually, then commit
git add -A && git commit -m "Phase 1: Decompose Runtime god object (Audit #1)"

# Phase 2: Fix collision
claude "Execute Phase 2 from CLAUDE.md. Read projectiles.h and projectiles.cpp first."

git add -A && git commit -m "Phase 2: Multi-target collision via spatial grid (Audit #2)"

# ... continue for each phase
```

### If a phase is too large:

```bash
claude "Execute Phase 1, Step 1.1 only — Extract InputSystem."
# review, then:
claude "Continue Phase 1, Step 1.2 — Extract RenderPipeline."
```

### If something breaks:

```bash
claude "Phase 2 broke the replay_tests. The hash changed. Read the test output, find what changed in collision ordering, and fix it. Do NOT change the test expectations — fix the implementation to preserve determinism."
```

## Tips for Best Results

### DO:
- **Run one phase per session.** Start a fresh Claude Code session for each
  phase if you hit context limits.
- **Let Claude read the files.** The prompt tells it to read specific files
  before coding. If you skip this, it will hallucinate API shapes.
- **Commit between phases.** This gives you rollback points and keeps
  Claude's diffs clean.
- **Paste compiler errors verbatim.** If the build fails, paste the full
  error into Claude Code. It's very good at fixing its own mistakes.
- **Use the verification commands.** Each phase ends with explicit build +
  test commands. Run them.

### DON'T:
- **Don't say "implement the whole audit."** That's 10 phases of work. It
  will lose context and produce broken code. One phase at a time.
- **Don't skip the reference-file reads.** If Claude doesn't read
  `runtime.h` before decomposing `Runtime`, it will invent wrong member
  names.
- **Don't let it rename existing files.** The prompt says "additive only"
  but reinforce this if Claude suggests renaming `projectiles.h` to
  `bullet_world.h` or similar.
- **Don't let it add dependencies you didn't ask for.** The prompt allows
  Catch2 and SDL_mixer only. Push back on anything else.

## Phase Dependency Order

```
Phase 1 (Decompose Runtime)
    └─► Phase 2 (Collision) — needs clean simTick()
        └─► Phase 5 (Active List) — builds on Phase 2's separated motion/collision
            └─► Phase 6 (Hot-Path Allocs) — needs Phase 5's fixed arrays
Phase 3 (FP Determinism) — independent, can run in parallel with Phase 1
Phase 4 (GPU Bullets) — independent
Phase 7 (Catch2) — independent, but benefits from Phases 2+5 (more code to test)
Phase 8 (Math Wrappers) — conditional, only if Phase 3 fails
Phase 9 (Audio) — independent, after Phase 1 ideally
Phase 10 (Hot-Reload) — after Phase 1
```

Minimum viable order for maximum impact:
**1 → 2 → 3 → 5 → 6 → 7**

Phases 4, 8, 9, 10 can be deferred or done in any order.

## Customization

If you want to change priorities, edit the `CLAUDE.md` file directly.
For example, to skip the audio system and prioritize performance:

- Delete Phase 9
- In the completion checklist, remove the audio line
- Renumber if you care about tidiness (optional — Claude handles gaps fine)

If you want to add your own constraints, add them to the
`HARD CONSTRAINTS` section. For example:

```markdown
7. **No SDL_mixer.** Use miniaudio instead (header-only, no FetchContent needed).
```
