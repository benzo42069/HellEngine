#include <engine/content_pipeline.h>
#include <engine/palette_fx_templates.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct ValidationError {
    std::string file;
    std::string message;
};

bool hasNumber(const nlohmann::json& obj, const char* key) {
    return obj.contains(key) && obj[key].is_number();
}

void ensureGuids(nlohmann::json& arr, const char* kind, const char* fallbackName) {
    if (!arr.is_array()) return;
    for (auto& item : arr) {
        if (!item.is_object()) continue;
        if (!item.contains("guid") || !item["guid"].is_string()) {
            const std::string logicalKey = item.value("id", item.value("name", std::string(fallbackName)));
            item["guid"] = engine::stableGuidForAsset(kind, logicalKey);
        }
    }
}

bool validatePatterns(const nlohmann::json& doc, std::vector<ValidationError>& errors, const std::string& file) {
    bool ok = true;
    if (!doc["patterns"].is_array()) {
        errors.push_back({file, "`patterns` must be array"});
        return false;
    }
    for (std::size_t pidx = 0; pidx < doc["patterns"].size(); ++pidx) {
        const auto& pattern = doc["patterns"][pidx];
        if (!pattern.is_object()) {
            errors.push_back({file, "pattern[" + std::to_string(pidx) + "] must be object"});
            ok = false;
            continue;
        }
        if (!pattern.contains("name") || !pattern["name"].is_string()) {
            errors.push_back({file, "pattern[" + std::to_string(pidx) + "] missing string `name`"});
            ok = false;
        }
    }
    return ok;
}

bool validateEntities(const nlohmann::json& doc, std::vector<ValidationError>& errors, const std::string& file) {
    bool ok = true;
    if (!doc["entities"].is_array()) {
        errors.push_back({file, "`entities` must be array"});
        return false;
    }
    for (std::size_t i = 0; i < doc["entities"].size(); ++i) {
        const auto& e = doc["entities"][i];
        if (!e.is_object()) {
            errors.push_back({file, "entity[" + std::to_string(i) + "] must be object"});
            ok = false;
            continue;
        }
        if (!e.contains("name") || !e["name"].is_string()) {
            errors.push_back({file, "entity[" + std::to_string(i) + "] missing string `name`"});
            ok = false;
        }
        if (!hasNumber(e, "maxHealth")) {
            errors.push_back({file, "entity[" + std::to_string(i) + "] missing numeric `maxHealth`"});
            ok = false;
        }
    }
    return ok;
}

void mergeByGuid(const nlohmann::json& src, nlohmann::json& dst, std::vector<std::string>& conflicts) {
    std::map<std::string, std::size_t> guidToIndex;
    for (std::size_t i = 0; i < dst.size(); ++i) {
        const std::string guid = dst[i].value("guid", std::string());
        if (!guid.empty()) guidToIndex[guid] = i;
    }

    for (const auto& item : src) {
        const std::string guid = item.value("guid", std::string());
        if (guid.empty()) continue;
        if (auto found = guidToIndex.find(guid); found != guidToIndex.end()) {
            conflicts.push_back(guid);
            dst[found->second] = item;
        } else {
            guidToIndex[guid] = dst.size();
            dst.push_back(item);
        }
    }
}

void appendAssetRegistry(nlohmann::json& pack, const nlohmann::json& arr, const char* kind) {
    for (const auto& item : arr) {
        nlohmann::json rec;
        rec["kind"] = kind;
        rec["guid"] = item.value("guid", "");
        rec["name"] = item.value("name", item.value("id", "unnamed"));
        pack["assetRegistry"].push_back(rec);
    }
}

} // namespace

int main(int argc, char** argv) {
    fs::path inputDir = "data";
    fs::path outputPak = "content.pak";
    std::string packId = "base";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--input" && i + 1 < argc) {
            inputDir = argv[++i];
            continue;
        }
        if (arg == "--output" && i + 1 < argc) {
            outputPak = argv[++i];
            continue;
        }
        if (arg == "--pack-id" && i + 1 < argc) {
            packId = argv[++i];
            continue;
        }
    }

    if (!fs::exists(inputDir)) {
        std::cerr << "Input directory does not exist: " << inputDir << "\n";
        return 1;
    }

    std::vector<ValidationError> errors;
    nlohmann::json mergedPatterns = nlohmann::json::array();
    nlohmann::json mergedEntities = nlohmann::json::array();
    nlohmann::json mergedTraits = nlohmann::json::array();
    nlohmann::json mergedArchetypes = nlohmann::json::array();
    nlohmann::json mergedEncounters = nlohmann::json::array();
    nlohmann::json mergedPaletteTemplates = nlohmann::json::array();
    nlohmann::json mergedGradients = nlohmann::json::array();
    nlohmann::json mergedFxPresets = nlohmann::json::array();
    std::vector<std::string> conflicts;

    for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

        std::ifstream in(entry.path());
        if (!in.good()) {
            errors.push_back({entry.path().string(), "cannot open file"});
            continue;
        }

        nlohmann::json doc;
        in >> doc;

        std::string migrationMessage;
        if (!engine::migratePackJson(doc, migrationMessage)) {
            errors.push_back({entry.path().string(), migrationMessage});
            continue;
        }

        if (doc.contains("patterns")) {
            if (validatePatterns(doc, errors, entry.path().string())) {
                ensureGuids(doc["patterns"], "pattern", "pattern");
                mergeByGuid(doc["patterns"], mergedPatterns, conflicts);
            }
        }

        if (doc.contains("entities")) {
            if (validateEntities(doc, errors, entry.path().string())) {
                ensureGuids(doc["entities"], "enemy", "entity");
                mergeByGuid(doc["entities"], mergedEntities, conflicts);
            }
        }

        if (doc.contains("traits") && doc["traits"].is_array()) {
            ensureGuids(doc["traits"], "trait", "trait");
            mergeByGuid(doc["traits"], mergedTraits, conflicts);
        }
        if (doc.contains("archetypes") && doc["archetypes"].is_array()) {
            ensureGuids(doc["archetypes"], "archetype", "archetype");
            mergeByGuid(doc["archetypes"], mergedArchetypes, conflicts);
        }
        if (doc.contains("encounters") && doc["encounters"].is_array()) {
            ensureGuids(doc["encounters"], "encounter", "encounter");
            mergeByGuid(doc["encounters"], mergedEncounters, conflicts);
        }
        if (doc.contains("paletteTemplates") && doc["paletteTemplates"].is_array()) {
            ensureGuids(doc["paletteTemplates"], "palette-template", "palette-template");
            mergeByGuid(doc["paletteTemplates"], mergedPaletteTemplates, conflicts);
        }
        if (doc.contains("gradients") && doc["gradients"].is_array()) {
            ensureGuids(doc["gradients"], "gradient", "gradient");
            mergeByGuid(doc["gradients"], mergedGradients, conflicts);
        }
        if (doc.contains("fxPresets") && doc["fxPresets"].is_array()) {
            ensureGuids(doc["fxPresets"], "fx-preset", "fx-preset");
            mergeByGuid(doc["fxPresets"], mergedFxPresets, conflicts);
        }
    }

    if (!errors.empty()) {
        for (const auto& e : errors) {
            std::cerr << "[schema] " << e.file << ": " << e.message << "\n";
        }
        std::cerr << "Validation failed; no pack generated.\n";
        return 2;
    }

    nlohmann::json pack;
    pack["packId"] = packId;
    pack["schemaVersion"] = 2;
    pack["packVersion"] = 3;
    pack["sourceDir"] = inputDir.string();
    pack["compatibility"] = {
        {"minRuntimePackVersion", 2},
        {"maxRuntimePackVersion", 3},
        {"notes", "Schema v2 requires GUID-based references; regenerate packs after schema edits."},
    };
    pack["patterns"] = mergedPatterns;
    pack["entities"] = mergedEntities;
    pack["traits"] = mergedTraits;
    pack["archetypes"] = mergedArchetypes;
    pack["encounters"] = mergedEncounters;
    pack["paletteTemplates"] = mergedPaletteTemplates;
    pack["gradients"] = mergedGradients;
    pack["fxPresets"] = mergedFxPresets;
    pack["assetRegistry"] = nlohmann::json::array();
    appendAssetRegistry(pack, mergedPatterns, "pattern");
    appendAssetRegistry(pack, mergedEntities, "enemy");
    appendAssetRegistry(pack, mergedTraits, "trait");
    appendAssetRegistry(pack, mergedArchetypes, "archetype");
    appendAssetRegistry(pack, mergedEncounters, "encounter");
    appendAssetRegistry(pack, mergedPaletteTemplates, "palette-template");
    appendAssetRegistry(pack, mergedGradients, "gradient");
    appendAssetRegistry(pack, mergedFxPresets, "fx-preset");

    pack["gradientLuts"] = nlohmann::json::array();
    for (const auto& g : mergedGradients) {
        engine::GradientDefinition gd;
        gd.name = g.value("name", "");
        if (g.contains("stops") && g["stops"].is_array()) {
            for (const auto& st : g["stops"]) {
                engine::GradientStop stop;
                stop.position = st.value("position", 0.0F);
                std::string error;
                engine::parseHexColor(st.value("color", "#FFFFFF"), stop.color, &error);
                gd.stops.push_back(stop);
            }
        }
        auto lut = engine::generateGradientLut(gd, 256);
        nlohmann::json lutRec;
        lutRec["name"] = gd.name;
        lutRec["width"] = lut.size();
        lutRec["pixels"] = nlohmann::json::array();
        for (const auto& c : lut) lutRec["pixels"].push_back(engine::toHexColor(c));
        pack["gradientLuts"].push_back(lutRec);
    }

    const std::string payloadNoHash = pack.dump();
    pack["contentHash"] = engine::fnv1a64Hex(payloadNoHash);

    if (!conflicts.empty()) {
        std::set<std::string> unique(conflicts.begin(), conflicts.end());
        pack["conflicts"] = nlohmann::json::array();
        for (const auto& c : unique) {
            pack["conflicts"].push_back(c);
            std::cerr << "[conflict] GUID override detected: " << c << "\n";
        }
    }

    std::ofstream out(outputPak, std::ios::binary);
    if (!out.good()) {
        std::cerr << "Unable to write output: " << outputPak << "\n";
        return 3;
    }

    const std::string payload = pack.dump();
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));

    std::cout << "Packed patterns=" << mergedPatterns.size() << " entities=" << mergedEntities.size() << " traits=" << mergedTraits.size()
              << " archetypes=" << mergedArchetypes.size() << " encounters=" << mergedEncounters.size()
              << " palettes=" << mergedPaletteTemplates.size() << " gradients=" << mergedGradients.size() << " fxPresets=" << mergedFxPresets.size()
              << " -> " << outputPak << "\n";
    return 0;
}
