#pragma once

#include <cmath>

namespace engine::dmath {

inline float sin(const float x) { return std::sin(x); }
inline float cos(const float x) { return std::cos(x); }
inline float atan2(const float y, const float x) { return std::atan2(y, x); }
inline float sqrt(const float x) { return std::sqrt(x); }

} // namespace engine::dmath
