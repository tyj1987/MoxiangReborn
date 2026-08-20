// mxh/client/CCharSelectState.cpp
// Phase B.2.2 — character-select state implementation.

#include "CCharSelectState.hpp"
#include "CEngine.hpp"
#include "CMainGame.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <utility>

#include "mxh/log/mlog.hpp"
#include "mxh/proto/protocol.hpp"
#include "mxh/ui/cPushupButton.hpp"
#include "mxh/ui/cWindowManager.hpp"

namespace mxh::client {

CharSelectUiCommand resolve_char_select_ui_command(
    const ClientUiActivation& activation) noexcept {
    static constexpr std::string_view kSlotIds[] = {
        "MT_FIRSTCHOSEBTN", "MT_SECONDCHOSEBTN", "MT_THIRDCHOSEBTN",
        "MT_FOURTHCHOSEBTN", "MT_FIFTHCHOSEBTN"};
    for (std::size_t i = 0; i < std::size(kSlotIds); ++i) {
        if (activation.legacy_id == kSlotIds[i]) {
            return {CharSelectUiCommandKind::SelectSlot, i};
        }
    }
    if (activation.legacy_id == "MT_ENTERBTN" ||
        activation.legacy_func == "CS_BtnFuncEnter") {
        return {CharSelectUiCommandKind::Enter, 0};
    }
    if (activation.legacy_func == "CS_BtnFuncCreateChar") {
        return {CharSelectUiCommandKind::Create, 0};
    }
    if (activation.legacy_func == "CS_BtnFuncDeleteChar") {
        return {CharSelectUiCommandKind::Delete, 0};
    }
    if (activation.legacy_func == "CS_BtnFuncLogOut") {
        return {CharSelectUiCommandKind::Logout, 0};
    }
    return {};
}

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

std::vector<std::uint8_t>
legacy_character_remove_syn_payload(std::uint32_t character_id) {
    return {
        static_cast<std::uint8_t>(character_id & 0xFF),
        static_cast<std::uint8_t>((character_id >> 8) & 0xFF),
        static_cast<std::uint8_t>((character_id >> 16) & 0xFF),
        static_cast<std::uint8_t>((character_id >> 24) & 0xFF)
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

    // Each BaseObjectInfo slot starts with
    // [chrid: u32][user_id: u32][name: char[17]]...
    constexpr std::size_t kMaxSlots   = 5;
    constexpr std::size_t kBaseOff    = 14;             // after CharNum + Standing
    constexpr std::size_t kSlotSize   = 35;
    constexpr std::size_t kChridOff   = 0;              // within a slot
    constexpr std::size_t kNameOff    = 8;
    constexpr std::size_t kNameSize   = 17;

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
        if (out[i].valid) {
            const auto* name_begin = payload.data() + base + kNameOff;
            const auto* name_end = std::find(
                name_begin, name_begin + kNameSize, std::uint8_t{0});
            out[i].name.assign(
                reinterpret_cast<const char*>(name_begin),
                reinterpret_cast<const char*>(name_end));
        }
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

void CCharSelectState::Start(CEngine* engine, bool use_hsel) {
    m_pEngine = engine;
    m_useHsel = use_hsel;

    if (engine && engine->playdh_root().has_value() && m_uiRuntime.empty()) {
        std::string ui_error;
        if (!m_uiRuntime.load(*engine->playdh_root(), "CharSelectDlg.bin",
                              mxh::ui::ResolutionMode::Low800x600,
                              &ui_error)) {
            MLOG_WARN("CCharSelectState: CharSelectDlg.bin load failed: %s",
                      ui_error.c_str());
        }
    }
    if (m_useHsel) {
        m_hsel = std::make_unique<mxh::crypto::HselStreamCipher>();
    }
    // Phase B.2.2: pull the LoginResult that CLoginState handed off
    // via the engine's transfer slot.  If a host later calls
    // SetLoginResult() explicitly, it overrides what was stashed.
    if (m_pEngine && m_pEngine->has_pending_transfer()) {
        auto v = m_pEngine->TakePendingTransfer();
        if (auto* login = std::get_if<LoginResult>(&v)) {
            m_login = std::move(*login);
            MLOG_DEBUG("CCharSelectState: pulled LoginResult from engine transfer slot "
                       "(agent=%s:%u, user_idx=%u)",
                       m_login.agent_addr.c_str(),
                       static_cast<unsigned>(m_login.agent_port),
                       static_cast<unsigned>(m_login.user_idx));
        }
    }
    if (m_started) return;
    m_started = true;
    if (!m_pEngine) {
        fail_with("Start() requires an engine-owned AgentSession");
        return;
    }
    if (m_login.agent_addr.empty() || m_login.agent_port == 0) {
        fail_with("Start() called before SetLoginResult() with valid agent address");
        return;
    }
    MLOG_INFO("CCharSelectState connecting to %s:%u (user_idx=%u)",
              m_login.agent_addr.c_str(),
              static_cast<unsigned>(m_login.agent_port),
              static_cast<unsigned>(m_login.user_idx));
    auto& session = m_pEngine->agent_session();
    auto e = mxh::net::NetError::Ok;
    if (!session.is_connected()) {
        e = session.connect(m_login.agent_addr, m_login.agent_port, m_useHsel);
    }
    if (e != mxh::net::NetError::Ok) {
        fail_with(std::string("TcpClient::connect to AgentServer failed: ") +
                  mxh::net::to_string(e));
    } else if (session.is_ready()) {
        send_list_syn();
    }
}

mxh::net::IEncryptor* CCharSelectState::encryptor_for(
    mxh::net::ConnectionId) {
    return m_hsel ? m_hsel.get() : nullptr;
}

void CCharSelectState::Release() {
    MLOG_DEBUG("CCharSelectState::Release");
    m_releasing = true;
    m_characters.clear();
    m_uiRuntime.clear();
    m_selectedChrid = 0;
    m_selectedMap   = 0;
    m_started       = false;
    m_listReceived  = false;
    m_selectSent    = false;
    m_removeSent    = false;
    m_listSynSent   = false;
    m_removeChrid   = 0;
    m_releasing     = false;
    m_failed        = false;
    m_failureReason.clear();
    setInitialized(false);
}

void CCharSelectState::Process() {
    tick();
    if (!m_pEngine) return;
    for (auto& event : m_pEngine->agent_session().events().drain()) {
        switch (event.kind) {
            case ClientRuntimeEventKind::Connected:
                on_connect(event.connection, event.detail);
                break;
            case ClientRuntimeEventKind::Message:
                on_message(event.connection, event.message);
                break;
            case ClientRuntimeEventKind::Disconnected:
                on_disconnect(event.connection, event.network_error);
                break;
            case ClientRuntimeEventKind::Error:
                fail_with(event.detail);
                break;
        }
    }
}

bool CCharSelectState::is_connected() const noexcept {
    return m_pEngine && m_pEngine->agent_session().is_connected();
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
            refresh_character_slot_ui();
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
        case UserConnProtocol::CharacterRemoveAck: {
            apply_character_remove_ack();
            break;
        }
        case UserConnProtocol::CharacterRemoveNack: {
            std::uint32_t reason = 0;
            if (msg.payload.size() >= sizeof(reason)) {
                std::memcpy(&reason, msg.payload.data(), sizeof(reason));
            }
            m_removeSent = false;
            m_removeChrid = 0;
            const auto message = reason == 3
                ? "Character cannot be deleted right now."
                : "Character deletion failed.";
            const auto message_id = reason == 3 ? 995 : 25;
            if (!m_uiRuntime.showMessage(message_id, message)) {
                MLOG_WARN("CCharSelectState: CharacterRemoveNack reason=%u",
                          static_cast<unsigned>(reason));
            }
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
                MLOG_INFO("CCharSelectState: HSEL agent session key imported");
                break;
            }
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
    if (!m_releasing && !m_selectSent && !m_failed) {
        fail_with(std::string("disconnected before CharacterSelectAck: ") +
                  mxh::net::to_string(reason));
    }
}

void CCharSelectState::send_list_syn() {
    if (m_listSynSent) return;
    const auto pl = legacy_character_list_syn_payload(
        m_login.user_idx, m_login.dist_auth_key);
    mxh::net::Message out;
    out.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListSyn);
    out.header.object_id = m_login.user_idx;
    out.payload          = pl;
    const auto e = m_pEngine->agent_session().send(out);
    if (e != mxh::net::NetError::Ok) {
        fail_with(std::string("send CharacterListSyn failed: ") +
                  mxh::net::to_string(e));
        return;
    }
    m_listSynSent = true;
    MLOG_INFO("CCharSelectState: sent CharacterListSyn (8B legacy payload)");
}

bool CCharSelectState::send_remove_syn(std::uint32_t character_id) {
    if (!is_connected() || m_removeSent || character_id == 0) return false;

    mxh::net::Message out;
    out.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterRemoveSyn);
    out.header.object_id = 0;
    out.payload = legacy_character_remove_syn_payload(character_id);
    const auto e = m_pEngine->agent_session().send(out);
    if (e != mxh::net::NetError::Ok) {
        MLOG_ERROR("CCharSelectState: send CharacterRemoveSyn failed: %s",
                   mxh::net::to_string(e));
        return false;
    }
    m_removeChrid = character_id;
    m_removeSent = true;
    MLOG_INFO("CCharSelectState: sent CharacterRemoveSyn chrid=%u",
              static_cast<unsigned>(character_id));
    return true;
}

void CCharSelectState::auto_select_first() {
    if (!m_autoSelectForTest) {
        m_selectedChrid = 0;
        return;
    }
    for (std::size_t index = 0; index < m_characters.size(); ++index) {
        const auto& slot = m_characters[index];
        if (slot.valid) {
            m_selectedChrid = slot.chrid;
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
    if (!is_connected()) {
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
    const auto e = m_pEngine->agent_session().send(out);
    if (e != mxh::net::NetError::Ok) {
        fail_with(std::string("send CharacterSelectSyn failed: ") +
                  mxh::net::to_string(e));
        return;
    }
    m_selectSent = true;
    MLOG_INFO("CCharSelectState: sent CharacterSelectSyn chrid=%u",
              static_cast<unsigned>(chrid));
}

bool CCharSelectState::SelectSlot(std::size_t slot_index) noexcept {
    if (slot_index >= m_characters.size() || !m_characters[slot_index].valid
        || m_selectSent || m_removeSent) return false;
    m_selectedChrid = m_characters[slot_index].chrid;
    refresh_character_slot_ui();
    return true;
}

bool CCharSelectState::ConfirmSelection() {
    if (m_selectedChrid == 0 || m_selectSent) return false;
    SelectCharacter(m_selectedChrid);
    return m_selectSent;
}

bool CCharSelectState::RequestCharacterCreation() {
    if (!m_pEngine || !m_listReceived) return false;
    const auto valid_count = static_cast<std::size_t>(std::count_if(
        m_characters.begin(), m_characters.end(),
        [](const CharacterSlot& slot) { return slot.valid; }));
    if (valid_count >= m_characters.size()) return false;
    m_pEngine->SetPendingTransfer(m_login);
    m_pEngine->RequestStateChange(static_cast<int>(GameStateId::CharMake));
    return true;
}

bool CCharSelectState::RequestCharacterDeletion() {
    if (m_selectedChrid == 0 || m_selectSent || m_removeSent) return false;
    const auto character_id = m_selectedChrid;
    auto selected = std::find_if(
        m_characters.begin(), m_characters.end(),
        [character_id](const CharacterSlot& slot) {
            return slot.valid && slot.chrid == character_id;
        });
    if (selected == m_characters.end()) return false;

    std::string prompt = "Delete this character?";
    if (!selected->name.empty()) prompt = "Delete " + selected->name + "?";
    return m_uiRuntime.showConfirmation(
        282, std::move(prompt),
        [this, character_id](bool confirmed) {
            if (confirmed) (void)send_remove_syn(character_id);
        });
}

bool CCharSelectState::select_adjacent(int direction) noexcept {
    if (m_characters.empty()) return false;
    std::size_t current = m_characters.size();
    for (std::size_t i = 0; i < m_characters.size(); ++i) {
        if (m_characters[i].valid &&
            m_characters[i].chrid == m_selectedChrid) {
            current = i;
            break;
        }
    }
    for (std::size_t step = 0; step < m_characters.size(); ++step) {
        const auto base = current == m_characters.size()
            ? (direction > 0 ? m_characters.size() - 1 : 0)
            : current;
        const auto offset = direction > 0
            ? step + 1
            : m_characters.size() - ((step + 1) % m_characters.size());
        const auto index = (base + offset) % m_characters.size();
        if (m_characters[index].valid) return SelectSlot(index);
    }
    return false;
}

bool CCharSelectState::handle_ui_activation(
    const ClientUiActivation& activation) {
    const auto command = resolve_char_select_ui_command(activation);
    switch (command.kind) {
        case CharSelectUiCommandKind::SelectSlot:
            return SelectSlot(command.slot_index);
        case CharSelectUiCommandKind::Create:
            return RequestCharacterCreation();
        case CharSelectUiCommandKind::Enter:
            return ConfirmSelection();
        case CharSelectUiCommandKind::Delete:
            return RequestCharacterDeletion();
        case CharSelectUiCommandKind::Logout:
            MLOG_INFO("CCharSelectState: logout requested; title routing pending");
            return true;
        case CharSelectUiCommandKind::None:
        default:
            return false;
    }
}

void CCharSelectState::apply_character_remove_ack() {
    const auto character_id = m_removeChrid != 0
        ? m_removeChrid : m_selectedChrid;
    const auto slot = std::find_if(
        m_characters.begin(), m_characters.end(),
        [character_id](const CharacterSlot& candidate) {
            return candidate.valid && candidate.chrid == character_id;
        });
    if (slot == m_characters.end()) {
        MLOG_WARN("CCharSelectState: CharacterRemoveAck for unknown chrid=%u",
                  static_cast<unsigned>(character_id));
    } else {
        *slot = CharacterSlot{};
    }
    m_selectedChrid = 0;
    m_removeChrid = 0;
    m_removeSent = false;
    refresh_character_slot_ui();
}

void CCharSelectState::refresh_character_slot_ui() {
    static constexpr std::string_view kSlotIds[] = {
        "MT_FIRSTCHOSEBTN", "MT_SECONDCHOSEBTN", "MT_THIRDCHOSEBTN",
        "MT_FOURTHCHOSEBTN", "MT_FIFTHCHOSEBTN"};
    for (std::size_t i = 0; i < std::size(kSlotIds); ++i) {
        auto* button = dynamic_cast<mxh::ui::cPushupButton*>(
            m_uiRuntime.findWindowByLegacyId(kSlotIds[i]));
        if (!button) continue;
        const bool valid = i < m_characters.size() && m_characters[i].valid;
        button->SetText(valid ? m_characters[i].name : std::string{});
        button->SetPushEx(valid && m_characters[i].chrid == m_selectedChrid);
    }
}

bool CCharSelectState::OnMouseButton(bool left, bool down,
                                     std::int32_t x, std::int32_t y) {
    auto result = m_uiRuntime.onMouseButton(left, down, x, y);
    if (result.activation) handle_ui_activation(*result.activation);
    return result.consumed;
}

bool CCharSelectState::OnMouseMove(std::int32_t x, std::int32_t y) {
    return m_uiRuntime.onMouseMove(x, y);
}

bool CCharSelectState::OnKeyEvent(bool down, std::uint32_t key) {
    if (m_uiRuntime.onKey(down, static_cast<std::int32_t>(key))) return true;
    if (!down) return false;
    switch (key) {
        case 0x26u: return select_adjacent(-1); // VK_UP
        case 0x28u: return select_adjacent(1);  // VK_DOWN
        case 0x0Du: return ConfirmSelection(); // VK_RETURN
        default: return false;
    }
}

bool CCharSelectState::OnChar(std::uint32_t ch) {
    return m_uiRuntime.onChar(static_cast<std::int32_t>(ch));
}

void CCharSelectState::dispatch_select_ack(std::uint16_t map_num) {
    m_selectedMap = map_num;
    MLOG_INFO("CCharSelectState: CharacterSelectAck chrid=%u map_num=%u",
              static_cast<unsigned>(m_selectedChrid),
              static_cast<unsigned>(map_num));
    if (m_pEngine) {
        m_pEngine->SetPendingTransfer(GameEntryRequest{
            m_selectedChrid, map_num});
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
