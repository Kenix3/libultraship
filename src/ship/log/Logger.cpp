#include "ship/log/Logger.h"

#include <iostream>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#include <libloaderapi.h>
#include <tchar.h>
#include <stringapiset.h>
#endif

namespace Ship {

Logger::Logger(const std::string& appName, const std::string& logFilePath)
    : Component("Logger"), mAppName(appName), mLogFilePath(logFilePath) {
}

Logger::~Logger() {
    SPDLOG_TRACE("destruct logger");
    if (spdlog::default_logger()) {
        spdlog::shutdown();
    }
}

std::shared_ptr<spdlog::logger> Logger::Get() const {
    return mLogger;
}

void Logger::OnInit(const nlohmann::json& /*initArgs*/) {
    try {
        spdlog::init_thread_pool(8192, 1);
        std::vector<spdlog::sink_ptr> sinks;

#if (!defined(_WIN32)) || defined(_DEBUG)
#if defined(_DEBUG) && defined(_WIN32)
        FreeConsole();
        if (AllocConsole() == 0) {
            throw std::system_error(GetLastError(), std::generic_category(), "Failed to create debug console");
        }

        SetConsoleOutputCP(CP_UTF8);

        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONIN$", "r", stdin);
        std::cout.clear();
        std::clog.clear();
        std::cerr.clear();
        std::cin.clear();

        HANDLE hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        HANDLE hConIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
        SetStdHandle(STD_ERROR_HANDLE, hConOut);
        SetStdHandle(STD_INPUT_HANDLE, hConIn);
        std::wcout.clear();
        std::wclog.clear();
        std::wcerr.clear();
        std::wcin.clear();
#endif
        auto systemConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sinks.push_back(systemConsoleSink);
#endif

        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(mLogFilePath, 1024 * 1024 * 10, 10);
        sinks.push_back(fileSink);

#ifdef _DEBUG
        mLogger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
        mLogger->set_level(spdlog::level::debug);
        mLogger->flush_on(spdlog::level::trace);
#else
        mLogger = std::make_shared<spdlog::async_logger>(mAppName, sinks.begin(), sinks.end(), spdlog::thread_pool(),
                                                         spdlog::async_overflow_policy::block);
        mLogger->set_level(spdlog::level::warn);
        mLogger->flush_on(spdlog::level::info);
#endif

        mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%@] [%l] %v");

        spdlog::register_logger(mLogger);
        spdlog::set_default_logger(mLogger);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
        throw;
    }
}

} // namespace Ship
