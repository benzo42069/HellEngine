#include <engine/content_watcher.h>

namespace engine {

void ContentWatcher::addWatchPath(const std::string& path, const ContentType type) {
    if (path.empty()) return;
    WatchEntry entry;
    entry.path = path;
    entry.type = type;
    if (std::filesystem::exists(entry.path)) {
        entry.lastModified = std::filesystem::last_write_time(entry.path);
        entry.seen = true;
    }
    entries_.push_back(std::move(entry));
}

std::vector<ContentChange> ContentWatcher::pollChanges() {
    std::vector<ContentChange> changes;
    for (WatchEntry& entry : entries_) {
        if (!std::filesystem::exists(entry.path)) continue;
        const auto current = std::filesystem::last_write_time(entry.path);
        if (!entry.seen) {
            entry.lastModified = current;
            entry.seen = true;
            continue;
        }
        if (current != entry.lastModified) {
            entry.lastModified = current;
            changes.push_back(ContentChange {.path = entry.path, .type = entry.type});
        }
    }
    return changes;
}

} // namespace engine
