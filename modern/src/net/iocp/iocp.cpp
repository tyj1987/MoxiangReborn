// iocp.cpp - IOCP server implementation.
//
// This file implements the high-performance IOCP server for Moxian-Reborn.
// Uses Windows IOCP for scalable async I/O with multiple worker threads.

#include "mxh/net/iocp/iocp.hpp"
#include "mxh/monitor/performance_monitor.hpp"

#include <iostream>
#include <stdexcept>

#ifdef MXH_PLATFORM_WINDOWS
#include <MSWSock.h>
#endif

namespace mxh::net::iocp {

// ============================================================================
// IocpConnection implementation
// ============================================================================

IocpConnection::IocpConnection(socket_t socket, ConnectionId id)
    : socket_(socket), id_(id) {
    recv_overlapped_.connection_id = id;
    recv_overlapped_.operation = IocpOperation::Recv;
    recv_overlapped_.reset();
    
    send_overlapped_.connection_id = id;
    send_overlapped_.operation = IocpOperation::Send;
    send_overlapped_.reset();
}

IocpConnection::~IocpConnection() {
    close();
}

bool IocpConnection::start_recv() {
    if (!connected_) return false;
    
    DWORD bytes_received = 0;
    DWORD flags = 0;
    
    recv_overlapped_.reset();
    
    int result = WSARecv(
        socket_,
        &recv_overlapped_.wsa_buf,
        1,
        &bytes_received,
        &flags,
        &recv_overlapped_.overlapped,
        nullptr
    );
    
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            std::cerr << "[IOCP] WSARecv failed: " << error << std::endl;
            return false;
        }
    }
    
    return true;
}

bool IocpConnection::send(const void* data, std::size_t size) {
    if (!connected_) return false;
    
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    // If currently sending, queue the data
    if (sending_) {
        std::vector<std::uint8_t> buffer(
            static_cast<const std::uint8_t*>(data),
            static_cast<const std::uint8_t*>(data) + size
        );
        send_queue_.push(std::move(buffer));
        return true;
    }
    
    // Send directly
    sending_ = true;
    
    send_overlapped_.reset();
    memcpy(send_overlapped_.buffer, data, std::min(size, kDefaultBufferSize));
    send_overlapped_.wsa_buf.len = static_cast<ULONG>(std::min(size, kDefaultBufferSize));
    
    DWORD bytes_sent = 0;
    int result = WSASend(
        socket_,
        &send_overlapped_.wsa_buf,
        1,
        &bytes_sent,
        0,
        &send_overlapped_.overlapped,
        nullptr
    );
    
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            std::cerr << "[IOCP] WSASend failed: " << error << std::endl;
            sending_ = false;
            return false;
        }
    }
    
    return true;
}

void IocpConnection::close() {
    if (socket_ != kInvalidSocket) {
#ifdef MXH_PLATFORM_WINDOWS
        CancelIoEx(reinterpret_cast<HANDLE>(socket_), nullptr);
#endif
        closesocket(socket_);
        socket_ = kInvalidSocket;
    }
    connected_ = false;
    std::lock_guard<std::mutex> lk(send_mutex_);
    while (!send_queue_.empty()) send_queue_.pop();
    sending_ = false;
}

void IocpConnection::process_send_queue() {
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    if (send_queue_.empty()) {
        sending_ = false;
        return;
    }
    
    auto& data = send_queue_.front();
    send_overlapped_.reset();
    memcpy(send_overlapped_.buffer, data.data(), std::min(data.size(), kDefaultBufferSize));
    send_overlapped_.wsa_buf.len = static_cast<ULONG>(std::min(data.size(), kDefaultBufferSize));
    
    DWORD bytes_sent = 0;
    int result = WSASend(
        socket_,
        &send_overlapped_.wsa_buf,
        1,
        &bytes_sent,
        0,
        &send_overlapped_.overlapped,
        nullptr
    );
    
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            std::cerr << "[IOCP] WSASend failed in queue: " << error << std::endl;
            sending_ = false;
            return;
        }
    }
    
    send_queue_.pop();
}

// ============================================================================
// IocpServer implementation
// ============================================================================

IocpServer::IocpServer() = default;

IocpServer::~IocpServer() {
    stop();
}

std::error_code IocpServer::start(const std::string& address, 
                                 std::uint16_t port,
                                 std::size_t worker_threads) {
    if (running_) {
        return std::make_error_code(std::errc::already_connected);
    }
    
#ifdef MXH_PLATFORM_WINDOWS
    // Initialize Winsock
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return std::make_error_code(std::errc::protocol_error);
    }
    
    // Create IOCP handle
    iocp_handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (iocp_handle_ == nullptr) {
        return std::make_error_code(std::errc::resource_unavailable_try_again);
    }
    
    // Create listen socket
    auto error = listen_socket_.create(Socket::Type::TCP);
    if (error) {
        return error;
    }
    
    // Set socket options
    set_socket_server_options(listen_socket_.native_handle());
    
    // Bind to address
    SocketAddress bind_addr(address, port);
    error = listen_socket_.bind(bind_addr);
    if (error) {
        return error;
    }
    
    // Start listening
    error = listen_socket_.listen();
    if (error) {
        return error;
    }
    
    // Load AcceptEx function
    if (!load_accept_ex()) {
        return std::make_error_code(std::errc::function_not_supported);
    }
    
    // Associate listen socket with IOCP
    if (CreateIoCompletionPort(
        reinterpret_cast<HANDLE>(listen_socket_.native_handle()),
        iocp_handle_,
        0,
        0) == nullptr) {
        return std::make_error_code(std::errc::io_error);
    }
    
    // Set running flag
    running_ = true;
    stopping_ = false;
    
    // Determine worker thread count
    if (worker_threads == 0) {
        worker_threads = get_cpu_core_count() * 2;
    }
    worker_thread_count_ = worker_threads;
    
    // Start worker threads
    for (std::size_t i = 0; i < worker_thread_count_; ++i) {
        worker_threads_.emplace_back(&IocpServer::worker_thread_func, this);
    }
    
    // Start accept thread
    accept_thread_ = std::thread(&IocpServer::accept_thread_func, this);
    
    std::cout << "[IOCP] Server started on " << address << ":" << port 
              << " with " << worker_thread_count_ << " worker threads" << std::endl;
    
    return std::error_code();
#else
    // POSIX implementation (epoll/kqueue)
    // TODO: Implement for Linux/macOS
    return std::make_error_code(std::errc::not_supported);
#endif
}

void IocpServer::stop() {
    if (!running_) return;
    
    stopping_ = true;
    
#ifdef MXH_PLATFORM_WINDOWS
    // Post completion to stop worker threads
    for (std::size_t i = 0; i < worker_thread_count_; ++i) {
        PostQueuedCompletionStatus(iocp_handle_, 0, 0, nullptr);
    }
    
    // Wait for worker threads
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
    
    // Wait for accept thread
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
    
    // Close all connections
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.clear();
    }
    
    // Close IOCP handle
    if (iocp_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(iocp_handle_);
        iocp_handle_ = INVALID_HANDLE_VALUE;
    }
    
    // Close listen socket
    listen_socket_.close();
    
    // Cleanup Winsock
    WSACleanup();
#endif
    
    running_ = false;
    std::cout << "[IOCP] Server stopped" << std::endl;
}

bool IocpServer::send(ConnectionId id, const void* data, std::size_t size) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = connections_.find(id);
    if (it == connections_.end()) {
        return false;
    }
    
    return it->second->send(data, size);
}

bool IocpServer::send(ConnectionId id, std::span<const std::uint8_t> data) {
    return send(id, data.data(), data.size());
}

void IocpServer::broadcast(const void* data, std::size_t size) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    for (auto& [id, connection] : connections_) {
        if (connection->is_connected()) {
            connection->send(data, size);
        }
    }
}

void IocpServer::broadcast(std::span<const std::uint8_t> data) {
    broadcast(data.data(), data.size());
}

void IocpServer::disconnect(ConnectionId id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = connections_.find(id);
    if (it != connections_.end()) {
        it->second->close();
        connections_.erase(it);
    }
}

std::size_t IocpServer::connection_count() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_.size();
}

bool IocpServer::is_connected(ConnectionId id) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = connections_.find(id);
    if (it == connections_.end()) {
        return false;
    }
    
    return it->second->is_connected();
}

void IocpServer::worker_thread_func() {
#ifdef MXH_PLATFORM_WINDOWS
    while (running_) {
        DWORD bytes_transferred = 0;
        ULONG_PTR completion_key = 0;
        OVERLAPPED* overlapped = nullptr;
        
        BOOL result = GetQueuedCompletionStatus(
            iocp_handle_,
            &bytes_transferred,
            &completion_key,
            &overlapped,
            INFINITE
        );
        
        if (stopping_) break;
        
        if (overlapped == nullptr) {
            // Shutdown signal
            continue;
        }
        
        OverlappedEx* overlapped_ex = reinterpret_cast<OverlappedEx*>(overlapped);
        
        if (!result || bytes_transferred == 0) {
            // Connection closed or error
            if (overlapped_ex->operation == IocpOperation::Recv ||
                overlapped_ex->operation == IocpOperation::Send) {
                ConnectionId id = overlapped_ex->connection_id;
                remove_connection(id);
                
                if (disconnect_handler_) {
                    disconnect_handler_(id);
                }
            }
            continue;
        }
        
        process_completion(overlapped_ex, bytes_transferred, completion_key);
    }
#endif
}

void IocpServer::accept_thread_func() {
#ifdef MXH_PLATFORM_WINDOWS
    while (running_ && !stopping_) {
        // Accept incoming connection
        sockaddr_in client_addr = {};
        int addr_len = sizeof(client_addr);
        
        SOCKET client_socket = accept(listen_socket_.native_handle(), 
                                     reinterpret_cast<sockaddr*>(&client_addr),
                                     &addr_len);
        
        if (client_socket == INVALID_SOCKET) {
            if (stopping_) break;
            std::cerr << "[IOCP] Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }
        
        // Create new connection
        auto connection = create_connection(client_socket);
        if (!connection) {
            closesocket(client_socket);
            continue;
        }
        
        // Set socket options
        set_socket_server_options(client_socket);
        
        // Associate with IOCP
        if (CreateIoCompletionPort(
            reinterpret_cast<HANDLE>(client_socket),
            iocp_handle_,
            reinterpret_cast<ULONG_PTR>(connection.get()),
            0) == nullptr) {
            std::cerr << "[IOCP] Failed to associate with IOCP: " << GetLastError() << std::endl;
            connection->close();
            continue;
        }
        
        // Set remote address
        // client_addr is a sockaddr_in (16 bytes). SocketAddress takes
        // a sockaddr_storage (128 bytes). Copy via a zero-initialised
        // sockaddr_storage so the unused bytes don't leak stack data.
        sockaddr_storage remote_storage{};
        std::memcpy(&remote_storage, &client_addr, sizeof(client_addr));
        SocketAddress remote_addr(remote_storage);
        connection->set_remote_address(remote_addr);
        connection->set_connected(true);
        
        // Start recv operation
        if (!connection->start_recv()) {
            std::cerr << "[IOCP] Failed to start recv for connection " << connection->id() << std::endl;
            connection->close();
            continue;
        }
        
        // Notify connect handler
        if (connect_handler_) {
            connect_handler_(connection->id(), remote_addr);
        }
        
        // Record connection in performance monitor
        auto& monitor = mxh::monitor::PerformanceMonitor::get_instance();
        monitor.record_connection();
        
        std::cout << "[IOCP] New connection: " << connection->id() 
                  << " from " << remote_addr.to_string() << std::endl;
    }
#endif
}

void IocpServer::process_completion(OverlappedEx* overlapped, 
                                   DWORD bytes_transferred, 
                                   ULONG_PTR completion_key) {
    switch (overlapped->operation) {
        case IocpOperation::Recv: {
            IocpConnection* connection = reinterpret_cast<IocpConnection*>(completion_key);
            handle_recv(connection, overlapped, bytes_transferred);
            break;
        }
        case IocpOperation::Send: {
            IocpConnection* connection = reinterpret_cast<IocpConnection*>(completion_key);
            handle_send(connection, overlapped, bytes_transferred);
            break;
        }
        default:
            std::cerr << "[IOCP] Unknown operation: " 
                      << static_cast<int>(overlapped->operation) << std::endl;
            break;
    }
}

void IocpServer::handle_accept(OverlappedEx* overlapped) {
    // AcceptEx completion handling
    // TODO: Implement AcceptEx for better performance
}

void IocpServer::handle_recv(IocpConnection* connection, 
                            OverlappedEx* overlapped, 
                            DWORD bytes_transferred) {
    if (bytes_transferred == 0) {
        // Connection closed
        ConnectionId id = connection->id();
        remove_connection(id);
        
        if (disconnect_handler_) {
            disconnect_handler_(id);
        }
        return;
    }
    
    // Process received data
    if (message_handler_) {
        // Record message in performance monitor
        auto& monitor = mxh::monitor::PerformanceMonitor::get_instance();
        monitor.record_message_received(bytes_transferred);
        
        message_handler_(connection->id(), 
                        std::span<const std::uint8_t>(
                            reinterpret_cast<const std::uint8_t*>(overlapped->buffer),
                            bytes_transferred));
    }
    
    // Continue receiving
    if (!connection->start_recv()) {
        ConnectionId id = connection->id();
        remove_connection(id);
        
        if (disconnect_handler_) {
            disconnect_handler_(id);
        }
    }
}

void IocpServer::handle_send(IocpConnection* connection,
                            OverlappedEx* overlapped,
                            DWORD bytes_transferred) {
    // Record message in performance monitor
    auto& monitor = mxh::monitor::PerformanceMonitor::get_instance();
    monitor.record_message_sent(bytes_transferred);
    
    // Send completed, process next in queue
    connection->process_send_queue();
}

std::shared_ptr<IocpConnection> IocpServer::create_connection(socket_t socket) {
    ConnectionId id = next_connection_id_.fetch_add(1);
    
    if (connections_.size() >= max_connections_) {
        std::cerr << "[IOCP] Max connections reached: " << max_connections_ << std::endl;
        return nullptr;
    }
    
    auto connection = std::make_shared<IocpConnection>(socket, id);
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[id] = connection;
    }
    
    return connection;
}

void IocpServer::remove_connection(ConnectionId id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = connections_.find(id);
    if (it != connections_.end()) {
        it->second->close();
        connections_.erase(it);
        
        // Record disconnection in performance monitor
        auto& monitor = mxh::monitor::PerformanceMonitor::get_instance();
        monitor.record_disconnection();
    }
}

bool IocpServer::post_recv(IocpConnection* connection) {
    return connection->start_recv();
}

bool IocpServer::post_accept() {
    // Post accept operation using AcceptEx
    // TODO: Implement AcceptEx for better performance
    return true;
}

bool IocpServer::load_accept_ex() {
#ifdef MXH_PLATFORM_WINDOWS
    GUID guid_accept_ex = WSAID_ACCEPTEX;
    DWORD bytes_returned = 0;
    
    if (WSAIoctl(
        listen_socket_.native_handle(),
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guid_accept_ex,
        sizeof(guid_accept_ex),
        &AcceptEx_,
        sizeof(AcceptEx_),
        &bytes_returned,
        nullptr,
        nullptr) == SOCKET_ERROR) {
        std::cerr << "[IOCP] Failed to load AcceptEx: " << WSAGetLastError() << std::endl;
        return false;
    }
    
    return true;
#else
    return false;
#endif
}

SocketAddress IocpServer::get_local_address() const {
    return listen_socket_.local_address();
}

// ============================================================================
// Utility functions
// ============================================================================

std::size_t get_cpu_core_count() {
    return std::thread::hardware_concurrency();
}

bool set_socket_nonblocking(socket_t socket) {
#ifdef MXH_PLATFORM_WINDOWS
    u_long mode = 1;
    return ioctlsocket(socket, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

bool set_socket_server_options(socket_t socket) {
    // Enable TCP_NODELAY
    BOOL no_delay = TRUE;
    if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, 
                  reinterpret_cast<const char*>(&no_delay), sizeof(no_delay)) == SOCKET_ERROR) {
        return false;
    }
    
    // Enable SO_REUSEADDR
    BOOL reuse_addr = TRUE;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,
                  reinterpret_cast<const char*>(&reuse_addr), sizeof(reuse_addr)) == SOCKET_ERROR) {
        return false;
    }
    
    // Set buffer sizes
    int recv_buf_size = 65536;
    int send_buf_size = 65536;
    setsockopt(socket, SOL_SOCKET, SO_RCVBUF, 
              reinterpret_cast<const char*>(&recv_buf_size), sizeof(recv_buf_size));
    setsockopt(socket, SOL_SOCKET, SO_SNDBUF,
              reinterpret_cast<const char*>(&send_buf_size), sizeof(send_buf_size));
    
    return true;
}

std::string format_error(const std::error_code& ec) {
    return ec.message() + " (" + std::to_string(ec.value()) + ")";
}

} // namespace mxh::net::iocp