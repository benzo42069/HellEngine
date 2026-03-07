#include <engine/content_pipeline.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
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

} // namespace engine
