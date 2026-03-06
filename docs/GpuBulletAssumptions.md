# ASSUMPTIONS: GPU-Driven Bullets (Upgrade 9)

## Scope Choice
Chosen scope: **Option 2 (Hybrid)** adapted to the current backend.

The current engine backend is SDL2 + `SDL_RenderGeometry` and does not expose D3D11/D3D12 compute shaders directly in this codebase.
To keep the feature safe and shippable without backend rewrite, this patch implements a **GPU-mass hybrid mode**:

- CPU deterministic path remains unchanged and is replay reference.
- GPU-hybrid path uses packed bullet buffers and bulk geometry rendering for very high visual bullet counts.

## Determinism
- **CPU mode**: fully deterministic (unchanged).
- **GPU-hybrid mode**: best-effort deterministic visual simulation only; not replay-authoritative.

## Collision Model
- GPU-hybrid bullets are currently **visual-only** (no per-bullet collision participation in gameplay logic).
- This is intentional for safety/performance in mass-scale mode.

## Limits
- Targeted for high bullet counts (100k+ hardware permitting).
- Final throughput depends on renderer driver and hardware.

## Slot Management Update (Phase 4)
- Emission no longer linearly scans for inactive entries.
- Runtime uses free-list slot allocation and cached active count in `GpuBulletSystem`.
- Complexity: `emit()` O(1), `activeCount()` O(1), expiry reclaim in `update()` O(k) over bullets processed that frame.
