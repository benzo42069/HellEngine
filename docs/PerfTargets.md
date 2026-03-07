# Performance Targets

These targets are intended for profiling in both Development and Release builds.

## Frame Budgets
- Simulation budget: **8.0 ms**
- Collision budget: **2.0 ms**
- Render budget: **4.0 ms**

Warnings are raised in the Profiler Overview panel and Perf HUD when a budget is exceeded.

## Diagnostics Counters
The profiler reports:
- Active bullets
- Bullet spawn rate (spawns/second)
- Broadphase checks per tick
- Narrowphase checks per tick
- Draw calls submitted
- Render batch flushes

## Benchmark Suite Thresholds (CI Local)

`benchmark_suite_tests --assert-thresholds` enforces the following max wall times:

- 10k bullets stress (80 ticks): **<= 250 ms**
- 25k bullets stress (60 ticks): **<= 450 ms**
- 50k bullets stress (40 ticks): **<= 700 ms**
- collision-heavy scene (25k bullets, 60 ticks): **<= 500 ms**
- render-heavy scene (50k sprites/frame, 30 frames): **<= 2200 ms**

These are guard-rail thresholds for regression detection in local CI, not absolute hardware-independent performance claims.

## One-command Local Health Check

```powershell
./tools/ci_local.ps1
```

This script runs:
1. configure + build
2. ctest suite
3. replay verify run
4. benchmark threshold checks

## Usage
- Open **Control Center** and inspect:
  - Profiler Overview
  - Bullet/Collision Diagnostics
  - Render Stats
- Toggle in-game Perf HUD with **F10**.


## Phase 5 active-list optimization note
- `ProjectileSystem` hot loops now iterate over compact active indices instead of scanning full capacity with active checks.
- Keep current CI thresholds unchanged unless repeated benchmark runs show stable margin gains across environments.

## GPU Bullet Rendering Target
- OpenGL bullet path must render **10,000 active bullets in a single draw call**.
- Target GPU time for bullet pass: **< 1.0 ms** on the reference profiling machine.
- CPU-side buffer rebuild remains authoritative each frame but must avoid per-frame allocations (preallocated max-capacity buffers only).
