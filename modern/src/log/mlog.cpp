// mxh/log/mlog.cpp
#include "mxh/log/mlog.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace mxh {

namespace {
std::mutex g_logMutex;
LogLevel g_minLevel = LogLevel::Debug;

const char* levelTag(LogLevel l) {
    switch (l) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO ";
    case LogLevel::Warn:  return "WARN ";
    case LogLevel::Error: return "ERROR";
    }
    return "?";
}

const char* basename(const char* path) {
    if (!path) return "?";
    const char* p = path;
    for (const char* q = path; *q; ++q) {
        if (*q == '/' || *q == '\\') p = q + 1;
    }
    return p;
}
} // namespace

void log_message(LogLevel level, const char* file, int line, const char* fmt, ...) {
    if (static_cast<int>(level) < static_cast<int>(g_minLevel)) return;

    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif

    std::lock_guard<std::mutex> lock(g_logMutex);
    std::fprintf(stderr, "[%02d:%02d:%02d.%03lld] %s %s:%d  %s\n",
                 tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
                 static_cast<long long>(ms),
                 levelTag(level), basename(file), line, buf);
    std::fflush(stderr);
}

} // namespace mxh