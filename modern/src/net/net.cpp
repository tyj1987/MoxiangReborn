// net.cpp - WinSock2-based TCP server/client implementation.
//
// Phase 4: replaces [Server]*/4DyuchiNET_Latest/ and [Lib]BaseNetwork/.
// Uses blocking sockets + thread-per-connection. Sufficient for the
// Moxian server demo; production should switch to IOCP/io_uring.

#include "mxh/net/net.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>

// Include Windows networking headers.
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
using socklen_t = int;
#else
#  include <arpa/inet.h>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <unistd.h>
using SOCKET = int;
#  define INVALID_SOCKET (-1)
#  define SOCKET_ERROR (-1)
#  define closesocket ::close
#endif

namespace mxh::net {

const char* to_string(NetError e) noexcept {
    switch (e) {
        case NetError::Ok: return "Ok";
        case NetError::NotStarted: return "NotStarted";
        case NetError::AlreadyStarted: return "AlreadyStarted";
        case NetError::BindFailed: return "BindFailed";
        case NetError::ListenFailed: return "ListenFailed";
        case NetError::ConnectFailed: return "ConnectFailed";
        case NetError::AcceptFailed: return "AcceptFailed";
        case NetError::SendFailed: return "SendFailed";
        case NetError::RecvFailed: return "RecvFailed";
        case NetError::Disconnected: return "Disconnected";
        case NetError::Timeout: return "Timeout";
        case NetError::BufferTooSmall: return "BufferTooSmall";
        case NetError::EncryptionFailed: return "EncryptionFailed";
        case NetError::DecryptionFailed: return "DecryptionFailed";
        default: return "Unknown";
    }
}

namespace {

class WsaGuard {
public:
    WsaGuard() {
#ifdef _WIN32
        WSADATA d;
        WSAStartup(MAKEWORD(2, 2), &d);
#endif
    }
    ~WsaGuard() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

WsaGuard& wsa_guard() {
    static WsaGuard g;
    return g;
}

}  // namespace

// ============================================================================
// Connection state
// ============================================================================
struct Connection {
    SOCKET sock = INVALID_SOCKET;
    std::string remote_addr;
    std::uint64_t id = 0;
    std::atomic<bool> active{true};
    IEncryptor* encryptor = nullptr;
};

// ============================================================================
// TcpServer::Impl
// ============================================================================
struct TcpServer::Impl {
    ServerConfig cfg;
    SOCKET listen_sock = INVALID_SOCKET;
    std::thread accept_thread;
    std::vector<std::thread> worker_threads;
    std::unordered_map<std::uint64_t, std::unique_ptr<Connection>> connections;
    std::mutex connections_mu;
    std::atomic<std::uint64_t> next_id{1};
    std::atomic<bool> stopping{false};

    void close_connection_locked(std::uint64_t id) {
        auto it = connections.find(id);
        if (it == connections.end()) return;
        if (it->second->sock != INVALID_SOCKET) {
            closesocket(it->second->sock);
            it->second->sock = INVALID_SOCKET;
        }
        connections.erase(it);
    }
};

// ============================================================================
// TcpServer
// ============================================================================
TcpServer::TcpServer(IConnectionHandler& handler) : handler_(handler) {
    wsa_guard();
    impl_ = std::make_unique<Impl>();
}

TcpServer::~TcpServer() {
    stop();
}

NetError TcpServer::start(const ServerConfig& cfg) {
    if (running_.load()) return NetError::AlreadyStarted;
    impl_->cfg = cfg;

    impl_->listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (impl_->listen_sock == INVALID_SOCKET) return NetError::ListenFailed;

    int yes = 1;
    setsockopt(impl_->listen_sock, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&yes), sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg.port);
    if (inet_pton(AF_INET, cfg.bind_address.c_str(), &addr.sin_addr) != 1) {
        closesocket(impl_->listen_sock);
        return NetError::BindFailed;
    }

    if (bind(impl_->listen_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))
        == SOCKET_ERROR) {
        closesocket(impl_->listen_sock);
        return NetError::BindFailed;
    }

    if (listen(impl_->listen_sock, 128) == SOCKET_ERROR) {
        closesocket(impl_->listen_sock);
        return NetError::ListenFailed;
    }

    running_.store(true);
    impl_->stopping.store(false);

    impl_->accept_thread = std::thread([this]() {
        while (!impl_->stopping.load()) {
            sockaddr_in client_addr{};
            socklen_t len = sizeof(client_addr);
            SOCKET client = accept(impl_->listen_sock,
                                    reinterpret_cast<sockaddr*>(&client_addr), &len);
            if (client == INVALID_SOCKET) {
                if (impl_->stopping.load()) break;
                continue;
            }

            char ip_str[64] = {};
            inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
            std::string remote = ip_str;
            remote += ":" + std::to_string(ntohs(client_addr.sin_port));

            std::uint64_t id = impl_->next_id.fetch_add(1);
            auto conn = std::make_unique<Connection>();
            conn->sock = client;
            conn->id = id;
            conn->remote_addr = remote;

            ConnectionId cid{id};
            if (!handler_.on_connect(cid, remote)) {
                closesocket(client);
                continue;
            }
            {
                std::lock_guard<std::mutex> lk(impl_->connections_mu);
                impl_->connections[id] = std::move(conn);
            }

            // Spawn worker for this connection.
            if ((std::size_t)impl_->worker_threads.size() <
                (std::size_t)impl_->cfg.worker_thread_count) {
                impl_->worker_threads.emplace_back([this, id]() {
                    // Each worker thread handles multiple connections
                    // (round-robin in production; for demo we use 1:1).
                });
            }

            // Handle this connection synchronously on a new thread (simple model).
            std::thread([this, id, remote]() {
                Connection* c = nullptr;
                {
                    std::lock_guard<std::mutex> lk(impl_->connections_mu);
                    auto it = impl_->connections.find(id);
                    if (it == impl_->connections.end()) return;
                    c = it->second.get();
                }
                c->encryptor = handler_.encryptor_for(ConnectionId{id});

                std::vector<std::uint8_t> buffer(impl_->cfg.recv_buffer_size);
                std::vector<std::uint8_t> carryover;

                while (!impl_->stopping.load() && c->active.load()) {
                    int n = recv(c->sock, reinterpret_cast<char*>(buffer.data()),
                                 static_cast<int>(buffer.size()), 0);
                    if (n <= 0) {
                        handler_.on_disconnect(ConnectionId{id}, NetError::Disconnected);
                        break;
                    }

                    // Append to carryover.
                    carryover.insert(carryover.end(), buffer.begin(),
                                     buffer.begin() + n);

                    // Parse messages: each message starts with MsgRoot (4 bytes)
                    // followed by optional length-prefixed payload.
                    while (carryover.size() >= sizeof(MsgRoot)) {
                        MsgRoot root{};
                        std::memcpy(&root, carryover.data(), sizeof(root));

                        // Wait for full MsgHeader.
                        if (carryover.size() < sizeof(MsgHeader)) break;

                        MsgHeader h{};
                        std::memcpy(&h, carryover.data(), sizeof(h));

                        // No length field in original protocol. For demo,
                        // we consume the rest of the buffer as one message
                        // (production should use PackedData or per-message
                        // length framing).
                        Message msg;
                        msg.header = h;
                        msg.payload.assign(carryover.begin() + sizeof(MsgHeader),
                                           carryover.end());

                        // Optional decryption.
                        if (c->encryptor) c->encryptor->decrypt(msg.payload);

                        handler_.on_message(ConnectionId{id}, msg);

                        // Consume entire carryover for this message.
                        carryover.clear();
                        break;  // wait for next recv cycle for more data
                    }
                }

                // Cleanup.
                std::lock_guard<std::mutex> lk(impl_->connections_mu);
                impl_->close_connection_locked(id);
            }).detach();
        }
    });

    return NetError::Ok;
}

void TcpServer::stop() {
    if (!running_.load()) return;
    impl_->stopping.store(true);

    if (impl_->listen_sock != INVALID_SOCKET) {
        closesocket(impl_->listen_sock);
        impl_->listen_sock = INVALID_SOCKET;
    }

    {
        std::lock_guard<std::mutex> lk(impl_->connections_mu);
        for (auto& [id, conn] : impl_->connections) {
            if (conn->sock != INVALID_SOCKET) closesocket(conn->sock);
        }
    }

    if (impl_->accept_thread.joinable()) impl_->accept_thread.join();
    for (auto& t : impl_->worker_threads) {
        if (t.joinable()) t.join();
    }
    impl_->worker_threads.clear();
    running_.store(false);
}

NetError TcpServer::send(ConnectionId id, const Message& msg) {
    Connection* c = nullptr;
    {
        std::lock_guard<std::mutex> lk(impl_->connections_mu);
        auto it = impl_->connections.find(id.value);
        if (it == impl_->connections.end()) return NetError::Disconnected;
        c = it->second.get();
    }
    if (c->sock == INVALID_SOCKET) return NetError::Disconnected;

    std::vector<std::uint8_t> out(msg.total_size());
    std::memcpy(out.data(), &msg.header, sizeof(msg.header));
    if (!msg.payload.empty()) {
        std::memcpy(out.data() + sizeof(msg.header),
                    msg.payload.data(), msg.payload.size());
    }
    if (c->encryptor) c->encryptor->encrypt(out);

    int total = static_cast<int>(out.size());
    int sent = 0;
    while (sent < total) {
        int n = ::send(c->sock, reinterpret_cast<const char*>(out.data() + sent),
                       total - sent, 0);
        if (n <= 0) return NetError::SendFailed;
        sent += n;
    }
    return NetError::Ok;
}

NetError TcpServer::broadcast(const Message& msg) {
    std::vector<ConnectionId> ids;
    {
        std::lock_guard<std::mutex> lk(impl_->connections_mu);
        ids.reserve(impl_->connections.size());
        for (auto& [id, _] : impl_->connections) ids.push_back(ConnectionId{id});
    }
    NetError last = NetError::Ok;
    for (auto id : ids) {
        NetError e = send(id, msg);
        if (e != NetError::Ok) last = e;
    }
    return last;
}

void TcpServer::disconnect(ConnectionId id) {
    std::lock_guard<std::mutex> lk(impl_->connections_mu);
    impl_->close_connection_locked(id.value);
}

std::size_t TcpServer::connection_count() const noexcept {
    std::lock_guard<std::mutex> lk(impl_->connections_mu);
    return impl_->connections.size();
}

// ============================================================================
// TcpClient
// ============================================================================
struct TcpClient::Impl {
    SOCKET sock = INVALID_SOCKET;
    std::atomic<bool> connected{false};
    std::uint64_t id = 0;
    Connection* conn = nullptr;
    TcpServer* server_proxy = nullptr;  // unused for real clients
};

TcpClient::TcpClient(IConnectionHandler& handler) : handler_(handler) {
    wsa_guard();
    impl_ = std::make_unique<Impl>();
}

TcpClient::~TcpClient() {
    disconnect();
}

NetError TcpClient::connect(const ClientConfig& cfg) {
    impl_->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (impl_->sock == INVALID_SOCKET) return NetError::ConnectFailed;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg.port);
    if (inet_pton(AF_INET, cfg.remote_address.c_str(), &addr.sin_addr) != 1) {
        closesocket(impl_->sock);
        impl_->sock = INVALID_SOCKET;
        return NetError::ConnectFailed;
    }

    if (::connect(impl_->sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))
        == SOCKET_ERROR) {
        closesocket(impl_->sock);
        impl_->sock = INVALID_SOCKET;
        return NetError::ConnectFailed;
    }
    impl_->connected.store(true);
    return NetError::Ok;
}

void TcpClient::disconnect() {
    if (impl_->sock != INVALID_SOCKET) {
        closesocket(impl_->sock);
        impl_->sock = INVALID_SOCKET;
    }
    impl_->connected.store(false);
}

bool TcpClient::is_connected() const noexcept { return impl_->connected.load(); }

NetError TcpClient::send(const Message& msg) {
    if (impl_->sock == INVALID_SOCKET) return NetError::Disconnected;
    std::vector<std::uint8_t> out(msg.total_size());
    std::memcpy(out.data(), &msg.header, sizeof(msg.header));
    if (!msg.payload.empty()) {
        std::memcpy(out.data() + sizeof(msg.header),
                    msg.payload.data(), msg.payload.size());
    }
    int n = ::send(impl_->sock, reinterpret_cast<const char*>(out.data()),
                   static_cast<int>(out.size()), 0);
    return n > 0 ? NetError::Ok : NetError::SendFailed;
}

ConnectionId TcpClient::id() const noexcept { return ConnectionId{impl_->id}; }

}  // namespace mxh::net