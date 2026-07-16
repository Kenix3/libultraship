#pragma once

#include <memory>
#include <string>
#include <spdlog/logger.h>
#include "ship/Component.h"

namespace Ship {

/**
 * @brief Component wrapper around an spdlog::logger instance.
 *
 * Logger owns the spdlog thread-pool, sinks, and default logger. It
 * initialises them in OnInit() so that the component hierarchy is
 * established before logging begins, and shuts them down in ~Logger()
 * so that no log calls race against destruction.
 *
 * **Required Context children:** None — Logger has no dependencies on
 * other components.
 *
 * Obtain the instance from `Context::GetChildren().GetFirst<Logger>()`.
 */
class Logger : public Component {
  public:
    /**
     * @brief Constructs a Logger with the given application name and log file path.
     *
     * The spdlog thread-pool and sinks are created in OnInit(); the
     * constructor is cheap and safe to call before the context is fully wired.
     *
     * @param appName     Human-readable application name used as the logger name
     *                    and included in the rotating log file name.
     * @param logFilePath Absolute path to the rotating log file.
     */
    Logger(const std::string& appName, const std::string& logFilePath);
    ~Logger() override;

    /**
     * @brief Returns the underlying spdlog logger, or nullptr before Init().
     * @return A shared pointer to the spdlog::logger.
     */
    std::shared_ptr<spdlog::logger> Get() const;

  protected:
    void OnInit(const nlohmann::json& initArgs) override;

  private:
    std::string mAppName;
    std::string mLogFilePath;
    std::shared_ptr<spdlog::logger> mLogger;
};

} // namespace Ship
