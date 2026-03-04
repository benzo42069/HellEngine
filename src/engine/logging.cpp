#include <engine/logging.h>

#include <iostream>

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

    std::ostream& os = level == LogLevel::Error ? std::cerr : std::cout;
    os << "[" << toString(level) << "] " << message << '\n';
}

void logDebug(const std::string& message) { Logger::instance().log(LogLevel::Debug, message); }
void logInfo(const std::string& message) { Logger::instance().log(LogLevel::Info, message); }
void logWarn(const std::string& message) { Logger::instance().log(LogLevel::Warn, message); }
void logError(const std::string& message) { Logger::instance().log(LogLevel::Error, message); }

} // namespace engine
