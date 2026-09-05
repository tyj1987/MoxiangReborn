// mxh/log/mlog.cpp
#include "mxh/log/mlog.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace mxh {

namespace {
std::mutex g_logMutex;
LogLevel g_minLevel = LogLevel::Debug;

// Bounded to 24 bytes; longer run ids are truncated. Stored as a
// short fixed-size buffer so we never allocate from a logger path.
constexpr std::size_t kMaxRunId = 24;
std::atomic<char> g_runId[kMaxRunId] = {};
std::atomic<bool> g_runIdSet = false;
std::atomic<char> g_processName[16] = {};
std::atomic<bool> g_processNameSet = false;

void copy_bounded(std::atomic<char>* dst, std::size_t cap, const char* src) {
    // Zero the whole buffer first so the null terminator is present
    // even when the source is empty.
    for (std::size_t i = 0; i < cap; ++i) dst[i].store('\0', std::memory_order_relaxed);
    if (!src) return;
    for (std::size_t i = 0; i + 1 < cap && src[i] != '\0'; ++i) {
        dst[i].store(src[i], std::memory_order_relaxed);
    }
}

void read_bounded(const std::atomic<char>* src, std::size_t cap, char* out, std::size_t out_cap) {
    if (out_cap == 0) return;
    std::size_t n = (cap < out_cap - 1) ? cap : out_cap - 1;
    for (std::size_t i = 0; i < n; ++i) {
        out[i] = src[i].load(std::memory_order_relaxed);
        if (out[i] == '\0') { out[i + 1] = '\0'; return; }
    }
    out[n] = '\0';
}

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

// Read MXH_RUN_ID / MXH_PROCESS env vars at first use.  Setting
// the env var before exec lets a launcher correlate every log line
// without touching CLI parsing.
void ensure_env_overrides() {
    if (!g_runIdSet.load(std::memory_order_acquire)) {
        const char* env = std::getenv("MXH_RUN_ID");
        if (env && *env) {
            copy_bounded(g_runId, kMaxRunId, env);
        }
        g_runIdSet.store(true, std::memory_order_release);
    }
    if (!g_processNameSet.load(std::memory_order_acquire)) {
        const char* env = std::getenv("MXH_PROCESS");
        if (env && *env) {
            copy_bounded(g_processName, sizeof(g_processName) / sizeof(g_processName[0]), env);
        }
        g_processNameSet.store(true, std::memory_order_release);
    }
}
} // namespace

void set_run_id(const char* run_id) noexcept {
    g_runIdSet.store(true, std::memory_order_release);
    copy_bounded(g_runId, kMaxRunId, run_id);
}

const char* run_id() noexcept {
    ensure_env_overrides();
    // We need a stable buffer for the returned pointer.  Use a
    // function-local static that we refill on every call (callers
    // treat the result as immediately-consumed).
    static thread_local char buf[kMaxRunId];
    read_bounded(g_runId, kMaxRunId, buf, sizeof(buf));
    return buf;
}

void set_process_name(const char* name) noexcept {
    g_processNameSet.store(true, std::memory_order_release);
    copy_bounded(g_processName, sizeof(g_processName) / sizeof(g_processName[0]), name);
}

const char* process_name() noexcept {
    ensure_env_overrides();
    static thread_local char buf[16];
    read_bounded(g_processName, sizeof(g_processName) / sizeof(g_processName[0]), buf, sizeof(buf));
    return buf;
}

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

    ensure_env_overrides();
    char run_id_buf[kMaxRunId];
    read_bounded(g_runId, kMaxRunId, run_id_buf, sizeof(run_id_buf));
    char proc_buf[16];
    read_bounded(g_processName, sizeof(g_processName) / sizeof(g_processName[0]), proc_buf, sizeof(proc_buf));
    const bool has_run_id = run_id_buf[0] != '\0';
    const bool has_proc   = proc_buf[0]   != '\0';

    std::lock_guard<std::mutex> lock(g_logMutex);
    if (has_run_id && has_proc) {
        std::fprintf(stderr, "[%02d:%02d:%02d.%03lld] %s %s[%s] %s:%d  %s\n",
                     tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
                     static_cast<long long>(ms),
                     levelTag(level), proc_buf, run_id_buf,
                     basename(file), line, buf);
    } else if (has_run_id) {
        std::fprintf(stderr, "[%02d:%02d:%02d.%03lld] %s [%s] %s:%d  %s\n",
                     tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
                     static_cast<long long>(ms),
                     levelTag(level), run_id_buf,
                     basename(file), line, buf);
    } else {
        std::fprintf(stderr, "[%02d:%02d:%02d.%03lld] %s %s:%d  %s\n",
                     tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
                     static_cast<long long>(ms),
                     levelTag(level), basename(file), line, buf);
    }
    std::fflush(stderr);
}

} // namespace mxh

#ifdef _WIN32
mxh::HeapSnapshot mxh::take_heap_snapshot() noexcept {
    mxh::HeapSnapshot snap{};
    PROCESS_MEMORY_COUNTERS pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        snap.working_set_bytes = pmc.WorkingSetSize;
        snap.peak_working_set_bytes = pmc.PeakWorkingSetSize;
        // PROCESS_MEMORY_COUNTERS does not expose PrivateUsage /
        // VirtualSize directly on this Windows SKU.  PagefileUsage
        // is the closest portable proxy and is what WinDbg's
        // !address -summary reports for the process.
        snap.private_bytes = pmc.PagefileUsage;
        snap.virtual_bytes = pmc.PeakPagefileUsage;
    }
    return snap;
}
#endif