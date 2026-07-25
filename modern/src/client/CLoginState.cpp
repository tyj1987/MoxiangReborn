// mxh/client/CLoginState.cpp
// Phase B.2.1 — login state implementation.
//
// State machine (driven by TcpClient callbacks + per-frame Process):
//   1. Start()  — host calls after SetGameState(Connect); kicks off
//                 the TcpClient connection to LoginServer (legacy mode).
//   2. on_connect() — TcpClient has connected; we don't do anything
//                     special because the server pushes DistConnectSuccess
//                     immediately after TCP accept.
//   3. on_message()
//        - cat=UserConn, proto=0 (DistConnectSuccess): stash auth_key
//          from MSGBASE.object_id, then send RequestLogin (38B legacy
//          payload).
//        - cat=UserConn, proto=2 (NotifyUserLoginAck): parse the 23B
//          legacy ack and request a state switch to CharSelect.
//        - anything else: log + ignore.
//   4. on_disconnect() — fail_with() unless we already received the
//                        LoginAck (the server may close right after
//                        sending it).
//
// 1:1 quirks preserved:
//   * The legacy MHClient waited for DistConnectSuccess *after* sending
//     the connect SYN; the modern TcpClient does the same (server
//     pushes DistConnectSuccess in on_connect handler before we ever
//     send a request).
//   * The auth_key is taken from MSGBASE.object_id, not from a payload
//     field.  This matches login_handler.cpp line 114:
//         msg.header.object_id = auth_key;

#include "CLoginState.hpp"
#include "CEngine.hpp"
#include "CMainGame.hpp"

#include <cstring>
#include <utility>

#include "mxh/log/mlog.hpp"
#include "mxh/proto/protocol.hpp"

namespace mxh::client {

// -------------------------------------------------------------------------
// Wire-format helpers (pure functions, unit-tested independently).
// -------------------------------------------------------------------------

std::vector<std::uint8_t>
legacy_request_login_payload(std::uint32_t auth_key,
                             const std::string& user_id,
                             const std::string& password) {
    // login_handler.cpp:handle_legacy_login expects exactly 38 bytes:
    //   [AuthKey:4B LE] [id:17B null-padded] [pw:17B null-padded]
    std::vector<std::uint8_t> out(38, 0);
    out[0] = static_cast<std::uint8_t>(auth_key & 0xFF);
    out[1] = static_cast<std::uint8_t>((auth_key >> 8) & 0xFF);
    out[2] = static_cast<std::uint8_t>((auth_key >> 16) & 0xFF);
    out[3] = static_cast<std::uint8_t>((auth_key >> 24) & 0xFF);

    auto copy_padded = [](std::uint8_t* dst, const std::string& s) {
        const auto n = std::min<std::size_t>(s.size(), 17);
        if (n) std::memcpy(dst, s.data(), n);
        // remaining 17 - n bytes stay zero
    };
    copy_padded(out.data() + 4,  user_id);
    copy_padded(out.data() + 21, password);
    return out;
}

std::optional<LegacyLoginAck>
parse_legacy_login_ack(std::span<const std::uint8_t> payload) {
    if (payload.size() < 23) return std::nullopt;
    LegacyLoginAck a;
    // [0..16]  agentip (16B, null-padded).  std::span has no find()
    // helper so we scan manually up to 16 bytes.
    std::size_t addr_end = 16;
    for (std::size_t i = 0; i < 16 && i < payload.size(); ++i) {
        if (payload[i] == 0) { addr_end = i; break; }
    }
    a.agent_addr.assign(reinterpret_cast<const char*>(payload.data()),
                        addr_end);
    // [16..18] agentport (u16 LE)
    a.agent_port = static_cast<std::uint16_t>(
        payload[16] | (static_cast<std::uint16_t>(payload[17]) << 8));
    // [18..22] userIdx (u32 LE)
    a.user_idx = static_cast<std::uint32_t>(
        payload[18]        | (static_cast<std::uint32_t>(payload[19]) << 8)
      | (static_cast<std::uint32_t>(payload[20]) << 16)
      | (static_cast<std::uint32_t>(payload[21]) << 24));
    // [22] user level
    a.user_level = payload[22];
    return a;
}

// -------------------------------------------------------------------------
// CLoginState
// -------------------------------------------------------------------------

CLoginState::CLoginState() = default;

CLoginState::~CLoginState() {
    if (m_client && m_client->is_connected()) {
        m_client->disconnect();
    }
}

void CLoginState::Init(void* /*pInitParam*/) {
    MLOG_DEBUG("CLoginState::Init (waiting for Start() from host)");
    setInitialized(true);
}

void CLoginState::Release() {
    MLOG_DEBUG("CLoginState::Release (disconnecting login client)");
    if (m_client) {
        if (m_client->is_connected()) m_client->disconnect();
        m_client.reset();
    }
    m_started     = false;
    m_ackReceived = false;
    m_failed      = false;
    m_failureReason.clear();
    setInitialized(false);
}

void CLoginState::Process() {
    tick();
    // The TcpClient runs its own recv thread; on_message is invoked from
    // there.  Nothing for us to do per-frame except keep the tick counter
    // moving so the host can drive any animation tied to it.
}

void CLoginState::Start(CEngine* engine, std::string host,
                        std::uint16_t port,
                        std::string user_id, std::string password) {
    m_pEngine = engine;
    m_host    = std::move(host);
    m_port    = port;
    m_userId  = std::move(user_id);
    m_password = std::move(password);

    if (m_started) {
        MLOG_DEBUG("CLoginState::Start called twice; ignoring second call");
        return;
    }
    m_started = true;

    MLOG_INFO("CLoginState connecting to %s:%u as '%s'",
              m_host.c_str(), static_cast<unsigned>(m_port),
              m_userId.c_str());
    m_client = std::make_unique<mxh::net::TcpClient>(*this);
    mxh::net::ClientConfig cfg;
    cfg.remote_address      = m_host;
    cfg.port                = m_port;
    cfg.use_legacy_framing  = true;  // 4DyuchiNET 2B length prefix
    cfg.use_encryption      = false; // Phase R-1 stub: no HSEL yet
    cfg.connect_timeout     = std::chrono::milliseconds(3000);
    auto e = m_client->connect(cfg);
    if (e != mxh::net::NetError::Ok) {
        fail_with(std::string("TcpClient::connect failed: ") +
                  mxh::net::to_string(e));
    }
}

bool CLoginState::is_connected() const noexcept {
    return m_client && m_client->is_connected();
}

bool CLoginState::on_connect(mxh::net::ConnectionId id,
                             const std::string& remote_addr) {
    MLOG_INFO("CLoginState::on_connect id=%llu from %s",
              static_cast<unsigned long long>(id.value),
              remote_addr.c_str());
    (void)id;
    (void)remote_addr;
    // Server will push DistConnectSuccess; on_message will follow.
    return true;
}

void CLoginState::on_message(mxh::net::ConnectionId id,
                              const mxh::net::Message& msg) {
    using mxh::proto::Category;
    using mxh::proto::UserConnProtocol;
    const auto cat   = static_cast<Category>(msg.header.category);
    const auto proto = static_cast<UserConnProtocol>(msg.header.protocol);
    MLOG_DEBUG("CLoginState::on_message id=%llu cat=%s proto=%d obj=%u payload=%zu",
               static_cast<unsigned long long>(id.value),
               mxh::proto::category_name(cat),
               static_cast<int>(proto),
               static_cast<unsigned>(msg.header.object_id),
               msg.payload.size());
    if (cat != Category::UserConn) {
        MLOG_WARN("CLoginState: unexpected category %s",
                  mxh::proto::category_name(cat));
        return;
    }
    switch (proto) {
        case UserConnProtocol::DistConnectSuccess: {
            m_authKey = msg.header.object_id;
            MLOG_INFO("CLoginState: got DistConnectSuccess auth_key=%u",
                      static_cast<unsigned>(m_authKey));
            // Build and send the RequestLogin legacy payload.
            const auto pl = legacy_request_login_payload(
                m_authKey, m_userId, m_password);
            mxh::net::Message out;
            out.header.category = static_cast<std::uint8_t>(Category::UserConn);
            out.header.protocol = static_cast<std::uint8_t>(
                UserConnProtocol::RequestLogin);
            out.header.object_id = 0;
            out.payload          = pl;
            const auto e = m_client->send(out);
            if (e != mxh::net::NetError::Ok) {
                fail_with(std::string("send RequestLogin failed: ") +
                          mxh::net::to_string(e));
                return;
            }
            MLOG_INFO("CLoginState: sent RequestLogin (38B legacy payload)");
            break;
        }
        case UserConnProtocol::NotifyUserLoginAck: {
            auto ack = parse_legacy_login_ack(msg.payload);
            if (!ack) {
                fail_with("LoginAck payload too short or malformed (need 23B)");
                return;
            }
            dispatch_login_ack(*ack);
            break;
        }
        case UserConnProtocol::NotifyUserLoginNack: {
            fail_with("LoginNack received (bad credentials?)");
            break;
        }
        default:
            MLOG_WARN("CLoginState: unhandled userconn proto=%d",
                      static_cast<int>(proto));
            break;
    }
}

void CLoginState::on_disconnect(mxh::net::ConnectionId id,
                                 mxh::net::NetError reason) {
    MLOG_INFO("CLoginState::on_disconnect id=%llu reason=%s",
              static_cast<unsigned long long>(id.value),
              mxh::net::to_string(reason));
    if (!m_ackReceived && !m_failed) {
        fail_with(std::string("disconnected before LoginAck: ") +
                  mxh::net::to_string(reason));
    }
}

void CLoginState::dispatch_login_ack(const LegacyLoginAck& ack) {
    m_userIdx   = ack.user_idx;
    m_agentAddr = ack.agent_addr;
    m_agentPort = ack.agent_port;
    m_ackReceived = true;
    MLOG_INFO("CLoginState: LoginAck agent=%s:%u user_idx=%u level=%u",
              m_agentAddr.c_str(),
              static_cast<unsigned>(m_agentPort),
              static_cast<unsigned>(m_userIdx),
              static_cast<unsigned>(ack.user_level));
    if (m_pEngine) {
        m_pEngine->RequestStateChange(
            static_cast<int>(GameStateId::CharSelect));
    } else {
        MLOG_WARN("CLoginState: no engine bound; cannot switch to CharSelect");
    }
}

void CLoginState::fail_with(const std::string& reason) {
    if (m_failed) return;
    m_failed = true;
    m_failureReason = reason;
    MLOG_ERROR("CLoginState: %s", reason.c_str());
}

} // namespace mxh::client
