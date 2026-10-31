#pragma once

#include "NetworkEventQueue.hpp"

#include "mxh/crypto/hsel_encryptor.hpp"
#include "mxh/client/session_retry.hpp"
#include "mxh/net/net.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

namespace mxh::client {

// One Agent connection survives CharSelect, CharMake, GameLoading and GameIn.
// Map traffic is sent through AgentServer and returned on this same session.
class AgentSession final : public mxh::net::IConnectionHandler {
public:
    AgentSession();
    ~AgentSession() override;

    AgentSession(const AgentSession&) = delete;
    AgentSession& operator=(const AgentSession&) = delete;

    [[nodiscard]] mxh::net::NetError connect(
        const std::string& host, std::uint16_t port, bool use_hsel = false);
    void disconnect();
    [[nodiscard]] mxh::net::NetError send(const mxh::net::Message& message);

    // Login and agent handshakes share one connect budget. A refusal inside
    // this window is retried by TcpClient; after it, connect returns
    // ConnectFailed and the player logs in again. There is no session resume.
    static constexpr std::chrono::milliseconds kConnectTimeout = kAgentConnectTimeout;

    [[nodiscard]] bool is_connected() const noexcept;
    [[nodiscard]] bool is_ready() const noexcept { return m_ready.load(std::memory_order_acquire); }
    [[nodiscard]] NetworkEventQueue& events() noexcept { return m_events; }

    bool on_connect(mxh::net::ConnectionId id, const std::string& remote_addr) override;
    void on_message(mxh::net::ConnectionId id, const mxh::net::Message& message) override;
    void on_disconnect(mxh::net::ConnectionId id, mxh::net::NetError reason) override;
    mxh::net::IEncryptor* encryptor_for(mxh::net::ConnectionId id) override;

private:
    std::unique_ptr<mxh::net::TcpClient> m_client;
    std::unique_ptr<mxh::crypto::HselStreamCipher> m_hsel;
    NetworkEventQueue m_events;
    std::atomic<bool> m_ready{false};
};

}  // namespace mxh::client
