# Sample Project Usage

Use this guide to run the included authored sample as a reference implementation.

## Sample content set
Primary sample content is in `data/` and validated in `docs/SampleVerticalSlice.md`.

## 1) Build runtime and tools
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 4
```

## 2) Compile sample content pack
```bash
./build/ContentPacker --input data --output content.pak
```

## 3) Run sample interactively
```bash
./build/EngineDemo --content-pack content.pak
```

## 4) Run sample verification suite
```bash
./build/EngineDemo --headless --ticks 600 --content-pack content.pak
./build/EngineDemo --replay-verify --headless --ticks 7200 --seed 1337 --content-pack content.pak
```

## 5) Try layered mod pack override behavior
```bash
./build/EngineDemo --content-pack "mods/base.pak;mods/override.pak"
```

Later packs override earlier assets by GUID.

## 6) Where to inspect authored examples
- Patterns: `data/patterns.json`
- Entities and boss phases: `data/entities.json`
- Encounter pacing: `data/encounters.json`
- Audio bindings: `data/audio.json`
