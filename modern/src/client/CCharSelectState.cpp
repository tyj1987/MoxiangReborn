// mxh/client/CCharSelectState.cpp
// Phase B.2.2 — character-select state implementation.

#include "CCharSelectState.hpp"
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
legacy_character_list_syn_payload(std::uint32_t user_id,
                                  std::uint32_t dist_auth_key) {
    // agent_handler.cpp:526-538: [user_id:4B LE][dist_auth_key:4B LE] = 8B
    std::vector<std::uint8_t> out(8);
    out[0] = static_cast<std::uint8_t>(user_id & 0xFF);
    out[1] = static_cast<std::uint8_t>((user_id >> 8) & 0xFF);
    out[2] = static_cast<std::uint8_t>((user_id >> 16) & 0xFF);
    out[3] = static_cast<std::uint8_t>((user_id >> 24) & 0xFF);
    out[4] = static_cast<std::uint8_t>(dist_auth_key & 0xFF);
    out[5] = static_cast<std::uint8_t>((dist_auth_key >> 8) & 0xFF);
    out[6] = static_cast<std::uint8_t>((dist_auth_key >> 16) & 0xFF);
    out[7] = static_cast<std::uint8_t>((dist_auth_key >> 24) & 0xFF);
    return out;
}

std::vector<std::uint8_t>
legacy_character_select_syn_payload(std::uint16_t channel) {
    // MSGBASE carries chrid in object_id; payload = [channel: u16 LE]
    return {
        static_cast<std::uint8_t>(channel & 0xFF),
        static_cast<std::uint8_t>((channel >> 8) & 0xFF)
    };
}

std::optional<std::vector<CharacterSlot>>
parse_legacy_character_list_ack(std::span<const std::uint8_t> payload) {
    // Layout (CHINA locale, no _CRYPTCHECK_):
    //   [0..4)    CharNum (i32 LE)
    //   [4..14)   StandingArrayNum[5] (5 * u16)
    //   [14..189) BaseObjectInfo[5]   (5 * 35B)
    //   [189..889) ChrTotalInfo[5]   (5 * 140B)
    // = 889 bytes total.
    if (payload.size() < 4) return std::nullopt;

    // Each BaseObjectInfo slot starts with [chrid: u32][user_id: u32][name...].
    // We only need chrid per slot.  Reading just the first 4 bytes of each
    // 35-byte slot is enough; if the slot block would read past the end
    // we return what we have so far (defensive — the modern server
    // always writes the full 889B).
    constexpr std::size_t kMaxSlots   = 5;
    constexpr std::size_t kBaseOff    = 14;             // after CharNum + Standing
    constexpr std::size_t kSlotSize   = 35;
    constexpr std::size_t kChridOff   = 0;              // within a slot

    std::vector<CharacterSlot> out(kMaxSlots);
    const std::size_t avail_for_slots = (payload.size() >= kBaseOff
                                         ? payload.size() - kBaseOff : 0);
    const std::size_t slots_readable  = std::min(kMaxSlots,
                                                avail_for_slots / kSlotSize);
    for (std::size_t i = 0; i < slots_readable; ++i) {
        const auto base = kBaseOff + i * kSlotSize + kChridOff;
        std::uint32_t chrid = 0;
        std::memcpy(&chrid, payload.data() + base, 4);
        out[i].chrid = chrid;
        out[i].valid = (chrid != 0);
    }
    return out;
}

std::optional<std::uint16_t>
parse_legacy_character_select_ack(std::span<const std::uint8_t> payload) {
    if (payload.size() < 1) return std::nullopt;
    return static_cast<std::uint16_t>(payload[0]);
}

// -------------------------------------------------------------------------
// CCharSelectState
// -------------------------------------------------------------------------

CCharSelectState::CCharSelectState() = default;

CCharSelectState::~CCharSelectState() {
    if (m_client && m_client->is_connected()) m_client->disconnect();
}

void CCharSelectState::Init(void* /*pInitParam*/) {
    MLOG_DEBUG("CCharSelectState::Init (waiting for Start() + SetLoginResult())");
    setInitialized(true);
}

void CCharSelectState::Start(CEngine* engine) {
    m_pEngine = engine;
    // Phase B.2.2: pull the LoginResult that CLoginState handed off
    // via the engine's transfer slot.  If a host later calls
    // SetLoginResult() explicitly, it overrides what was stashed.
    if (m_pEngine && m_pEngine->has_pending_transfer()) {
        auto v = m_pEngine->TakePendingTransfer();
        if (v.type() == typeid(LoginResult)) {
            m_login = std::any_cast<LoginResult>(v);
            MLOG_DEBUG("CCharSelectState: pulled LoginResult from engine transfer slot "
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
    MLOG_INFO("CCharSelectState connecting to %s:%u (user_idx=%u)",
              m_login.agent_addr.c_str(),
              static_cast<unsigned>(m_login.agent_port),
              static_cast<unsigned>(m_login.user_idx));
    m_client = std::make_unique<mxh::net::TcpClient>(*this);
    mxh::net::ClientConfig cfg;
    cfg.remote_address     = m_login.agent_addr;
    cfg.port               = m_login.agent_port;
    cfg.use_legacy_framing = true;
    cfg.use_encryption     = false;
    cfg.connect_timeout    = std::chrono::milliseconds(3000);
    auto e = m_client->connect(cfg);
    if (e != mxh::net::NetError::Ok) {
        fail_with(std::string("TcpClient::connect to AgentServer failed: ") +
                  mxh::net::to_string(e));
    }
}

void CCharSelectState::Release() {
    MLOG_DEBUG("CCharSelectState::Release");
    if (m_client) {
        if (m_client->is_connected()) m_client->disconnect();
        m_client.reset();
    }
    m_characters.clear();
    m_selectedChrid = 0;
    m_selectedMap   = 0;
    m_started       = false;
    m_listReceived  = false;
    m_selectSent    = false;
    m_failed        = false;
    m_failureReason.clear();
    setInitialized(false);
}

void CCharSelectState::Process() { tick(); }

bool CCharSelectState::is_connected() const noexcept {
    return m_client && m_client->is_connected();
}

bool CCharSelectState::on_connect(mxh::net::ConnectionId id,
                                   const std::string& remote_addr) {
    MLOG_INFO("CCharSelectState::on_connect id=%llu from %s",
              static_cast<unsigned long long>(id.value),
              remote_addr.c_str());
    (void)id;
    (void)remote_addr;
    return true;
}

void CCharSelectState::on_message(mxh::net::ConnectionId id,
                                   const mxh::net::Message& msg) {
    using mxh::proto::Category;
    using mxh::proto::UserConnProtocol;
    const auto cat   = static_cast<Category>(msg.header.category);
    const auto proto = static_cast<UserConnProtocol>(msg.header.protocol);
    MLOG_DEBUG("CCharSelectState::on_message id=%llu cat=%s proto=%d obj=%u payload=%zu",
               static_cast<unsigned long long>(id.value),
               mxh::proto::category_name(cat),
               static_cast<int>(proto),
               static_cast<unsigned>(msg.header.object_id),
               msg.payload.size());
    if (cat != Category::UserConn) {
        MLOG_WARN("CCharSelectState: unexpected category %s",
                  mxh::proto::category_name(cat));
        return;
    }
    switch (proto) {
        case UserConnProtocol::AgentConnectSuccess: {
            MLOG_INFO("CCharSelectState: got AgentConnectSuccess (auth_key=%u)",
                      static_cast<unsigned>(msg.header.object_id));
            send_list_syn();
            break;
        }
        case UserConnProtocol::CharacterListAck: {
            auto list = parse_legacy_character_list_ack(msg.payload);
            if (!list) {
                fail_with("CharacterListAck too short (< 4 bytes)");
                return;
            }
            m_characters  = std::move(*list);
            m_listReceived = true;
            MLOG_INFO("CCharSelectState: CharacterListAck char_count derived from list, "
                      "first valid chrid=%u",
                      static_cast<unsigned>(m_characters.empty()
                                            ? 0u
                                            : (m_characters[0].valid
                                               ? m_characters[0].chrid : 0u)));
            auto_select_first();
            break;
        }
        case UserConnProtocol::CharacterListNack: {
            fail_with("CharacterListNack received");
            break;
        }
        case UserConnProtocol::CharacterSelectAck: {
            auto map = parse_legacy_character_select_ack(msg.payload);
            if (!map) {
                fail_with("CharacterSelectAck payload too short");
                return;
            }
            dispatch_select_ack(*map);
            break;
        }
        case UserConnProtocol::CharacterSelectNack: {
            fail_with("CharacterSelectNack received (no matching character in DB)");
            break;
        }
        default:
            MLOG_WARN("CCharSelectState: unhandled userconn proto=%d",
                      static_cast<int>(proto));
            break;
    }
}

void CCharSelectState::on_disconnect(mxh::net::ConnectionId id,
                                      mxh::net::NetError reason) {
    MLOG_INFO("CCharSelectState::on_disconnect id=%llu reason=%s",
              static_cast<unsigned long long>(id.value),
              mxh::net::to_string(reason));
    if (!m_selectSent && !m_failed) {
        fail_with(std::string("disconnected before CharacterSelectAck: ") +
                  mxh::net::to_string(reason));
    }
}

void CCharSelectState::send_list_syn() {
    const auto pl = legacy_character_list_syn_payload(
        m_login.user_idx, m_login.dist_auth_key);
    mxh::net::Message out;
    out.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListSyn);
    out.header.object_id = m_login.user_idx;
    out.payload          = pl;
    const auto e = m_client->send(out);
    if (e != mxh::net::NetError::Ok) {
        fail_with(std::string("send CharacterListSyn failed: ") +
                  mxh::net::to_string(e));
        return;
    }
    MLOG_INFO("CCharSelectState: sent CharacterListSyn (8B legacy payload)");
}

void CCharSelectState::auto_select_first() {
    for (const auto& slot : m_characters) {
        if (slot.valid) {
            SelectCharacter(slot.chrid);
            return;
        }
    }
    // No characters in the list (DB has zero characters for this user).
    // We can't issue a meaningful CharacterSelectSyn, so we just log
    // and stay in this state.  Host can call SelectCharacter() later
    // once a character is created via the (Phase B.4+) creation UI.
    MLOG_WARN("CCharSelectState: ListAck has no valid character slots; "
              "waiting for host to call SelectCharacter()");
}

void CCharSelectState::SelectCharacter(std::uint32_t chrid) {
    if (!m_client || !m_client->is_connected()) {
        fail_with("SelectCharacter: not connected to AgentServer");
        return;
    }
    if (m_selectSent) {
        MLOG_DEBUG("CCharSelectState: SelectCharacter already sent, ignoring");
        return;
    }
    m_selectedChrid = chrid;
    const auto pl   = legacy_character_select_syn_payload(0);
    mxh::net::Message out;
    out.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterSelectSyn);
    out.header.object_id = chrid;
    out.payload          = pl;
    const auto e = m_client->send(out);
    if (e != mxh::net::NetError::Ok) {
        fail_with(std::string("send CharacterSelectSyn failed: ") +
                  mxh::net::to_string(e));
        return;
    }
    m_selectSent = true;
    MLOG_INFO("CCharSelectState: sent CharacterSelectSyn chrid=%u",
              static_cast<unsigned>(chrid));
}

void CCharSelectState::dispatch_select_ack(std::uint16_t map_num) {
    m_selectedMap = map_num;
    MLOG_INFO("CCharSelectState: CharacterSelectAck chrid=%u map_num=%u",
              static_cast<unsigned>(m_selectedChrid),
              static_cast<unsigned>(map_num));
    if (m_pEngine) {
        m_pEngine->RequestStateChange(
            static_cast<int>(GameStateId::GameLoading));
    } else {
        MLOG_WARN("CCharSelectState: no engine bound; cannot switch to GameLoading");
    }
}

void CCharSelectState::fail_with(const std::string& reason) {
    if (m_failed) return;
    m_failed = true;
    m_failureReason = reason;
    MLOG_ERROR("CCharSelectState: %s", reason.c_str());
}

} // namespace mxh::client
