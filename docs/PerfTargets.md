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

## Usage
- Open **Control Center** and inspect:
  - Profiler Overview
  - Bullet/Collision Diagnostics
  - Render Stats
- Toggle in-game Perf HUD with **F10**.
