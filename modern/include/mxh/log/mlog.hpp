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
// can be correlated.  Empty string disables the prefix.
void log_message(LogLevel level, const char* file, int line, const char* fmt, ...);

void set_run_id(const char* run_id) noexcept;
const char* run_id() noexcept;

void set_process_name(const char* name) noexcept;
const char* process_name() noexcept;

#ifdef _WIN32
// Phase 0 §6.5 heap probe: take a single PROCESS_MEMORY_COUNTERS
// snapshot.  No allocations, no locks beyond the kernel's
// short-lived GetProcessMemoryInfo critical section.  Output is
// passed back to the caller; the MLOG_HEAP macro logs it through
// the standard pipeline so it lands in client.stderr.log with
// the run-id prefix (plan §6.2).
struct HeapSnapshot {
    std::uint64_t working_set_bytes;
    std::uint64_t peak_working_set_bytes;
    std::uint64_t private_bytes;
    std::uint64_t virtual_bytes;
};
HeapSnapshot take_heap_snapshot() noexcept;
#endif

} // namespace mxh

#define MLOG_DEBUG(fmt, ...) ::mxh::log_message(::mxh::LogLevel::Debug, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define MLOG_INFO(fmt, ...)  ::mxh::log_message(::mxh::LogLevel::Info,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define MLOG_WARN(fmt, ...)  ::mxh::log_message(::mxh::LogLevel::Warn,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define MLOG_ERROR(fmt, ...) ::mxh::log_message(::mxh::LogLevel::Error, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// Heap probe macro: emits one MLOG_INFO line with the four
// memory counters as decimal bytes.  Designed to be called at
// every state transition (Enter / Exit) so the post-mortem MLOG
// timeline carries an annotated memory profile.  Use
// MLOG_HEAP(label) where label is a short C string token.
#ifdef _WIN32
#define MLOG_HEAP(label) do { \
    const auto _hs = ::mxh::take_heap_snapshot(); \
    MLOG_INFO("heap label=%s ws=%llu peak_ws=%llu private=%llu virtual=%llu", \
              (label), \
              static_cast<unsigned long long>(_hs.working_set_bytes), \
              static_cast<unsigned long long>(_hs.peak_working_set_bytes), \
              static_cast<unsigned long long>(_hs.private_bytes), \
              static_cast<unsigned long long>(_hs.virtual_bytes)); \
} while (0)
#else
#define MLOG_HEAP(label) do { } while (0)
#endif
