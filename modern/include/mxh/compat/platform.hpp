// platform.hpp - Cross-platform compatibility layer for Moxian-Reborn.
//
// Provides platform detection, type aliases, and abstraction macros
// to support Windows, Linux, and macOS builds from a single codebase.
//
// Usage:
//   #include "mxh/compat/platform.hpp"
//   #if MXH_PLATFORM_WINDOWS
//     // Windows-specific code
//   #elif MXH_PLATFORM_LINUX
//     // Linux-specific code
//   #endif

#pragma once

#include <string>
#include <cstdint>

// ============================================================================
// Platform detection
// =============================================================================

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define MXH_PLATFORM_WINDOWS 1
    #define MXH_PLATFORM_NAME "Windows"
#elif defined(__linux__)
    #define MXH_PLATFORM_LINUX 1
    #define MXH_PLATFORM_NAME "Linux"
#elif defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define MXH_PLATFORM_MACOS 1
        #define MXH_PLATFORM_NAME "macOS"
    #else
        #error "Unsupported Apple platform"
    #endif
#else
    #error "Unsupported platform"
#endif

// ============================================================================
// Compiler detection
// ============================================================================

#if defined(_MSC_VER)
    #define MXH_COMPILER_MSVC 1
    #define MXH_COMPILER_NAME "MSVC"
#elif defined(__clang__)
    #define MXH_COMPILER_CLANG 1
    #define MXH_COMPILER_NAME "Clang"
#elif defined(__GNUC__)
    #define MXH_COMPILER_GCC 1
    #define MXH_COMPILER_NAME "GCC"
#endif

// ============================================================================
// Architecture detection
// ============================================================================

#if defined(_M_X64) || defined(__x86_64__)
    #define MXH_ARCH_X64 1
    #define MXH_ARCH_NAME "x64"
#elif defined(_M_IX86) || defined(__i386__)
    #define MXH_ARCH_X86 1
    #define MXH_ARCH_NAME "x86"
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define MXH_ARCH_ARM64 1
    #define MXH_ARCH_NAME "ARM64"
#endif

// ============================================================================
// Windows-specific includes and definitions
// ============================================================================

#if MXH_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")

    // Ensure NI_MAXHOST and NI_MAXSERV are defined
    #ifndef NI_MAXHOST
        #define NI_MAXHOST 1025
    #endif
    #ifndef NI_MAXSERV
        #define NI_MAXSERV 32
    #endif

    // Socket type aliases
    using socket_t = SOCKET;
    constexpr socket_t kInvalidSocket = INVALID_SOCKET;
    constexpr int kSocketError = SOCKET_ERROR;

    // Close socket
    inline int close_socket(socket_t s) { return closesocket(s); }

    // Get last error
    inline int get_socket_error() { return WSAGetLastError(); }

    // Initialize socket subsystem
    inline bool init_sockets() {
        WSADATA wsa_data;
        return WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
    }

    // Cleanup socket subsystem
    inline void cleanup_sockets() { WSACleanup(); }

// ============================================================================
// POSIX (Linux/macOS) includes and definitions
// ============================================================================

#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <netdb.h>

    // Socket type aliases
    using socket_t = int;
    constexpr socket_t kInvalidSocket = -1;
    constexpr int kSocketError = -1;

    // Close socket
    inline int close_socket(socket_t s) { return ::close(s); }

    // Get last error
    inline int get_socket_error() { return errno; }

    // Initialize socket subsystem (no-op on POSIX)
    inline bool init_sockets() { return true; }

    // Cleanup socket subsystem (no-op on POSIX)
    inline void cleanup_sockets() {}

    // WSA error codes mapped to POSIX
    #define WSAEWOULDBLOCK EWOULDBLOCK
    #define WSAEINPROGRESS EINPROGRESS
    #define WSAEINTR EINTR
    #define WSAEINVAL EINVAL
    #define WSAECONNRESET ECONNRESET
    #define WSAECONNREFUSED ECONNREFUSED
    #define WSAETIMEDOUT ETIMEDOUT
    #define WSAENOTCONN ENOTCONN
    #define WSAEADDRINUSE EADDRINUSE
#endif

// ============================================================================
// Cross-platform socket address helpers
// ============================================================================

namespace mxh::compat {

// Convert sockaddr to human-readable string
inline std::string sockaddr_to_string(const struct sockaddr* addr, socklen_t addr_len) {
    if (!addr || addr_len == 0) return "";

    char host[NI_MAXHOST] = {0};
    char service[NI_MAXSERV] = {0};
    int result = getnameinfo(addr, addr_len, host, sizeof(host), service, sizeof(service),
                             NI_NUMERICHOST | NI_NUMERICSERV);
    if (result != 0) {
        return "unknown";
    }
    return std::string(host) + ":" + std::string(service);
}

// Set socket blocking mode. enable=true -> non-blocking, false -> blocking.
inline bool set_non_blocking(socket_t sock, bool enable) {
#if MXH_PLATFORM_WINDOWS
    unsigned long mode = enable ? 1 : 0;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return false;
    if (enable) {
        return fcntl(sock, F_SETFL, flags | O_NONBLOCK) == 0;
    } else {
        return fcntl(sock, F_SETFL, flags & ~O_NONBLOCK) == 0;
    }
#endif
}

// Set TCP_NODELAY option
inline bool set_tcp_nodelay(socket_t sock, bool enable) {
    int flag = enable ? 1 : 0;
    return setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag),
                      sizeof(flag)) == 0;
}

// Set SO_REUSEADDR option
inline bool set_reuse_addr(socket_t sock, bool enable) {
    int flag = enable ? 1 : 0;
    return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&flag),
                      sizeof(flag)) == 0;
}

} // namespace mxh::compat

// ============================================================================
// Cross-platform thread helpers
// ============================================================================

#if MXH_PLATFORM_WINDOWS
    #include <process.h>
    using thread_id_t = DWORD;
    inline thread_id_t get_current_thread_id() { return GetCurrentThreadId(); }
#else
    #include <pthread.h>
    using thread_id_t = pthread_t;
    inline thread_id_t get_current_thread_id() { return pthread_self(); }
#endif

// ============================================================================
// Cross-platform filesystem helpers
// ============================================================================

#if __cplusplus >= 201703L
    #include <filesystem>
    namespace fs = std::filesystem;
#else
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#endif
