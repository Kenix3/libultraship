#pragma once
#include <stdint.h>
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif
API_EXPORT void luslog(const char* file, int32_t line, int32_t logLevel, const char* msg);
API_EXPORT void lusprintf(const char* file, int32_t line, int32_t logLevel, const char* fmt, ...);
#ifdef __cplusplus
}
#endif

#define LUSLOG_LEVEL_TRACE 0
#define LUSLOG_LEVEL_DEBUG 1
#define LUSLOG_LEVEL_INFO 2
#define LUSLOG_LEVEL_WARN 3
#define LUSLOG_LEVEL_ERROR 4
#define LUSLOG_LEVEL_CRITICAL 5
#define LUSLOG_LEVEL_OFF 6

#define LUSLOG(level, msg, ...) lusprintf(__FILE__, __LINE__, level, msg, ##__VA_ARGS__)
#define LUSLOG_TRACE(msg, ...) LUSLOG(LUSLOG_LEVEL_TRACE,    msg, ##__VA_ARGS__)
#define LUSLOG_DEBUG(msg, ...) LUSLOG(LUSLOG_LEVEL_DEBUG,    msg, ##__VA_ARGS__)
#define LUSLOG_INFO(msg, ...) LUSLOG(LUSLOG_LEVEL_INFO,     msg, ##__VA_ARGS__)
#define LUSLOG_WARN(msg, ...) LUSLOG(LUSLOG_LEVEL_WARN,     msg, ##__VA_ARGS__)
#define LUSLOG_ERROR(msg, ...) LUSLOG(LUSLOG_LEVEL_ERROR,    msg, ##__VA_ARGS__)
#define LUSLOG_CRITICAL(msg, ...) LUSLOG(LUSLOG_LEVEL_CRITICAL, msg, ##__VA_ARGS__)
