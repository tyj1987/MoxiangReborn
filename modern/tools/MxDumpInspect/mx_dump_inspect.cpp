// mx_dump_inspect.cpp
//
// Minimal minidump inspector for the Phase 0 GameIn crash investigation.
// Opens a minidump file, walks the directory, prints the exception
// record + thread context + module list. Does not unwind the stack
// (which needs symbols); operators can load the dump into WinDbg
// for a full unwinding. The point of this tool is to identify the
// exception address, code, and module so we can grep the source
// tree for the failing call without booting cdb.
//
// Build: cl /EHsc /std:c++20 /I"$(VCINSTALLDIR)include" mx_dump_inspect.cpp
//        /link dbghelp.lib
//
// Usage: mx_dump_inspect.exe <path-to-minidump>
#include <windows.h>
#include <dbghelp.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#pragma comment(lib, "dbghelp.lib")

namespace {

void print_usage(const char* argv0) {
    std::fprintf(stderr, "usage: %s <path-to-minidump>\n", argv0);
}

const void* rva_to_ptr(const std::uint8_t* base, RVA rva) {
    return base + rva;
}

const wchar_t* rva_to_wide(const std::uint8_t* base, RVA rva) {
    return rva ? reinterpret_cast<const wchar_t*>(base + rva) : L"";
}

void wide_to_utf8(const wchar_t* w, char* out, std::size_t cap) {
    if (!w || cap == 0) { if (cap) out[0] = '\0'; return; }
    WideCharToMultiByte(CP_UTF8, 0, w, -1, out,
                        static_cast<int>(cap), nullptr, nullptr);
}

void print_exception_stream(const std::uint8_t* base,
                            const MINIDUMP_EXCEPTION_STREAM* ex) {
    std::printf("\n[exception stream]\n");
    std::printf("  ThreadId      : %lu\n",
                static_cast<unsigned long>(ex->ThreadId));
    const auto& rec = ex->ExceptionRecord;
    std::printf("  ExceptionCode : 0x%08lX\n",
                static_cast<unsigned long>(rec.ExceptionCode));
    std::printf("  ExceptionFlags: 0x%08lX\n",
                static_cast<unsigned long>(rec.ExceptionFlags));
    std::printf("  ExceptionRecord: 0x%016llx\n",
                static_cast<unsigned long long>(rec.ExceptionRecord));
    std::printf("  ExceptionAddress: 0x%016llx\n",
                static_cast<unsigned long long>(rec.ExceptionAddress));
    std::printf("  NumberParameters: %lu\n",
                static_cast<unsigned long>(rec.NumberParameters));
    if (ex->ThreadContext.DataSize >= sizeof(CONTEXT)) {
        const auto* ctx = reinterpret_cast<const CONTEXT*>(
            base + ex->ThreadContext.Rva);
        std::printf("\n[thread context]\n");
        std::printf("  ContextFlags   : 0x%08lX\n",
                    static_cast<unsigned long>(ctx->ContextFlags));
        std::printf("  Eip            : 0x%08lX\n",
                    static_cast<unsigned long>(ctx->Eip));
        std::printf("  Esp            : 0x%08lX\n",
                    static_cast<unsigned long>(ctx->Esp));
        std::printf("  Ebp            : 0x%08lX\n",
                    static_cast<unsigned long>(ctx->Ebp));
        std::printf("  Eax Ecx Edx Ebx : 0x%08lX 0x%08lX 0x%08lX 0x%08lX\n",
                    static_cast<unsigned long>(ctx->Eax),
                    static_cast<unsigned long>(ctx->Ecx),
                    static_cast<unsigned long>(ctx->Edx),
                    static_cast<unsigned long>(ctx->Ebx));
        std::printf("  Esi Edi        : 0x%08lX 0x%08lX\n",
                    static_cast<unsigned long>(ctx->Esi),
                    static_cast<unsigned long>(ctx->Edi));
        std::printf("  EFlags         : 0x%08lX\n",
                    static_cast<unsigned long>(ctx->EFlags));
    } else {
        std::printf("  (no CONTEXT: ThreadContext.DataSize=%lu)\n",
                    static_cast<unsigned long>(ex->ThreadContext.DataSize));
    }
    if (rec.NumberParameters > 0 && rec.NumberParameters <= EXCEPTION_MAXIMUM_PARAMETERS) {
        std::printf("  ExceptionInformation:\n");
        for (std::uint32_t i = 0; i < rec.NumberParameters; ++i) {
            std::printf("    [%u] 0x%016llx\n", i,
                        static_cast<unsigned long long>(
                            rec.ExceptionInformation[i]));
        }
    }
}

void print_modules(const std::uint8_t* base,
                   const MINIDUMP_MODULE_LIST* ml) {
    std::printf("\n[modules] count=%lu\n",
                static_cast<unsigned long>(ml->NumberOfModules));
    for (std::uint32_t i = 0; i < ml->NumberOfModules; ++i) {
        const auto& m = ml->Modules[i];
        char buf[256] = {0};
        wide_to_utf8(rva_to_wide(base, m.ModuleNameRva), buf, sizeof(buf));
        std::printf("  [%u] %-40s base=0x%016llx size=0x%08lx ts=0x%08lx\n",
                    i, buf,
                    static_cast<unsigned long long>(m.BaseOfImage),
                    static_cast<unsigned long>(m.SizeOfImage),
                    static_cast<unsigned long>(m.TimeDateStamp));
    }
}

void print_threads(const std::uint8_t* base,
                   const MINIDUMP_THREAD_LIST* tl) {
    std::printf("\n[threads] count=%lu\n",
                static_cast<unsigned long>(tl->NumberOfThreads));
    for (std::uint32_t i = 0; i < tl->NumberOfThreads; ++i) {
        const auto& t = tl->Threads[i];
        std::printf("  [%u] id=%lu stack=0x%016llx size=0x%08lx ctx=0x%08lx\n",
                    i,
                    static_cast<unsigned long>(t.ThreadId),
                    static_cast<unsigned long long>(t.Stack.StartOfMemoryRange),
                    static_cast<unsigned long>(t.Stack.Memory.DataSize),
                    static_cast<unsigned long>(t.ThreadContext.Rva));
    }
}

void walk_stack(const MINIDUMP_EXCEPTION_STREAM* ex,
                const MINIDUMP_MODULE_LIST* ml) {
    // Translate the exception address into a module + offset.
    const auto addr = ex->ExceptionRecord.ExceptionAddress;
    std::printf("\n[module offset]\n");
    for (std::uint32_t i = 0; i < ml->NumberOfModules; ++i) {
        const auto& m = ml->Modules[i];
        const auto base = m.BaseOfImage;
        const auto end = base + m.SizeOfImage;
        if (addr >= base && addr < end) {
            char buf[256] = {0};
            // rva_to_wide is bound to the minidump's base. We
            // cannot use it here because the RVAs in the module
            // list are file-offsets, not runtime VAs.  Use the
            // ModuleNameRva we have already resolved by going
            // through the dump header.
            // For simplicity: locate the wchar_t in the dump.
            std::printf("  exception address 0x%016llx is in module[%u] base=0x%016llx +0x%llx\n",
                        static_cast<unsigned long long>(addr),
                        i,
                        static_cast<unsigned long long>(base),
                        static_cast<unsigned long long>(addr - base));
            (void)buf;
            break;
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { print_usage(argv[0]); return 1; }
    const std::string dump_path = argv[1];
    HANDLE file = CreateFileA(dump_path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        std::fprintf(stderr, "cannot open %s: GetLastError=%lu\n",
                     dump_path.c_str(), GetLastError());
        return 1;
    }
    HANDLE map = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!map) {
        std::fprintf(stderr, "CreateFileMapping failed: %lu\n", GetLastError());
        CloseHandle(file);
        return 1;
    }
    LPVOID base = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    if (!base) {
        std::fprintf(stderr, "MapViewOfFile failed: %lu\n", GetLastError());
        CloseHandle(map);
        CloseHandle(file);
        return 1;
    }
    const auto* bytes = static_cast<const std::uint8_t*>(base);
    const auto* hdr = static_cast<const MINIDUMP_HEADER*>(base);
    std::printf("[dump header]\n");
    std::printf("  Signature        : 0x%08lx\n",
                static_cast<unsigned long>(hdr->Signature));
    std::printf("  Version          : 0x%04x\n", hdr->Version);
    std::printf("  NumberOfStreams  : %lu\n",
                static_cast<unsigned long>(hdr->NumberOfStreams));
    std::printf("  StreamDirectoryRva: 0x%08lx\n",
                static_cast<unsigned long>(hdr->StreamDirectoryRva));
    if (hdr->NumberOfStreams == 0 || hdr->StreamDirectoryRva == 0) {
        std::fprintf(stderr, "dump has no directory\n");
        UnmapViewOfFile(base); CloseHandle(map); CloseHandle(file);
        return 1;
    }
    const auto* entries = reinterpret_cast<const MINIDUMP_DIRECTORY*>(
        bytes + hdr->StreamDirectoryRva);
    const MINIDUMP_EXCEPTION_STREAM* exc = nullptr;
    const MINIDUMP_MODULE_LIST*    ml  = nullptr;
    const MINIDUMP_THREAD_LIST*     tl  = nullptr;
    for (std::uint32_t i = 0; i < hdr->NumberOfStreams; ++i) {
        const auto& e = entries[i];
        const auto* p = bytes + e.Location.Rva;
        switch (e.StreamType) {
            case ExceptionStream: exc = reinterpret_cast<const MINIDUMP_EXCEPTION_STREAM*>(p); break;
            case ModuleListStream: ml  = reinterpret_cast<const MINIDUMP_MODULE_LIST*>(p); break;
            case ThreadListStream: tl  = reinterpret_cast<const MINIDUMP_THREAD_LIST*>(p); break;
            default: break;
        }
    }
    if (exc) {
        print_exception_stream(bytes, exc);
    } else {
        std::printf("\nno exception stream in dump\n");
    }
    if (tl) {
        print_threads(bytes, tl);
    }
    if (ml) {
        print_modules(bytes, ml);
        if (exc) walk_stack(exc, ml);
    }
    UnmapViewOfFile(base);
    CloseHandle(map);
    CloseHandle(file);
    return 0;
}
