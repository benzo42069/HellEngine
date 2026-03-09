# Audio Workflow for Content Creators

This guide covers how to author and validate runtime audio behavior.

## 1) Authoring source
Audio content is authored in JSON (typically `data/audio.json`) and compiled into `content.pak` by `ContentPacker`.

## 2) Runtime behavior model


Source-vs-runtime boundary reminder:
- `audio` blocks are source-authored input.
- Runtime consumes audio data only after `ContentPacker` compiles it into `content.pak` alongside import metadata (`importRegistry`, `animationBuild`, `variantBuild`, etc.).

- Runtime mixes audio through `AudioSystem`.
- Gameplay emits audio events (hit, graze, damage, enemy death, boss warning, UI click/confirm).
- Audio dispatch is presentation-side and not part of deterministic sim hashing.

## 3) Build + validate workflow
```bash
./build/ContentPacker --input data --output content.pak
./build/EngineDemo --content-pack content.pak
```

Headless replay validation can still be run in audio-enabled projects:
```bash
./build/EngineDemo --replay-verify --headless --ticks 1200 --seed 1337 --content-pack content.pak
```

## 4) Mix tuning from CLI
Use command-line overrides while tuning balance:
- `--audio-master <0..1>`
- `--audio-music <0..1>`
- `--audio-sfx <0..1>`

Example:
```bash
./build/EngineDemo --content-pack content.pak --audio-master 0.9 --audio-music 0.6 --audio-sfx 1.0
```

## 5) Creator tips
- Keep event names stable; avoid renaming without updating references.
- Validate boss warnings and phase transitions audibly in playtests.
- Treat missing audio assets as fix-now authoring errors even if runtime degrades gracefully.


## 6) Finalized event and bus model
- Buses are intentionally minimal and creator-facing: `master`, `music`, `sfx`.
- Event bindings are one-per-event (`hit`, `graze`, `player_damage`, `enemy_death`, `boss_warning`, `boss_phase_shift`, `defensive_special`, `run_clear`, `ui_click`, `ui_confirm`).
- Runtime maps gameplay presentation events into authored event bindings; sim code never directly plays clips.

## 7) Authoring validation and resilience rules
- Audio parse now fails fast for duplicate clip ids, duplicate event bindings, empty clip paths, and unknown clip references (including `audio.music`).
- Runtime still degrades gracefully for missing files at load time (`logWarn` + skip) so packs remain inspectable during iteration.
- On successful configure, looped authored `audio.music` is auto-started and kept in sync with bus volume updates.
