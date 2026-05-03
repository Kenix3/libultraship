#include "ship/log/Logger.h"
#include <spdlog/spdlog.h>

namespace Ship {

Logger::Logger(std::shared_ptr<spdlog::logger> logger) : Component("Logger"), mLogger(std::move(logger)) {
    MarkInitialized();
}

std::shared_ptr<spdlog::logger> Logger::Get() const {
    return mLogger;
}

} // namespace Ship
