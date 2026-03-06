#include <engine/replay.h>

#include <engine/content_pipeline.h>
#include <engine/deterministic_rng.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace engine {

namespace {
std::uint64_t mix64(std::uint64_t h, std::uint64_t v) {
    h ^= v + 0x9E3779B97F4A7C15ULL + (h << 6U) + (h >> 2U);
    return h;
}
} // namespace

void ReplayRecorder::begin(const std::uint64_t simulationSeed, std::string contentVersion, std::string contentHash, const std::uint32_t hashPeriodTicks) {
    data_ = {};
    data_.header.simulationSeed = simulationSeed;
    data_.header.contentVersion = std::move(contentVersion);
    data_.header.contentHash = std::move(contentHash);
    data_.header.hashPeriodTicks = std::max(hashPeriodTicks, 1U);
}

void ReplayRecorder::recordTickInput(const std::uint32_t inputMask) {
    data_.inputsPerTick.push_back(inputMask);
}

void ReplayRecorder::recordStateSample(const ReplayStateSample& sample) {
    data_.stateSamples.push_back(sample);
}

const ReplayData& ReplayRecorder::data() const { return data_; }

bool ReplayRecorder::save(const std::string& path) const {
    nlohmann::json j;
    j["header"] = {
        {"simulationSeed", data_.header.simulationSeed},
        {"contentVersion", data_.header.contentVersion},
        {"contentHash", data_.header.contentHash},
        {"hashPeriodTicks", data_.header.hashPeriodTicks},
    };
    j["inputsPerTick"] = data_.inputsPerTick;
    j["stateSamples"] = nlohmann::json::array();
    for (const ReplayStateSample& s : data_.stateSamples) {
        j["stateSamples"].push_back({
            {"tick", s.tick},
            {"totalHash", s.totalHash},
            {"bulletsHash", s.bulletsHash},
            {"entitiesHash", s.entitiesHash},
            {"runStateHash", s.runStateHash},
        });
    }

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
    data_.header.contentHash = h.value("contentHash", std::string {});
    data_.header.hashPeriodTicks = h.value("hashPeriodTicks", 60U);

    if (j.contains("inputsPerTick") && j["inputsPerTick"].is_array()) {
        for (const auto& v : j["inputsPerTick"]) data_.inputsPerTick.push_back(v.get<std::uint32_t>());
    }
    if (j.contains("stateSamples") && j["stateSamples"].is_array()) {
        for (const auto& v : j["stateSamples"]) {
            ReplayStateSample sample;
            sample.tick = v.value("tick", 0ULL);
            sample.totalHash = v.value("totalHash", 0ULL);
            sample.bulletsHash = v.value("bulletsHash", 0ULL);
            sample.entitiesHash = v.value("entitiesHash", 0ULL);
            sample.runStateHash = v.value("runStateHash", 0ULL);
            data_.stateSamples.push_back(sample);
        }
    }
    return true;
}

bool ReplayPlayer::validFor(const std::uint64_t simulationSeed, const std::string& contentVersion, const std::string& contentHash) const {
    return data_.header.simulationSeed == simulationSeed
        && data_.header.contentVersion == contentVersion
        && data_.header.contentHash == contentHash;
}

std::uint32_t ReplayPlayer::inputForTick(const std::uint64_t tick) const {
    if (tick >= data_.inputsPerTick.size()) return 0;
    return data_.inputsPerTick[static_cast<std::size_t>(tick)];
}

bool ReplayPlayer::verifyStateSample(const ReplayStateSample& sample, ReplayMismatch* mismatch) const {
    for (const ReplayStateSample& expected : data_.stateSamples) {
        if (expected.tick != sample.tick) continue;

        auto setMismatch = [&](ReplaySubsystem subsystem, std::uint64_t exp, std::uint64_t act) {
            if (mismatch) {
                mismatch->tick = sample.tick;
                mismatch->subsystem = subsystem;
                mismatch->expected = exp;
                mismatch->actual = act;
            }
        };

        if (expected.totalHash != sample.totalHash) {
            if (expected.bulletsHash != sample.bulletsHash) {
                setMismatch(ReplaySubsystem::Bullets, expected.bulletsHash, sample.bulletsHash);
            } else if (expected.entitiesHash != sample.entitiesHash) {
                setMismatch(ReplaySubsystem::Entities, expected.entitiesHash, sample.entitiesHash);
            } else if (expected.runStateHash != sample.runStateHash) {
                setMismatch(ReplaySubsystem::RunState, expected.runStateHash, sample.runStateHash);
            } else {
                setMismatch(ReplaySubsystem::None, expected.totalHash, sample.totalHash);
            }
            return false;
        }

        return true;
    }
    return true;
}

std::uint32_t ReplayPlayer::hashPeriodTicks() const {
    return std::max(data_.header.hashPeriodTicks, 1U);
}

std::string buildContentVersionTag(const std::string& contentPackPath) {
    return std::string("pack:") + contentPackPath;
}

std::string buildContentHashTag(const std::string& contentPackPath) {
    const std::vector<std::string> packs = splitPackPaths(contentPackPath);
    std::uint64_t h = 1469598103934665603ULL;
    for (const std::string& p : packs) {
        std::ifstream in(p, std::ios::binary);
        if (!in.good()) {
            h = mix64(h, stableHash64(p));
            continue;
        }
        std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        h = mix64(h, stableHash64(p));
        h = mix64(h, stableHash64(bytes));
    }
    return fnv1a64Hex(std::to_string(h));
}

std::uint64_t computeReplayStateHash(
    const std::uint64_t tick,
    const std::uint64_t bulletsHash,
    const std::uint64_t entitiesHash,
    const std::uint64_t runStateHash
) {
    std::uint64_t h = 1469598103934665603ULL;
    h = mix64(h, tick);
    h = mix64(h, bulletsHash);
    h = mix64(h, entitiesHash);
    h = mix64(h, runStateHash);
    return h;
}

const char* toString(const ReplaySubsystem subsystem) {
    switch (subsystem) {
        case ReplaySubsystem::Bullets: return "bullets";
        case ReplaySubsystem::Entities: return "entities";
        case ReplaySubsystem::RunState: return "run_state";
        case ReplaySubsystem::None:
        default: return "aggregate";
    }
}

} // namespace engine
