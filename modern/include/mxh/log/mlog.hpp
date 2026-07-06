// mxh/log/mlog.hpp
// Minimal logging for mxh modules. Replaces game's void LOG(...) macro (which
// conflicts with Windows msplog.h LOG function).
#pragma once

#include <cstdarg>
#include <cstdio>

namespace mxh {

enum class LogLevel { Debug, Info, Warn, Error };

void log_message(LogLevel level, const char* file, int line, const char* fmt, ...);

} // namespace mxh

#define MLOG_DEBUG(fmt, ...) ::mxh::log_message(::mxh::LogLevel::Debug, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define MLOG_INFO(fmt, ...)  ::mxh::log_message(::mxh::LogLevel::Info,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define MLOG_WARN(fmt, ...)  ::mxh::log_message(::mxh::LogLevel::Warn,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define MLOG_ERROR(fmt, ...) ::mxh::log_message(::mxh::LogLevel::Error, __FILE__, __LINE__, fmt, ##__VA_ARGS__)