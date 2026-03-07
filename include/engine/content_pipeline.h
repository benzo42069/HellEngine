#pragma once

#include <nlohmann/json_fwd.hpp>

#include <engine/audio_system.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine {

struct PackMetadata {
    std::string packId;
    int schemaVersion {1};
    int packVersion {1};
    std::string contentHash;
    int minRuntimePackVersion {1};
    int maxRuntimePackVersion {9999};
};

enum class ArtAssetKind {
    Sprite,
    Texture,
};

struct ArtImportSettings {
    std::string colorWorkflow {"full-color"};
    float pivotX {0.5F};
    float pivotY {0.5F};
    std::string collisionBoundsPolicy {"tight"};
    std::string atlasGroup {"default"};
    std::string filter {"linear"};
    std::string mipPreference {"none"};
    std::string variantGroup;
    std::string animationGroup;
    float animationFps {0.0F};
    bool animationSequenceFromFilename {false};
    std::string animationNamingRegex;
    std::string animationSet;
    std::string animationState;
    std::string animationDirection;
    int animationFrame {-1};
    std::string variantNamingRegex;
    std::string variantName;
    float variantWeight {1.0F};
    std::string paletteTemplate;
};

struct SourceArtAssetRecord {
    std::string guid;
    std::string name;
    std::string sourcePath;
    ArtAssetKind kind {ArtAssetKind::Sprite};
    ArtImportSettings settings;
    std::string manifestPath;
};

struct ImportedArtAssetRecord {
    SourceArtAssetRecord source;
    std::string sourceFingerprint;
    std::string settingsFingerprint;
    std::string importFingerprint;
    std::vector<std::string> dependencies;
};

struct AtlasBuildPlan {
    std::string group;
    std::vector<std::string> assetGuids;
    std::string colorWorkflow;
};

struct AnimationClipBuildPlan {
    std::string animationSet;
    std::string state;
    std::string direction;
    float fps {0.0F};
    std::vector<std::string> frameAssetGuids;
};

struct VariantOptionBuildPlan {
    std::string assetGuid;
    std::string variantName;
    float weight {1.0F};
    std::string paletteTemplate;
};

struct VariantGroupBuildPlan {
    std::string group;
    std::vector<VariantOptionBuildPlan> options;
};

struct ArtImportValidationError {
    std::string file;
    std::string message;
};

std::vector<std::string> splitPackPaths(const std::string& joinedPaths);
std::string stableGuidForAsset(const std::string& kind, const std::string& logicalKey);
std::string fnv1a64Hex(const std::string& bytes);
bool migratePackJson(nlohmann::json& json, std::string& message);
bool parsePackMetadata(const nlohmann::json& json, PackMetadata& out, std::string& error);
bool parseSourceArtManifest(const nlohmann::json& manifest,
                            const std::string& manifestPath,
                            std::vector<SourceArtAssetRecord>& out,
                            std::vector<ArtImportValidationError>& errors);
bool importSourceArtAssets(const std::vector<SourceArtAssetRecord>& sourceAssets,
                           std::vector<ImportedArtAssetRecord>& imported,
                           std::vector<ArtImportValidationError>& errors);
std::vector<AtlasBuildPlan> buildAtlasPlans(const std::vector<ImportedArtAssetRecord>& importedAssets);
std::vector<AnimationClipBuildPlan> buildAnimationClipPlans(const std::vector<ImportedArtAssetRecord>& importedAssets,
                                                            std::vector<ArtImportValidationError>& errors);
std::vector<VariantGroupBuildPlan> buildVariantGroupPlans(const std::vector<ImportedArtAssetRecord>& importedAssets,
                                                          std::vector<ArtImportValidationError>& errors);
std::unordered_map<std::string, std::string> extractImportFingerprintByGuid(const nlohmann::json& pack);
bool parseAudioContentDatabase(const nlohmann::json& doc, AudioContentDatabase& out, std::string& error);

} // namespace engine
