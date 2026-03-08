#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace engine {

enum class AudioBus : std::uint8_t {
    Master = 0,
    Music,
    Sfx,
};

enum class AudioEventId : std::uint8_t {
    Hit = 0,
    Graze,
    PlayerDamage,
    EnemyDeath,
    BossWarning,
    UiClick,
    UiConfirm,
};

struct AudioClipRecord {
    std::string id;
    std::string path;
    AudioBus bus {AudioBus::Sfx};
    bool loop {false};
    float baseGain {1.0F};
};

struct AudioEventBinding {
    AudioEventId event {AudioEventId::Hit};
    std::string clipId;
    float gain {1.0F};
};

struct AudioContentDatabase {
    std::vector<AudioClipRecord> clips;
    std::vector<AudioEventBinding> events;
    std::string musicClipId;
};

} // namespace engine
