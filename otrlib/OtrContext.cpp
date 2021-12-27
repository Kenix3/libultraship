#include "OTRContext.h"
#include <iostream>
#include "OTRResourceMgr.h"
#include "OTRInput.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace OtrLib {
    static int32_t dwNextContextId;
    static int32_t dwRunningCount;

    OTRContext::OTRContext(std::string mainPath, std::string patchesDirectory) {
        dwRunningCount++;
        contextId = dwNextContextId++;

        try {
            // Setup Logging
            Name = "OtrLib" + contextId;
            Logger = spdlog::create_async<spdlog::sinks::basic_file_sink_mt>("async_file_logger", "logs/" + Name + ".log");
            Logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%@] [%l] %v");
            if (contextId == 0) {
                spdlog::set_default_logger(Logger);
            }
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cout << "Log initialization failed: " << ex.what() << std::endl;
        }


        if (dwRunningCount == 1) {
            OTRInput::StartInput();
        }

        ResourceMgr = std::make_shared<OTRResourceMgr>(mainPath, patchesDirectory);
    }

    OTRContext::~OTRContext() {
        if (dwRunningCount == 1) {
            OTRInput::StopInput();
        }

        // Kill Logging
        try {
            spdlog::drop(Name);
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cout << "Log de-initialization failed: " << ex.what() << std::endl;
        }

        dwRunningCount--;
    }
}