#include <engine/deterministic_math.h>

#include <algorithm>
#include <limits>
#include <numbers>

namespace engine::dmath {

namespace {
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kHalfPi = std::numbers::pi_v<float> * 0.5F;
constexpr float kTwoPi = std::numbers::pi_v<float> * 2.0F;

float wrapSignedPi(float x) {
    while (x > kPi) x -= kTwoPi;
    while (x < -kPi) x += kTwoPi;
    return x;
}
} // namespace

float sin(float x) {
    x = wrapSignedPi(x);
    const float x2 = x * x;
    const float x3 = x2 * x;
    const float x5 = x3 * x2;
    const float x7 = x5 * x2;
    return x - (x3 / 6.0F) + (x5 / 120.0F) - (x7 / 5040.0F);
}

float cos(float x) {
    return sin(x + kHalfPi);
}

float atan2(float y, float x) {
    if (x == 0.0F) {
        if (y > 0.0F) return kHalfPi;
        if (y < 0.0F) return -kHalfPi;
        return 0.0F;
    }

    const float absY = std::abs(y) + 1e-10F;
    float angle = 0.0F;
    if (x >= 0.0F) {
        const float r = (x - absY) / (x + absY);
        const float r2 = r * r;
        angle = 0.78539816339F - 0.78539816339F * r + 0.1821F * r * (1.0F - r2);
    } else {
        const float r = (x + absY) / (absY - x);
        const float r2 = r * r;
        angle = 2.35619449019F - 0.78539816339F * r + 0.1821F * r * (1.0F - r2);
    }
    return y < 0.0F ? -angle : angle;
}

float sqrt(float x) {
    if (x <= 0.0F) return 0.0F;
    float g = x > 1.0F ? x : 1.0F;
    g = 0.5F * (g + x / g);
    g = 0.5F * (g + x / g);
    g = 0.5F * (g + x / g);
    return g;
}

} // namespace engine::dmath
