// mxh/log/mlog.hpp
// Minimal logging for mxh modules. Replaces game's void LOG(...) macro (which
// conflicts with Windows msplog.h LOG function).
#pragma once

#include <cstdarg>
#include <cstdint>
#include <cstdio>

namespace mxh {

enum class LogLevel { Debug, Info, Warn, Error };

// Cross-process run identifier. When set (via set_run_id or
// MXH_RUN_ID env var), every log line includes the run id so
// concurrent client / LoginServer / AgentServer / MapServer logs
// can be correlated. Empty string disables the prefix.
void log_message(LogLevel level, const char* file, int line, const char* fmt, ...);

void set_run_id(const char* run_id) noexcept;
const char* run_id() noexcept;

void set_process_name(const char* name) noexcept;
const char* process_name() noexcept;

} // namespace mxh

#define MLOG_DEBUG(fmt, ...) ::mxh::log_message(::mxh::LogLevel::Debug, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define MLOG_INFO(fmt, ...)  ::mxh::log_message(::mxh::LogLevel::Info,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define MLOG_WARN(fmt, ...)  ::mxh::log_message(::mxh::LogLevel::Warn,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define MLOG_ERROR(fmt, ...) ::mxh::log_message(::mxh::LogLevel::Error, __FILE__, __LINE__, fmt, ##__VA_ARGS__)