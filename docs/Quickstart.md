# Quickstart

This is the fastest path from clone to playable build with custom content.

## 1) Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 4
```

## 2) (Recommended) Build a content pack

The runtime default config points to `content.pak`. Generate it from local JSON content:

```bash
./build/ContentPacker --input data --output content.pak
```

Or build from the example packs in this repo:

```bash
./build/ContentPacker --input examples/content_packs --output content.pak --pack-id starter
```

## 3) Run

```bash
./build/EngineDemo
```

Headless smoke run:

```bash
./build/EngineDemo --headless --ticks 300
```

## 4) Verify determinism

```bash
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

## 5) Edit content safely

- Edit files in `data/` or `examples/content_packs/`.
- Rebuild `content.pak` with `ContentPacker`.
- Relaunch or hot reload in runtime/editor tools.
- Malformed files are reported via structured `error_report=...` logs and runtime falls back to last-good/default content.

## 6) Useful runtime toggles

- `` ` ``: debug HUD on/off
- `F10`: perf HUD on/off
- `F11`: compiled graph VM on/off
- `F12`: bullet simulation mode toggle (CpuCollisionDeterministic / CpuMassRender when enabled by runtime wiring)

