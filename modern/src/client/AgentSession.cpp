#include "AgentSession.hpp"

#include "mxh/log/mlog.hpp"
#include "mxh/proto/protocol.hpp"

#include <chrono>
#include <cstring>
#include <utility>

namespace mxh::client {

AgentSession::AgentSession() = default;
AgentSession::~AgentSession() { disconnect(); }

mxh::net::NetError AgentSession::connect(
    const std::string& host, std::uint16_t port, bool use_hsel) {
    if (is_connected()) return mxh::net::NetError::Ok;
    disconnect();
    m_events.clear();
    m_ready.store(false, std::memory_order_release);
    if (use_hsel) m_hsel = std::make_unique<mxh::crypto::HselStreamCipher>();

    m_client = std::make_unique<mxh::net::TcpClient>(*this);
    mxh::net::ClientConfig config;
    config.remote_address = host;
    config.port = port;
    config.use_legacy_framing = true;
    config.use_encryption = use_hsel;
    config.connect_timeout = std::chrono::milliseconds(3000);
    const auto result = m_client->connect(config);
    if (result != mxh::net::NetError::Ok) m_client.reset();
    return result;
}

void AgentSession::disconnect() {
    m_ready.store(false, std::memory_order_release);
    if (m_client) {
        if (m_client->is_connected()) m_client->disconnect();
        m_client.reset();
    }
    m_hsel.reset();
}

mxh::net::NetError AgentSession::send(const mxh::net::Message& message) {
    if (!m_client || !m_client->is_connected()) return mxh::net::NetError::Disconnected;
    return m_client->send(message);
}

bool AgentSession::is_connected() const noexcept {
    return m_client && m_client->is_connected();
}

bool AgentSession::on_connect(mxh::net::ConnectionId id, const std::string& remote_addr) {
    ClientRuntimeEvent event;
    event.kind = ClientRuntimeEventKind::Connected;
    event.connection = id;
    event.detail = remote_addr;
    (void)m_events.push(std::move(event));
    return true;
}

void AgentSession::on_message(
    mxh::net::ConnectionId id, const mxh::net::Message& message) {
    const auto hsel_protocol = static_cast<std::uint8_t>(mxh::proto::kModernHselKey);
    if (message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn)
        && message.header.protocol == hsel_protocol) {
        if (message.payload.size() < sizeof(mxh::crypto::HselInit)) {
            ClientRuntimeEvent event;
            event.kind = ClientRuntimeEventKind::Error;
            event.connection = id;
            event.detail = "HselKey payload too short";
            (void)m_events.push(std::move(event));
            return;
        }
        mxh::crypto::HselInit init{};
        std::memcpy(&init, message.payload.data(), sizeof(init));
        if (!m_hsel || !m_hsel->import_init(init)) {
            ClientRuntimeEvent event;
            event.kind = ClientRuntimeEventKind::Error;
            event.connection = id;
            event.detail = "HselKey import failed";
            (void)m_events.push(std::move(event));
        }
        return;
    }
    if (message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn)
        && message.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::AgentConnectSuccess)) {
        m_ready.store(true, std::memory_order_release);
    }
    ClientRuntimeEvent event;
    event.kind = ClientRuntimeEventKind::Message;
    event.connection = id;
    event.message = message;
    if (!m_events.push(std::move(event))) {
        MLOG_ERROR("AgentSession event queue overflow; packet dropped");
    }
}

void AgentSession::on_disconnect(mxh::net::ConnectionId id, mxh::net::NetError reason) {
    m_ready.store(false, std::memory_order_release);
    ClientRuntimeEvent event;
    event.kind = ClientRuntimeEventKind::Disconnected;
    event.connection = id;
    event.network_error = reason;
    (void)m_events.push(std::move(event));
}

mxh::net::IEncryptor* AgentSession::encryptor_for(mxh::net::ConnectionId) {
    return m_hsel.get();
}

}  // namespace mxh::client
