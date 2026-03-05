#include <engine/crash_handler.h>

#include <engine/diagnostics.h>
#include <engine/logging.h>

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

namespace engine {
namespace {
std::string gBuildStamp;

std::string nowStamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);
    std::tm tm {};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

void writeCrashReport(const ErrorReport& err) {
    std::filesystem::create_directories("crashes");
    const std::string stamp = nowStamp();
    const std::string reportPath = "crashes/crash_report_" + stamp + ".txt";

    std::ofstream out(reportPath);
    if (!out.good()) return;

    out << "Engine crash report\n";
    out << "build_stamp=" << gBuildStamp << "\n";
    out << "error=" << toJson(err) << "\n\n";
    out << "Recent log tail:\n";
    for (const std::string& line : Logger::instance().recentLines(128)) {
        out << line << "\n";
    }

#ifdef _WIN32
    const std::string dumpPath = "crashes/crash_dump_" + stamp + ".dmp";
    HANDLE hFile = CreateFileA(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei {};
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = nullptr;
        mei.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, nullptr, nullptr, nullptr);
        CloseHandle(hFile);
        out << "minidump=" << dumpPath << "\n";
    }
#else
    out << "minidump=unsupported_on_this_platform\n";
#endif
}

[[noreturn]] void handleTerminate() {
    ErrorReport err = makeError(ErrorCode::CrashUnhandledException, "Unhandled exception / terminate");
    pushStack(err, "std::terminate");
    writeCrashReport(err);
    std::_Exit(1);
}

void handleSignal(const int sig) {
    ErrorReport err = makeError(ErrorCode::CrashSignal, "Fatal signal");
    addContext(err, "signal", std::to_string(sig));
    pushStack(err, "signal_handler");
    writeCrashReport(err);
    std::_Exit(128 + sig);
}
} // namespace

void installCrashHandlers(const std::string& buildStamp) {
#ifndef NDEBUG
    (void)buildStamp;
#else
    gBuildStamp = buildStamp;
    std::set_terminate(handleTerminate);
    std::signal(SIGSEGV, handleSignal);
    std::signal(SIGABRT, handleSignal);
    std::signal(SIGILL, handleSignal);
    std::signal(SIGFPE, handleSignal);
#endif
}

} // namespace engine
