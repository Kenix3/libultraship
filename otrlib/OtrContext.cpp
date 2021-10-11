#include "OtrContext.h"
#include <iostream>
#include "OTRResourceMgr.h"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace OtrLib {
    static int32_t nextContextId;

    OtrContext::OtrContext() {
        try
        {
            contextId = nextContextId++;
            Name = "OtrLib" + contextId;
            Logger = spdlog::create_async<spdlog::sinks::basic_file_sink_mt>("async_file_logger", "logs/otr.txt");
            Logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%@] [%l] %v");

            if (contextId == 0) {
                spdlog::set_default_logger(Logger);
            }
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cout << "Log initialization failed: " << ex.what() << std::endl;
        }

        ResourceMgr = std::make_shared<OTRResourceMgr>();
    }

    OtrContext::~OtrContext() {
        try
        {
            spdlog::drop(Name);
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cout << "Log de-initialization failed: " << ex.what() << std::endl;
        }
    }
}