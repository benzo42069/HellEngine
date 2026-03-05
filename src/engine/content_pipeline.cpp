#include <engine/content_pipeline.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
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

} // namespace engine
