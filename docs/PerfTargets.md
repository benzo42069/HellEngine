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


## CPU Mass Bullet Render Path Targets
- Classification: CPU-driven mass-bullet simulation + CPU-side quad preparation + batched renderer submission.
- Scale target: sustain 100k active visual bullets in stress test updates without allocation churn (hardware dependent for frame time).
- Profiling counters to monitor:
  - `activeCount()` for live bullets.
  - `preparedQuadCount()` for render prep output volume.
- This path is non-authoritative for gameplay collision/replay until a dedicated hybrid ownership model is implemented.


## Additional Runtime Targets (Phase 7)
- GL bullet renderer: **10,000 bullets < 1.0 ms** GPU time on reference machine.
- Post-processing chain (bloom + vignette + composite): **all passes combined < 2.0 ms at 1080p**.
- Content hot-reload responsiveness:
  - detection latency: **< 100 ms**
  - runtime swap/apply latency: **< 50 ms**
