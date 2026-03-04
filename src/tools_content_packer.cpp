#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct ValidationError {
    std::string file;
    std::string message;
};

bool hasNumber(const nlohmann::json& j, const char* key) {
    return j.contains(key) && (j[key].is_number_float() || j[key].is_number_integer() || j[key].is_number_unsigned());
}

bool validateLayer(const nlohmann::json& layer, std::vector<ValidationError>& errors, const std::string& file, std::size_t idx) {
    bool ok = true;
    if (!layer.is_object()) {
        errors.push_back({file, "layer[" + std::to_string(idx) + "] must be object"});
        return false;
    }
    if (!layer.contains("name") || !layer["name"].is_string()) {
        errors.push_back({file, "layer[" + std::to_string(idx) + "] missing string `name`"});
        ok = false;
    }
    if (!layer.contains("type") || !layer["type"].is_string()) {
        errors.push_back({file, "layer[" + std::to_string(idx) + "] missing string `type`"});
        ok = false;
    }
    if (!hasNumber(layer, "bulletCount") || !hasNumber(layer, "bulletSpeed") || !hasNumber(layer, "cooldownSeconds")) {
        errors.push_back({file, "layer[" + std::to_string(idx) + "] requires numeric bulletCount, bulletSpeed, cooldownSeconds"});
        ok = false;
    }
    return ok;
}

bool validatePatterns(const nlohmann::json& doc, std::vector<ValidationError>& errors, const std::string& file) {
    bool ok = true;
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
        if (pattern.contains("layers")) {
            if (!pattern["layers"].is_array() || pattern["layers"].empty()) {
                errors.push_back({file, "pattern[" + std::to_string(pidx) + "].layers must be non-empty array"});
                ok = false;
            } else {
                for (std::size_t lidx = 0; lidx < pattern["layers"].size(); ++lidx) {
                    ok = validateLayer(pattern["layers"][lidx], errors, file, lidx) && ok;
                }
            }
        } else {
            ok = validateLayer(pattern, errors, file, 0) && ok;
        }
    }
    return ok;
}

bool validateEntities(const nlohmann::json& doc, std::vector<ValidationError>& errors, const std::string& file) {
    bool ok = true;
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
        if (!e.contains("type") || !e["type"].is_string()) {
            errors.push_back({file, "entity[" + std::to_string(i) + "] missing string `type`"});
            ok = false;
        }
        if (!e.contains("movement") || !e["movement"].is_string()) {
            errors.push_back({file, "entity[" + std::to_string(i) + "] missing string `movement`"});
            ok = false;
        }
        if (!hasNumber(e, "maxHealth")) {
            errors.push_back({file, "entity[" + std::to_string(i) + "] missing numeric `maxHealth`"});
            ok = false;
        }
    }
    return ok;
}

} // namespace

int main(int argc, char** argv) {
    fs::path inputDir = "data";
    fs::path outputPak = "content.pak";

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
    }

    if (!fs::exists(inputDir)) {
        std::cerr << "Input directory does not exist: " << inputDir << "\n";
        return 1;
    }

    std::vector<ValidationError> errors;
    nlohmann::json mergedPatterns = nlohmann::json::array();
    nlohmann::json mergedEntities = nlohmann::json::array();

    for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

        std::ifstream in(entry.path());
        if (!in.good()) {
            errors.push_back({entry.path().string(), "cannot open file"});
            continue;
        }

        nlohmann::json doc;
        in >> doc;

        if (doc.contains("patterns")) {
            if (!doc["patterns"].is_array()) {
                errors.push_back({entry.path().string(), "`patterns` must be array"});
            } else if (validatePatterns(doc, errors, entry.path().string())) {
                for (const auto& p : doc["patterns"]) mergedPatterns.push_back(p);
            }
        }

        if (doc.contains("entities")) {
            if (!doc["entities"].is_array()) {
                errors.push_back({entry.path().string(), "`entities` must be array"});
            } else if (validateEntities(doc, errors, entry.path().string())) {
                for (const auto& e : doc["entities"]) mergedEntities.push_back(e);
            }
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
    pack["packVersion"] = 2;
    pack["sourceDir"] = inputDir.string();
    pack["patterns"] = mergedPatterns;
    pack["entities"] = mergedEntities;

    std::ofstream out(outputPak, std::ios::binary);
    if (!out.good()) {
        std::cerr << "Unable to write output: " << outputPak << "\n";
        return 3;
    }

    const std::string payload = pack.dump();
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));

    std::cout << "Packed patterns=" << mergedPatterns.size() << " entities=" << mergedEntities.size() << " -> " << outputPak << "\n";
    return 0;
}
