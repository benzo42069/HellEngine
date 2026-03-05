#pragma once

#include <string>
#include <vector>

namespace engine {

enum class ErrorCode {
    None = 0,
    ContentLoadFailed,
    ContentFallbackActivated,
    HotReloadFailed,
    ReplayValidationFailed,
    RuntimeInitFailed,
    CrashUnhandledException,
    CrashSignal,
};

struct ErrorContextEntry {
    std::string key;
    std::string value;
};

struct ErrorReport {
    ErrorCode code {ErrorCode::None};
    std::string message;
    std::vector<ErrorContextEntry> context;
    std::vector<std::string> stack;
};

const char* toString(ErrorCode code);
ErrorReport makeError(ErrorCode code, std::string message);
void addContext(ErrorReport& report, std::string key, std::string value);
void pushStack(ErrorReport& report, std::string frame);
std::string toJson(const ErrorReport& report);
void logErrorReport(const ErrorReport& report);

} // namespace engine
