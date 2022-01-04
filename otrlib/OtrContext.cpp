#include "OTRContext.h"
#include <iostream>
#include "OTRResourceMgr.h"
#include "OTRWindow.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

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
            Logger = spdlog::create_async<spdlog::sinks::basic_file_sink_mt>("async_file_logger", "logs/" + Name + ".log");
            Logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%@] [%l] %v");
            spdlog::set_level(spdlog::level::trace);
            spdlog::set_default_logger(Logger);
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cout << "Log initialization failed: " << ex.what() << std::endl;
        }
    }
}