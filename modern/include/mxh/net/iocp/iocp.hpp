// iocp.hpp - High-performance IOCP server for Moxian-Reborn.
//
// This module provides a modern IOCP-based network server with:
//   - Windows IOCP for scalable async I/O
//   - Thread pool for worker threads
//   - Zero-copy buffer management
//   - Connection pooling and recycling
//   - Message framing and protocol handling
//
// Design:
//   - Proactor pattern (async operations with completion handlers)
//   - Scalable to thousands of concurrent connections
//   - Low-latency message processing
//   - Memory-efficient buffer management
//
// Usage:
//   IocpServer server;
//   server.start(address, port, worker_threads);
//   server.set_message_handler([](ConnectionId id, const Message& msg) {
//       // Handle incoming message
//   });
//   server.send(connection_id, data);
//   server.stop();

#pragma once

#include "mxh/compat/platform.hpp"
#include "mxh/net/socket.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <span>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>

#ifdef _WIN32
// AcceptEx / GetAcceptExSockaddrs come from mswsock.h, and the
// LPFN_ACCEPTEX function-pointer type lives there too. Must come
// AFTER <winsock2.h> (transitively included by platform.hpp).
#include <mswsock.h>
#endif
#include <vector>

namespace mxh::net::iocp {

// ============================================================================
// Forward declarations
// ============================================================================
class IocpConnection;
class IocpServer;

// ============================================================================
// Types and constants
// ============================================================================

// Connection ID type
using ConnectionId = std::uint64_t;

// Buffer size constants
constexpr std::size_t kDefaultBufferSize = 8192;
constexpr std::size_t kMaxBufferSize = 65536;
constexpr std::size_t kMaxConnections = 10000;

// IOCP operation types
enum class IocpOperation {
    Accept,
    Recv,
    Send,
    Disconnect,
    Custom
};

// Overlapped structure for IOCP
struct OverlappedEx {
    OVERLAPPED overlapped = {};
    IocpOperation operation = IocpOperation::Recv;
    WSABUF wsa_buf = {};
    char buffer[kDefaultBufferSize] = {};
    ConnectionId connection_id = 0;
    
    void reset() {
        memset(&overlapped, 0, sizeof(overlapped));
        wsa_buf.buf = buffer;
        wsa_buf.len = sizeof(buffer);
    }
};

// ============================================================================
// Connection class
// ============================================================================

class IocpConnection {
public:
    IocpConnection(socket_t socket, ConnectionId id);
    ~IocpConnection();
    
    // Disable copy
    IocpConnection(const IocpConnection&) = delete;
    IocpConnection& operator=(const IocpConnection&) = delete;
    
    // Start async operations
    bool start_recv();
    bool send(const void* data, std::size_t size);
    
    // Getters
    ConnectionId id() const { return id_; }
    socket_t socket() const { return socket_; }
    bool is_connected() const { return connected_; }
    
    // Address info
    void set_remote_address(const SocketAddress& addr) { remote_addr_ = addr; }
    SocketAddress remote_address() const { return remote_addr_; }
    
    // Buffer management
    OverlappedEx* get_recv_overlapped() { return &recv_overlapped_; }
    OverlappedEx* get_send_overlapped() { return &send_overlapped_; }

    // Process the next pending send. Called by IocpServer after an
    // async send completes; moved to public so the friend-less server
    // class can call it from handle_send() (Phase 10.11 fix).
    void process_send_queue();

    // Connection state
    void set_connected(bool connected) { connected_ = connected; }
    void close();
    
private:
    socket_t socket_;
    ConnectionId id_;
    std::atomic<bool> connected_{false};
    SocketAddress remote_addr_;
    
    OverlappedEx recv_overlapped_;
    OverlappedEx send_overlapped_;
    
    // Send queue for pending sends
    std::mutex send_mutex_;
    std::queue<std::vector<std::uint8_t>> send_queue_;
    std::atomic<bool> sending_{false};
};

// ============================================================================
// Message handler callback types
// ============================================================================

using ConnectHandler = std::function<void(ConnectionId id, const SocketAddress& addr)>;
using DisconnectHandler = std::function<void(ConnectionId id)>;
using MessageHandler = std::function<void(ConnectionId id, std::span<const std::uint8_t> data)>;
using ErrorHandler = std::function<void(ConnectionId id, const std::error_code& ec)>;

// ============================================================================
// IOCP Server class
// ============================================================================

class IocpServer {
public:
    IocpServer();
    ~IocpServer();
    
    // Disable copy
    IocpServer(const IocpServer&) = delete;
    IocpServer& operator=(const IocpServer&) = delete;
    
    // Server control
    [[nodiscard]] std::error_code start(const std::string& address, 
                                       std::uint16_t port,
                                       std::size_t worker_threads = 0);
    void stop();
    bool is_running() const { return running_; }
    
    // Send data to connection
    bool send(ConnectionId id, const void* data, std::size_t size);
    bool send(ConnectionId id, std::span<const std::uint8_t> data);
    
    // Broadcast to all connections
    void broadcast(const void* data, std::size_t size);
    void broadcast(std::span<const std::uint8_t> data);
    
    // Disconnect connection
    void disconnect(ConnectionId id);
    
    // Connection management
    std::size_t connection_count() const;
    bool is_connected(ConnectionId id) const;
    
    // Set handlers
    void set_connect_handler(ConnectHandler handler) { connect_handler_ = std::move(handler); }
    void set_disconnect_handler(DisconnectHandler handler) { disconnect_handler_ = std::move(handler); }
    void set_message_handler(MessageHandler handler) { message_handler_ = std::move(handler); }
    void set_error_handler(ErrorHandler handler) { error_handler_ = std::move(handler); }
    
    // Configuration
    void set_max_connections(std::size_t max) { max_connections_ = max; }
    void set_buffer_size(std::size_t size) { buffer_size_ = std::min(size, kMaxBufferSize); }
    
private:
    // IOCP handle
    HANDLE iocp_handle_ = INVALID_HANDLE_VALUE;
    
    // Listen socket
    Socket listen_socket_;
    
    // Worker threads
    std::vector<std::thread> worker_threads_;
    std::size_t worker_thread_count_ = 0;
    
    // Accept thread
    std::thread accept_thread_;
    
    // Connection management
    mutable std::mutex connections_mutex_;
    std::unordered_map<ConnectionId, std::shared_ptr<IocpConnection>> connections_;
    std::atomic<ConnectionId> next_connection_id_{1};
    std::size_t max_connections_ = kMaxConnections;
    std::size_t buffer_size_ = kDefaultBufferSize;
    
    // Server state
    std::atomic<bool> running_{false};
    std::atomic<bool> stopping_{false};
    
    // Handlers
    ConnectHandler connect_handler_;
    DisconnectHandler disconnect_handler_;
    MessageHandler message_handler_;
    ErrorHandler error_handler_;
    
    // AcceptEx function pointer
    LPFN_ACCEPTEX AcceptEx_ = nullptr;
    
    // Worker thread function
    void worker_thread_func();
    
    // Accept thread function
    void accept_thread_func();
    
    // Process IOCP completion
    void process_completion(OverlappedEx* overlapped, DWORD bytes_transferred, ULONG_PTR completion_key);
    
    // Handle accept completion
    void handle_accept(OverlappedEx* overlapped);
    
    // Handle recv completion
    void handle_recv(IocpConnection* connection, OverlappedEx* overlapped, DWORD bytes_transferred);
    
    // Handle send completion
    void handle_send(IocpConnection* connection, OverlappedEx* overlapped, DWORD bytes_transferred);
    
    // Create new connection
    std::shared_ptr<IocpConnection> create_connection(socket_t socket);
    
    // Remove connection
    void remove_connection(ConnectionId id);
    
    // Post recv operation
    bool post_recv(IocpConnection* connection);
    
    // Post accept operation
    bool post_accept();
    
    // Load AcceptEx function
    bool load_accept_ex();
    
    // Get local address
    SocketAddress get_local_address() const;
};

// ============================================================================
// Utility functions
// ============================================================================

// Get number of CPU cores
std::size_t get_cpu_core_count();

// Set socket to non-blocking mode
bool set_socket_nonblocking(socket_t socket);

// Set socket options for server
bool set_socket_server_options(socket_t socket);

// Format error message
std::string format_error(const std::error_code& ec);

} // namespace mxh::net::iocp