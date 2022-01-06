#include "OTRContext.h"
#include <iostream>
#include <vector>
#include "OTRResourceMgr.h"
#include "OTRWindow.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace OtrLib {
    std::weak_ptr<OTRContext> OTRContext::Context;

    std::shared_ptr<OTRContext> OTRContext::GetInstance() {
        return Context.lock();
    }

    std::shared_ptr<OTRContext> OTRContext::CreateInstance(std::string Name, std::string MainPath, std::string PatchesPath) {
        if (Context.expired()) {
            if (!MainPath.empty()) {
                auto Shared = std::make_shared<OTRContext>(Name, MainPath, PatchesPath);
                Context = Shared;
                Shared->InitWindow();

                return Shared;
            } else {
                SPDLOG_ERROR("No Main Archive passed to create instance");
            }
        } else {
            SPDLOG_DEBUG("Trying to create a context when it already exists.");
        }

        return GetInstance();
    }

    OTRContext::OTRContext(std::string Name, std::string MainPath, std::string PatchesPath) : Name(Name), MainPath(MainPath), PatchesPath(PatchesPath) {
        InitLogging();
    }

    OTRContext::~OTRContext() {
        SPDLOG_INFO("destruct OTRContext");

        Window = nullptr;
        ResourceMgr = nullptr;
        Config = nullptr;
    }

    void OTRContext::InitWindow() {
        ResourceMgr = std::make_shared<OTRResourceMgr>(OTRContext::GetInstance(), MainPath, PatchesPath);
        Window = std::make_shared<OTRWindow>(OTRContext::GetInstance());
        Config = std::make_shared<OTRConfigFile>(OTRContext::GetInstance(), "otr.ini");
    }

    void OTRContext::InitLogging() {
        try {
            // Setup Logging
            spdlog::init_thread_pool(8192, 1);
            auto ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            auto FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/" + GetName() + ".log");
            std::vector<spdlog::sink_ptr> Sinks{ ConsoleSink, FileSink };
            Logger = std::make_shared<spdlog::async_logger>(GetName(), Sinks.begin(), Sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
            GetLogger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%@] [%l] %v");
            spdlog::set_level(spdlog::level::trace);
            spdlog::register_logger(GetLogger());
            spdlog::set_default_logger(GetLogger());
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cout << "Log initialization failed: " << ex.what() << std::endl;
        }
    }
}