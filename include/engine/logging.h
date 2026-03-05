#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace engine {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

class Logger {
  public:
    static Logger& instance();

    void setLevel(LogLevel level);
    void log(LogLevel level, const std::string& message);
    [[nodiscard]] std::vector<std::string> recentLines(std::size_t maxLines) const;

  private:
    Logger() = default;

    mutable std::mutex mutex_;
    LogLevel level_ {LogLevel::Info};
    std::deque<std::string> recentLines_ {};
};

void logDebug(const std::string& message);
void logInfo(const std::string& message);
void logWarn(const std::string& message);
void logError(const std::string& message);

} // namespace engine
