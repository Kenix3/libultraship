#pragma once

#include <stdint.h>
#include "ship/Api.h"

// NOLINTBEGIN(readability-identifier-naming)

/**
 * @brief Writes a plain message to the Ship log at the given level.
 *
 * This is the low-level logging function. Prefer the LUSLOG_* convenience macros
 * which automatically capture @c __FILE__ and @c __LINE__.
 *
 * @param file     Source file name (typically @c __FILE__).
 * @param line     Source line number (typically @c __LINE__).
 * @param logLevel Severity level (one of the LUSLOG_LEVEL_* constants).
 * @param msg      Null-terminated message string.
 */
API_EXPORT void luslog(const char* file, int32_t line, int32_t logLevel, const char* msg);

/**
 * @brief Formats and writes a message to the Ship log at the given level.
 *
 * Prefer the LUSLOG_* convenience macros which automatically capture @c __FILE__
 * and @c __LINE__.
 *
 * @param file     Source file name (typically @c __FILE__).
 * @param line     Source line number (typically @c __LINE__).
 * @param logLevel Severity level (one of the LUSLOG_LEVEL_* constants).
 * @param fmt      printf-style format string.
 * @param ...      Format arguments.
 */
API_EXPORT void lusprintf(const char* file, int32_t line, int32_t logLevel, const char* fmt, ...);

// NOLINTEND(readability-identifier-naming)

/** @brief Log level constant: emit all messages, including the most verbose trace output. */
#define LUSLOG_LEVEL_TRACE 0
/** @brief Log level constant: verbose debug output useful during development. */
#define LUSLOG_LEVEL_DEBUG 1
/** @brief Log level constant: informational messages about normal operation. */
#define LUSLOG_LEVEL_INFO 2
/** @brief Log level constant: warnings about unexpected but recoverable conditions. */
#define LUSLOG_LEVEL_WARN 3
/** @brief Log level constant: errors that indicate a failure in the current operation. */
#define LUSLOG_LEVEL_ERROR 4
/** @brief Log level constant: critical failures that may cause the application to terminate. */
#define LUSLOG_LEVEL_CRITICAL 5
/** @brief Log level constant: disable all logging output. */
#define LUSLOG_LEVEL_OFF 6

/** @brief Writes a formatted log message at an explicit @p level. */
#define LUSLOG(level, msg, ...) lusprintf(__FILE__, __LINE__, level, msg, ##__VA_ARGS__)
/** @brief Writes a formatted log message at TRACE level. */
#define LUSLOG_TRACE(msg, ...) LUSLOG(LUSLOG_LEVEL_TRACE, msg, ##__VA_ARGS__)
/** @brief Writes a formatted log message at DEBUG level. */
#define LUSLOG_DEBUG(msg, ...) LUSLOG(LUSLOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)
/** @brief Writes a formatted log message at INFO level. */
#define LUSLOG_INFO(msg, ...) LUSLOG(LUSLOG_LEVEL_INFO, msg, ##__VA_ARGS__)
/** @brief Writes a formatted log message at WARN level. */
#define LUSLOG_WARN(msg, ...) LUSLOG(LUSLOG_LEVEL_WARN, msg, ##__VA_ARGS__)
/** @brief Writes a formatted log message at ERROR level. */
#define LUSLOG_ERROR(msg, ...) LUSLOG(LUSLOG_LEVEL_ERROR, msg, ##__VA_ARGS__)
/** @brief Writes a formatted log message at CRITICAL level. */
#define LUSLOG_CRITICAL(msg, ...) LUSLOG(LUSLOG_LEVEL_CRITICAL, msg, ##__VA_ARGS__)