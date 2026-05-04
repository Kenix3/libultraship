#pragma once

#include <memory>
#include <string>
#include <spdlog/logger.h>
#include "ship/Component.h"

namespace Ship {

/**
 * @brief Component wrapper around an spdlog::logger instance.
 *
 * Logger exposes a shared spdlog logger within the component system,
 * allowing it to participate in the Component hierarchy and be looked up by name.
 *
 * **Required Context children:** None — Logger has no dependencies on
 * other components.
 *
 * Obtain the instance from `Context::GetChildren().GetFirst<Logger>()`.
 */
class Logger : public Component {
  public:
    /**
     * @brief Constructs a Logger wrapping the given logger.
     * @param logger The spdlog logger instance to wrap.
     */
    explicit Logger(std::shared_ptr<spdlog::logger> logger);
    ~Logger() override = default;

    /**
     * @brief Returns the underlying spdlog logger.
     * @return A shared pointer to the spdlog::logger.
     */
    std::shared_ptr<spdlog::logger> Get() const;

  private:
    std::shared_ptr<spdlog::logger> mLogger;
};

} // namespace Ship
