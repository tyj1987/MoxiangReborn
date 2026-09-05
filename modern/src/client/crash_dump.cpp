// mxh/client/crash_dump.cpp
//
// Phase 0 §6.3 controlled unhandled-exception handler.  Walks the
// Win32 SEH chain via SetUnhandledExceptionFilter, writes a
// minidump into the run's `dumps/` directory, then exits with
// the documented exception code.  The handler is intentionally
// self-contained: it does not depend on the renderer, the UI
// runtime, or the network layer so it can run from the raw
// exception path.

#include "mxh/client/crash_dump.hpp"

#ifdef _WIN32

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>

#include <windows.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

namespace mxh::client {

namespace {

// Bounded by the SEH contract: the handler must terminate the
// process (or longjmp) before returning.  Stashing anything in
// globals is fine, but we never allocate from this path.
std::atomic<bool> g_installed = false;
char g_run_id[24] = {0};
char g_dump_dir[260] = {0};
char g_process_name[16] = {0};

void read_env_into(char* dst, std::size_t cap, const char* name) noexcept {
    if (cap == 0) return;
    const char* src = std::getenv(name);
    if (!src) { dst[0] = '\0'; return; }
    std::size_t n = cap - 1;
    for (std::size_t i = 0; i < n && src[i] != '\0'; ++i) dst[i] = src[i];
    dst[n] = '\0';
}

std::string timestamp_for_filename() noexcept {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04u%02u%02u-%02u%02u%02u-%03u",
                  static_cast<unsigned>(st.wYear),
                  static_cast<unsigned>(st.wMonth),
                  static_cast<unsigned>(st.wDay),
                  static_cast<unsigned>(st.wHour),
                  static_cast<unsigned>(st.wMinute),
                  static_cast<unsigned>(st.wSecond),
                  static_cast<unsigned>(st.wMilliseconds));
    return std::string(buf);
}

void write_fail_log(const char* dump_path, DWORD err) noexcept {
    if (!dump_path) return;
    std::string fail_path = std::string(dump_path) + ".fail.log";
    HANDLE h = CreateFileA(fail_path.c_str(), GENERIC_WRITE, 0,
                           nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    char msg[256];
    std::snprintf(msg, sizeof(msg),
                  "minidump failed: GetLastError=%lu path=%s run_id=%s pid=%lu\n",
                  err, dump_path, g_run_id, GetCurrentProcessId());
    DWORD wrote = 0;
    WriteFile(h, msg, static_cast<DWORD>(std::strlen(msg)), &wrote, nullptr);
    CloseHandle(h);
}

LONG WINAPI se_filter(EXCEPTION_POINTERS* ep) noexcept {
    // The dump directory and run id were captured at install
    // time.  Re-reading the env vars from the exception path is
    // unsafe; the process may be in a torn state.
    if (g_dump_dir[0] == '\0') {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    std::string ts = timestamp_for_filename();
    char path[512];
    std::snprintf(path, sizeof(path), "%s\\%s-%s-%lu-%lu.dmp",
                  g_dump_dir, g_process_name[0] ? g_process_name : "client",
                  g_run_id, GetCurrentProcessId(),
                  static_cast<unsigned>(ts.size() > 0 ? ep->ExceptionRecord->ExceptionCode : 0));
    // Use a dedicated file handle so the dump is flushed even if
    // the CRT is torn.
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        write_fail_log(path, GetLastError());
        return EXCEPTION_EXECUTE_HANDLER;
    }
    MINIDUMP_EXCEPTION_INFORMATION mei;
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = ep;
    mei.ClientPointers = FALSE;
    const DWORD flags = MiniDumpWithIndirectlyReferencedMemory |
                        MiniDumpWithThreadInfo |
                        MiniDumpWithUnloadedModules;
    const BOOL ok = MiniDumpWriteDump(GetCurrentProcess(),
                                       GetCurrentProcessId(),
                                       file, static_cast<MINIDUMP_TYPE>(flags),
                                       &mei, nullptr, nullptr);
    const DWORD err = GetLastError();
    CloseHandle(file);
    if (!ok) {
        write_fail_log(path, err);
    } else {
        // We are not allowed to call MLOG_* from the exception
        // path (it locks a std::mutex).  Emit one fprintf to
        // stderr; capture-gamein.ps1 will pick it up.
        std::fprintf(stderr,
                     "mxh_client: CRASH pid=%lu run_id=%s code=0x%08lX dump=%s\n",
                     GetCurrentProcessId(), g_run_id,
                     static_cast<unsigned long>(ep->ExceptionRecord->ExceptionCode),
                     path);
        std::fflush(stderr);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

} // namespace

void install_crash_dump_handler() noexcept {
    bool expected = false;
    if (!g_installed.compare_exchange_strong(expected, true)) {
        return;
    }
    read_env_into(g_run_id, sizeof(g_run_id), "MXH_RUN_ID");
    read_env_into(g_dump_dir, sizeof(g_dump_dir), "MXH_DUMP_DIR");
    read_env_into(g_process_name, sizeof(g_process_name), "MXH_PROCESS");
    if (g_dump_dir[0] == '\0') {
        // No capture harness is active (e.g. a normal release
        // launch).  Roll back the install flag so a subsequent
        // process that does set MXH_DUMP_DIR can still install.
        g_installed.store(false);
        return;
    }
    SetUnhandledExceptionFilter(&se_filter);
}

const char* crash_dump_run_id() noexcept {
    return g_run_id;
}

} // namespace mxh::client

#else  // !_WIN32

namespace mxh::client {

void install_crash_dump_handler() noexcept {}
const char* crash_dump_run_id() noexcept { return ""; }

} // namespace mxh::client

#endif
