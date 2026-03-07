#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace engine {

enum class ContentType {
    Patterns,
    Entities,
    Traits,
    Difficulty,
    Palettes,
};

struct ContentChange {
    std::string path;
    ContentType type;
};

class ContentWatcher {
  public:
    void addWatchPath(const std::string& path, ContentType type);
    [[nodiscard]] std::vector<ContentChange> pollChanges();

  private:
    struct WatchEntry {
        std::string path;
        ContentType type;
        std::filesystem::file_time_type lastModified;
    };

    std::vector<WatchEntry> entries_;
};

} // namespace engine
