# ASSUMPTIONS: CPU Mass Bullet Render Path

## Scope Choice
Chosen scope: **Reposition (Path B)**.

The previous "GPU bullet" naming implied GPU-side simulation architecture that is not present in this codebase.
The runtime path is now explicitly documented as **CPU mass bullet render**:

- CPU simulation/update and quad generation.
- SDL geometry submission for batching.
- No compute-shader projectile simulation in this path.

## Determinism
- `CpuCollisionDeterministic` mode remains replay-authoritative and deterministic.
- `CpuMassRender` is currently a mass-render-oriented mode boundary and not wired as gameplay-authoritative collision logic.

## Collision Model
- CPU mass render bullets should be treated as presentation-path bullets unless explicitly integrated into collision ownership paths.

## Limits
- Capacity and preallocation support 100k+ bullets in stress tests (hardware/perf budget dependent).
- Update/render traversal now iterates compact active slots, avoiding full-capacity scan cost in hot loops.

## Slot Management Update
- Emission is O(1) via free-list.
- Active traversal is O(k) over active slots.
- Prepared quads are tracked for profiling via `preparedQuadCount()`.
