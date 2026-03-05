#include <engine/logging.h>

#include <iostream>
#include <vector>

namespace engine {

namespace {
const char* toString(const LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}
} // namespace

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setLevel(const LogLevel level) {
    std::scoped_lock lock(mutex_);
    level_ = level;
}

void Logger::log(const LogLevel level, const std::string& message) {
    std::scoped_lock lock(mutex_);
    if (static_cast<int>(level) < static_cast<int>(level_)) {
        return;
    }

    const std::string line = std::string("[") + toString(level) + "] " + message;
    recentLines_.push_back(line);
    while (recentLines_.size() > 512) {
        recentLines_.pop_front();
    }

    std::ostream& os = level == LogLevel::Error ? std::cerr : std::cout;
    os << line << '\n';
}

std::vector<std::string> Logger::recentLines(const std::size_t maxLines) const {
    std::scoped_lock lock(mutex_);
    const std::size_t count = std::min(maxLines, recentLines_.size());
    const std::size_t start = recentLines_.size() - count;
    std::vector<std::string> out;
    out.reserve(count);
    for (std::size_t i = start; i < recentLines_.size(); ++i) {
        out.push_back(recentLines_[i]);
    }
    return out;
}

void logDebug(const std::string& message) { Logger::instance().log(LogLevel::Debug, message); }
void logInfo(const std::string& message) { Logger::instance().log(LogLevel::Info, message); }
void logWarn(const std::string& message) { Logger::instance().log(LogLevel::Warn, message); }
void logError(const std::string& message) { Logger::instance().log(LogLevel::Error, message); }

} // namespace engine
