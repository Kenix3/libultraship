#pragma once

#include <memory>
#include <string>
#include "ship/Component.h"

namespace spdlog {
class logger;
}

namespace Ship {

class LoggerComponent : public Component {
  public:
    explicit LoggerComponent(std::shared_ptr<spdlog::logger> logger);
    ~LoggerComponent() override = default;

    std::shared_ptr<spdlog::logger> GetLogger() const;

  private:
    std::shared_ptr<spdlog::logger> mLogger;
};

} // namespace Ship
