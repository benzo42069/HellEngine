#include <engine/content_pipeline.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <map>
#include <regex>
#include <set>
#include <sstream>

namespace engine {

namespace {
std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string normalizeAssetKind(const std::string& kind) {
    const std::string lowered = toLower(kind);
    if (lowered == "texture") return "texture";
    return "sprite";
}

bool isKnownColorWorkflow(const std::string& workflow) {
    return workflow == "full-color" || workflow == "grayscale" || workflow == "monochrome";
}

bool isKnownCollisionPolicy(const std::string& policy) {
    return policy == "none" || policy == "tight" || policy == "rect";
}

bool isKnownFilter(const std::string& filter) {
    return filter == "nearest" || filter == "linear";
}

bool isKnownMipPreference(const std::string& mipPreference) {
    return mipPreference == "none" || mipPreference == "generate";
}

std::string jsonFingerprint(const nlohmann::json& value) {
    return fnv1a64Hex(value.dump());
}


bool isIdentifierLike(const std::string& value) {
    if (value.empty()) return false;
    for (const char c : value) {
        const bool ok = std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.';
        if (!ok) return false;
    }
    return true;
}

bool hasValidWeight(float weight) {
    return std::isfinite(weight) && weight > 0.0F;
}

bool parseAnimationFromFilename(const std::string& sourcePath,
                                const std::string& regexPattern,
                                std::string& outSet,
                                std::string& outState,
                                std::string& outDirection,
                                int& outFrame,
                                std::string& error) {
    try {
        const std::regex pattern(regexPattern);
        const std::string stem = std::filesystem::path(sourcePath).stem().string();
        std::smatch match;
        if (!std::regex_match(stem, match, pattern)) {
            error = "filename `" + stem + "` does not match animationNamingRegex";
            return false;
        }
        if (match.size() < 5) {
            error = "animationNamingRegex must provide 4 capture groups: set/state/direction/frame";
            return false;
        }
        outSet = match[1].str();
        outState = match[2].str();
        outDirection = match[3].str();
        outFrame = std::stoi(match[4].str());
        return true;
    } catch (const std::exception& ex) {
        error = std::string("invalid animationNamingRegex: ") + ex.what();
        return false;
    }
}

bool parseVariantFromFilename(const std::string& sourcePath,
                              const std::string& regexPattern,
                              std::string& outGroup,
                              std::string& outVariant,
                              std::string& error) {
    try {
        const std::regex pattern(regexPattern);
        const std::string stem = std::filesystem::path(sourcePath).stem().string();
        std::smatch match;
        if (!std::regex_match(stem, match, pattern)) {
            error = "filename `" + stem + "` does not match variantNamingRegex";
            return false;
        }
        if (match.size() < 3) {
            error = "variantNamingRegex must provide 2 capture groups: group/variant";
            return false;
        }
        outGroup = match[1].str();
        outVariant = match[2].str();
        return true;
    } catch (const std::exception& ex) {
        error = std::string("invalid variantNamingRegex: ") + ex.what();
        return false;
    }
}

} // namespace

std::vector<std::string> splitPackPaths(const std::string& joinedPaths) {
    std::vector<std::string> out;
    std::string token;
    for (char c : joinedPaths) {
        if (c == ';' || c == ',') {
            token = trim(token);
            if (!token.empty()) out.push_back(token);
            token.clear();
        } else {
            token.push_back(c);
        }
    }
    token = trim(token);
    if (!token.empty()) out.push_back(token);
    return out;
}

std::string fnv1a64Hex(const std::string& bytes) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char b : bytes) {
        hash ^= static_cast<std::uint64_t>(b);
        hash *= 1099511628211ULL;
    }
    std::ostringstream os;
    os << std::hex << hash;
    return os.str();
}

std::string stableGuidForAsset(const std::string& kind, const std::string& logicalKey) {
    return kind + "-" + fnv1a64Hex(toLower(kind + ":" + logicalKey));
}

bool migratePackJson(nlohmann::json& json, std::string& message) {
    int schema = json.value("schemaVersion", 1);
    if (schema > 2) {
        message = "Unsupported schemaVersion=" + std::to_string(schema) + ". Please run ContentPacker migration tooling.";
        return false;
    }

    if (schema == 1) {
        auto migrateArray = [](nlohmann::json& root, const char* key, const char* kind) {
            if (!root.contains(key) || !root[key].is_array()) return;
            for (auto& item : root[key]) {
                if (!item.is_object()) continue;
                if (!item.contains("guid") || !item["guid"].is_string()) {
                    const std::string logicalKey = item.value("id", item.value("name", std::string("unnamed")));
                    item["guid"] = stableGuidForAsset(kind, logicalKey);
                }
            }
        };
        migrateArray(json, "patterns", "pattern");
        migrateArray(json, "entities", "enemy");
        migrateArray(json, "traits", "trait");
        migrateArray(json, "archetypes", "archetype");
        migrateArray(json, "encounters", "encounter");
        json["schemaVersion"] = 2;
        message = "Migrated content pack schema from v1 to v2 (added GUID fields).";
    }

    return true;
}

bool parsePackMetadata(const nlohmann::json& json, PackMetadata& out, std::string& error) {
    out.schemaVersion = json.value("schemaVersion", 1);
    out.packVersion = json.value("packVersion", 1);
    out.packId = json.value("packId", std::string("default-pack"));
    out.contentHash = json.value("contentHash", std::string());

    if (json.contains("compatibility") && json["compatibility"].is_object()) {
        const auto& c = json["compatibility"];
        out.minRuntimePackVersion = c.value("minRuntimePackVersion", 1);
        out.maxRuntimePackVersion = c.value("maxRuntimePackVersion", 9999);
    }

    if (out.schemaVersion > 2) {
        error = "Unsupported content schemaVersion=" + std::to_string(out.schemaVersion);
        return false;
    }

    return true;
}

bool parseSourceArtManifest(const nlohmann::json& manifest,
                            const std::string& manifestPath,
                            std::vector<SourceArtAssetRecord>& out,
                            std::vector<ArtImportValidationError>& errors) {
    if (!manifest.contains("assets") || !manifest["assets"].is_array()) {
        errors.push_back({manifestPath, "`assets` array missing from art import manifest"});
        return false;
    }

    bool ok = true;
    for (std::size_t index = 0; index < manifest["assets"].size(); ++index) {
        const auto& asset = manifest["assets"][index];
        if (!asset.is_object()) {
            errors.push_back({manifestPath, "asset[" + std::to_string(index) + "] must be an object"});
            ok = false;
            continue;
        }

        const std::string sourcePath = asset.value("source", std::string());
        if (sourcePath.empty()) {
            errors.push_back({manifestPath, "asset[" + std::to_string(index) + "] missing required `source`"});
            ok = false;
            continue;
        }

        SourceArtAssetRecord rec;
        rec.sourcePath = sourcePath;
        rec.manifestPath = manifestPath;
        rec.name = asset.value("name", std::filesystem::path(sourcePath).stem().string());
        const std::string kind = normalizeAssetKind(asset.value("kind", std::string("sprite")));
        rec.kind = (kind == "texture") ? ArtAssetKind::Texture : ArtAssetKind::Sprite;
        rec.guid = asset.value("guid", stableGuidForAsset(kind, sourcePath));

        if (asset.contains("settings") && asset["settings"].is_object()) {
            const auto& s = asset["settings"];
            rec.settings.colorWorkflow = toLower(s.value("colorWorkflow", rec.settings.colorWorkflow));
            rec.settings.pivotX = s.value("pivotX", rec.settings.pivotX);
            rec.settings.pivotY = s.value("pivotY", rec.settings.pivotY);
            rec.settings.collisionBoundsPolicy = toLower(s.value("collisionBoundsPolicy", rec.settings.collisionBoundsPolicy));
            rec.settings.atlasGroup = s.value("atlasGroup", rec.settings.atlasGroup);
            rec.settings.filter = toLower(s.value("filter", rec.settings.filter));
            rec.settings.mipPreference = toLower(s.value("mipPreference", rec.settings.mipPreference));
            rec.settings.variantGroup = s.value("variantGroup", rec.settings.variantGroup);
            rec.settings.animationGroup = s.value("animationGroup", rec.settings.animationGroup);
            rec.settings.animationFps = s.value("animationFps", rec.settings.animationFps);
            rec.settings.animationSequenceFromFilename = s.value("animationSequenceFromFilename", rec.settings.animationSequenceFromFilename);
            rec.settings.animationNamingRegex = s.value("animationNamingRegex", rec.settings.animationNamingRegex);
            rec.settings.animationSet = s.value("animationSet", rec.settings.animationSet);
            rec.settings.animationState = s.value("animationState", rec.settings.animationState);
            rec.settings.animationDirection = s.value("animationDirection", rec.settings.animationDirection);
            rec.settings.animationFrame = s.value("animationFrame", rec.settings.animationFrame);
            rec.settings.variantNamingRegex = s.value("variantNamingRegex", rec.settings.variantNamingRegex);
            rec.settings.variantName = s.value("variantName", rec.settings.variantName);
            rec.settings.variantWeight = s.value("variantWeight", rec.settings.variantWeight);
            rec.settings.paletteTemplate = s.value("paletteTemplate", rec.settings.paletteTemplate);
        }

        if (!isKnownColorWorkflow(rec.settings.colorWorkflow)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` has unsupported colorWorkflow"});
            ok = false;
        }
        if (!isKnownCollisionPolicy(rec.settings.collisionBoundsPolicy)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` has unsupported collisionBoundsPolicy"});
            ok = false;
        }
        if (!isKnownFilter(rec.settings.filter)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` has unsupported filter"});
            ok = false;
        }
        if (!isKnownMipPreference(rec.settings.mipPreference)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` has unsupported mipPreference"});
            ok = false;
        }
        if (rec.settings.pivotX < 0.0F || rec.settings.pivotX > 1.0F || rec.settings.pivotY < 0.0F || rec.settings.pivotY > 1.0F) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` pivot must be in [0,1]"});
            ok = false;
        }
        if (!rec.settings.animationGroup.empty() && !isIdentifierLike(rec.settings.animationGroup)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` animationGroup contains unsupported characters"});
            ok = false;
        }
        if (!rec.settings.animationSet.empty() && !isIdentifierLike(rec.settings.animationSet)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` animationSet contains unsupported characters"});
            ok = false;
        }
        if (!rec.settings.animationState.empty() && !isIdentifierLike(rec.settings.animationState)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` animationState contains unsupported characters"});
            ok = false;
        }
        if (!rec.settings.animationDirection.empty() && !isIdentifierLike(rec.settings.animationDirection)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` animationDirection contains unsupported characters"});
            ok = false;
        }
        if (!rec.settings.variantGroup.empty() && !isIdentifierLike(rec.settings.variantGroup)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` variantGroup contains unsupported characters"});
            ok = false;
        }
        if (!rec.settings.variantName.empty() && !isIdentifierLike(rec.settings.variantName)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` variantName contains unsupported characters"});
            ok = false;
        }
        if (!hasValidWeight(rec.settings.variantWeight)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` variantWeight must be > 0"});
            ok = false;
        }
        if (rec.settings.animationFps < 0.0F || !std::isfinite(rec.settings.animationFps)) {
            errors.push_back({manifestPath, "asset `" + rec.sourcePath + "` animationFps must be >= 0"});
            ok = false;
        }

        out.push_back(rec);
    }

    return ok;
}

bool importSourceArtAssets(const std::vector<SourceArtAssetRecord>& sourceAssets,
                           std::vector<ImportedArtAssetRecord>& imported,
                           std::vector<ArtImportValidationError>& errors) {
    bool ok = true;

    for (const auto& source : sourceAssets) {
        if (source.sourcePath.empty()) {
            errors.push_back({source.manifestPath, "asset has empty source path"});
            ok = false;
            continue;
        }

        if (!std::filesystem::exists(source.sourcePath)) {
            errors.push_back({source.manifestPath, "source art file missing: " + source.sourcePath});
            ok = false;
            continue;
        }

        const std::string ext = toLower(std::filesystem::path(source.sourcePath).extension().string());
        if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".bmp" && ext != ".tga") {
            errors.push_back({source.manifestPath, "unsupported art format for " + source.sourcePath});
            ok = false;
            continue;
        }

        ImportedArtAssetRecord record;
        record.source = source;

        const auto fileSize = std::filesystem::file_size(source.sourcePath);
        const auto writeTime = std::filesystem::last_write_time(source.sourcePath).time_since_epoch().count();

        nlohmann::json settingsJson = {
            {"colorWorkflow", source.settings.colorWorkflow},
            {"pivotX", source.settings.pivotX},
            {"pivotY", source.settings.pivotY},
            {"collisionBoundsPolicy", source.settings.collisionBoundsPolicy},
            {"atlasGroup", source.settings.atlasGroup},
            {"filter", source.settings.filter},
            {"mipPreference", source.settings.mipPreference},
            {"variantGroup", source.settings.variantGroup},
            {"animationGroup", source.settings.animationGroup},
            {"animationFps", source.settings.animationFps},
            {"animationSequenceFromFilename", source.settings.animationSequenceFromFilename},
            {"animationNamingRegex", source.settings.animationNamingRegex},
            {"animationSet", source.settings.animationSet},
            {"animationState", source.settings.animationState},
            {"animationDirection", source.settings.animationDirection},
            {"animationFrame", source.settings.animationFrame},
            {"variantNamingRegex", source.settings.variantNamingRegex},
            {"variantName", source.settings.variantName},
            {"variantWeight", source.settings.variantWeight},
            {"paletteTemplate", source.settings.paletteTemplate},
        };

        record.sourceFingerprint = fnv1a64Hex(source.sourcePath + ":" + std::to_string(fileSize) + ":" + std::to_string(writeTime));
        record.settingsFingerprint = jsonFingerprint(settingsJson);
        record.importFingerprint = fnv1a64Hex(record.sourceFingerprint + ":" + record.settingsFingerprint);
        record.dependencies = {source.sourcePath, source.manifestPath};

        imported.push_back(record);
    }

    return ok;
}

std::vector<AtlasBuildPlan> buildAtlasPlans(const std::vector<ImportedArtAssetRecord>& importedAssets) {
    std::map<std::string, AtlasBuildPlan> plans;

    for (const auto& asset : importedAssets) {
        std::string key = asset.source.settings.atlasGroup + ":" + asset.source.settings.colorWorkflow;
        auto& plan = plans[key];
        plan.group = asset.source.settings.atlasGroup;
        plan.colorWorkflow = asset.source.settings.colorWorkflow;
        plan.assetGuids.push_back(asset.source.guid);
    }

    std::vector<AtlasBuildPlan> out;
    out.reserve(plans.size());
    for (auto& [key, plan] : plans) {
        (void)key;
        out.push_back(plan);
    }
    return out;
}


std::vector<AnimationClipBuildPlan> buildAnimationClipPlans(const std::vector<ImportedArtAssetRecord>& importedAssets,
                                                            std::vector<ArtImportValidationError>& errors) {
    std::map<std::string, std::map<int, std::string>> groupedFrames;
    std::map<std::string, AnimationClipBuildPlan> plans;

    for (const auto& asset : importedAssets) {
        std::string set = asset.source.settings.animationSet;
        std::string state = asset.source.settings.animationState;
        std::string direction = asset.source.settings.animationDirection;
        int frame = asset.source.settings.animationFrame;

        if (asset.source.settings.animationSequenceFromFilename) {
            const std::string pattern = asset.source.settings.animationNamingRegex.empty()
                                            ? std::string("^([A-Za-z0-9_-]+)_([A-Za-z0-9_-]+)_([A-Za-z0-9_-]+)_([0-9]+)$")
                                            : asset.source.settings.animationNamingRegex;
            std::string parseError;
            if (!parseAnimationFromFilename(asset.source.sourcePath, pattern, set, state, direction, frame, parseError)) {
                errors.push_back({asset.source.manifestPath, "asset `" + asset.source.sourcePath + "` " + parseError});
                continue;
            }
        } else if (!asset.source.settings.animationGroup.empty() && (set.empty() || state.empty())) {
            std::string parseError;
            const std::string pattern = "^([A-Za-z0-9_-]+)_([A-Za-z0-9_-]+)(?:_([A-Za-z0-9_-]+))?_([0-9]+)$";
            if (!parseAnimationFromFilename(asset.source.sourcePath, pattern, set, state, direction, frame, parseError)) {
                errors.push_back({asset.source.manifestPath,
                                  "asset `" + asset.source.sourcePath + "` animationGroup requires animationSet/animationState/animationFrame or filename convention"});
                continue;
            }
        }

        if (set.empty() || state.empty()) continue;
        if (frame < 0) {
            errors.push_back({asset.source.manifestPath,
                              "asset `" + asset.source.sourcePath + "` animation clip frame index must be provided"});
            continue;
        }

        const float fps = asset.source.settings.animationFps > 0.0F ? asset.source.settings.animationFps : 12.0F;
        const std::string key = set + ":" + state + ":" + direction;
        auto& plan = plans[key];
        if (plan.animationSet.empty()) {
            plan.animationSet = set;
            plan.state = state;
            plan.direction = direction;
            plan.fps = fps;
        } else if (std::abs(plan.fps - fps) > 0.001F) {
            errors.push_back({asset.source.manifestPath,
                              "animation clip `" + key + "` has inconsistent FPS across frames"});
            continue;
        }

        auto& frameMap = groupedFrames[key];
        if (frameMap.find(frame) != frameMap.end()) {
            errors.push_back({asset.source.manifestPath,
                              "animation clip `" + key + "` duplicates frame index " + std::to_string(frame)});
            continue;
        }
        frameMap[frame] = asset.source.guid;
    }

    std::vector<AnimationClipBuildPlan> out;
    for (auto& [key, plan] : plans) {
        (void)key;
        const auto frameIt = groupedFrames.find(plan.animationSet + ":" + plan.state + ":" + plan.direction);
        if (frameIt == groupedFrames.end()) continue;
        int expected = 0;
        for (const auto& [frame, guid] : frameIt->second) {
            if (frame != expected) {
                errors.push_back({"animation-plan", "animation clip `" + plan.animationSet + ":" + plan.state + ":" + plan.direction
                    + "` has non-contiguous frame index at " + std::to_string(frame) + ", expected " + std::to_string(expected)});
                expected = frame;
            }
            plan.frameAssetGuids.push_back(guid);
            ++expected;
        }
        out.push_back(plan);
    }
    return out;
}

std::vector<VariantGroupBuildPlan> buildVariantGroupPlans(const std::vector<ImportedArtAssetRecord>& importedAssets,
                                                          std::vector<ArtImportValidationError>& errors) {
    std::map<std::string, VariantGroupBuildPlan> groups;
    std::map<std::string, std::set<std::string>> variantNamesByGroup;

    for (const auto& asset : importedAssets) {
        std::string group = asset.source.settings.variantGroup;
        std::string variant = asset.source.settings.variantName;

        if (!asset.source.settings.variantNamingRegex.empty()) {
            std::string parseError;
            if (!parseVariantFromFilename(asset.source.sourcePath, asset.source.settings.variantNamingRegex, group, variant, parseError)) {
                errors.push_back({asset.source.manifestPath, "asset `" + asset.source.sourcePath + "` " + parseError});
                continue;
            }
        }

        if (group.empty()) continue;
        if (variant.empty()) variant = std::filesystem::path(asset.source.sourcePath).stem().string();

        auto& build = groups[group];
        build.group = group;
        if (variantNamesByGroup[group].find(variant) != variantNamesByGroup[group].end()) {
            errors.push_back({asset.source.manifestPath,
                              "variant group `" + group + "` has duplicate variant name `" + variant + "`"});
            continue;
        }
        variantNamesByGroup[group].insert(variant);

        build.options.push_back({
            asset.source.guid,
            variant,
            asset.source.settings.variantWeight,
            asset.source.settings.paletteTemplate,
        });
    }

    std::vector<VariantGroupBuildPlan> out;
    out.reserve(groups.size());
    for (auto& [name, group] : groups) {
        (void)name;
        if (group.options.empty()) continue;
        out.push_back(group);
    }
    return out;
}

std::unordered_map<std::string, std::string> extractImportFingerprintByGuid(const nlohmann::json& pack) {
    std::unordered_map<std::string, std::string> out;
    if (!pack.contains("importRegistry") || !pack["importRegistry"].is_array()) return out;

    for (const auto& entry : pack["importRegistry"]) {
        if (!entry.is_object()) continue;
        const std::string guid = entry.value("guid", std::string());
        const std::string fingerprint = entry.value("importFingerprint", std::string());
        if (!guid.empty() && !fingerprint.empty()) out[guid] = fingerprint;
    }

    return out;
}



bool parseAudioContentDatabase(const nlohmann::json& doc, AudioContentDatabase& out, std::string& error) {
    out = {};
    if (!doc.contains("audio") || !doc["audio"].is_object()) {
        return true;
    }

    const auto& audio = doc["audio"];
    if (audio.contains("clips")) {
        if (!audio["clips"].is_array()) {
            error = "`audio.clips` must be an array";
            return false;
        }
        for (const auto& clipJson : audio["clips"]) {
            if (!clipJson.is_object()) {
                error = "`audio.clips[]` must be objects";
                return false;
            }
            AudioClipRecord clip;
            clip.id = clipJson.value("id", std::string());
            clip.path = clipJson.value("path", std::string());
            const std::string bus = toLower(clipJson.value("bus", std::string("sfx")));
            if (bus == "music") clip.bus = AudioBus::Music;
            else if (bus == "master") clip.bus = AudioBus::Master;
            else clip.bus = AudioBus::Sfx;
            clip.loop = clipJson.value("loop", false);
            clip.baseGain = clipJson.value("baseGain", 1.0F);
            if (clip.id.empty()) {
                error = "audio clip id cannot be empty";
                return false;
            }
            out.clips.push_back(std::move(clip));
        }
    }

    if (audio.contains("events")) {
        if (!audio["events"].is_array()) {
            error = "`audio.events` must be an array";
            return false;
        }
        for (const auto& eventJson : audio["events"]) {
            if (!eventJson.is_object()) {
                error = "`audio.events[]` must be objects";
                return false;
            }
            AudioEventBinding binding;
            const std::string name = toLower(eventJson.value("name", std::string()));
            if (name == "hit") binding.event = AudioEventId::Hit;
            else if (name == "graze") binding.event = AudioEventId::Graze;
            else if (name == "player_damage") binding.event = AudioEventId::PlayerDamage;
            else if (name == "enemy_death") binding.event = AudioEventId::EnemyDeath;
            else if (name == "boss_warning") binding.event = AudioEventId::BossWarning;
            else if (name == "ui_click") binding.event = AudioEventId::UiClick;
            else if (name == "ui_confirm") binding.event = AudioEventId::UiConfirm;
            else {
                error = "unsupported audio event name: " + name;
                return false;
            }
            binding.clipId = eventJson.value("clip", std::string());
            binding.gain = eventJson.value("gain", 1.0F);
            if (binding.clipId.empty()) {
                error = "audio event clip id cannot be empty";
                return false;
            }
            out.events.push_back(std::move(binding));
        }
    }

    out.musicClipId = audio.value("music", std::string());
    return true;
}
} // namespace engine
