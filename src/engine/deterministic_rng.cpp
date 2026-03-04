#include <engine/deterministic_rng.h>

#include <functional>

namespace engine {

DeterministicRng::DeterministicRng(const std::uint64_t seed)
    : state_(seed == 0 ? 0x9E3779B97F4A7C15ULL : seed) {}

std::uint64_t DeterministicRng::nextU64() {
    state_ += 0x9E3779B97F4A7C15ULL;
    std::uint64_t z = state_;
    z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31U);
}

float DeterministicRng::nextFloat01() {
    constexpr double maxInv = 1.0 / static_cast<double>(UINT64_MAX);
    return static_cast<float>(static_cast<double>(nextU64()) * maxInv);
}

RngStreams::RngStreams(const std::uint64_t runSeed)
    : runSeed_(runSeed) {}

DeterministicRng& RngStreams::stream(const std::string& name) {
    auto iter = streams_.find(name);
    if (iter != streams_.end()) {
        return iter->second;
    }

    const std::uint64_t seed = runSeed_ ^ static_cast<std::uint64_t>(std::hash<std::string> {}(name));
    auto [insertedIter, _] = streams_.emplace(name, DeterministicRng(seed));
    return insertedIter->second;
}

} // namespace engine
