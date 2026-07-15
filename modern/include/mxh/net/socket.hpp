// socket.hpp - Cross-platform socket abstraction for Moxian-Reborn.
//
// Wraps platform-specific socket operations behind a uniform interface.
// Supports Windows (WinSock2) and POSIX (Linux/macOS) platforms.
//
// Design:
//   - RAII socket wrapper with automatic cleanup
//   - Non-blocking I/O support
//   - TCP_NODELAY and SO_REUSEADDR options
//   - Address resolution and connection
//   - Thread-safe send/receive operations

#pragma once

#include "mxh/compat/platform.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <chrono>
#include <string_view>
#include <system_error>
#include <vector>

namespace mxh::net {

// ============================================================================
// Socket address wrapper
// ============================================================================

class SocketAddress {
public:
    SocketAddress() = default;
    SocketAddress(std::string_view host, std::uint16_t port);

    // Construct from sockaddr_storage
    explicit SocketAddress(const struct sockaddr_storage& addr);

    [[nodiscard]] const struct sockaddr* data() const noexcept {
        return reinterpret_cast<const struct sockaddr*>(&addr_);
    }
    [[nodiscard]] struct sockaddr* data() noexcept {
        return reinterpret_cast<struct sockaddr*>(&addr_);
    }
    [[nodiscard]] socklen_t size() const noexcept { return addr_len_; }

    [[nodiscard]] std::string host() const;
    [[nodiscard]] std::uint16_t port() const;
    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] bool is_valid() const noexcept { return addr_len_ > 0; }

private:
    struct sockaddr_storage addr_ = {};
    socklen_t addr_len_ = 0;
};

// ============================================================================
// Socket class (RAII wrapper)
// ============================================================================

class Socket {
public:
    // Socket types
    enum class Type { TCP, UDP };

    // Socket states
    enum class State { Invalid, Created, Bound, Listening, Connected, Closed };

    Socket() = default;
    explicit Socket(Type type);
    ~Socket();

    // Move-only
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // Create socket
    [[nodiscard]] std::error_code create(Type type);

    // Bind to address
    [[nodiscard]] std::error_code bind(const SocketAddress& addr);

    // Listen for connections
    [[nodiscard]] std::error_code listen(int backlog = 128);

    // Accept incoming connection
    [[nodiscard]] std::pair<Socket, std::error_code> accept();

    // Connect to remote address
    [[nodiscard]] std::error_code connect(const SocketAddress& addr);

    // Send data
    [[nodiscard]] std::pair<std::size_t, std::error_code> send(
        std::span<const std::uint8_t> data);

    // Receive data
    [[nodiscard]] std::pair<std::size_t, std::error_code> receive(
        std::span<std::uint8_t> buffer);

    // Close socket
    void close();

    // Options
    [[nodiscard]] std::error_code set_non_blocking(bool enable);
    [[nodiscard]] std::error_code set_tcp_nodelay(bool enable);
    [[nodiscard]] std::error_code set_reuse_addr(bool enable);
    [[nodiscard]] std::error_code set_send_buffer_size(int size);
    [[nodiscard]] std::error_code set_recv_buffer_size(int size);

    // Set connect timeout. If a non-blocking connect is still pending after
    // this duration, Socket::connect() returns SocketErrc::Timeout. Default
    // is 5000ms; 0 disables the timeout (blocks indefinitely).
    void set_connect_timeout(std::chrono::milliseconds t) noexcept {
        connect_timeout_ = t;
    }
    [[nodiscard]] std::chrono::milliseconds connect_timeout() const noexcept {
        return connect_timeout_;
    }

    // State queries
    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] bool is_valid() const noexcept { return sock_ != kInvalidSocket; }
    [[nodiscard]] socket_t native_handle() const noexcept { return sock_; }

    // Get local address
    [[nodiscard]] SocketAddress local_address() const;

    // Get remote address
    [[nodiscard]] SocketAddress remote_address() const;

private:
    socket_t sock_ = kInvalidSocket;
    State state_ = State::Invalid;
    Type type_ = Type::TCP;
    bool non_blocking_ = false;
    std::chrono::milliseconds connect_timeout_{5000};
};

// ============================================================================
// Socket initialization RAII guard
// ============================================================================

class SocketGuard {
public:
    SocketGuard() { init_sockets(); }
    ~SocketGuard() { cleanup_sockets(); }

    SocketGuard(const SocketGuard&) = delete;
    SocketGuard& operator=(const SocketGuard&) = delete;
};

// ============================================================================
// Error codes
// ============================================================================

enum class SocketErrc {
    Success = 0,
    NotInitialized,
    CreateFailed,
    BindFailed,
    ListenFailed,
    AcceptFailed,
    ConnectFailed,
    SendFailed,
    ReceiveFailed,
    Timeout,
    WouldBlock,
    ConnectionReset,
    ConnectionRefused,
    AddressInUse,
    InvalidArgument,
};

// Custom error category
class SocketErrorCategory : public std::error_category {
public:
    [[nodiscard]] const char* name() const noexcept override { return "socket"; }
    [[nodiscard]] std::string message(int ev) const override;
};

[[nodiscard]] const std::error_category& socket_error_category() noexcept;
[[nodiscard]] std::error_code make_error_code(SocketErrc ec) noexcept;

} // namespace mxh::net

// Make SocketErrc work with std::error_code
namespace std {
template <>
struct is_error_code_enum<mxh::net::SocketErrc> : std::true_type {};
} // namespace std
