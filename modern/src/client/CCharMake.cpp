// mxh/client/CCharMake.cpp
// Phase B.4 - character-creation state implementation.

#include "CCharMake.hpp"
#include "CEngine.hpp"
#include "CMainGame.hpp"

#include <cstring>
#include <utility>

#include "mxh/log/mlog.hpp"
#include "mxh/proto/protocol.hpp"

namespace mxh::client {

namespace {

constexpr std::size_t kMaxNameLength = 16;  // MAX_NAME_LENGTH (legacy)
constexpr std::size_t kMakePayload  = 59;   // sizeof(CHARACTERMAKEINFO)-8

void put_u32(std::vector<std::uint8_t>& out, std::size_t off,
             std::uint32_t v) {
    out[off + 0] = static_cast<std::uint8_t>(v & 0xFF);
    out[off + 1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
    out[off + 2] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
    out[off + 3] = static_cast<std::uint8_t>((v >> 24) & 0xFF);
}

void put_f32(std::vector<std::uint8_t>& out, std::size_t off, float f) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &f, 4);
    put_u32(out, off, bits);
}

} // namespace

std::vector<std::uint8_t>
legacy_character_make_syn_payload(const CharacterMakeParams& params,
                                  std::uint32_t user_id) {
    // 59 bytes, all fields zero-initialised first (legacy client memsets
    // the CHARACTERMAKEINFO before filling it).
    std::vector<std::uint8_t> out(kMakePayload, 0);

    // [0..17) Name[17]: 16 chars + NUL, truncated, zero-padded.
    std::memcpy(out.data(), params.name.c_str(),
                std::min<std::size_t>(kMaxNameLength, params.name.size()));

    // [17..21) UserID - the server overwrites this anyway, but the legacy
    // client sent the logged-in user id here.
    put_u32(out, 17, user_id);

    // [21..26) appearance fields.
    out[21] = params.sex_type;
    out[22] = params.body_type;
    out[23] = params.hair_type;
    out[24] = params.face_type;
    out[25] = params.start_area;

    // [26..30) bDuplCheck = FALSE (legacy client never set it before send).
    // [30..50) WearedItemIdx[10] = 0 (no starter items).

    // [50] StandingArrayNum: legacy client sends -1 (0xFF).
    out[50] = 0xFF;

    // [51..55) Height, [55..59) Width.
    put_f32(out, 51, params.height);
    put_f32(out, 55, params.width);
    return out;
}

// -------------------------------------------------------------------------
// CCharMake
// -------------------------------------------------------------------------

CCharMake::CCharMake() = default;

CCharMake::~CCharMake() {
    if (m_client && m_client->is_connected()) m_client->disconnect();
}

void CCharMake::Init(void* /*pInitParam*/) {
    MLOG_DEBUG("CCharMake::Init (waiting for Start() + SetLoginResult())");
    setInitialized(true);
}

void CCharMake::Start(CEngine* engine, bool use_hsel) {
    m_pEngine = engine;
    m_useHsel = use_hsel;
    if (m_useHsel) {
        m_hsel = std::make_unique<mxh::crypto::HselStreamCipher>();
    }
    if (m_pEngine && m_pEngine->has_pending_transfer()) {
        auto v = m_pEngine->TakePendingTransfer();
        if (v.type() == typeid(LoginResult)) {
            m_login = std::any_cast<LoginResult>(v);
            MLOG_DEBUG("CCharMake: pulled LoginResult from engine transfer slot "
                       "(agent=%s:%u, user_idx=%u)",
                       m_login.agent_addr.c_str(),
                       static_cast<unsigned>(m_login.agent_port),
                       static_cast<unsigned>(m_login.user_idx));
        }
    }
    if (m_started) return;
    m_started = true;
    if (m_login.agent_addr.empty() || m_login.agent_port == 0) {
        fail_with("Start() called before SetLoginResult() with valid agent address");
        return;
    }
    MLOG_INFO("CCharMake connecting to %s:%u (user_idx=%u)",
              m_login.agent_addr.c_str(),
              static_cast<unsigned>(m_login.agent_port),
              static_cast<unsigned>(m_login.user_idx));
    m_client = std::make_unique<mxh::net::TcpClient>(*this);
    mxh::net::ClientConfig cfg;
    cfg.remote_address     = m_login.agent_addr;
    cfg.port               = m_login.agent_port;
    cfg.use_legacy_framing = true;
    cfg.use_encryption     = m_useHsel;  // Phase R-1: HSEL agent session
    cfg.connect_timeout    = std::chrono::milliseconds(3000);
    auto e = m_client->connect(cfg);
    if (e != mxh::net::NetError::Ok) {
        fail_with(std::string("TcpClient::connect to AgentServer failed: ") +
                  mxh::net::to_string(e));
    }
}

mxh::net::IEncryptor* CCharMake::encryptor_for(mxh::net::ConnectionId) {
    return m_hsel ? m_hsel.get() : nullptr;
}

void CCharMake::Release() {
    MLOG_DEBUG("CCharMake::Release");
    if (m_client) {
        if (m_client->is_connected()) m_client->disconnect();
        m_client.reset();
    }
    m_pending      = CharacterMakeParams{};
    m_started      = false;
    m_connectAcked = false;
    m_makeSent     = false;
    m_failed       = false;
    m_failureReason.clear();
    setInitialized(false);
}

void CCharMake::Process() { tick(); }

bool CCharMake::is_connected() const noexcept {
    return m_client && m_client->is_connected();
}

bool CCharMake::on_connect(mxh::net::ConnectionId id,
                           const std::string& remote_addr) {
    MLOG_INFO("CCharMake::on_connect id=%llu from %s",
              static_cast<unsigned long long>(id.value),
              remote_addr.c_str());
    (void)id;
    (void)remote_addr;
    return true;
}

void CCharMake::on_message(mxh::net::ConnectionId id,
                           const mxh::net::Message& msg) {
    using mxh::proto::Category;
    using mxh::proto::UserConnProtocol;
    const auto cat   = static_cast<Category>(msg.header.category);
    const auto proto = static_cast<UserConnProtocol>(msg.header.protocol);
    MLOG_DEBUG("CCharMake::on_message id=%llu cat=%s proto=%d obj=%u payload=%zu",
               static_cast<unsigned long long>(id.value),
               mxh::proto::category_name(cat),
               static_cast<int>(proto),
               static_cast<unsigned>(msg.header.object_id),
               msg.payload.size());
    if (cat != Category::UserConn) {
        MLOG_WARN("CCharMake: unexpected category %s",
                  mxh::proto::category_name(cat));
        return;
    }
    switch (proto) {
        case UserConnProtocol::AgentConnectSuccess: {
            m_connectAcked = true;
            MLOG_INFO("CCharMake: got AgentConnectSuccess (auth_key=%u)",
                      static_cast<unsigned>(msg.header.object_id));
            // The host drives SubmitCharacter() once the user finishes
            // the creation form.  If a submit was already queued (host
            // called SubmitCharacter before the connect ack arrived), send
            // it now.
            if (m_makeSent) send_make_syn();
            break;
        }
        case UserConnProtocol::CharacterListAck: {
            // Success: the agent re-sends the refreshed character list
            // after creating the character (legacy RCreateCharacter ->
            // UserIDXSendAndCharacterBaseInfo).  Switch back to CharSelect
            // exactly like the legacy client does on CHARACTERLIST_ACK.
            MLOG_INFO("CCharMake: CharacterListAck received after create; "
                      "switching to CharSelect");
            if (m_pEngine) {
                m_pEngine->SetPendingTransfer(m_login);
                m_pEngine->RequestStateChange(
                    static_cast<int>(GameStateId::CharSelect));
            } else {
                MLOG_WARN("CCharMake: no engine bound; cannot switch to CharSelect");
            }
            break;
        }
        case UserConnProtocol::CharacterMakeAck: {
            // No-op marker from the modern agent; the ListAck drives the
            // state transition (same as the legacy client).
            MLOG_DEBUG("CCharMake: CharacterMakeAck (ignored, waiting for ListAck)");
            break;
        }
        case UserConnProtocol::CharacterMakeNack: {
            fail_with("CharacterMakeNack received (name taken or invalid params)");
            break;
        }
        default:
            if (proto == static_cast<UserConnProtocol>(
                             mxh::proto::kModernHselKey)) {
                if (msg.payload.size() < sizeof(mxh::crypto::HselInit)) {
                    fail_with("HselKey payload too short");
                    break;
                }
                mxh::crypto::HselInit init{};
                std::memcpy(&init, msg.payload.data(), sizeof(init));
                if (!m_hsel || !m_hsel->import_init(init)) {
                    fail_with("HselKey import failed");
                    break;
                }
                MLOG_INFO("CCharMake: HSEL agent session key imported");
                break;
            }
            MLOG_WARN("CCharMake: unhandled userconn proto=%d",
                      static_cast<int>(proto));
            break;
    }
}

void CCharMake::on_disconnect(mxh::net::ConnectionId id,
                              mxh::net::NetError reason) {
    MLOG_INFO("CCharMake::on_disconnect id=%llu reason=%s",
              static_cast<unsigned long long>(id.value),
              mxh::net::to_string(reason));
    if (!m_makeSent && !m_failed) {
        fail_with(std::string("disconnected before create completed: ") +
                  mxh::net::to_string(reason));
    }
}

bool CCharMake::SubmitCharacter(const CharacterMakeParams& params) {
    // Legacy server-side validation (AgentNetworkMsgParser
    // CheckCharacterMakeInfo): sex <= 1, hair <= 4, face <= 4.
    if (params.sex_type > 1 || params.hair_type > 4 || params.face_type > 4) {
        fail_with("SubmitCharacter: invalid appearance params");
        return false;
    }
    if (params.name.empty()) {
        fail_with("SubmitCharacter: empty name");
        return false;
    }
    if (!m_client || !m_client->is_connected()) {
        fail_with("SubmitCharacter: not connected to AgentServer");
        return false;
    }
    if (m_makeSent) {
        MLOG_DEBUG("CCharMake: submit already in flight, ignoring");
        return false;
    }
    m_pending = params;
    m_makeSent = true;
    if (m_connectAcked) send_make_syn();
    return true;
}

void CCharMake::send_make_syn() {
    const auto pl = legacy_character_make_syn_payload(
        m_pending, m_login.user_idx);
    mxh::net::Message out;
    out.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterMakeSyn);
    out.header.object_id = m_login.user_idx;
    out.payload          = pl;
    const auto e = m_client->send(out);
    if (e != mxh::net::NetError::Ok) {
        fail_with(std::string("send CharacterMakeSyn failed: ") +
                  mxh::net::to_string(e));
        return;
    }
    MLOG_INFO("CCharMake: sent CharacterMakeSyn name='%s' (59B legacy payload)",
              m_pending.name.c_str());
}

void CCharMake::fail_with(const std::string& reason) {
    m_failed = true;
    m_failureReason = reason;
    MLOG_ERROR("CCharMake: %s", reason.c_str());
}

} // namespace mxh::client
