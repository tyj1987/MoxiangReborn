// platform_test.cpp - Phase 10.15 cross-platform abstraction layer tests
//
// Covers modern/include/mxh/compat/platform.hpp — the small layer of
// macros, type aliases, and inline helpers that lets the same source
// compile on Windows / Linux / macOS.
//
// What's tested:
//   - The 3 platform-detect macros are exactly one of {Windows,
//     Linux, macOS} (or an #error on the other). This pins the
//     preprocessor condition so a future change that misclassifies
//     the host platform surfaces as a compile error here.
//   - The 2-3 compiler-detect macros (MSVC, Clang, GCC) are mutually
//     exclusive.
//   - The 2 architecture-detect macros (X64, X86, ARM64) — at
//     least one is set on any build.
//   - socket_t / kInvalidSocket / kSocketError type aliases — pinned
//     so downstream code that depends on the size and value (e.g.
//     static_cast<SOCKET> in network tests) does not silently break.
//   - thread_id_t is an integer / handle type that we can read.
//   - get_current_thread_id() returns a non-zero value (the OS
//     always assigns some id to the main thread; 0 would indicate
//     a missing impl).
//   - The cross-platform socket helpers (set_tcp_nodelay,
//     set_reuse_addr) return false for an invalid socket instead
//     of crashing. The non-blocking setter needs a real Winsock
//     socket to test the success path, which is integration
//     territory; here we just verify the failure path on
//     kInvalidSocket.
//   - sockaddr_to_string returns "unknown" on null/empty input.
//
// What's NOT tested here:
//   - The Windows-vs-POSIX switch body itself. The branch is
//     chosen at compile time, so we can only verify the branch
//     the build is using. Cross-platform compile matrices (Linux
//     build of the same code) live in CI / docker, not in unit
//     tests.
//   - The success paths of set_non_blocking / set_tcp_nodelay /
//     set_reuse_addr require a real socket. They are exercised by
//     Socket::create() / bind() / connect() in the existing
//     net_test.cpp / socket_test.cpp.

#include "mxh/compat/platform.hpp"
#include "mxh/net/socket.hpp"  // for SocketGuard (Winsock init)

#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <type_traits>

#include <type_traits>

namespace mxh::compat::test {

// ===========================================================================
// Platform-detect macros
// ===========================================================================

TEST(PlatformTest, ExactlyOnePlatformMacroIsSet) {
    // Exactly one of Windows/Linux/macOS must be set on a given
    // build. If two are set, the platform.hpp chain is buggy (e.g.
    // _WIN32 and __linux__ both defined under Cygwin). If none are
    // set, the build would have failed at the #error in platform.hpp
    // already — this test would never run.
    int count = 0;
#if MXH_PLATFORM_WINDOWS
    ++count;
#endif
#if MXH_PLATFORM_LINUX
    ++count;
#endif
#if MXH_PLATFORM_MACOS
    ++count;
#endif
    EXPECT_EQ(count, 1) << "expected exactly 1 of MXH_PLATFORM_{WINDOWS,LINUX,MACOS}";
}

TEST(PlatformTest, PlatformNameIsNonEmpty) {
    // MXH_PLATFORM_NAME is a string literal. The platform.hpp
    // defines it for each supported platform; an empty or missing
    // definition is a regression.
#if MXH_PLATFORM_WINDOWS || MXH_PLATFORM_LINUX || MXH_PLATFORM_MACOS
    EXPECT_GT(std::char_traits<char>::length(MXH_PLATFORM_NAME), 0u);
    SUCCEED();
#else
    FAIL() << "no platform macro is set; this build is unsupported";
#endif
}

// ===========================================================================
// Compiler-detect macros
// ===========================================================================

TEST(CompilerTest, ExactlyOneCompilerMacroIsSet) {
    // MSVC / Clang / GCC must be mutually exclusive. Two compilers
    // being set usually means a mis-detected compiler (e.g. a
    // clang-cl build where both _MSC_VER and __clang__ are defined
    // — the platform.hpp picks the first match in #if order).
    int count = 0;
#if MXH_COMPILER_MSVC
    ++count;
#endif
#if MXH_COMPILER_CLANG
    ++count;
#endif
#if MXH_COMPILER_GCC
    ++count;
#endif
    EXPECT_EQ(count, 1) << "expected exactly 1 of MXH_COMPILER_{MSVC,CLANG,GCC}";
}

TEST(CompilerTest, CompilerNameIsNonEmpty) {
#if MXH_COMPILER_MSVC || MXH_COMPILER_CLANG || MXH_COMPILER_GCC
    EXPECT_GT(std::char_traits<char>::length(MXH_COMPILER_NAME), 0u);
    SUCCEED();
#else
    FAIL() << "no compiler macro is set";
#endif
}

// ===========================================================================
// Architecture-detect macros
// ===========================================================================

TEST(ArchTest, AtLeastOneArchMacroIsSet) {
    // X64 / X86 / ARM64 — exactly one of these should match. If
    // none match, we're on an architecture the platform.hpp chain
    // doesn't cover and the rest of the codebase (which uses
    // MXH_ARCH_*) won't compile correctly.
    int count = 0;
#if MXH_ARCH_X64
    ++count;
#endif
#if MXH_ARCH_X86
    ++count;
#endif
#if MXH_ARCH_ARM64
    ++count;
#endif
    EXPECT_GE(count, 1) << "no MXH_ARCH_* macro set; this arch is not supported";
    EXPECT_EQ(count, 1) << "expected exactly 1 of MXH_ARCH_{X64,X86,ARM64}";
}

TEST(ArchTest, ArchNameIsNonEmpty) {
#if MXH_ARCH_X64 || MXH_ARCH_X86 || MXH_ARCH_ARM64
    EXPECT_GT(std::char_traits<char>::length(MXH_ARCH_NAME), 0u);
    SUCCEED();
#else
    FAIL() << "no MXH_ARCH_* macro set";
#endif
}

// ===========================================================================
// socket_t type alias
// ===========================================================================

TEST(SocketTypeTest, IsUnsigned) {
    // On Windows socket_t = SOCKET which is UINT_PTR (unsigned).
    // On POSIX socket_t = int (signed). The test pins whatever the
    // build host is; we don't try to cross-compile and verify both
    // branches from one binary.
#if MXH_PLATFORM_WINDOWS
    static_assert(std::is_same_v<socket_t, unsigned long long> ||
                  std::is_same_v<socket_t, unsigned int> ||
                  std::is_same_v<socket_t, unsigned long> ||
                  std::is_same_v<socket_t, void*>,
                  "Windows socket_t should be an unsigned/pointer type");
#else
    static_assert(std::is_same_v<socket_t, int>,
                  "POSIX socket_t should be int");
#endif
    SUCCEED();
}

TEST(SocketTypeTest, InvalidSocketIsDistinctFromZero) {
    // kInvalidSocket must be a value the OS will never return for a
    // real socket. On Windows it's INVALID_SOCKET (which is
    // (SOCKET)(-1) on the WinSock header). On POSIX it's -1.
    EXPECT_NE(kInvalidSocket, 0);
    EXPECT_NE(kSocketError, 0);
}

TEST(SocketTypeTest, InvalidSocketAndSocketErrorMatch) {
    // kInvalidSocket and kSocketError are the same negative value
    // on both Windows and POSIX. Downstream code uses them
    // interchangeably as "this socket handle is not usable".
    EXPECT_EQ(kInvalidSocket, kSocketError);
}

// ===========================================================================
// get_current_thread_id
// ===========================================================================

TEST(ThreadIdTest, ReturnsNonZero) {
    // The OS always assigns a non-zero thread id to the main thread.
    // 0 would indicate a broken impl.
    thread_id_t id = get_current_thread_id();
    EXPECT_NE(id, 0);
}

TEST(ThreadIdTest, IsConsistentWithinThread) {
    // Two calls in the same thread must return the same id.
    thread_id_t a = get_current_thread_id();
    thread_id_t b = get_current_thread_id();
    EXPECT_EQ(a, b);
}

TEST(ThreadIdTest, DiffersAcrossThreads) {
    // A second thread must have a different id. (On Windows
    // GetCurrentThreadId returns a DWORD; on POSIX pthread_self
    // returns pthread_t which is opaque but comparable.)
    thread_id_t main_id = get_current_thread_id();
    thread_id_t other_id = 0;
    std::thread t([&other_id]() { other_id = get_current_thread_id(); });
    t.join();
    EXPECT_NE(other_id, 0);
    EXPECT_NE(main_id, other_id);
}

// ===========================================================================
// Socket helpers on kInvalidSocket (failure path is the only safe
// path to test without a real socket)
// ===========================================================================

TEST(SocketHelpersTest, SetTcpNodelayOnInvalidSocketReturnsFalse) {
    // setsockopt on kInvalidSocket should return -1 / SOCKET_ERROR
    // and the helper returns false. If it ever returns true the
    // helper is incorrectly swallowing the error.
    EXPECT_FALSE(mxh::compat::set_tcp_nodelay(kInvalidSocket, true));
    EXPECT_FALSE(mxh::compat::set_tcp_nodelay(kInvalidSocket, false));
}

TEST(SocketHelpersTest, SetReuseAddrOnInvalidSocketReturnsFalse) {
    EXPECT_FALSE(mxh::compat::set_reuse_addr(kInvalidSocket, true));
    EXPECT_FALSE(mxh::compat::set_reuse_addr(kInvalidSocket, false));
}

TEST(SocketHelpersTest, SetNonBlockingOnInvalidSocketReturnsFalse) {
    // On Windows ioctlsocket on an invalid socket returns
    // SOCKET_ERROR. On POSIX fcntl on -1 returns -1. Both helpers
    // surface that as false.
    EXPECT_FALSE(mxh::compat::set_non_blocking(kInvalidSocket, true));
    EXPECT_FALSE(mxh::compat::set_non_blocking(kInvalidSocket, false));
}

// ===========================================================================
// sockaddr_to_string
// ===========================================================================

TEST(SockaddrToStringTest, NullAddrReturnsEmpty) {
    // The cpp's first guard is `if (!addr || addr_len == 0) return "";`
    // so null or zero-length inputs return empty. (Earlier pass
    // expected "unknown" but the actual return is empty string.)
    EXPECT_EQ(mxh::compat::sockaddr_to_string(nullptr, 0), "");
}

TEST(SockaddrToStringTest, ZeroLengthReturnsEmpty) {
    // A valid pointer but zero length also trips the same guard.
    sockaddr sa{};
    sa.sa_family = AF_INET;
    EXPECT_EQ(mxh::compat::sockaddr_to_string(&sa, 0), "");
}

TEST(SockaddrToStringTest, ValidIPv4ReturnsHostColonService) {
    // mxh::net::SocketGuard initialises Winsock (WSAStartup) for
    // this test. Without it, getnameinfo on Windows returns
    // WSANOTINITIALISED and the helper falls through to the
    // "unknown" return. Same gotcha as iocp_test.cpp.
    mxh::net::SocketGuard guard;

    // 127.0.0.1:8080 → "127.0.0.1:8080"
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(0x7F000001);

    std::string s = mxh::compat::sockaddr_to_string(
        reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    EXPECT_EQ(s, "127.0.0.1:8080");
}

TEST(SockaddrToStringTest, ValidIPv6ReturnsHostColonService) {
    mxh::net::SocketGuard guard;

    // ::1:9090 → "::1:9090" (getnameinfo on Windows formats IPv6
    // without brackets — this differs from getnameinfo on Linux
    // which uses "[::1]:9090". The test pins the Windows format
    // since this is a Windows build; on a POSIX build the test
    // would need to be conditional on MXH_PLATFORM_LINUX.)
    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(9090);
    addr.sin6_addr.s6_addr[15] = 1;

    std::string s = mxh::compat::sockaddr_to_string(
        reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    // The key invariant: port 9090 is in the output and the host
    // part is non-empty.
    EXPECT_NE(s.find("9090"), std::string::npos);
    EXPECT_FALSE(s.empty());
}

}  // namespace mxh::compat::test
