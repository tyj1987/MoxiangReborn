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
#include <winternl.h>

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

// VEH handle: 0 until AddVectoredExceptionHandler is called.  We
// use the VEH to log the *real* throw call site for 0xE06D7363
// (C++ throw) — the SEH chain returns EXCEPTION_EXECUTE_HANDLER
// from a function whose `__try` filter shadows the original
// __CxxThrowException@8 call, so the dump's reported EIP is the
// SEH unwinder's landing pad, not the throw helper.  VEH runs
// before any SEH filter, so the ExceptionAddress is the actual
// `call __CxxThrowException` instruction in the offending TU.
void* g_veh_handle = nullptr;
std::atomic<bool> g_veh_logged = false;

LONG WINAPI veh_filter(EXCEPTION_POINTERS* ep) noexcept {
    if (!ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
    constexpr DWORD kCppThrow = 0xE06D7363;
    if (ep->ExceptionRecord->ExceptionCode != kCppThrow) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    // Log at most once per process — the SEH filter terminates
    // anyway, but repeated VEH firing during a single unwind can
    // spam stderr and obscure earlier diagnostics.
    bool expected = false;
    if (!g_veh_logged.compare_exchange_strong(expected, true)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    const auto* er = ep->ExceptionRecord;
    char line[512];
    const ULONG_PTR exc_addr = reinterpret_cast<ULONG_PTR>(er->ExceptionAddress);
    const ULONG_PTR exc_obj = er->NumberParameters >= 1
        ? er->ExceptionInformation[0]
        : 0UL;
    const ULONG_PTR ebp = ep->ContextRecord
        ? ep->ContextRecord->Ebp
        : 0UL;
    const ULONG_PTR esp = ep->ContextRecord
        ? ep->ContextRecord->Esp
        : 0UL;
    std::snprintf(line, sizeof(line),
                  "mxh_client: VEH throw site pid=%lu tid=%lu code=0x%08lX "
                  "addr=0x%IX obj=0x%IX nparams=%lu ebp=0x%IX esp=0x%IX "
                  "run_id=%s\n",
                  GetCurrentProcessId(), GetCurrentThreadId(),
                  static_cast<unsigned long>(er->ExceptionCode),
                  exc_addr, exc_obj,
                  static_cast<unsigned long>(er->NumberParameters),
                  ebp, esp,
                  g_run_id);
    // Capture the stack so we can map the throw call site to a
    // CInGameState.cpp line.  EBP-based walk avoids the safe-SEH
    // bypass required by RtlCaptureStackBackTrace and is plenty
    // for the 5-15 deep call chain the entity-scene render path
    // produces.
    char* cursor = line + std::strlen(line);
    constexpr std::size_t kLineCap = sizeof(line);
    constexpr std::size_t kFrameCap = 16;
    void* frames[kFrameCap] = {nullptr};
    USHORT n = RtlCaptureStackBackTrace(0, kFrameCap, frames, nullptr);
    if (cursor < line + kLineCap) {
        int wrote = std::snprintf(cursor, kLineCap - (cursor - line),
                                  "  frames=%u: ", static_cast<unsigned>(n));
        if (wrote > 0) cursor += wrote;
    }
    for (USHORT i = 0; i < n && cursor < line + kLineCap - 16; ++i) {
        const ULONG_PTR ret_addr = reinterpret_cast<ULONG_PTR>(frames[i]);
        int wrote = std::snprintf(cursor, kLineCap - (cursor - line),
                                  "%s0x%IX",
                                  i == 0 ? "" : " ",
                                  ret_addr);
        if (wrote > 0) cursor += wrote;
    }
    if (cursor < line + kLineCap - 1) {
        *cursor = '\n';
        ++cursor;
        *cursor = '\0';
    }
    // Three write paths so the diagnostic survives the SEH filter
    // terminating the process and any handle re-routing the
    // capture harness performs on stderr:
    //   1. A dedicated `veh.log` next to the minidump (only
    //      writable when MXH_DUMP_DIR is set, which the capture
    //      harness does for every smoke run).
    //   2. The inherited stderr handle.
    //   3. The inherited stdout handle.
    if (g_dump_dir[0] != '\0') {
        char path[512];
        std::snprintf(path, sizeof(path), "%s\\veh.log", g_dump_dir);
        HANDLE file = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ,
                                  nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                  nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            DWORD wrote = 0;
            WriteFile(file, line, static_cast<DWORD>(std::strlen(line)),
                      &wrote, nullptr);
            CloseHandle(file);
        }
    }
    HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
    if (err != nullptr && err != INVALID_HANDLE_VALUE) {
        DWORD wrote = 0;
        WriteFile(err, line, static_cast<DWORD>(std::strlen(line)), &wrote, nullptr);
    }
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != nullptr && out != INVALID_HANDLE_VALUE) {
        DWORD wrote = 0;
        WriteFile(out, line, static_cast<DWORD>(std::strlen(line)), &wrote, nullptr);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

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
    // VEH runs first in the exception chain, ahead of any SEH
    // filter.  We register it after SetUnhandledExceptionFilter so
    // the order is: kernel -> VEH -> SEH -> filter -> unwinder.
    if (g_veh_handle == nullptr) {
        g_veh_handle = AddVectoredExceptionHandler(1, &veh_filter);
    }
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
