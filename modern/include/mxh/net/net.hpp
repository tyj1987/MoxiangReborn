// net.hpp - Modern TCP networking abstraction for Moxian servers.
//
// Replaces [Server]*/4DyuchiNET_Latest/ (Win32 Overlapped I/O) and
// [Lib]BaseNetwork/ (COM wrapper) with a portable, modern C++ interface.
//
// Backend: WinSock2 + std::thread (overlapped I/O) for production.
// Future: IOCP / io_uring for higher concurrency.
//
// Design goals:
//   - Stable interface that survives backend swaps
//   - Compatible with MSGROOT/MSGBASE message framing from [CC]Header/Protocol.h
//   - Support both server (StartServer) and client (ConnectTo) semantics
//   - Optional HSEL-style encryption (Phase 3 integration)

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace mxh::net {

// MSGROOT/MSGBASE-compatible message header (packed, matches
// [CC]Header/CommonStruct.h). Original is 8 bytes:
//   [checksum: u8] [code: i8] [category: u8] [protocol: u8] [object_id: u32]
// Note: original protocol has NO length field — payload boundary is
// determined by application protocol (e.g. PackedData wraps sub-messages
// with their own length prefix).
#pragma pack(push, 1)
struct MsgHeader {
    std::uint8_t  checksum;  // 1
    std::int8_t   code;      // 1
    std::uint8_t  category;  // 1 (MP_CATEGORY)
    std::uint8_t  protocol;  // 1 (sub-protocol)
    std::uint32_t object_id; // 4 (target entity ID)
};
static_assert(sizeof(MsgHeader) == 8, "MsgHeader must be 8 bytes (1:1 with MSGBASE)");

struct MsgRoot {
    std::uint8_t  checksum;
    std::int8_t   code;
    std::uint8_t  category;
    std::uint8_t  protocol;
};
static_assert(sizeof(MsgRoot) == 4, "MsgRoot must be 4 bytes (1:1 with MSGROOT)");
#pragma pack(pop)

// Full message (header + payload).
struct Message {
    MsgHeader header{};
    std::vector<std::uint8_t> payload;

    [[nodiscard]] std::size_t total_size() const noexcept {
        return sizeof(MsgHeader) + payload.size();
    }
};

// Connection identifier (opaque to user code).
struct ConnectionId {
    std::uint64_t value = 0;
    [[nodiscard]] bool valid() const noexcept { return value != 0; }
    [[nodiscard]] explicit operator bool() const noexcept { return valid(); }
    bool operator==(const ConnectionId& o) const noexcept { return value == o.value; }
    bool operator<(const ConnectionId& o) const noexcept { return value < o.value; }
};

inline ConnectionId make_connection_id(std::uint64_t v) { return {v}; }
constexpr ConnectionId kInvalidConnectionId{};

// Network error categories.
enum class NetError {
    Ok = 0,
    NotStarted,
    AlreadyStarted,
    BindFailed,
    ListenFailed,
    ConnectFailed,
    AcceptFailed,
    SendFailed,
    RecvFailed,
    Disconnected,
    Timeout,
    BufferTooSmall,
    EncryptionFailed,
    DecryptionFailed,
    Unknown,
};

[[nodiscard]] const char* to_string(NetError e) noexcept;

// Optional encryption hook. Phase 3 will plug in AES-256-GCM.
// Interface signature preserved for backward compat with HSEL.
class IEncryptor {
public:
    virtual ~IEncryptor() = default;
    // Encrypt a buffer in place (or to output buffer).
    virtual NetError encrypt(std::span<std::uint8_t> data) = 0;
    virtual NetError decrypt(std::span<std::uint8_t> data) = 0;
    // Seed/handshake. Original HSEL creates random keys per session.
    virtual void seed() = 0;
};

// Connection-level events. User code implements these to react to traffic.
class IConnectionHandler {
public:
    virtual ~IConnectionHandler() = default;

    // Called when a new client connects. Return true to accept.
    virtual bool on_connect(ConnectionId id, const std::string& remote_addr) {
        (void)id;
        (void)remote_addr;
        return true;
    }

    // Called when a complete message has been received (decrypted if applicable).
    virtual void on_message(ConnectionId id, const Message& msg) = 0;

    // Called when a connection is closed (remote or local).
    virtual void on_disconnect(ConnectionId id, NetError reason) {
        (void)id;
        (void)reason;
    }

    // Optional: pre-encryption/decryption (Phase 3 integration).
    virtual IEncryptor* encryptor_for(ConnectionId) { return nullptr; }
};

// TCP server configuration.
struct ServerConfig {
    std::string bind_address = "0.0.0.0";
    std::uint16_t port = 6001;
    int worker_thread_count = 4;
    int max_connections = 4096;
    int recv_buffer_size = 65536;     // per-connection
    int send_buffer_size = 65536;
    bool use_encryption = false;
    bool use_legacy_framing = false;  // Phase 7.6: 4DyuchiNET 2-byte length prefix
    std::chrono::milliseconds idle_timeout{120000};
};

// TCP client configuration.
struct ClientConfig {
    std::string remote_address;
    std::uint16_t port = 0;
    std::chrono::milliseconds connect_timeout{5000};
    bool use_encryption = false;
    bool use_legacy_framing = false;  // Phase 7.6: 4DyuchiNET compatibility
};

// Asynchronous TCP server.
class TcpServer {
public:
    explicit TcpServer(IConnectionHandler& handler);
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    [[nodiscard]] NetError start(const ServerConfig& cfg);
    void stop();
    [[nodiscard]] bool is_running() const noexcept { return running_.load(); }

    // Send a message to a specific connection.
    [[nodiscard]] NetError send(ConnectionId id, const Message& msg);

    // Broadcast to all connections.
    [[nodiscard]] NetError broadcast(const Message& msg);

    // Disconnect a specific connection.
    void disconnect(ConnectionId id);

    [[nodiscard]] std::size_t connection_count() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    IConnectionHandler& handler_;
    std::atomic<bool> running_{false};
};

// Minimal send-only interface for outbound message delivery. Production
// code uses TcpClient; tests can subclass ITcpSender to capture / verify
// outgoing messages without spinning up a real socket.
//
// Phase 12.1 P2-13: this interface was added so AgentHandler can hold an
// ITcpSender* (instead of a concrete TcpClient*) when forwarding GameInSyn
// / GameOutSyn to MapServer. The concrete TcpClient below implements
// ITcpSender, and unit tests use a MockTcpSender to verify "did the
// agent actually send the GameOutSyn?" without needing a real map server.
class ITcpSender {
public:
    virtual ~ITcpSender() = default;
    [[nodiscard]] virtual NetError send(const Message& msg) = 0;
    [[nodiscard]] virtual bool is_connected() const noexcept = 0;
};

// Asynchronous TCP client.
class TcpClient : public ITcpSender {
public:
    explicit TcpClient(IConnectionHandler& handler);
    ~TcpClient() override;

    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    [[nodiscard]] NetError connect(const ClientConfig& cfg);
    void disconnect();
    [[nodiscard]] bool is_connected() const noexcept override;

    [[nodiscard]] NetError send(const Message& msg) override;

    // Set the connection ID after connect completes (used for callbacks).
    [[nodiscard]] ConnectionId id() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    IConnectionHandler& handler_;
};

}  // namespace mxh::net