#include "ship/log/LoggerComponent.h"
#include <spdlog/spdlog.h>

namespace Ship {

LoggerComponent::LoggerComponent(std::shared_ptr<spdlog::logger> logger)
    : Component("LoggerComponent"), mLogger(std::move(logger)) {
}

std::shared_ptr<spdlog::logger> LoggerComponent::Get() const {
    return mLogger;
}

} // namespace Ship
