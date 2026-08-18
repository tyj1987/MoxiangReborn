// net.cpp - WinSock2-based TCP server/client implementation.
//
// Phase 4: replaces [Server]*/4DyuchiNET_Latest/ and [Lib]BaseNetwork/.
// Uses blocking sockets + thread-per-connection. Sufficient for the
// Moxian server demo; production should switch to IOCP/io_uring.

#include "mxh/net/net.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>

// Include Windows networking headers.
#ifdef _WIN32
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
    std::atomic<bool> ready_for_reap{false};
    std::atomic<bool> disconnect_notified{false};
    IEncryptor* encryptor = nullptr;

    // Phase 10e: Per-connection async send queue + dedicated sender thread.
    // Decouples callers (drain_to / handler threads) from blocking I/O.
    // Each connection has its own sender thread so a slow/stuck client
    // cannot block sends to other connections.
    std::mutex              send_mu;
    std::condition_variable send_cv;
    std::vector<std::vector<std::uint8_t>> send_queue;  // serialized frames
    std::thread             sender_thread;
    std::thread             recv_thread;
};

// ============================================================================
// TcpServer::Impl
// ============================================================================
struct TcpServer::Impl {
    ServerConfig cfg;
    SOCKET listen_sock = INVALID_SOCKET;
    std::thread accept_thread;
    std::thread reaper_thread;
    std::vector<std::thread> worker_threads;
    std::unordered_map<std::uint64_t, std::shared_ptr<Connection>> connections;
    std::mutex connections_mu;
    std::atomic<std::uint64_t> next_id{1};
    std::atomic<bool> stopping{false};

    void reap_inactive_connections() {
        std::vector<std::shared_ptr<Connection>> retired;
        {
            std::lock_guard<std::mutex> lk(connections_mu);
            for (auto it = connections.begin(); it != connections.end();) {
                if (it->second->active.load() || !it->second->ready_for_reap.load()) {
                    ++it;
                    continue;
                }
                retired.push_back(std::move(it->second));
                it = connections.erase(it);
            }
        }
        for (auto& conn : retired) {
            conn->send_cv.notify_one();
            if (conn->sender_thread.joinable()) conn->sender_thread.join();
            if (conn->recv_thread.joinable()) conn->recv_thread.join();
        }
    }

    void close_connection_locked(std::uint64_t id) {
        auto it = connections.find(id);
        if (it == connections.end()) return;
        if (it->second->sock != INVALID_SOCKET) {
            closesocket(it->second->sock);
            it->second->sock = INVALID_SOCKET;
        }
        it->second->active.store(false);
        it->second->send_cv.notify_one();
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
            auto conn = std::make_shared<Connection>();
            conn->sock = client;
            conn->id = id;
            conn->remote_addr = remote;
            // Phase 6.4 (M2 step 3): Assign encryptor BEFORE on_connect so that
            // any DistConnect-style reply sent during on_connect is encrypted
            // when use_encryption=true. Previously, encryptor was only set in
            // the per-connection recv thread spawned below, leaving a race
            // window where reply_() during on_connect used null encryptor and
            // went out plaintext while the client expected ciphertext.
            conn->encryptor = handler_.encryptor_for(ConnectionId{id});

            ConnectionId cid{id};
            // Register the connection BEFORE on_connect so replies sent
            // synchronously from the handler (e.g. the HSEL key-delivery
            // handshake) resolve to this connection instead of being
            // dropped with Disconnected.
            {
                std::lock_guard<std::mutex> lk(impl_->connections_mu);
                impl_->connections[id] = std::move(conn);
            }
            if (!handler_.on_connect(cid, remote)) {
                {
                    std::lock_guard<std::mutex> lk(impl_->connections_mu);
                    impl_->connections.erase(id);
                }
                closesocket(client);
                continue;
            }

            // Spawn worker for this connection.
            if ((std::size_t)impl_->worker_threads.size() <
                (std::size_t)impl_->cfg.worker_thread_count) {
                impl_->worker_threads.emplace_back([this, id]() {
                    // Each worker thread handles multiple connections
                    // (round-robin in production; for demo we use 1:1).
                });
            }

            // Phase 10e: Start per-connection sender thread.
            // The sender thread drains the connection's send queue and performs
            // blocking ::send() calls in isolation — a slow client only blocks
            // its own sender thread, never the main loop or other connections.
            {
                std::lock_guard<std::mutex> lk(impl_->connections_mu);
                auto* c_ptr = impl_->connections.at(id).get();
                c_ptr->sender_thread = std::thread([this, c_ptr]() {
                    while (c_ptr->active.load()) {
                        std::vector<std::vector<std::uint8_t>> batch;
                        {
                            std::unique_lock<std::mutex> slk(c_ptr->send_mu);
                            c_ptr->send_cv.wait_for(slk,
                                std::chrono::milliseconds(200),
                                [&] { return !c_ptr->send_queue.empty()
                                          || !c_ptr->active.load(); });
                            if (c_ptr->send_queue.empty()) continue;
                            batch.swap(c_ptr->send_queue);
                        }
                        for (auto& frame : batch) {
                            int total = static_cast<int>(frame.size());
                            int sent = 0;
                            while (sent < total) {
                                int n = ::send(c_ptr->sock,
                                    reinterpret_cast<const char*>(frame.data() + sent),
                                    total - sent, 0);
                                if (n <= 0) {
                                    c_ptr->active.store(false);
                                    goto sender_exit;
                                }
                                sent += n;
                            }
                        }
                    }
                sender_exit:
                    // Close socket (sole owner — recv thread no longer closes).
                    std::lock_guard<std::mutex> slk(c_ptr->send_mu);
                    if (c_ptr->sock != INVALID_SOCKET) {
                        closesocket(c_ptr->sock);
                        c_ptr->sock = INVALID_SOCKET;
                    }
                });
            }

            // Handle this connection's recv on a new thread — track it so stop() can join.
            std::thread t([this, id, remote]() {
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
                        if (!c->disconnect_notified.exchange(true)) {
                            handler_.on_disconnect(ConnectionId{id}, NetError::Disconnected);
                        }
                        break;
                    }

                    // Append to carryover.
                    carryover.insert(carryover.end(), buffer.begin(),
                                     buffer.begin() + n);

                    // Debug: log raw received bytes
                    if (impl_->cfg.use_legacy_framing) {
                        std::cerr << "[net] recv " << n << " bytes, carryover=" << carryover.size() << " hex:";
                        for (int i = 0; i < n && i < 64; ++i)
                            fprintf(stderr, " %02x", buffer[i]);
                        fprintf(stderr, "\n");
                    }

                    // Phase 7.6: 4DyuchiNET legacy framing support.
                    // Original client sends: [2B length WORD LE] [Category:1B] [Protocol:1B] [dwObjectID:4B] [payload]
                    // Original MSGBASE header is 6 bytes (NOT 8 like modern MsgHeader).
                    while (true) {
                        if (impl_->cfg.use_legacy_framing) {
                            // Legacy mode: need at least 2 bytes for length prefix
                            if (carryover.size() < 2) break;
                            
                            // Read 2-byte little-endian length prefix
                            std::uint16_t msg_len = 0;
                            msg_len |= static_cast<std::uint16_t>(carryover[0]);
                            msg_len |= static_cast<std::uint16_t>(carryover[1]) << 8;
                            
                            // Wait for complete message (2B length + msg_len bytes)
                            if (carryover.size() < std::size_t(2 + msg_len)) break;
                            
                            // Extract message body (without length prefix)
                            std::vector<std::uint8_t> msg_body(
                                carryover.begin() + 2,
                                carryover.begin() + 2 + msg_len);
                            
                            // Consume from carryover
                            carryover.erase(carryover.begin(),
                                           carryover.begin() + 2 + msg_len);
                            
                            // Parse legacy 8-byte MSGBASE header (with _CRYPTCHECK_):
                            //   [CheckSum:1B] [Code:1B] [Category:1B] [Protocol:1B] [dwObjectID:4B]
                            constexpr std::size_t LEGACY_HEADER_SIZE = 8;
                            if (msg_body.size() < LEGACY_HEADER_SIZE) {
                                std::cerr << "[net] legacy: message too short ("
                                          << msg_body.size() << " bytes)\n";
                                continue;
                            }

                            // Phase 6.4 (M2 step 3): Decrypt whole msg_body BEFORE
                            // parsing. Previously decryption targeted msg.payload
                            // only, leaving the parsed header fields as
                            // encrypted bytes. Symmetric with TcpServer::send
                            // which encrypts the whole msg_body.
                            if (c->encryptor) c->encryptor->decrypt(msg_body);

                            MsgHeader h{};
                            h.checksum  = msg_body[0];
                            h.code      = msg_body[1];
                            h.category  = msg_body[2];
                            h.protocol  = msg_body[3];
                            std::memcpy(&h.object_id, msg_body.data() + 4, 4);

                            Message msg;
                            msg.header = h;
                            msg.payload.assign(msg_body.begin() + LEGACY_HEADER_SIZE,
                                               msg_body.end());
                            
                            handler_.on_message(ConnectionId{id}, msg);
                        } else {
                            // Modern mode: [8B MsgHeader] [payload]
                            if (carryover.size() < sizeof(MsgRoot)) break;
                            
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
                        }
                        // Continue processing more messages in carryover.
                        // The break statements inside each branch handle
                        // the case where there is not enough data.
                        continue;
                    }
                }

                // Cleanup: mark inactive and wake sender thread.
                // Sender thread is responsible for closing the socket.
                c->active.store(false);
                c->send_cv.notify_one();
                std::cerr << "[net] recv thread conn=" << id << " exiting\n";
            });
            bool orphaned = false;
            {
                std::lock_guard<std::mutex> lk(impl_->connections_mu);
                auto it = impl_->connections.find(id);
                if (it != impl_->connections.end()) {
                    it->second->recv_thread = std::move(t);
                    it->second->ready_for_reap.store(true);
                } else {
                    orphaned = true;
                }
            }
            if (orphaned && t.joinable()) t.join();
        }
    });

    // Join completed connection threads during normal operation. Retaining
    // them until stop() leaks two Windows thread handles per connection.
    impl_->reaper_thread = std::thread([this]() {
        while (!impl_->stopping.load()) {
            impl_->reap_inactive_connections();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    return NetError::Ok;
}

void TcpServer::stop() {
    if (!running_.load()) return;
    impl_->stopping.store(true);

    // Phase 10e: Wake all sender threads so they notice active==false and exit.
    std::vector<ConnectionId> stop_disconnects;
    {
        std::lock_guard<std::mutex> lk(impl_->connections_mu);
        for (auto& [_, conn] : impl_->connections) {
            conn->active.store(false);
            if (!conn->disconnect_notified.exchange(true)) {
                stop_disconnects.push_back(ConnectionId{conn->id});
            }
            conn->send_cv.notify_one();
        }
    }
    for (const auto id : stop_disconnects) {
        handler_.on_disconnect(id, NetError::Disconnected);
    }

    // Close listen socket so accept() unblocks.
    if (impl_->listen_sock != INVALID_SOCKET) {
        closesocket(impl_->listen_sock);
        impl_->listen_sock = INVALID_SOCKET;
    }

    // Close all client sockets so recv() calls unblock immediately.
    {
        std::lock_guard<std::mutex> lk(impl_->connections_mu);
        for (auto& [id, conn] : impl_->connections) {
            if (conn->sock != INVALID_SOCKET) closesocket(conn->sock);
        }
    }

    if (impl_->accept_thread.joinable()) impl_->accept_thread.join();
    if (impl_->reaper_thread.joinable()) impl_->reaper_thread.join();

    // Phase 10e + R-1 fix: move the per-connection threads out and join
    // them WITHOUT holding connections_mu. A handler that is still inside
    // on_message and calling server->send() needs connections_mu to
    // resolve its connection; joining under the lock would deadlock the
    // shutdown (the recv thread waits for the lock stop() holds).
    std::vector<std::thread> sender_handles;
    std::vector<std::thread> recv_handles;
    {
        std::lock_guard<std::mutex> lk(impl_->connections_mu);
        for (auto& [_, conn] : impl_->connections) {
            if (conn->sender_thread.joinable()) {
                sender_handles.push_back(std::move(conn->sender_thread));
            }
            if (conn->recv_thread.joinable()) {
                recv_handles.push_back(std::move(conn->recv_thread));
            }
        }
    }
    for (auto& t : sender_handles) {
        if (t.joinable()) t.join();
    }
    for (auto& t : recv_handles) {
        if (t.joinable()) t.join();
    }
    {
        std::lock_guard<std::mutex> lk(impl_->connections_mu);
        impl_->connections.clear();
    }

    for (auto& t : impl_->worker_threads) {
        if (t.joinable()) t.join();
    }
    impl_->worker_threads.clear();
    running_.store(false);
}

NetError TcpServer::send(ConnectionId id, const Message& msg) {
    std::shared_ptr<Connection> connection;
    {
        std::lock_guard<std::mutex> lk(impl_->connections_mu);
        auto it = impl_->connections.find(id.value);
        if (it == impl_->connections.end()) return NetError::Disconnected;
        connection = it->second;
    }
    Connection* c = connection.get();
    if (!c->active.load()) return NetError::Disconnected;

    // Phase 7.6: Build message body
    std::vector<std::uint8_t> msg_body;
    if (impl_->cfg.use_legacy_framing) {
        // Legacy mode: 8-byte MSGBASE header [CheckSum:1B][Code:1B][Category:1B][Protocol:1B][dwObjectID:4B]
        constexpr std::size_t LEGACY_HDR = 8;
        msg_body.resize(LEGACY_HDR + msg.payload.size());
        msg_body[0] = 0;  // CheckSum = 0
        msg_body[1] = 0;  // Code = 0 (must be 0 when crypt not inited; client checks Code == GetDeCRCConvertChar() which is 0)
        msg_body[2] = msg.header.category;
        msg_body[3] = msg.header.protocol;
        std::memcpy(msg_body.data() + 4, &msg.header.object_id, 4);
        if (!msg.payload.empty()) {
            std::memcpy(msg_body.data() + LEGACY_HDR,
                        msg.payload.data(), msg.payload.size());
        }
    } else {
        // Modern mode: 8-byte MsgHeader
        msg_body.resize(msg.total_size());
        std::memcpy(msg_body.data(), &msg.header, sizeof(msg.header));
        if (!msg.payload.empty()) {
            std::memcpy(msg_body.data() + sizeof(msg.header),
                        msg.payload.data(), msg.payload.size());
        }
    }
    if (c->encryptor) c->encryptor->encrypt(msg_body);

    // Debug: log raw send bytes
    if (impl_->cfg.use_legacy_framing) {
        std::cerr << "[net] send msg_body size=" << msg_body.size() << " hex:";
        for (std::size_t i = 0; i < msg_body.size() && i < 64; ++i)
            fprintf(stderr, " %02x", msg_body[i]);
        fprintf(stderr, "\n");
    }

    // Phase 7.6: Add 2-byte length prefix for legacy framing
    std::vector<std::uint8_t> out;
    if (impl_->cfg.use_legacy_framing) {
        out.resize(2 + msg_body.size());
        std::uint16_t len = static_cast<std::uint16_t>(msg_body.size());
        out[0] = static_cast<std::uint8_t>(len & 0xFF);
        out[1] = static_cast<std::uint8_t>((len >> 8) & 0xFF);
        std::memcpy(out.data() + 2, msg_body.data(), msg_body.size());
        std::cerr << "[net] send wire size=" << out.size() << " hex:";
        for (std::size_t i = 0; i < out.size() && i < 64; ++i)
            fprintf(stderr, " %02x", out[i]);
        fprintf(stderr, "\n");
    } else {
        out = std::move(msg_body);
    }

    // Phase 10e: Enqueue to per-connection send queue.
    // The dedicated sender thread drains this asynchronously.
    // This makes send() O(queue_push) — never blocks the caller.
    {
        std::lock_guard<std::mutex> slk(c->send_mu);
        if (c->sock == INVALID_SOCKET || !c->active.load())
            return NetError::Disconnected;
        c->send_queue.push_back(std::move(out));
    }
    c->send_cv.notify_one();
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
    std::thread recv_thread;
    IEncryptor* encryptor = nullptr;
    bool use_legacy_framing = false;  // Phase 7.6: 4DyuchiNET compatibility
    std::mutex send_mu;  // Serialize send with recv to prevent WSAECONNRESET
};

TcpClient::TcpClient(IConnectionHandler& handler) : handler_(handler) {
    wsa_guard();
    impl_ = std::make_unique<Impl>();
}

TcpClient::~TcpClient() {
    disconnect();
}

NetError TcpClient::connect(const ClientConfig& cfg) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg.port);
    if (inet_pton(AF_INET, cfg.remote_address.c_str(), &addr.sin_addr) != 1) {
        return NetError::ConnectFailed;
    }

    // ClientConfig has always exposed connect_timeout, but the synchronous
    // implementation previously ignored it and failed after one connect().
    // Retry with a fresh socket until the deadline so transient accept-backlog
    // or ephemeral-port pressure does not abort a commercial login flow.
    const auto deadline = std::chrono::steady_clock::now() + cfg.connect_timeout;
    do {
        impl_->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (impl_->sock != INVALID_SOCKET &&
            ::connect(impl_->sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))
                != SOCKET_ERROR) {
            break;
        }
        if (impl_->sock != INVALID_SOCKET) closesocket(impl_->sock);
        impl_->sock = INVALID_SOCKET;
        if (std::chrono::steady_clock::now() >= deadline) {
            return NetError::ConnectFailed;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } while (true);

    if (impl_->sock == INVALID_SOCKET) {
        return NetError::ConnectFailed;
    }
    impl_->connected.store(true);

    // Assign a connection ID and get encryptor from handler.
    impl_->id = 1;  // single-connection client
    impl_->encryptor = handler_.encryptor_for(ConnectionId{impl_->id});
    impl_->use_legacy_framing = cfg.use_legacy_framing;

    // Notify the handler the TCP handshake completed. The server side already
    // calls on_connect in TcpServer::on_accept; the client side was missing
    // this call, so handlers that send their first message in on_connect (e.g.
    // CInGameState sending GameInSyn) had to wait for Process() to retry.
    handler_.on_connect(ConnectionId{impl_->id}, cfg.remote_address);

    // Start receive thread.
    impl_->recv_thread = std::thread([this]() {
        std::vector<std::uint8_t> buffer(65536);
        std::vector<std::uint8_t> carryover;

        while (impl_->connected.load()) {
            int n = recv(impl_->sock, reinterpret_cast<char*>(buffer.data()),
                         static_cast<int>(buffer.size()), 0);
            if (n < 0) {
                int err = WSAGetLastError();
                // WSAEINTR (10004) is transient: Winsock sometimes returns it
                // when the socket close races with the recv (e.g. the previous
                // TcpClient::disconnect() closesocket() while a new connection
                // is starting up, or when the legacy server briefly cancels a
                // blocking call to flush its send buffer). Retry instead of
                // tearing the connection down; real closes come through as
                // n == 0 (graceful) or err == WSAECONNABORTED/WSAECONNRESET.
                if (err == WSAEINTR) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                int so_err = 0;
                socklen_t so_len = sizeof(so_err);
                if (impl_->sock != INVALID_SOCKET) {
                    getsockopt(impl_->sock, SOL_SOCKET, SO_ERROR,
                               reinterpret_cast<char*>(&so_err), &so_len);
                }
                std::cerr << "[net] client recv n=" << n << " err=" << err
                          << " so_error=" << so_err
                          << " carryover=" << carryover.size() << "B\n";
                impl_->connected.store(false);
                handler_.on_disconnect(ConnectionId{impl_->id}, NetError::Disconnected);
                break;
            }
            if (n == 0) {
                // Graceful close (FIN) from peer.
                std::cerr << "[net] client recv n=0 carryover=" << carryover.size() << "B\n";
                impl_->connected.store(false);
                handler_.on_disconnect(ConnectionId{impl_->id}, NetError::Disconnected);
                break;
            }
            carryover.insert(carryover.end(), buffer.begin(), buffer.begin() + n);

            // Phase 7.6: 4DyuchiNET legacy framing support for client.
            while (true) {
                if (impl_->use_legacy_framing) {
                    // Legacy mode: [2B length LE] [8B MSGBASE] [payload]
                    if (carryover.size() < 2) break;

                    // Read 2-byte little-endian length prefix
                    std::uint16_t msg_len = 0;
                    msg_len |= static_cast<std::uint16_t>(carryover[0]);
                    msg_len |= static_cast<std::uint16_t>(carryover[1]) << 8;

                    // Wait for complete message (2B length + msg_len bytes)
                    if (carryover.size() < std::size_t(2 + msg_len)) break;

                    // Extract message body (without length prefix)
                    std::vector<std::uint8_t> msg_body(
                        carryover.begin() + 2,
                        carryover.begin() + 2 + msg_len);

                    // Consume from carryover
                    carryover.erase(carryover.begin(),
                                   carryover.begin() + 2 + msg_len);

                    // Parse MsgHeader from message body
                    if (msg_body.size() < sizeof(MsgHeader)) {
                        std::cerr << "[net] client legacy: message too short ("
                                  << msg_body.size() << " bytes)\n";
                        continue;
                    }

                    // Phase 6.4 (M2 step 3): Decrypt whole msg_body BEFORE
                    // parsing. Symmetric with TcpServer::send / TcpClient::send
                    // which encrypt the whole msg_body.
                    if (impl_->encryptor) impl_->encryptor->decrypt(msg_body);

                    MsgHeader h{};
                    std::memcpy(&h, msg_body.data(), sizeof(h));

                    Message msg;
                    msg.header = h;
                    msg.payload.assign(msg_body.begin() + sizeof(MsgHeader),
                                       msg_body.end());

                    handler_.on_message(ConnectionId{impl_->id}, msg);
                } else {
                    // Modern mode: [8B MsgHeader] [payload]
                    if (carryover.size() < sizeof(MsgRoot)) break;

                    if (carryover.size() < sizeof(MsgHeader)) break;

                    MsgHeader h{};
                    std::memcpy(&h, carryover.data(), sizeof(h));

                    Message msg;
                    msg.header = h;
                    msg.payload.assign(carryover.begin() + sizeof(MsgHeader),
                                       carryover.end());

                    // Optional decryption.
                    if (impl_->encryptor) impl_->encryptor->decrypt(msg.payload);

                    handler_.on_message(ConnectionId{impl_->id}, msg);

                    // Consume entire carryover for this message.
                    carryover.clear();
                }
                // Continue processing more messages in carryover.
                continue;
            }
        }
    });

    return NetError::Ok;
}

void TcpClient::disconnect() {
    impl_->connected.store(false);  // make recv thread exit loop first
    {
        std::lock_guard<std::mutex> lk(impl_->send_mu);
        if (impl_->sock != INVALID_SOCKET) {
            shutdown(impl_->sock, SD_BOTH);
            closesocket(impl_->sock);
            impl_->sock = INVALID_SOCKET;
        }
    }
    // Join after releasing lock to avoid deadlock with send().
    if (impl_->recv_thread.joinable()) impl_->recv_thread.join();
}

bool TcpClient::is_connected() const noexcept { return impl_->connected.load(); }

NetError TcpClient::send(const Message& msg) {
    // Phase 7.6: Build message body first (MsgHeader + payload) — outside lock
    std::vector<std::uint8_t> msg_body(msg.total_size());
    std::memcpy(msg_body.data(), &msg.header, sizeof(msg.header));
    if (!msg.payload.empty()) {
        std::memcpy(msg_body.data() + sizeof(msg.header),
                    msg.payload.data(), msg.payload.size());
    }
    // Phase 4.4: optional encryption (same as TcpServer::send).
    if (impl_->encryptor) {
        NetError enc = impl_->encryptor->encrypt(msg_body);
        if (enc != NetError::Ok) return enc;
    }

    // Phase 7.6: Add 2-byte length prefix for legacy framing
    std::vector<std::uint8_t> out;
    if (impl_->use_legacy_framing) {
        out.resize(2 + msg_body.size());
        // Write 2-byte little-endian length prefix
        std::uint16_t len = static_cast<std::uint16_t>(msg_body.size());
        out[0] = static_cast<std::uint8_t>(len & 0xFF);
        out[1] = static_cast<std::uint8_t>((len >> 8) & 0xFF);
        std::memcpy(out.data() + 2, msg_body.data(), msg_body.size());
    } else {
        out = std::move(msg_body);
    }

    // send_mu protects socket validity check + ::send loop from
    // concurrent disconnect() / recv-thread close.
    std::lock_guard<std::mutex> lk(impl_->send_mu);
    if (impl_->sock == INVALID_SOCKET || !impl_->connected.load())
        return NetError::Disconnected;

    int total = static_cast<int>(out.size());
    int sent = 0;
    while (sent < total) {
        int n = ::send(impl_->sock, reinterpret_cast<const char*>(out.data() + sent),
                       total - sent, 0);
        if (n <= 0) {
            int wsa_err = WSAGetLastError();
            std::cerr << "[net] client send failed at sent=" << sent << "/" << total
                      << " n=" << n << " wsa_err=" << wsa_err << "\n";
            return NetError::SendFailed;
        }
        sent += n;
    }
    return NetError::Ok;
}

ConnectionId TcpClient::id() const noexcept { return ConnectionId{impl_->id}; }

}  // namespace mxh::net
