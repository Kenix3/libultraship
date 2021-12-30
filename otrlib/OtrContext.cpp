#include "OTRContext.h"
#include <iostream>
#include "OTRResourceMgr.h"
#include "OTRWindow.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace OtrLib {
    std::shared_ptr<OTRContext> OTRContext::Context = nullptr;

    std::shared_ptr<OTRContext> OTRContext::GetInstance() {
        return Context;
    }

    std::shared_ptr<OTRContext> OTRContext::CreateInstance(std::string Name, std::string MainPath, std::string PatchesPath) {
        if (Context == nullptr) {
            if (!MainPath.empty()) {
                Context = std::make_shared<OTRContext>(Name, MainPath, PatchesPath);
            } else {
                spdlog::error("No Main Archive passed to create instance");
            }
        } else {
            spdlog::debug("Trying to create a context when it already exists.");
        }

        return Context;
    }

    OTRContext::OTRContext(std::string Name, std::string MainPath, std::string PatchesPath) : Name(Name) {
        try {
            // Setup Logging
            Logger = spdlog::create_async<spdlog::sinks::basic_file_sink_mt>("async_file_logger", "logs/" + Name + ".log");
            Logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%@] [%l] %v");
            spdlog::set_level(spdlog::level::info);
            spdlog::set_default_logger(Logger);
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cout << "Log initialization failed: " << ex.what() << std::endl;
        }

        ResourceMgr = std::make_shared<OTRResourceMgr>(std::make_shared<OTRContext>(*this), MainPath, PatchesPath);
        Window = std::make_shared<OTRWindow>(std::make_shared<OTRContext>(*this));
        Config = std::make_shared<OTRConfigFile>(std::make_shared<OTRContext>(*this), "otr.ini");
    }

    OTRContext::~OTRContext() {
        // Kill Logging
        try {
            spdlog::drop(Name);
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cout << "Log de-initialization failed: " << ex.what() << std::endl;
        }
    }
}