// modern/src/portal/portal_log.hpp
// Minimal logging for portal — mirrors mxh::MLOG_* but writes to stderr/stdout.

#pragma once

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <cstring>

namespace mxh::portal {

enum class LogLevel { Debug, Info, Warn, Error };

inline const char* level_str(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::Debug: return "DBG";
        case LogLevel::Info:  return "INF";
        case LogLevel::Warn:  return "WRN";
        case LogLevel::Error: return "ERR";
    }
    return "???";
}

inline void portal_log(LogLevel lvl, const char* file, int line, const char* fmt, ...) {
    char buf[32];
    std::time_t now = std::time(nullptr);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    fprintf(stderr, "[%s] [%s] %s:%d: ", buf, level_str(lvl), file, line);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

}  // namespace mxh::portal

// User-facing macros — no file/line for brevity in the portal context
#define PLATFORM_LOG_DEBUG(fmt, ...) ::mxh::portal::portal_log(::mxh::portal::LogLevel::Debug, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define PLATFORM_LOG_INFO(fmt, ...)  ::mxh::portal::portal_log(::mxh::portal::LogLevel::Info,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define PLATFORM_LOG_WARN(fmt, ...)  ::mxh::portal::portal_log(::mxh::portal::LogLevel::Warn,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define PLATFORM_LOG_ERROR(fmt, ...) ::mxh::portal::portal_log(::mxh::portal::LogLevel::Error, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define MLOG_DEBUG PLATFORM_LOG_DEBUG
#define MLOG_INFO  PLATFORM_LOG_INFO
#define MLOG_WARN  PLATFORM_LOG_WARN
#define MLOG_ERROR PLATFORM_LOG_ERROR
