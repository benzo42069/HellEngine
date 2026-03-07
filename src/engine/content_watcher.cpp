#include <engine/content_watcher.h>

#include <system_error>

namespace engine {

namespace {
std::filesystem::file_time_type readLastWriteTimeOrMin(const std::string& path) {
    std::error_code ec;
    const auto ts = std::filesystem::last_write_time(path, ec);
    if (ec) return std::filesystem::file_time_type::min();
    return ts;
}
} // namespace

void ContentWatcher::addWatchPath(const std::string& path, const ContentType type) {
    WatchEntry entry;
    entry.path = path;
    entry.type = type;
    entry.lastModified = readLastWriteTimeOrMin(path);
    entries_.push_back(std::move(entry));
}

std::vector<ContentChange> ContentWatcher::pollChanges() {
    std::vector<ContentChange> changes;
    for (WatchEntry& entry : entries_) {
        const auto currentModified = readLastWriteTimeOrMin(entry.path);
        if (currentModified == entry.lastModified) continue;
        entry.lastModified = currentModified;
        changes.push_back(ContentChange {.path = entry.path, .type = entry.type});
    }
    return changes;
}

} // namespace engine
