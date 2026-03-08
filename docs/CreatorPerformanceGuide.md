# Performance Guidance for Content Creators

Use this guide to keep content readable and performant before release.

## 1) Creator performance goals
- Maintain readable projectile fields.
- Avoid authored spike patterns that cause avoidable frame-time jumps.
- Keep replay verification stable for final packs.

## 2) Pattern authoring guidance
- Start low and scale: increase bullet count in small steps.
- Prefer repeated cadence over one-shot giant bursts.
- In graph loops, always include spacing (`wait`) when density rises.
- Reserve peak density for short windows.

## 3) Encounter/boss pacing guidance
- Ramp pressure over phases/waves, don’t front-load maximum intensity.
- Pair intense sections with short recovery windows.
- Use telegraph windows before major cadence changes.

## 4) Asset and shader guidance
- Group import atlases intentionally with `atlasGroup`.
- Keep animation clip FPS consistent within a clip.
- Use grayscale/monochrome + palette workflows for recolors instead of duplicating full-color variants.

## 5) Validation commands to run before shipping
```bash
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

In visual runs, use debug toggles:
- `F10` perf HUD
- `` ` `` debug HUD

## 6) Red flags
- Replay verify intermittently fails with unchanged seed and pack.
- One authored phase causes persistent perf drops.
- Content pack emits repeated validation/fallback warnings.
- High-density patterns are unreadable without telegraph or spacing.
