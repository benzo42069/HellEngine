# Quickstart

For external onboarding, use `docs/GettingStarted.md`.

This page remains a compact command-only variant.

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 4
```

## Build content pack

```bash
./build/ContentPacker --input data --output content.pak
```

## Run

```bash
./build/EngineDemo --content-pack content.pak
```

## Headless smoke

```bash
./build/EngineDemo --headless --ticks 300 --content-pack content.pak
```

## Replay verify

```bash
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```
