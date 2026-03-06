#include <engine/patterns.h>
#include <engine/deterministic_math.h>

#include <engine/content_pipeline.h>
#include <engine/diagnostics.h>
#include <engine/logging.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numbers>
#include <unordered_map>

namespace engine {

namespace {
PatternType parsePatternType(const std::string& type) {
    if (type == "radial") return PatternType::RadialBurst;
    if (type == "spiral") return PatternType::Spiral;
    if (type == "spread") return PatternType::Spread;
    if (type == "wave") return PatternType::Wave;
    if (type == "aimed") return PatternType::Aimed;
    return PatternType::RadialBurst;
}

float degToRad(const float deg) { return deg * std::numbers::pi_v<float> / 180.0F; }

Vec2 velocityFromDeg(const float deg, const float speed) {
    const float rad = degToRad(deg);
    return Vec2 {dmath::cos(rad) * speed, dmath::sin(rad) * speed};
}

PatternLayer parseLayer(const nlohmann::json& item) {
    PatternLayer layer;
    layer.name = item.value("name", "layer");
    layer.type = parsePatternType(item.value("type", "radial"));
    layer.bulletCount = item.value("bulletCount", 24U);
    layer.bulletSpeed = item.value("bulletSpeed", 120.0F);
    layer.projectileRadius = item.value("projectileRadius", 3.0F);
    layer.cooldownSeconds = item.value("cooldownSeconds", 0.15F);
    layer.angleStepDeg = item.value("angleStepDeg", 8.0F);
    layer.spreadAngleDeg = item.value("spreadAngleDeg", 35.0F);
    layer.waveAmplitudeDeg = item.value("waveAmplitudeDeg", 20.0F);
    layer.waveFrequencyHz = item.value("waveFrequencyHz", 1.5F);
    layer.aimJitterDeg = item.value("aimJitterDeg", 0.0F);


    if (item.contains("behavior") && item["behavior"].is_object()) {
        const auto& b = item["behavior"];
        layer.projectileBehavior.homingTurnRateDegPerSec = b.value("homingTurnRateDegPerSec", 0.0F);
        layer.projectileBehavior.homingMaxAngleStepDeg = b.value("homingMaxAngleStepDeg", 0.0F);
        layer.projectileBehavior.curvedAngularVelocityDegPerSec = b.value("curvedAngularVelocityDegPerSec", 0.0F);
        layer.projectileBehavior.accelerationPerSec = b.value("accelerationPerSec", 0.0F);
        layer.projectileBehavior.dragPerSec = b.value("dragPerSec", 0.0F);
        layer.projectileBehavior.maxBounces = b.value("maxBounces", 0U);
        layer.projectileBehavior.splitCount = b.value("splitCount", 0U);
        layer.projectileBehavior.splitAngleSpreadDeg = b.value("splitAngleSpreadDeg", 0.0F);
        layer.projectileBehavior.splitDelaySeconds = b.value("splitDelaySeconds", 0.0F);
        layer.projectileBehavior.explodeRadius = b.value("explodeRadius", 0.0F);
        layer.projectileBehavior.explodeShards = b.value("explodeShards", 0U);
        layer.projectileBehavior.beamSegmentSamples = b.value("beamSegmentSamples", 0U);
        layer.projectileBehavior.beamDurationSeconds = b.value("beamDurationSeconds", 0.0F);
    }
    if (item.contains("modifiers")) {
        const auto& m = item["modifiers"];
        layer.modifiers.rotationDeg = m.value("rotationDeg", 0.0F);
        layer.modifiers.phaseOffsetDeg = m.value("phaseOffsetDeg", 0.0F);
        layer.modifiers.speedCurveAmp = m.value("speedCurveAmp", 0.0F);
        layer.modifiers.angleOscillationDeg = m.value("angleOscillationDeg", 0.0F);
        layer.modifiers.angleOscillationHz = m.value("angleOscillationHz", 1.0F);
        layer.modifiers.deterministicJitterDeg = m.value("deterministicJitterDeg", 0.0F);
    }

    if (item.contains("timeline") && item["timeline"].is_array()) {
        for (const auto& t : item["timeline"]) {
            layer.timeline.push_back(PatternTimelineEvent {
                .timeSeconds = t.value("timeSeconds", 0.0F),
                .extraRotationDeg = t.value("extraRotationDeg", 0.0F),
                .speedScale = t.value("speedScale", 1.0F),
            });
        }
        std::sort(layer.timeline.begin(), layer.timeline.end(), [](const PatternTimelineEvent& a, const PatternTimelineEvent& b) {
            return a.timeSeconds < b.timeSeconds;
        });
    }

    if (item.contains("subpatterns") && item["subpatterns"].is_array()) {
        for (const auto& s : item["subpatterns"]) {
            layer.subpatterns.push_back(PatternSubpatternRef {
                .layerIndex = s.value("layerIndex", 0U),
                .phaseOffsetDeg = s.value("phaseOffsetDeg", 0.0F),
            });
        }
    }

    return layer;
}

PatternDefinition parsePatternDefinition(const nlohmann::json& item) {
    PatternDefinition def;
    def.name = item.value("name", "unnamed");
    def.seedOffset = item.value("seedOffset", 0ULL);
    def.loopSequence = item.value("loopSequence", true);

    if (item.contains("layers") && item["layers"].is_array()) {
        for (const auto& layerJson : item["layers"]) {
            def.layers.push_back(parseLayer(layerJson));
        }
    } else {
        // Backward-compatible single-layer parsing.
        def.layers.push_back(parseLayer(item));
    }

    if (item.contains("sequence") && item["sequence"].is_array()) {
        for (const auto& stepJson : item["sequence"]) {
            PatternSequenceStep step;
            step.durationSeconds = stepJson.value("durationSeconds", 0.5F);
            if (stepJson.contains("activeLayers") && stepJson["activeLayers"].is_array()) {
                for (const auto& idx : stepJson["activeLayers"]) {
                    step.activeLayers.push_back(idx.get<std::size_t>());
                }
            }
            def.sequence.push_back(step);
        }
    }

    if (def.sequence.empty()) {
        PatternSequenceStep allLayers;
        allLayers.durationSeconds = 1.0F;
        for (std::size_t i = 0; i < def.layers.size(); ++i) {
            allLayers.activeLayers.push_back(i);
        }
        def.sequence.push_back(allLayers);
    }

    return def;
}
} // namespace

bool PatternBank::loadFromFile(const std::string& filePath) {
    const std::vector<std::string> packPaths = splitPackPaths(filePath);
    if (packPaths.empty()) {
        return false;
    }

    patterns_.clear();
    compiledGraphs_.clear();
    graphDiagnostics_.clear();
    std::unordered_map<std::string, std::size_t> guidToIndex;

    for (const std::string& packPath : packPaths) {
        std::ifstream in(packPath);
        if (!in.good()) {
            logWarn("Pattern pack not found: " + packPath);
            continue;
        }

        nlohmann::json json;
        try {
            in >> json;
        } catch (const std::exception& ex) {
            ErrorReport err = makeError(ErrorCode::ContentLoadFailed, std::string("JSON parse failed: ") + ex.what());
            addContext(err, "path", packPath);
            pushStack(err, "PatternBank::loadFromFile/parse");
            logErrorReport(err);
            continue;
        }

        std::string migrationMessage;
        if (!migratePackJson(json, migrationMessage)) {
            logError("Pattern pack schema error: " + migrationMessage + " path=" + packPath);
            return false;
        }
        if (!migrationMessage.empty()) {
            logWarn(migrationMessage + " path=" + packPath);
        }

        PackMetadata metadata;
        std::string metaError;
        if (!parsePackMetadata(json, metadata, metaError)) {
            logError("Pattern pack metadata error: " + metaError + " path=" + packPath);
            return false;
        }
        constexpr int kRuntimePackVersion = 3;
        if (kRuntimePackVersion < metadata.minRuntimePackVersion || kRuntimePackVersion > metadata.maxRuntimePackVersion) {
            logError("Pattern pack compatibility mismatch for pack=" + metadata.packId + " path=" + packPath + " runtimeVersion=" + std::to_string(kRuntimePackVersion));
            return false;
        }

        if (!json.contains("patterns") || !json["patterns"].is_array()) {
            logWarn("Pattern pack missing `patterns` array: " + packPath);
            continue;
        }

        for (const nlohmann::json& item : json["patterns"]) {
            PatternDefinition parsed = parsePatternDefinition(item);
            if (parsed.guid.empty()) parsed.guid = stableGuidForAsset("pattern", parsed.name);
            if (auto found = guidToIndex.find(parsed.guid); found != guidToIndex.end()) {
                logWarn("Pattern conflict override guid=" + parsed.guid + " old=" + patterns_[found->second].name + " new=" + parsed.name + " pack=" + metadata.packId);
                patterns_[found->second] = std::move(parsed);
            } else {
                guidToIndex[parsed.guid] = patterns_.size();
                patterns_.push_back(std::move(parsed));
            }
        }

        if (json.contains("graphs") && json["graphs"].is_array()) {
            std::vector<PatternGraphAsset> graphs;
            std::vector<PatternGraphDiagnostic> readDiagnostics;
            if (loadPatternGraphsFromFile(packPath, graphs, readDiagnostics)) {
                graphDiagnostics_.insert(graphDiagnostics_.end(), readDiagnostics.begin(), readDiagnostics.end());
                PatternGraphCompiler compiler;
                for (const PatternGraphAsset& graph : graphs) {
                    CompiledPatternGraph compiled;
                    if (compiler.compile(graph, compiled)) {
                        compiledGraphs_.push_back(std::move(compiled));
                    } else {
                        graphDiagnostics_.insert(graphDiagnostics_.end(), compiled.diagnostics.begin(), compiled.diagnostics.end());
                    }
                }
            }
        }
    }

    return !patterns_.empty();
}

std::size_t PatternBank::findPatternIndexByGuidOrName(const std::string& ref) const {
    for (std::size_t i = 0; i < patterns_.size(); ++i) {
        if (patterns_[i].guid == ref || patterns_[i].name == ref) {
            return i;
        }
    }
    return patterns_.size();
}

void PatternBank::loadFallbackDefaults() {
    patterns_.clear();
    compiledGraphs_.clear();
    graphDiagnostics_.clear();

    PatternDefinition basic;
    basic.guid = stableGuidForAsset("pattern", "Fallback Composite");
    basic.name = "Fallback Composite";
    basic.seedOffset = 101;
    basic.layers = {
        PatternLayer {.name = "radial", .type = PatternType::RadialBurst, .bulletCount = 24, .bulletSpeed = 140.0F, .cooldownSeconds = 0.20F},
        PatternLayer {.name = "spiral", .type = PatternType::Spiral, .bulletCount = 8, .bulletSpeed = 160.0F, .cooldownSeconds = 0.06F, .angleStepDeg = 14.0F},
    };
    basic.sequence = {
        PatternSequenceStep {.durationSeconds = 1.2F, .activeLayers = {0}},
        PatternSequenceStep {.durationSeconds = 1.2F, .activeLayers = {0, 1}},
    };
    patterns_.push_back(basic);
}

const std::vector<PatternDefinition>& PatternBank::patterns() const { return patterns_; }

const std::vector<CompiledPatternGraph>& PatternBank::compiledGraphs() const { return compiledGraphs_; }

const std::vector<PatternGraphDiagnostic>& PatternBank::graphDiagnostics() const { return graphDiagnostics_; }

void PatternPlayer::setBank(const PatternBank* bank) {
    bank_ = bank;
    patternIndex_ = 0;
    resetCurrentPattern();
}

void PatternPlayer::setRunSeed(const std::uint64_t runSeed) {
    runSeed_ = runSeed;
    rng_ = DeterministicRng(runSeed_);
}

void PatternPlayer::setPatternIndex(const std::size_t index) {
    if (!bank_ || bank_->patterns().empty()) return;
    patternIndex_ = std::min(index, bank_->patterns().size() - 1U);
    resetCurrentPattern();
}

void PatternPlayer::cyclePattern(const int direction) {
    if (!bank_ || bank_->patterns().empty()) return;
    const int count = static_cast<int>(bank_->patterns().size());
    int idx = static_cast<int>(patternIndex_);
    idx = (idx + direction) % count;
    if (idx < 0) idx += count;
    patternIndex_ = static_cast<std::size_t>(idx);
    resetCurrentPattern();
}

void PatternPlayer::resetCurrentPattern() {
    patternTime_ = 0.0F;
    sequenceTime_ = 0.0F;
    sequenceStepIndex_ = 0;

    layerStates_.clear();
    if (const PatternDefinition* def = activePattern()) {
        layerStates_.assign(def->layers.size(), LayerRuntimeState {});
    }

    std::uint64_t seed = runSeed_;
    if (const PatternDefinition* def = activePattern()) {
        seed ^= def->seedOffset;
        seed ^= static_cast<std::uint64_t>(patternIndex_) * 0x9E3779B97F4A7C15ULL;
    }
    rng_ = DeterministicRng(seed);
}

void PatternPlayer::togglePause() { paused_ = !paused_; }

void PatternPlayer::setPaused(const bool paused) { paused_ = paused; }

bool PatternPlayer::paused() const { return paused_; }

void PatternPlayer::setPlaybackSpeed(const float speed) { playbackSpeed_ = std::clamp(speed, 0.1F, 4.0F); }

void PatternPlayer::adjustPlaybackSpeed(const float delta) { setPlaybackSpeed(playbackSpeed_ + delta); }

float PatternPlayer::playbackSpeed() const { return playbackSpeed_; }

void PatternPlayer::setRuntimeModifiers(const float cooldownScale, const int extraBullets, const float jitterAddDeg) {
    runtimeCooldownScale_ = std::clamp(cooldownScale, 0.5F, 2.0F);
    runtimeExtraBullets_ = std::max(extraBullets, -16);
    runtimeJitterAddDeg_ = jitterAddDeg;
}

void PatternPlayer::update(
    const float fixedDt,
    const Vec2 origin,
    const Vec2 aimTarget,
    const std::function<void(const ProjectileSpawn&)>& emitProjectile
) {
    if (paused_) return;
    const PatternDefinition* pattern = activePattern();
    if (!pattern) return;

    const float dtScaled = fixedDt * playbackSpeed_;
    patternTime_ += dtScaled;
    sequenceTime_ += dtScaled;

    if (!pattern->sequence.empty()) {
        const PatternSequenceStep& step = pattern->sequence[sequenceStepIndex_];
        const float stepDuration = std::max(step.durationSeconds, 0.01F);
        while (sequenceTime_ >= stepDuration) {
            sequenceTime_ -= stepDuration;
            if (sequenceStepIndex_ + 1 < pattern->sequence.size()) {
                ++sequenceStepIndex_;
            } else if (pattern->loopSequence) {
                sequenceStepIndex_ = 0;
            } else {
                sequenceStepIndex_ = pattern->sequence.size() - 1;
                sequenceTime_ = std::min(sequenceTime_, stepDuration - 0.0001F);
                break;
            }
        }
    }

    const PatternSequenceStep& step = pattern->sequence[sequenceStepIndex_];
    for (const std::size_t layerIndex : step.activeLayers) {
        if (layerIndex >= pattern->layers.size()) continue;

        LayerRuntimeState& st = layerStates_[layerIndex];
        st.cooldownTimer -= dtScaled;
        const float cooldown = std::max(pattern->layers[layerIndex].cooldownSeconds * runtimeCooldownScale_, 0.01F);
        while (st.cooldownTimer <= 0.0F) {
            emitLayer(*pattern, layerIndex, 0.0F, origin, aimTarget, emitProjectile);
            st.cooldownTimer += cooldown;
        }
    }
}

std::size_t PatternPlayer::patternIndex() const { return patternIndex_; }

const PatternDefinition* PatternPlayer::activePattern() const {
    if (!bank_ || bank_->patterns().empty()) return nullptr;
    return &bank_->patterns()[patternIndex_];
}

std::size_t PatternPlayer::activeSequenceStep() const { return sequenceStepIndex_; }

std::size_t PatternPlayer::activeLayerCountForStep() const {
    const PatternDefinition* p = activePattern();
    if (!p || p->sequence.empty()) return 0;
    return p->sequence[sequenceStepIndex_].activeLayers.size();
}

const PatternTimelineEvent* PatternPlayer::currentTimelineEvent(const PatternLayer& layer, LayerRuntimeState& state, const float localTime) const {
    if (layer.timeline.empty()) return nullptr;

    while (state.timelineCursor + 1 < layer.timeline.size() && localTime >= layer.timeline[state.timelineCursor + 1].timeSeconds) {
        ++state.timelineCursor;
    }
    return &layer.timeline[state.timelineCursor];
}

void PatternPlayer::emitLayer(
    const PatternDefinition& pattern,
    const std::size_t layerIndex,
    const float phaseOffsetDeg,
    const Vec2 origin,
    const Vec2 aimTarget,
    const std::function<void(const ProjectileSpawn&)>& emitProjectile
) {
    if (layerIndex >= pattern.layers.size()) return;

    PatternLayer const& layer = pattern.layers[layerIndex];
    LayerRuntimeState& state = layerStates_[layerIndex];
    const PatternTimelineEvent* timeline = currentTimelineEvent(layer, state, patternTime_);

    const float timelineRot = timeline ? timeline->extraRotationDeg : 0.0F;
    const float timelineSpeedScale = timeline ? timeline->speedScale : 1.0F;

    const float speedCurve = 1.0F + layer.modifiers.speedCurveAmp * dmath::sin(patternTime_ * 2.0F * std::numbers::pi_v<float>);
    const float speedScale = std::max(0.1F, timelineSpeedScale * speedCurve);

    const float osc = layer.modifiers.angleOscillationDeg *
                      dmath::sin(patternTime_ * layer.modifiers.angleOscillationHz * 2.0F * std::numbers::pi_v<float>);

    const float baseRotation = layer.modifiers.rotationDeg + layer.modifiers.phaseOffsetDeg + phaseOffsetDeg + timelineRot + osc;

    const auto jitteredAngle = [&](float angle) {
        const float jitterAmount = layer.modifiers.deterministicJitterDeg + runtimeJitterAddDeg_;
        const float jitter = (rng_.nextFloat01() * 2.0F - 1.0F) * jitterAmount;
        return angle + jitter;
    };

    const auto emitAtDeg = [&](const float angleDeg, const float speedMultiplier = 1.0F) {
        emitProjectile(ProjectileSpawn {
            .pos = origin,
            .vel = velocityFromDeg(jitteredAngle(angleDeg), layer.bulletSpeed * speedScale * speedMultiplier),
            .radius = layer.projectileRadius,
            .behavior = layer.projectileBehavior,
        });
    };

    switch (layer.type) {
        case PatternType::RadialBurst: {
            const std::uint32_t count = std::max(1, static_cast<int>(layer.bulletCount) + runtimeExtraBullets_);
            for (std::uint32_t i = 0; i < count; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(count);
                emitAtDeg(baseRotation + 360.0F * t);
            }
            break;
        }
        case PatternType::Spiral: {
            const std::uint32_t count = std::max(1, static_cast<int>(layer.bulletCount) + runtimeExtraBullets_);
            for (std::uint32_t i = 0; i < count; ++i) {
                emitAtDeg(baseRotation + state.spiralAccumDeg + static_cast<float>(i) * (360.0F / static_cast<float>(count)));
            }
            state.spiralAccumDeg += layer.angleStepDeg;
            break;
        }
        case PatternType::Spread: {
            const std::uint32_t count = std::max(1, static_cast<int>(layer.bulletCount) + runtimeExtraBullets_);
            const float start = -layer.spreadAngleDeg * 0.5F;
            const float step = count > 1 ? layer.spreadAngleDeg / static_cast<float>(count - 1) : 0.0F;
            for (std::uint32_t i = 0; i < count; ++i) {
                emitAtDeg(baseRotation + start + step * static_cast<float>(i));
            }
            break;
        }
        case PatternType::Wave: {
            const std::uint32_t count = std::max(1, static_cast<int>(layer.bulletCount) + runtimeExtraBullets_);
            const float wave = dmath::sin(patternTime_ * layer.waveFrequencyHz * 2.0F * std::numbers::pi_v<float>) * layer.waveAmplitudeDeg;
            for (std::uint32_t i = 0; i < count; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(count);
                const float angle = baseRotation + wave + t * 360.0F;
                const float localSpeed = 0.8F + 0.2F * dmath::sin((patternTime_ + t) * 5.0F);
                emitAtDeg(angle, localSpeed);
            }
            break;
        }
        case PatternType::Aimed: {
            const std::uint32_t count = std::max(1, static_cast<int>(layer.bulletCount) + runtimeExtraBullets_);
            const float dx = aimTarget.x - origin.x;
            const float dy = aimTarget.y - origin.y;
            const float aimDeg = dmath::atan2(dy, dx) * 180.0F / std::numbers::pi_v<float>;
            const float spreadStart = -layer.spreadAngleDeg * 0.5F;
            const float spreadStep = count > 1 ? layer.spreadAngleDeg / static_cast<float>(count - 1) : 0.0F;
            for (std::uint32_t i = 0; i < count; ++i) {
                const float jitter = (rng_.nextFloat01() * 2.0F - 1.0F) * layer.aimJitterDeg;
                emitAtDeg(baseRotation + aimDeg + spreadStart + spreadStep * static_cast<float>(i) + jitter);
            }
            break;
        }
    }

    for (const PatternSubpatternRef& sub : layer.subpatterns) {
        if (sub.layerIndex != layerIndex) {
            emitLayer(pattern, sub.layerIndex, baseRotation + sub.phaseOffsetDeg, origin, aimTarget, emitProjectile);
        }
    }
}

std::string toString(const PatternType type) {
    switch (type) {
        case PatternType::RadialBurst: return "radial";
        case PatternType::Spiral: return "spiral";
        case PatternType::Spread: return "spread";
        case PatternType::Wave: return "wave";
        case PatternType::Aimed: return "aimed";
    }
    return "unknown";
}

} // namespace engine
