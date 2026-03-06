#include <engine/entities.h>
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
EntityType parseEntityType(const std::string& s) {
    if (s == "player") return EntityType::Player;
    if (s == "enemy") return EntityType::Enemy;
    if (s == "boss") return EntityType::Boss;
    if (s == "resource") return EntityType::ResourceNode;
    if (s == "hazard") return EntityType::Hazard;
    return EntityType::Enemy;
}

MovementBehavior parseMovement(const std::string& s) {
    if (s == "static") return MovementBehavior::Static;
    if (s == "linear") return MovementBehavior::Linear;
    if (s == "sine") return MovementBehavior::Sine;
    if (s == "chase") return MovementBehavior::Chase;
    return MovementBehavior::Static;
}

Vec2 velocityFromDeg(const float deg, const float speed) {
    const float r = deg * std::numbers::pi_v<float> / 180.0F;
    return {dmath::cos(r) * speed, dmath::sin(r) * speed};
}

void parseResourceYield(const nlohmann::json& json, ResourceYield& y) {
    y.upgradeCurrency = json.value("upgradeCurrency", 0.0F);
    y.healthRecovery = json.value("healthRecovery", 0.0F);
    y.buffDurationSeconds = json.value("buffDurationSeconds", 0.0F);
}
} // namespace

bool EntityDatabase::loadFromFile(const std::string& path) {
    const std::vector<std::string> packPaths = splitPackPaths(path);
    if (packPaths.empty()) return false;

    templates_.clear();
    std::unordered_map<std::string, std::size_t> guidToIndex;

    for (const std::string& packPath : packPaths) {
        std::ifstream in(packPath);
        if (!in.good()) {
            logWarn("Entity pack not found: " + packPath);
            continue;
        }

        nlohmann::json json;
        try {
            in >> json;
        } catch (const std::exception& ex) {
            ErrorReport err = makeError(ErrorCode::ContentLoadFailed, std::string("JSON parse failed: ") + ex.what());
            addContext(err, "path", packPath);
            pushStack(err, "EntityDatabase::loadFromFile/parse");
            logErrorReport(err);
            continue;
        }

        std::string migrationMessage;
        if (!migratePackJson(json, migrationMessage)) {
            logError("Entity pack schema error: " + migrationMessage + " path=" + packPath);
            return false;
        }
        if (!migrationMessage.empty()) {
            logWarn(migrationMessage + " path=" + packPath);
        }

        PackMetadata metadata;
        std::string metaError;
        if (!parsePackMetadata(json, metadata, metaError)) {
            logError("Entity pack metadata error: " + metaError + " path=" + packPath);
            return false;
        }
        constexpr int kRuntimePackVersion = 3;
        if (kRuntimePackVersion < metadata.minRuntimePackVersion || kRuntimePackVersion > metadata.maxRuntimePackVersion) {
            logError("Entity pack compatibility mismatch for pack=" + metadata.packId + " path=" + packPath + " runtimeVersion=" + std::to_string(kRuntimePackVersion));
            return false;
        }

        if (!json.contains("entities") || !json["entities"].is_array()) continue;

        for (const auto& e : json["entities"]) {
            EntityTemplate t;
            t.name = e.value("name", "entity");
            t.guid = e.value("guid", stableGuidForAsset("enemy", t.name));
            if (t.guid.empty()) t.guid = stableGuidForAsset("enemy", t.name);
            t.type = parseEntityType(e.value("type", "enemy"));
            t.movement = parseMovement(e.value("movement", "static"));
            if (e.contains("spawnPosition") && e["spawnPosition"].is_array() && e["spawnPosition"].size() >= 2) {
                t.spawnPosition = {e["spawnPosition"][0].get<float>(), e["spawnPosition"][1].get<float>()};
            }
            if (e.contains("baseVelocity") && e["baseVelocity"].is_array() && e["baseVelocity"].size() >= 2) {
                t.baseVelocity = {e["baseVelocity"][0].get<float>(), e["baseVelocity"][1].get<float>()};
            }
            t.sineAmplitude = e.value("sineAmplitude", 0.0F);
            t.sineFrequencyHz = e.value("sineFrequencyHz", 1.0F);
            t.maxHealth = e.value("maxHealth", 10.0F);
            t.contactDamage = e.value("contactDamage", 1.0F);
            t.interactionRadius = e.value("interactionRadius", 16.0F);
            if (e.contains("resourceYield") && e["resourceYield"].is_object()) {
                parseResourceYield(e["resourceYield"], t.resourceYield);
            }
            t.attacksEnabled = e.value("attacksEnabled", false);
            t.attackPatternGuid = e.value("attackPatternGuid", "");
            t.attackPatternName = e.value("attackPatternName", "");
            t.attackIntervalSeconds = e.value("attackIntervalSeconds", 0.5F);

            if (e.contains("boss") && e["boss"].is_object()) {
                const auto& boss = e["boss"];
                t.boss.enabled = true;
                t.boss.introDurationSeconds = boss.value("introDurationSeconds", 1.2F);
                if (boss.contains("rewardDrop") && boss["rewardDrop"].is_object()) {
                    parseResourceYield(boss["rewardDrop"], t.boss.rewardDrop);
                }
                if (boss.contains("phases") && boss["phases"].is_array()) {
                    for (const auto& p : boss["phases"]) {
                        BossPhase phase;
                        phase.attackPatternGuid = p.value("attackPatternGuid", "");
                        phase.attackPatternName = p.value("attackPatternName", t.attackPatternName);
                        phase.movement = parseMovement(p.value("movement", "static"));
                        phase.durationSeconds = p.value("durationSeconds", 4.0F);
                        phase.difficultyScale = p.value("difficultyScale", 1.0F);
                        t.boss.phases.push_back(phase);
                    }
                }
            }

            if (e.contains("spawnRule") && e["spawnRule"].is_object()) {
                const auto& sr = e["spawnRule"];
                t.spawnRule.enabled = sr.value("enabled", false);
                t.spawnRule.initialDelaySeconds = sr.value("initialDelaySeconds", 0.0F);
                t.spawnRule.intervalSeconds = sr.value("intervalSeconds", 1.0F);
                t.spawnRule.maxAlive = sr.value("maxAlive", 1U);
            }

            if (auto found = guidToIndex.find(t.guid); found != guidToIndex.end()) {
                logWarn("Entity conflict override guid=" + t.guid + " old=" + templates_[found->second].name + " new=" + t.name + " pack=" + metadata.packId);
                templates_[found->second] = std::move(t);
            } else {
                guidToIndex[t.guid] = templates_.size();
                templates_.push_back(std::move(t));
            }
        }
    }

    return !templates_.empty();
}

void EntityDatabase::loadFallbackDefaults() {
    templates_.clear();

    EntityTemplate enemy;
    enemy.name = "Basic Enemy";
    enemy.guid = stableGuidForAsset("enemy", enemy.name);
    enemy.type = EntityType::Enemy;
    enemy.movement = MovementBehavior::Sine;
    enemy.spawnPosition = {0.0F, -180.0F};
    enemy.baseVelocity = {0.0F, 25.0F};
    enemy.sineAmplitude = 80.0F;
    enemy.sineFrequencyHz = 0.6F;
    enemy.maxHealth = 20.0F;
    enemy.contactDamage = 2.0F;
    enemy.interactionRadius = 16.0F;
    enemy.attacksEnabled = true;
    enemy.attackPatternName = "Composed Helix";
    enemy.attackPatternGuid = stableGuidForAsset("pattern", enemy.attackPatternName);
    enemy.attackIntervalSeconds = 0.35F;
    enemy.spawnRule = SpawnRule {.enabled = true, .initialDelaySeconds = 0.5F, .intervalSeconds = 2.5F, .maxAlive = 4};
    templates_.push_back(enemy);

    EntityTemplate boss;
    boss.name = "Fallback Warden";
    boss.guid = stableGuidForAsset("enemy", boss.name);
    boss.type = EntityType::Boss;
    boss.movement = MovementBehavior::Linear;
    boss.spawnPosition = {0.0F, -220.0F};
    boss.baseVelocity = {0.0F, 20.0F};
    boss.maxHealth = 300.0F;
    boss.attacksEnabled = true;
    boss.attackPatternName = "Wave Weave";
    boss.attackPatternGuid = stableGuidForAsset("pattern", boss.attackPatternName);
    boss.attackIntervalSeconds = 0.6F;
    boss.boss.enabled = true;
    boss.boss.introDurationSeconds = 1.5F;
    boss.boss.phases = {
        BossPhase {.attackPatternGuid = stableGuidForAsset("pattern", "Wave Weave"), .attackPatternName = "Wave Weave", .movement = MovementBehavior::Linear, .durationSeconds = 4.0F, .difficultyScale = 1.0F},
        BossPhase {.attackPatternGuid = stableGuidForAsset("pattern", "Composed Helix"), .attackPatternName = "Composed Helix", .movement = MovementBehavior::Chase, .durationSeconds = 5.0F, .difficultyScale = 1.3F},
    };
    boss.boss.rewardDrop = ResourceYield {.upgradeCurrency = 100.0F, .healthRecovery = 20.0F, .buffDurationSeconds = 4.0F};
    boss.spawnRule = SpawnRule {.enabled = true, .initialDelaySeconds = 6.0F, .intervalSeconds = 20.0F, .maxAlive = 1};
    templates_.push_back(boss);
}

const std::vector<EntityTemplate>& EntityDatabase::templates() const { return templates_; }

void EntitySystem::setTemplates(const std::vector<EntityTemplate>* templates) {
    templates_ = templates;
    spawnerState_.assign(templates ? templates->size() : 0, SpawnerState {});
}

void EntitySystem::setPatternBank(const PatternBank* bank) { patternBank_ = bank; }

void EntitySystem::setRunSeed(const std::uint64_t seed) { rng_ = DeterministicRng(seed); }

void EntitySystem::reset() {
    entities_.clear();
    stats_ = {};
    for (auto& s : spawnerState_) {
        s = SpawnerState {};
    }

    if (!templates_) return;
    for (std::size_t i = 0; i < templates_->size(); ++i) {
        const EntityTemplate& t = (*templates_)[i];
        if (!t.spawnRule.enabled) {
            spawnEntity(i, 1.0F);
        }
    }
}

void EntitySystem::spawnEntity(const std::size_t templateIndex, const float enemyHealthScale) {
    if (!templates_ || templateIndex >= templates_->size()) return;
    const EntityTemplate& t = (*templates_)[templateIndex];

    EntityInstance e;
    e.templateIndex = templateIndex;
    e.position = t.spawnPosition;
    e.health = t.maxHealth * std::max(0.1F, enemyHealthScale);
    e.fireCooldown = t.attackIntervalSeconds;
    e.introPlaying = t.type == EntityType::Boss && t.boss.enabled;
    if (e.introPlaying) {
        e.phaseTimeRemaining = std::max(0.0F, t.boss.introDurationSeconds);
    }
    entities_.push_back(e);
}

void EntitySystem::applyHarvest(const EntityTemplate& t, const EntityRuntimeModifiers& runtimeMods) {
    stats_.harvestedNodes += 1;
    const float mult = std::max(runtimeMods.harvestYieldMultiplier, 0.0F);
    stats_.upgradeCurrency += t.resourceYield.upgradeCurrency * mult;
    stats_.healthRecoveryAccum += t.resourceYield.healthRecovery * mult;
    stats_.buffTimeRemaining = std::max(stats_.buffTimeRemaining, t.resourceYield.buffDurationSeconds);
}

void EntitySystem::applyRewardDrop(const ResourceYield& reward, const EntityRuntimeModifiers& runtimeMods) {
    const float mult = std::max(runtimeMods.harvestYieldMultiplier, 0.0F);
    stats_.upgradeCurrency += reward.upgradeCurrency * mult;
    stats_.healthRecoveryAccum += reward.healthRecovery * mult;
    stats_.buffTimeRemaining = std::max(stats_.buffTimeRemaining, reward.buffDurationSeconds);
}

void EntitySystem::emitPatternFromTemplate(
    const EntityTemplate& t,
    const std::string& patternOverride,
    const Vec2 origin,
    const Vec2 playerPos,
    ProjectileSystem& projectiles,
    const EntityRuntimeModifiers& runtimeMods,
    const float difficultyScale
) {
    if (!patternBank_) return;
    const std::string& patternName = patternOverride.empty() ? t.attackPatternName : patternOverride;
    if (patternName.empty()) return;

    const auto& patterns = patternBank_->patterns();
    const std::size_t patternIdx = patternBank_->findPatternIndexByGuidOrName(patternName);
    if (patternIdx >= patterns.size()) return;

    const PatternDefinition& pattern = patterns[patternIdx];
    if (pattern.layers.empty()) return;

    const PatternLayer& layer = pattern.layers.front();
    const float speed = layer.bulletSpeed * std::max(runtimeMods.enemyProjectileSpeedScale, 0.1F) * std::max(0.2F, difficultyScale);

    switch (layer.type) {
        case PatternType::RadialBurst: {
            const std::uint32_t bullets = std::max(1U, static_cast<std::uint32_t>(std::round(static_cast<float>(layer.bulletCount) * std::max(0.5F, difficultyScale))));
            for (std::uint32_t i = 0; i < bullets; ++i) {
                const float a = 360.0F * static_cast<float>(i) / static_cast<float>(bullets);
                projectiles.spawn({origin, velocityFromDeg(a, speed), layer.projectileRadius, ProjectileBehavior {}, ProjectileAllegiance::Enemy});
            }
            break;
        }
        case PatternType::Aimed:
        default: {
            const float base = dmath::atan2(playerPos.y - origin.y, playerPos.x - origin.x) * 180.0F / std::numbers::pi_v<float>;
            const float spread = layer.spreadAngleDeg;
            const std::uint32_t bullets = std::max(1U, static_cast<std::uint32_t>(std::round(static_cast<float>(layer.bulletCount) * std::max(0.5F, difficultyScale))));
            for (std::uint32_t i = 0; i < bullets; ++i) {
                const float t01 = bullets > 1 ? static_cast<float>(i) / static_cast<float>(bullets - 1) : 0.5F;
                const float jitter = (rng_.nextFloat01() * 2.0F - 1.0F) * layer.modifiers.deterministicJitterDeg;
                const float a = base - spread * 0.5F + spread * t01 + jitter;
                projectiles.spawn({origin, velocityFromDeg(a, speed), layer.projectileRadius, ProjectileBehavior {}, ProjectileAllegiance::Enemy});
            }
            break;
        }
    }
}

void EntitySystem::update(const float dt, ProjectileSystem& projectiles, const Vec2 playerPos, const EntityRuntimeModifiers& runtimeMods) {
    if (!templates_) return;

    stats_.buffTimeRemaining = std::max(0.0F, stats_.buffTimeRemaining - dt);

    for (std::size_t i = 0; i < templates_->size(); ++i) {
        const EntityTemplate& t = (*templates_)[i];
        if (!t.spawnRule.enabled) continue;

        std::uint32_t aliveCount = 0;
        for (const auto& e : entities_) {
            if (e.alive && e.templateIndex == i) ++aliveCount;
        }

        SpawnerState& s = spawnerState_[i];
        if (!s.initialized) {
            s.timer = t.spawnRule.initialDelaySeconds;
            s.initialized = true;
        }

        s.timer -= dt;
        while (s.timer <= 0.0F && aliveCount < t.spawnRule.maxAlive) {
            spawnEntity(i, runtimeMods.enemyHealthScale);
            ++aliveCount;
            s.timer += std::max(t.spawnRule.intervalSeconds, 0.01F);
        }
    }

    for (auto& e : entities_) {
        if (!e.alive) continue;
        const EntityTemplate& t = (*templates_)[e.templateIndex];
        e.ageSeconds += dt;

        MovementBehavior movement = t.movement;
        std::string attackPattern = !t.attackPatternGuid.empty() ? t.attackPatternGuid : t.attackPatternName;
        float difficultyScale = 1.0F;

        if (t.type == EntityType::Boss && t.boss.enabled && !t.boss.phases.empty()) {
            if (e.introPlaying) {
                e.phaseTimeRemaining -= dt;
                if (e.phaseTimeRemaining <= 0.0F) {
                    e.introPlaying = false;
                    e.activeBossPhase = 0;
                    e.phaseTimeRemaining = std::max(0.4F, t.boss.phases[0].durationSeconds);
                }
            } else {
                const BossPhase& phase = t.boss.phases[e.activeBossPhase];
                movement = phase.movement;
                attackPattern = !phase.attackPatternGuid.empty() ? phase.attackPatternGuid : phase.attackPatternName;
                difficultyScale = phase.difficultyScale;
                e.phaseTimeRemaining -= dt;

                if (e.phaseTimeRemaining <= 0.0F) {
                    if (e.activeBossPhase + 1 < t.boss.phases.size()) {
                        ++e.activeBossPhase;
                        stats_.bossPhaseTransitions += 1;
                        e.phaseTimeRemaining = std::max(0.4F, t.boss.phases[e.activeBossPhase].durationSeconds);
                    } else {
                        e.alive = false;
                        stats_.defeatedBosses += 1;
                        applyRewardDrop(t.boss.rewardDrop, runtimeMods);
                        continue;
                    }
                }
            }
        }

        switch (movement) {
            case MovementBehavior::Static:
                break;
            case MovementBehavior::Linear:
                e.position.x += t.baseVelocity.x * dt;
                e.position.y += t.baseVelocity.y * dt;
                break;
            case MovementBehavior::Sine:
                e.position.y += t.baseVelocity.y * dt;
                e.position.x = t.spawnPosition.x + dmath::sin(e.ageSeconds * t.sineFrequencyHz * 2.0F * std::numbers::pi_v<float>) * t.sineAmplitude;
                break;
            case MovementBehavior::Chase: {
                const float dx = playerPos.x - e.position.x;
                const float dy = playerPos.y - e.position.y;
                const float len = dmath::sqrt(dx * dx + dy * dy);
                if (len > 0.001F) {
                    const float speed = std::max(std::abs(t.baseVelocity.x) + std::abs(t.baseVelocity.y), 40.0F) * std::max(0.7F, difficultyScale);
                    e.position.x += dx / len * speed * dt;
                    e.position.y += dy / len * speed * dt;
                }
                break;
            }
        }

        if (t.type == EntityType::ResourceNode) {
            const float dx = e.position.x - playerPos.x;
            const float dy = e.position.y - playerPos.y;
            const float rr = std::max(t.interactionRadius, 1.0F);
            if (dx * dx + dy * dy <= rr * rr) {
                applyHarvest(t, runtimeMods);
                e.alive = false;
                continue;
            }
        }

        if (t.attacksEnabled && !(t.type == EntityType::Boss && e.introPlaying)) {
            const float fireRateBuff = stats_.buffTimeRemaining > 0.0F ? 0.75F : 1.0F;
            const float traitFireScale = std::max(runtimeMods.enemyFireRateScale, 0.1F);
            const float difficultyFireScale = std::max(0.4F, difficultyScale);
            e.fireCooldown -= dt;
            if (e.fireCooldown <= 0.0F) {
                emitPatternFromTemplate(t, attackPattern, e.position, playerPos, projectiles, runtimeMods, difficultyScale);
                e.fireCooldown += std::max(t.attackIntervalSeconds * fireRateBuff * traitFireScale / difficultyFireScale, 0.05F);
            }
        }

        if (std::fabs(e.position.x) > 520.0F || std::fabs(e.position.y) > 520.0F) {
            e.alive = false;
        }
    }

    stats_.aliveTotal = 0;
    stats_.aliveEnemies = 0;
    stats_.aliveBosses = 0;
    stats_.aliveResources = 0;
    stats_.aliveHazards = 0;
    for (const auto& e : entities_) {
        if (!e.alive) continue;
        ++stats_.aliveTotal;
        const EntityType type = (*templates_)[e.templateIndex].type;
        if (type == EntityType::Enemy) ++stats_.aliveEnemies;
        if (type == EntityType::Boss) ++stats_.aliveBosses;
        if (type == EntityType::ResourceNode) ++stats_.aliveResources;
        if (type == EntityType::Hazard) ++stats_.aliveHazards;
    }
}

void EntitySystem::debugDraw(DebugDraw& draw) const {
    if (!templates_) return;
    for (const auto& e : entities_) {
        if (!e.alive) continue;
        const EntityTemplate& t = (*templates_)[e.templateIndex];
        Color c {180, 180, 180, 255};
        if (t.type == EntityType::Enemy) c = {255, 80, 80, 255};
        if (t.type == EntityType::Boss) c = {255, 40, 200, 255};
        if (t.type == EntityType::ResourceNode) c = {80, 255, 120, 255};
        if (t.type == EntityType::Hazard) c = {255, 210, 60, 255};
        draw.circle(e.position, 10.0F, c, 18);
        if (t.type == EntityType::Boss && t.boss.enabled) {
            draw.circle(e.position, 16.0F, Color {255, 120, 230, 190}, 24);
        }
    }
}

const EntityStats& EntitySystem::stats() const { return stats_; }

std::string toString(const EntityType type) {
    switch (type) {
        case EntityType::Player: return "player";
        case EntityType::Enemy: return "enemy";
        case EntityType::Boss: return "boss";
        case EntityType::ResourceNode: return "resource";
        case EntityType::Hazard: return "hazard";
    }
    return "unknown";
}

} // namespace engine

namespace engine {

void EntitySystem::appendCollisionTargets(std::vector<CollisionTarget>& outTargets, const std::uint32_t idBase) const {
    if (!templates_) return;
    for (std::size_t i = 0; i < entities_.size(); ++i) {
        const EntityInstance& e = entities_[i];
        if (!e.alive) continue;
        const EntityTemplate& t = (*templates_)[e.templateIndex];
        if (t.type != EntityType::Enemy && t.type != EntityType::Boss && t.type != EntityType::Hazard) continue;
        outTargets.push_back(CollisionTarget {.pos = e.position, .radius = t.interactionRadius, .id = idBase + static_cast<std::uint32_t>(i), .team = 1U});
    }
}

void EntitySystem::processCollisionEvents(const std::span<const CollisionEvent> events) {
    if (!templates_) return;
    for (const CollisionEvent& e : events) {
        if (e.targetId < 1000U) continue;
        const std::size_t idx = static_cast<std::size_t>(e.targetId - 1000U);
        if (idx >= entities_.size()) continue;
        EntityInstance& inst = entities_[idx];
        if (!inst.alive) continue;
        inst.health -= 1.0F;
        if (inst.health <= 0.0F) {
            inst.alive = false;
            const EntityTemplate& t = (*templates_)[inst.templateIndex];
            if (t.type == EntityType::Boss) {
                stats_.defeatedBosses += 1;
                if (t.boss.enabled) applyRewardDrop(t.boss.rewardDrop, {});
            }
        }
    }
}

} // namespace engine
