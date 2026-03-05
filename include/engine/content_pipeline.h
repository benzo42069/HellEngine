#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <string>
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

std::vector<std::string> splitPackPaths(const std::string& joinedPaths);
std::string stableGuidForAsset(const std::string& kind, const std::string& logicalKey);
std::string fnv1a64Hex(const std::string& bytes);
bool migratePackJson(nlohmann::json& json, std::string& message);
bool parsePackMetadata(const nlohmann::json& json, PackMetadata& out, std::string& error);

} // namespace engine
