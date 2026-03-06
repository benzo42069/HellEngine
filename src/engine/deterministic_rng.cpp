#include <engine/deterministic_rng.h>

#include <string_view>

namespace engine {

namespace {
std::uint64_t fnv1a64(std::string_view text) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : text) {
        hash ^= static_cast<std::uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}
} // namespace

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

std::uint64_t stableHash64(const std::string_view text) { return fnv1a64(text); }

RngStreams::RngStreams(const std::uint64_t runSeed)
    : runSeed_(runSeed) {}

DeterministicRng& RngStreams::stream(const std::string& name) {
    auto iter = streams_.find(name);
    if (iter != streams_.end()) {
        return iter->second;
    }

    const std::uint64_t seed = runSeed_ ^ stableHash64(name);
    auto [insertedIter, _] = streams_.emplace(name, DeterministicRng(seed));
    return insertedIter->second;
}

} // namespace engine
