#include <engine/diagnostics.h>

#include <engine/logging.h>

#include <nlohmann/json.hpp>

namespace engine {

const char* toString(const ErrorCode code) {
    switch (code) {
        case ErrorCode::ContentLoadFailed: return "content_load_failed";
        case ErrorCode::ContentFallbackActivated: return "content_fallback_activated";
        case ErrorCode::HotReloadFailed: return "hot_reload_failed";
        case ErrorCode::ReplayValidationFailed: return "replay_validation_failed";
        case ErrorCode::RuntimeInitFailed: return "runtime_init_failed";
        case ErrorCode::CrashUnhandledException: return "crash_unhandled_exception";
        case ErrorCode::CrashSignal: return "crash_signal";
        case ErrorCode::None:
        default: return "none";
    }
}

ErrorReport makeError(const ErrorCode code, std::string message) {
    ErrorReport r;
    r.code = code;
    r.message = std::move(message);
    return r;
}

void addContext(ErrorReport& report, std::string key, std::string value) {
    report.context.push_back(ErrorContextEntry {.key = std::move(key), .value = std::move(value)});
}

void pushStack(ErrorReport& report, std::string frame) {
    report.stack.push_back(std::move(frame));
}

std::string toJson(const ErrorReport& report) {
    nlohmann::json j;
    j["code"] = toString(report.code);
    j["message"] = report.message;
    j["context"] = nlohmann::json::array();
    for (const ErrorContextEntry& c : report.context) {
        j["context"].push_back({{"key", c.key}, {"value", c.value}});
    }
    j["stack"] = report.stack;
    return j.dump();
}

void logErrorReport(const ErrorReport& report) {
    logError("error_report=" + toJson(report));
}

} // namespace engine
