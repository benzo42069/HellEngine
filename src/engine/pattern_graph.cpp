#include <engine/pattern_graph.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numbers>

namespace engine {

namespace {
PatternGraphNodeType parseNodeType(const std::string& type) {
    if (type == "emit_ring") return PatternGraphNodeType::EmitRing;
    if (type == "emit_spread") return PatternGraphNodeType::EmitSpread;
    if (type == "emit_spiral") return PatternGraphNodeType::EmitSpiral;
    if (type == "emit_wave") return PatternGraphNodeType::EmitWave;
    if (type == "emit_aimed") return PatternGraphNodeType::EmitAimed;
    if (type == "wait") return PatternGraphNodeType::Wait;
    if (type == "loop") return PatternGraphNodeType::Loop;
    if (type == "branch") return PatternGraphNodeType::Branch;
    if (type == "subpattern") return PatternGraphNodeType::Subpattern;
    if (type == "modifier_rotate") return PatternGraphNodeType::ModifierRotate;
    if (type == "modifier_phase") return PatternGraphNodeType::ModifierPhaseOffset;
    if (type == "modifier_symmetry") return PatternGraphNodeType::ModifierSymmetry;
    if (type == "modifier_speed_curve") return PatternGraphNodeType::ModifierSpeedCurve;
    if (type == "modifier_angle_curve") return PatternGraphNodeType::ModifierAngleCurve;
    if (type == "difficulty_scalar") return PatternGraphNodeType::DifficultyScalar;
    if (type == "parameter_override") return PatternGraphNodeType::ParameterOverride;
    if (type == "random_range") return PatternGraphNodeType::RandomRange;
    return PatternGraphNodeType::Wait;
}

float degToRad(const float deg) { return deg * std::numbers::pi_v<float> / 180.0F; }

Vec2 velocityFromDeg(const float deg, const float speed) {
    const float rad = degToRad(deg);
    return Vec2 {std::cos(rad) * speed, std::sin(rad) * speed};
}
} // namespace

bool loadPatternGraphsFromFile(const std::string& filePath, std::vector<PatternGraphAsset>& outAssets, std::vector<PatternGraphDiagnostic>& outDiagnostics) {
    outAssets.clear();
    outDiagnostics.clear();

    std::ifstream input(filePath);
    if (!input.good()) return false;

    nlohmann::json root;
    input >> root;
    if (!root.contains("graphs") || !root["graphs"].is_array()) return false;

    for (const auto& g : root["graphs"]) {
        PatternGraphAsset asset;
        asset.id = g.value("id", "graph");
        if (!g.contains("nodes") || !g["nodes"].is_array()) {
            outDiagnostics.push_back(PatternGraphDiagnostic {.nodeId = asset.id, .message = "graph missing nodes array", .warning = false});
            continue;
        }

        for (const auto& n : g["nodes"]) {
            PatternGraphNode node;
            node.id = n.value("id", "node");
            node.type = parseNodeType(n.value("type", "wait"));
            if (n.contains("params") && n["params"].is_object()) {
                for (auto it = n["params"].begin(); it != n["params"].end(); ++it) {
                    node.params[it.key()] = it.value().get<float>();
                }
            }
            if (n.contains("outputs") && n["outputs"].is_array()) {
                for (const auto& o : n["outputs"]) node.outputs.push_back(o.get<std::string>());
            }
            asset.nodes.push_back(node);
        }

        std::sort(asset.nodes.begin(), asset.nodes.end(), [](const PatternGraphNode& a, const PatternGraphNode& b) {
            return a.id < b.id;
        });
        outAssets.push_back(std::move(asset));
    }

    return true;
}

bool PatternGraphCompiler::compile(const PatternGraphAsset& asset, CompiledPatternGraph& out) const {
    out = {};
    out.id = asset.id;

    if (asset.nodes.empty()) {
        out.diagnostics.push_back(PatternGraphDiagnostic {.nodeId = asset.id, .message = "empty graph", .warning = false});
        return false;
    }

    std::uint32_t estimatedSpawnPerCycle = 0;

    for (const PatternGraphNode& node : asset.nodes) {
        PatternOp op;
        switch (node.type) {
            case PatternGraphNodeType::EmitRing:
            case PatternGraphNodeType::EmitSpread:
            case PatternGraphNodeType::EmitSpiral:
            case PatternGraphNodeType::EmitWave:
            case PatternGraphNodeType::EmitAimed: {
                op.code = PatternOpCode::Emit;
                op.u0 = static_cast<std::uint32_t>(node.type);
                op.a = node.params.contains("count") ? node.params.at("count") : 12.0F;
                op.b = node.params.contains("speed") ? node.params.at("speed") : 120.0F;
                op.c = node.params.contains("radius") ? node.params.at("radius") : 3.0F;
                op.d = node.params.contains("angle") ? node.params.at("angle") : 0.0F;
                estimatedSpawnPerCycle += static_cast<std::uint32_t>(std::max(1.0F, op.a));

                if (node.type == PatternGraphNodeType::EmitRing) {
                    const std::uint32_t count = static_cast<std::uint32_t>(std::max(1.0F, op.a));
                    const std::size_t start = out.constantAnglesDeg.size();
                    for (std::uint32_t i = 0; i < count; ++i) {
                        const float t = static_cast<float>(i) / static_cast<float>(count);
                        out.constantAnglesDeg.push_back(360.0F * t);
                    }
                    op.u1 = static_cast<std::uint32_t>(start);
                }
                break;
            }
            case PatternGraphNodeType::Wait:
                op.code = PatternOpCode::Wait;
                op.a = node.params.contains("seconds") ? node.params.at("seconds") : 0.1F;
                break;
            case PatternGraphNodeType::Loop: {
                op.code = PatternOpCode::JumpIfLoop;
                op.u0 = static_cast<std::uint32_t>(node.params.contains("count") ? std::max(0.0F, node.params.at("count")) : 1.0F);
                op.u1 = static_cast<std::uint32_t>(node.params.contains("target") ? std::max(0.0F, node.params.at("target")) : 0.0F);
                if (op.u0 > 64U) {
                    out.diagnostics.push_back(PatternGraphDiagnostic {.nodeId = node.id, .message = "loop count exceeds bound (64)", .warning = false});
                }
                break;
            }
            case PatternGraphNodeType::ModifierRotate:
                op.code = PatternOpCode::SetRotate;
                op.a = node.params.contains("deg") ? node.params.at("deg") : 0.0F;
                break;
            case PatternGraphNodeType::ModifierPhaseOffset:
                op.code = PatternOpCode::SetPhaseOffset;
                op.a = node.params.contains("deg") ? node.params.at("deg") : 0.0F;
                break;
            case PatternGraphNodeType::DifficultyScalar:
                op.code = PatternOpCode::SetDifficulty;
                op.a = node.params.contains("value") ? node.params.at("value") : 1.0F;
                break;
            case PatternGraphNodeType::RandomRange:
                op.code = PatternOpCode::RandomRange;
                op.a = node.params.contains("min") ? node.params.at("min") : -3.0F;
                op.b = node.params.contains("max") ? node.params.at("max") : 3.0F;
                op.u0 = static_cast<std::uint32_t>(node.params.contains("stream") ? std::max(0.0F, node.params.at("stream")) : 0.0F);
                break;
            default:
                op.code = PatternOpCode::Wait;
                op.a = 0.0F;
                out.diagnostics.push_back(PatternGraphDiagnostic {.nodeId = node.id, .message = "node type compiled as no-op/wait", .warning = true});
                break;
        }
        out.ops.push_back(op);
    }

    out.ops.push_back(PatternOp {.code = PatternOpCode::End});

    const float nominalCycleSeconds = 1.0F;
    out.staticSpawnRateEstimatePerSecond = static_cast<float>(estimatedSpawnPerCycle) / nominalCycleSeconds;
    if (out.staticSpawnRateEstimatePerSecond > 3000.0F) {
        out.diagnostics.push_back(PatternGraphDiagnostic {.nodeId = asset.id, .message = "spawn rate estimate exceeds 3000/s", .warning = true});
    }

    const bool hasError = std::any_of(out.diagnostics.begin(), out.diagnostics.end(), [](const PatternGraphDiagnostic& d) { return !d.warning; });
    return !hasError;
}

void PatternGraphVm::reset(RuntimeState& state, const std::uint64_t seed) const {
    state = {};
    for (std::size_t i = 0; i < state.rngStreams.size(); ++i) {
        state.rngStreams[i] = DeterministicRng(seed ^ (0x9E3779B97F4A7C15ULL + static_cast<std::uint64_t>(i) * 0x1000003DULL));
    }
}

void PatternGraphVm::execute(
    const CompiledPatternGraph& graph,
    RuntimeState& state,
    const float dt,
    const Vec2 origin,
    const Vec2 aimTarget,
    const std::function<void(const ProjectileSpawn&)>& emitProjectile
) const {
    if (graph.ops.empty()) return;

    if (state.waitTimer > 0.0F) {
        state.waitTimer = std::max(0.0F, state.waitTimer - dt);
        return;
    }

    std::uint32_t safety = 0;
    while (state.pc < graph.ops.size() && safety++ < 256U) {
        const PatternOp& op = graph.ops[state.pc];
        switch (op.code) {
            case PatternOpCode::Emit: {
                const std::uint32_t count = static_cast<std::uint32_t>(std::max(1.0F, op.a));
                const float speed = op.b * state.difficultyScalar;
                const float radius = op.c;
                const PatternGraphNodeType emitType = static_cast<PatternGraphNodeType>(op.u0);

                if (emitType == PatternGraphNodeType::EmitRing && op.u1 + count <= graph.constantAnglesDeg.size()) {
                    for (std::uint32_t i = 0; i < count; ++i) {
                        const float angle = graph.constantAnglesDeg[op.u1 + i] + op.d + state.rotateDeg + state.phaseOffsetDeg;
                        emitProjectile(ProjectileSpawn {.pos = origin, .vel = velocityFromDeg(angle, speed), .radius = radius});
                    }
                } else if (emitType == PatternGraphNodeType::EmitAimed) {
                    const float a = std::atan2(aimTarget.y - origin.y, aimTarget.x - origin.x) * 180.0F / std::numbers::pi_v<float> + op.d + state.rotateDeg;
                    emitProjectile(ProjectileSpawn {.pos = origin, .vel = velocityFromDeg(a, speed), .radius = radius});
                } else {
                    for (std::uint32_t i = 0; i < count; ++i) {
                        const float t = count > 1 ? static_cast<float>(i) / static_cast<float>(count - 1U) : 0.5F;
                        const float spread = (t - 0.5F) * 60.0F;
                        const float angle = op.d + spread + state.rotateDeg + state.phaseOffsetDeg;
                        emitProjectile(ProjectileSpawn {.pos = origin, .vel = velocityFromDeg(angle, speed), .radius = radius});
                    }
                }
                ++state.pc;
                break;
            }
            case PatternOpCode::Wait:
                state.waitTimer = std::max(0.0F, op.a);
                ++state.pc;
                return;
            case PatternOpCode::SetRotate:
                state.rotateDeg = op.a;
                ++state.pc;
                break;
            case PatternOpCode::SetPhaseOffset:
                state.phaseOffsetDeg = op.a;
                ++state.pc;
                break;
            case PatternOpCode::SetDifficulty:
                state.difficultyScalar = std::max(0.1F, op.a);
                ++state.pc;
                break;
            case PatternOpCode::RandomRange: {
                const std::size_t streamIndex = static_cast<std::size_t>(op.u0 % state.rngStreams.size());
                const float r = state.rngStreams[streamIndex].nextFloat01();
                state.phaseOffsetDeg = op.a + (op.b - op.a) * r;
                ++state.pc;
                break;
            }
            case PatternOpCode::JumpIfLoop: {
                const std::size_t loopSlot = state.pc % state.loopCounter.size();
                const std::uint32_t count = std::min<std::uint32_t>(op.u0, 64U);
                if (state.loopCounter[loopSlot] + 1U < count) {
                    ++state.loopCounter[loopSlot];
                    state.pc = std::min<std::size_t>(op.u1, graph.ops.size() - 1U);
                } else {
                    state.loopCounter[loopSlot] = 0;
                    ++state.pc;
                }
                break;
            }
            case PatternOpCode::End:
                state.pc = 0;
                return;
        }
    }
}

} // namespace engine
