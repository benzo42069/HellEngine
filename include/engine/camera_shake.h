#pragma once

#include <engine/math_types.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace engine {

enum class ShakeProfile : std::uint8_t {
    Impact,
    BossRumble,
    GrazeTremor,
    SpecialPulse,
    Explosion,
    Ambient,
};

struct ShakeParams {
    ShakeProfile profile {ShakeProfile::Impact};
    float amplitude {2.0F};
    float duration {0.15F};
    Vec2 direction {0.0F, 0.0F};
    float frequency {30.0F};
    float damping {8.0F};
};

class CameraShakeSystem {
  public:
    [[nodiscard]] Vec2 update(float dt);
    void trigger(const ShakeParams& params);
    void clear();

  private:
    static constexpr int kMaxActive = 4;
    static constexpr float kMaxAxisOffset = 20.0F;

    struct ActiveShake {
        ShakeParams params;
        float elapsed {0.0F};
        bool active {false};
    };

    [[nodiscard]] static Vec2 normalized(Vec2 v);
    [[nodiscard]] static float noise(float value);
    [[nodiscard]] static Vec2 profileOffset(const ActiveShake& shake);

    std::array<ActiveShake, kMaxActive> shakes_ {};
    std::size_t nextReplaceIndex_ {0};
};

} // namespace engine
