#include <engine/replay.h>

#include <nlohmann/json.hpp>

#include <fstream>

namespace engine {

void ReplayRecorder::begin(const std::uint64_t simulationSeed, std::string contentVersion) {
    data_ = {};
    data_.header.simulationSeed = simulationSeed;
    data_.header.contentVersion = std::move(contentVersion);
}

void ReplayRecorder::recordTick(const std::uint32_t inputMask, const std::uint64_t stateHash) {
    data_.inputsPerTick.push_back(inputMask);
    data_.stateHashes.push_back(stateHash);
}

const ReplayData& ReplayRecorder::data() const { return data_; }

bool ReplayRecorder::save(const std::string& path) const {
    nlohmann::json j;
    j["header"] = {
        {"simulationSeed", data_.header.simulationSeed},
        {"contentVersion", data_.header.contentVersion},
    };
    j["inputsPerTick"] = data_.inputsPerTick;
    j["stateHashes"] = data_.stateHashes;

    std::ofstream out(path);
    if (!out.good()) return false;
    out << j.dump(2);
    return true;
}

bool ReplayPlayer::load(const std::string& path) {
    std::ifstream in(path);
    if (!in.good()) return false;

    nlohmann::json j;
    in >> j;

    data_ = {};
    if (!j.contains("header") || !j["header"].is_object()) return false;
    const auto& h = j["header"];
    data_.header.simulationSeed = h.value("simulationSeed", 0ULL);
    data_.header.contentVersion = h.value("contentVersion", std::string {});

    if (j.contains("inputsPerTick") && j["inputsPerTick"].is_array()) {
        for (const auto& v : j["inputsPerTick"]) data_.inputsPerTick.push_back(v.get<std::uint32_t>());
    }
    if (j.contains("stateHashes") && j["stateHashes"].is_array()) {
        for (const auto& v : j["stateHashes"]) data_.stateHashes.push_back(v.get<std::uint64_t>());
    }
    return true;
}

bool ReplayPlayer::validFor(const std::uint64_t simulationSeed, const std::string& contentVersion) const {
    return data_.header.simulationSeed == simulationSeed && data_.header.contentVersion == contentVersion;
}

std::uint32_t ReplayPlayer::inputForTick(const std::uint64_t tick) const {
    if (tick >= data_.inputsPerTick.size()) return 0;
    return data_.inputsPerTick[static_cast<std::size_t>(tick)];
}

bool ReplayPlayer::verifyTick(const std::uint64_t tick, const std::uint64_t stateHash) const {
    if (tick >= data_.stateHashes.size()) return true;
    return data_.stateHashes[static_cast<std::size_t>(tick)] == stateHash;
}

std::string buildContentVersionTag(const std::string& contentPackPath) {
    return std::string("phase17:") + contentPackPath;
}

std::uint64_t computeReplayStateHash(
    const std::uint64_t tick,
    const std::uint32_t projectiles,
    const std::uint32_t entities,
    const std::uint32_t collisions,
    const float currency
) {
    std::uint64_t h = 1469598103934665603ULL;
    auto mix = [&](const std::uint64_t v) {
        h ^= v;
        h *= 1099511628211ULL;
    };
    mix(tick);
    mix(projectiles);
    mix(entities);
    mix(collisions);
    mix(static_cast<std::uint64_t>(currency * 100.0F));
    return h;
}

} // namespace engine
