#pragma once

#include <mutex>
#include <string>

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

  private:
    Logger() = default;

    std::mutex mutex_;
    LogLevel level_ {LogLevel::Info};
};

void logDebug(const std::string& message);
void logInfo(const std::string& message);
void logWarn(const std::string& message);
void logError(const std::string& message);

} // namespace engine
