#pragma once

#include <memory>
#include <string>
#include "ship/Component.h"

namespace spdlog {
class logger;
}

namespace Ship {

/**
 * @brief Component wrapper around an spdlog::logger instance.
 *
 * LoggerComponent exposes a shared spdlog logger within the component system,
 * allowing it to participate in the Component hierarchy and be looked up by name.
 */
class LoggerComponent : public Component {
  public:
    /**
     * @brief Constructs a LoggerComponent wrapping the given logger.
     * @param logger The spdlog logger instance to wrap.
     */
    explicit LoggerComponent(std::shared_ptr<spdlog::logger> logger);
    ~LoggerComponent() override = default;

    /**
     * @brief Returns the underlying spdlog logger.
     * @return A shared pointer to the spdlog::logger.
     */
    std::shared_ptr<spdlog::logger> GetLogger() const;

  private:
    std::shared_ptr<spdlog::logger> mLogger;
};

} // namespace Ship
