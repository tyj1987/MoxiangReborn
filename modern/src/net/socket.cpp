// socket.cpp - Cross-platform socket implementation.

#include "mxh/net/socket.hpp"

#include <cstring>
#include <chrono>
#include <iostream>

namespace mxh::net {

// ============================================================================
// SocketAddress implementation
// ============================================================================

SocketAddress::SocketAddress(std::string_view host, std::uint16_t port) {
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* result = nullptr;
    std::string host_str(host);
    std::string port_str = std::to_string(port);

    if (getaddrinfo(host_str.c_str(), port_str.c_str(), &hints, &result) == 0 && result) {
        std::memcpy(&addr_, result->ai_addr, result->ai_addrlen);
        addr_len_ = static_cast<socklen_t>(result->ai_addrlen);
        freeaddrinfo(result);
    }
}

SocketAddress::SocketAddress(const struct sockaddr_storage& addr) : addr_(addr) {
    if (addr.ss_family == AF_INET) {
        addr_len_ = sizeof(struct sockaddr_in);
    } else if (addr.ss_family == AF_INET6) {
        addr_len_ = sizeof(struct sockaddr_in6);
    }
}

std::string SocketAddress::host() const {
    char host[NI_MAXHOST] = {0};
    getnameinfo(data(), size(), host, sizeof(host), nullptr, 0, NI_NUMERICHOST);
    return host;
}

std::uint16_t SocketAddress::port() const {
    if (addr_.ss_family == AF_INET) {
        return ntohs(reinterpret_cast<const struct sockaddr_in*>(&addr_)->sin_port);
    } else if (addr_.ss_family == AF_INET6) {
        return ntohs(reinterpret_cast<const struct sockaddr_in6*>(&addr_)->sin6_port);
    }
    return 0;
}

std::string SocketAddress::to_string() const {
    return host() + ":" + std::to_string(port());
}

// ============================================================================
// Socket implementation
// ============================================================================

Socket::Socket(Type type) : type_(type) {
    (void)create(type);
}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept
    : sock_(other.sock_), state_(other.state_), type_(other.type_) {
    other.sock_ = kInvalidSocket;
    other.state_ = State::Invalid;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        sock_ = other.sock_;
        state_ = other.state_;
        type_ = other.type_;
        other.sock_ = kInvalidSocket;
        other.state_ = State::Invalid;
    }
    return *this;
}

std::error_code Socket::create(Type type) {
    if (is_valid()) {
        close();
    }

    type_ = type;
    int domain = AF_INET;
    int sock_type = (type == Type::TCP) ? SOCK_STREAM : SOCK_DGRAM;
    int protocol = (type == Type::TCP) ? IPPROTO_TCP : IPPROTO_UDP;

    sock_ = ::socket(domain, sock_type, protocol);
    if (sock_ == kInvalidSocket) {
        return make_error_code(SocketErrc::CreateFailed);
    }

    state_ = State::Created;
    return make_error_code(SocketErrc::Success);
}

std::error_code Socket::bind(const SocketAddress& addr) {
    if (!is_valid()) {
        return make_error_code(SocketErrc::NotInitialized);
    }

    if (::bind(sock_, addr.data(), addr.size()) == kSocketError) {
        return make_error_code(SocketErrc::BindFailed);
    }

    state_ = State::Bound;
    return make_error_code(SocketErrc::Success);
}

std::error_code Socket::listen(int backlog) {
    if (!is_valid()) {
        return make_error_code(SocketErrc::NotInitialized);
    }

    if (::listen(sock_, backlog) == kSocketError) {
        return make_error_code(SocketErrc::ListenFailed);
    }

    state_ = State::Listening;
    return make_error_code(SocketErrc::Success);
}

std::pair<Socket, std::error_code> Socket::accept() {
    if (!is_valid() || state_ != State::Listening) {
        return {Socket(), make_error_code(SocketErrc::NotInitialized)};
    }

    struct sockaddr_storage client_addr = {};
    socklen_t addr_len = sizeof(client_addr);

    socket_t client_sock = ::accept(sock_, reinterpret_cast<struct sockaddr*>(&client_addr),
                                     &addr_len);
    if (client_sock == kInvalidSocket) {
        return {Socket(), make_error_code(SocketErrc::AcceptFailed)};
    }

    Socket client_socket;
    client_socket.sock_ = client_sock;
    client_socket.state_ = State::Connected;
    client_socket.type_ = type_;

    return {std::move(client_socket), make_error_code(SocketErrc::Success)};
}

std::error_code Socket::connect(const SocketAddress& addr) {
    if (!is_valid()) {
        return make_error_code(SocketErrc::NotInitialized);
    }

    // On platforms where non-blocking connect + select-with-timeout is the
    // correct pattern, we always do non-blocking then poll. Default timeout
    // is 5s; setting it to 0 falls back to the previous blocking behavior
    // (useful for callers that explicitly want to wait).
    const bool want_timeout = connect_timeout_.count() > 0;
    const bool saved_non_blocking = non_blocking_;
    if (want_timeout && !saved_non_blocking) {
        auto ec = set_non_blocking(true);
        if (ec) return ec;
    }

    int rc = ::connect(sock_, addr.data(), addr.size());
    if (rc == 0) {
        state_ = State::Connected;
        if (want_timeout && !saved_non_blocking) {
            auto ec = set_non_blocking(false);
            if (ec) return ec;
        }
        return make_error_code(SocketErrc::Success);
    }

    int err = get_socket_error();
    bool in_progress =
        (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS);
    if (!in_progress) {
        if (err == WSAECONNREFUSED) {
            return make_error_code(SocketErrc::ConnectionRefused);
        }
        if (err == WSAETIMEDOUT) {
            return make_error_code(SocketErrc::Timeout);
        }
        return make_error_code(SocketErrc::ConnectFailed);
    }

    if (!want_timeout) {
        // Caller asked for infinite wait: restore blocking mode and re-call.
        if (!saved_non_blocking) {
            auto ec = set_non_blocking(false);
            if (ec) return ec;
        }
        // Retry blocking; the second ::connect either succeeds immediately
        // (socket already in connected state) or blocks until kernel decides.
        if (::connect(sock_, addr.data(), addr.size()) == 0) {
            state_ = State::Connected;
            return make_error_code(SocketErrc::Success);
        }
        err = get_socket_error();
        if (err == WSAECONNREFUSED) {
            return make_error_code(SocketErrc::ConnectionRefused);
        }
        if (err == WSAETIMEDOUT) {
            return make_error_code(SocketErrc::Timeout);
        }
        return make_error_code(SocketErrc::ConnectFailed);
    }

    // Poll for completion (writable = connect finished).
    fd_set writefds;
    fd_set exceptfds;
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(sock_, &writefds);
    FD_SET(sock_, &exceptfds);

    timeval tv;
    tv.tv_sec = static_cast<long>(connect_timeout_.count() / 1000);
    tv.tv_usec = static_cast<long>((connect_timeout_.count() % 1000) * 1000);

    int sel = ::select(static_cast<int>(sock_) + 1, nullptr, &writefds,
                        &exceptfds, &tv);

    // Restore blocking mode regardless of outcome.
    if (!saved_non_blocking) {
        auto ec = set_non_blocking(false);
        if (ec) return ec;
    }

    if (sel == 0) {
        return make_error_code(SocketErrc::Timeout);
    }
    if (sel < 0) {
        return make_error_code(SocketErrc::ConnectFailed);
    }

    if (FD_ISSET(sock_, &exceptfds)) {
        // SO_ERROR populated; surface it.
        int so_err = 0;
        socklen_t len = sizeof(so_err);
        if (::getsockopt(sock_, SOL_SOCKET, SO_ERROR,
                         reinterpret_cast<char*>(&so_err), &len) == 0
            && so_err != 0) {
            if (so_err == WSAECONNREFUSED) {
                return make_error_code(SocketErrc::ConnectionRefused);
            }
            if (so_err == WSAETIMEDOUT) {
                return make_error_code(SocketErrc::Timeout);
            }
        }
        return make_error_code(SocketErrc::ConnectFailed);
    }

    // writable + not exceptional => connected.
    state_ = State::Connected;
    return make_error_code(SocketErrc::Success);
}

std::pair<std::size_t, std::error_code> Socket::send(
    std::span<const std::uint8_t> data) {
    if (!is_valid() || state_ != State::Connected) {
        return {0, make_error_code(SocketErrc::NotInitialized)};
    }

    int sent = ::send(sock_, reinterpret_cast<const char*>(data.data()),
                      static_cast<int>(data.size()), 0);
    if (sent == kSocketError) {
        int err = get_socket_error();
        if (err == WSAEWOULDBLOCK) {
            return {0, make_error_code(SocketErrc::WouldBlock)};
        }
        if (err == WSAECONNRESET) {
            return {0, make_error_code(SocketErrc::ConnectionReset)};
        }
        return {0, make_error_code(SocketErrc::SendFailed)};
    }

    return {static_cast<std::size_t>(sent), make_error_code(SocketErrc::Success)};
}

std::pair<std::size_t, std::error_code> Socket::receive(
    std::span<std::uint8_t> buffer) {
    if (!is_valid() || state_ != State::Connected) {
        return {0, make_error_code(SocketErrc::NotInitialized)};
    }

    int received = ::recv(sock_, reinterpret_cast<char*>(buffer.data()),
                          static_cast<int>(buffer.size()), 0);
    if (received == kSocketError) {
        int err = get_socket_error();
        if (err == WSAEWOULDBLOCK) {
            return {0, make_error_code(SocketErrc::WouldBlock)};
        }
        if (err == WSAECONNRESET) {
            return {0, make_error_code(SocketErrc::ConnectionReset)};
        }
        return {0, make_error_code(SocketErrc::ReceiveFailed)};
    }
    if (received == 0) {
        // Connection closed by peer
        return {0, make_error_code(SocketErrc::ConnectionReset)};
    }

    return {static_cast<std::size_t>(received), make_error_code(SocketErrc::Success)};
}

void Socket::close() {
    if (sock_ != kInvalidSocket) {
        close_socket(sock_);
        sock_ = kInvalidSocket;
        state_ = State::Closed;
    }
}

std::error_code Socket::set_non_blocking(bool enable) {
    if (!is_valid()) {
        return make_error_code(SocketErrc::NotInitialized);
    }
    if (!compat::set_non_blocking(sock_, enable)) {
        return make_error_code(SocketErrc::InvalidArgument);
    }
    non_blocking_ = enable;
    return make_error_code(SocketErrc::Success);
}

std::error_code Socket::set_tcp_nodelay(bool enable) {
    if (!is_valid()) {
        return make_error_code(SocketErrc::NotInitialized);
    }
    if (!compat::set_tcp_nodelay(sock_, enable)) {
        return make_error_code(SocketErrc::InvalidArgument);
    }
    return make_error_code(SocketErrc::Success);
}

std::error_code Socket::set_reuse_addr(bool enable) {
    if (!is_valid()) {
        return make_error_code(SocketErrc::NotInitialized);
    }
    if (!compat::set_reuse_addr(sock_, enable)) {
        return make_error_code(SocketErrc::InvalidArgument);
    }
    return make_error_code(SocketErrc::Success);
}

std::error_code Socket::set_send_buffer_size(int size) {
    if (!is_valid()) {
        return make_error_code(SocketErrc::NotInitialized);
    }
    if (setsockopt(sock_, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&size),
                   sizeof(size)) == kSocketError) {
        return make_error_code(SocketErrc::InvalidArgument);
    }
    return make_error_code(SocketErrc::Success);
}

std::error_code Socket::set_recv_buffer_size(int size) {
    if (!is_valid()) {
        return make_error_code(SocketErrc::NotInitialized);
    }
    if (setsockopt(sock_, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&size),
                   sizeof(size)) == kSocketError) {
        return make_error_code(SocketErrc::InvalidArgument);
    }
    return make_error_code(SocketErrc::Success);
}

SocketAddress Socket::local_address() const {
    if (!is_valid()) return {};

    struct sockaddr_storage addr = {};
    socklen_t len = sizeof(addr);
    getsockname(sock_, reinterpret_cast<struct sockaddr*>(&addr), &len);
    return SocketAddress(addr);
}

SocketAddress Socket::remote_address() const {
    if (!is_valid()) return {};

    struct sockaddr_storage addr = {};
    socklen_t len = sizeof(addr);
    getpeername(sock_, reinterpret_cast<struct sockaddr*>(&addr), &len);
    return SocketAddress(addr);
}

// ============================================================================
// Error category implementation
// ============================================================================

std::string SocketErrorCategory::message(int ev) const {
    switch (static_cast<SocketErrc>(ev)) {
        case SocketErrc::Success: return "Success";
        case SocketErrc::NotInitialized: return "Socket not initialized";
        case SocketErrc::CreateFailed: return "Failed to create socket";
        case SocketErrc::BindFailed: return "Failed to bind socket";
        case SocketErrc::ListenFailed: return "Failed to listen";
        case SocketErrc::AcceptFailed: return "Failed to accept connection";
        case SocketErrc::ConnectFailed: return "Failed to connect";
        case SocketErrc::SendFailed: return "Failed to send data";
        case SocketErrc::ReceiveFailed: return "Failed to receive data";
        case SocketErrc::Timeout: return "Operation timed out";
        case SocketErrc::WouldBlock: return "Operation would block";
        case SocketErrc::ConnectionReset: return "Connection reset by peer";
        case SocketErrc::ConnectionRefused: return "Connection refused";
        case SocketErrc::AddressInUse: return "Address already in use";
        case SocketErrc::InvalidArgument: return "Invalid argument";
        default: return "Unknown error";
    }
}

const std::error_category& socket_error_category() noexcept {
    static SocketErrorCategory category;
    return category;
}

std::error_code make_error_code(SocketErrc ec) noexcept {
    return {static_cast<int>(ec), socket_error_category()};
}

} // namespace mxh::net
