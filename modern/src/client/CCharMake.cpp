// mxh/client/CCharMake.cpp
// Phase B.4 - character-creation state implementation.

#include "CCharMake.hpp"
#include "CEngine.hpp"
#include "CMainGame.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <utility>

#include "mxh/ui/cEditBox.hpp"
#include "mxh/ui/cStatic.hpp"
#include "mxh/ui/cWindowManager.hpp"

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

void put_u16(std::vector<std::uint8_t>& out, std::size_t off,
             std::uint16_t v) {
    out[off + 0] = static_cast<std::uint8_t>(v & 0xFFu);
    out[off + 1] = static_cast<std::uint8_t>((v >> 8) & 0xFFu);
}

void put_f32(std::vector<std::uint8_t>& out, std::size_t off, float f) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &f, 4);
    put_u32(out, off, bits);
}

constexpr std::size_t category_index(CharMakeOptionCategory category) noexcept {
    return static_cast<std::size_t>(category);
}

} // namespace

bool CharacterMakeFormModel::initialize(
    const CharMakeOptionCatalog& catalog) noexcept {
    if (!catalog.hasChinaBaseline()) return false;
    m_catalog = &catalog;
    m_indices.fill(0);
    m_params = CharacterMakeParams{};
    if (!apply(CharMakeOptionCategory::Sex, 0)) return false;
    if (!apply(CharMakeOptionCategory::Weapon, 0)) return false;
    if (!apply(CharMakeOptionCategory::StartArea, 0)) return false;
    if (catalog.find(CharMakeOptionCategory::Attribute)) {
        apply(CharMakeOptionCategory::Attribute, 0);
    }
    return true;
}

bool CharacterMakeFormModel::apply(CharMakeOptionCategory category,
                                   std::size_t index) noexcept {
    if (!m_catalog) return false;
    const auto* group = m_catalog->find(category);
    if (!group || group->options.empty()) return false;
    index %= group->options.size();
    m_indices[category_index(category)] = index;
    const auto value = group->options[index].value;
    switch (category) {
        case CharMakeOptionCategory::Sex:
            m_params.sex_type = static_cast<std::uint8_t>(value);
            m_indices[category_index(CharMakeOptionCategory::MaleHair)] = 0;
            m_indices[category_index(CharMakeOptionCategory::FemaleHair)] = 0;
            m_indices[category_index(CharMakeOptionCategory::MaleFace)] = 0;
            m_indices[category_index(CharMakeOptionCategory::FemaleFace)] = 0;
            m_indices[category_index(CharMakeOptionCategory::Cloth)] = 0;
            m_indices[category_index(CharMakeOptionCategory::Boot)] = 0;
            apply(m_params.sex_type == 0 ? CharMakeOptionCategory::MaleHair
                                         : CharMakeOptionCategory::FemaleHair, 0);
            apply(m_params.sex_type == 0 ? CharMakeOptionCategory::MaleFace
                                         : CharMakeOptionCategory::FemaleFace, 0);
            apply(CharMakeOptionCategory::Cloth, 0);
            apply(CharMakeOptionCategory::Boot, 0);
            break;
        case CharMakeOptionCategory::MaleHair:
            if (m_params.sex_type == 0)
                m_params.hair_type = static_cast<std::uint8_t>(value);
            break;
        case CharMakeOptionCategory::FemaleHair:
            if (m_params.sex_type != 0)
                m_params.hair_type = static_cast<std::uint8_t>(value);
            break;
        case CharMakeOptionCategory::MaleFace:
            if (m_params.sex_type == 0)
                m_params.face_type = static_cast<std::uint8_t>(value);
            break;
        case CharMakeOptionCategory::FemaleFace:
            if (m_params.sex_type != 0)
                m_params.face_type = static_cast<std::uint8_t>(value);
            break;
        case CharMakeOptionCategory::Cloth:
            m_params.weared_item_idx[2] = static_cast<std::uint16_t>(value);
            break;
        case CharMakeOptionCategory::Boot:
            m_params.weared_item_idx[3] = static_cast<std::uint16_t>(value);
            break;
        case CharMakeOptionCategory::Weapon:
            m_params.weared_item_idx[1] = static_cast<std::uint16_t>(value);
            break;
        case CharMakeOptionCategory::StartArea:
            m_params.start_area = static_cast<std::uint8_t>(value);
            break;
        case CharMakeOptionCategory::Attribute:
            // CHINA skips this leading group in the recovered legacy loader.
            break;
    }
    return true;
}

bool CharacterMakeFormModel::rotate(CharMakeOptionCategory category,
                                    int direction) noexcept {
    if (!m_catalog || direction == 0) return false;
    const auto* group = m_catalog->find(category);
    if (!group || group->options.empty()) return false;
    const auto current = selectedIndex(category);
    const auto count = group->options.size();
    const auto next = direction > 0
        ? (current + 1) % count
        : (current + count - 1) % count;
    return apply(category, next);
}

std::size_t CharacterMakeFormModel::selectedIndex(
    CharMakeOptionCategory category) const noexcept {
    return m_indices[category_index(category)];
}

const CharMakeOption* CharacterMakeFormModel::selectedOption(
    CharMakeOptionCategory category) const noexcept {
    if (!m_catalog) return nullptr;
    const auto* group = m_catalog->find(category);
    if (!group || group->options.empty()) return nullptr;
    return &group->options[selectedIndex(category) % group->options.size()];
}

CharMakeUiCommand resolve_char_make_ui_command(
    const ClientUiActivation& activation) noexcept {
    using Category = CharMakeOptionCategory;
    struct Arrow {
        std::string_view id;
        Category category;
        int direction;
    };
    static constexpr Arrow arrows[] = {
        {"CMID_SexLeft", Category::Sex, -1},
        {"CMID_SexRight", Category::Sex, 1},
        {"CMID_HairLeft", Category::MaleHair, -1},
        {"CMID_HairRight", Category::MaleHair, 1},
        {"CMID_FaceLeft", Category::MaleFace, -1},
        {"CMID_FaceRight", Category::MaleFace, 1},
        {"CMID_ClothLeft", Category::Cloth, -1},
        {"CMID_ClothRight", Category::Cloth, 1},
        {"CMID_BootLeft", Category::Boot, -1},
        {"CMID_BootRight", Category::Boot, 1},
        {"CMID_WeaponLeft", Category::Weapon, -1},
        {"CMID_WeaponRight", Category::Weapon, 1},
        {"CMID_AttribLeft", Category::Attribute, -1},
        {"CMID_AttribRight", Category::Attribute, 1},
    };
    for (const auto& arrow : arrows) {
        if (activation.legacy_id == arrow.id) {
            return {CharMakeUiCommandKind::Rotate,
                    arrow.category, arrow.direction};
        }
    }
    if (activation.legacy_id == "CMID_OverlapCheck" ||
        activation.legacy_func == "CM_OverlapCheckBtnFunc") {
        return {CharMakeUiCommandKind::CheckName, Category::Sex, 0};
    }
    if (activation.legacy_id == "CMID_CharMake" ||
        activation.legacy_func == "CM_CharMakeBtnFunc") {
        return {CharMakeUiCommandKind::Submit, Category::Sex, 0};
    }
    if (activation.legacy_id == "CMID_CharCancel" ||
        activation.legacy_func == "CM_CharCancelBtnFunc") {
        return {CharMakeUiCommandKind::Cancel, Category::Sex, 0};
    }
    return {};
}

std::vector<std::uint8_t> legacy_character_name_check_payload(
    std::string_view name) {
    std::vector<std::uint8_t> payload(kMaxNameLength + 1, 0);
    std::memcpy(payload.data(), name.data(),
                std::min<std::size_t>(kMaxNameLength, name.size()));
    return payload;
}

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
    // [30..50) WearedItemIdx[10], exact legacy equipment slot order.
    for (std::size_t index = 0; index < params.weared_item_idx.size(); ++index) {
        put_u16(out, 30 + index * 2, params.weared_item_idx[index]);
    }

    // [50] StandingArrayNum: defaults to the legacy -1 (0xFF) sentinel.
    out[50] = params.standing_array_num;

    // [51..55) Height, [55..59) Width.
    put_f32(out, 51, params.height);
    put_f32(out, 55, params.width);
    return out;
}

// -------------------------------------------------------------------------
// CCharMake
// -------------------------------------------------------------------------

CCharMake::CCharMake() = default;

CCharMake::~CCharMake() = default;

void CCharMake::Init(void* /*pInitParam*/) {
    MLOG_DEBUG("CCharMake::Init (waiting for Start() + SetLoginResult())");
    setInitialized(true);
}

void CCharMake::Start(CEngine* engine, bool use_hsel) {
    m_pEngine = engine;
    m_useHsel = use_hsel;

    if (engine && engine->playdh_root().has_value() && m_uiRuntime.empty()) {
        std::string ui_error;
        if (!m_uiRuntime.load(*engine->playdh_root(), "CharMakeNewDlg.bin",
                              mxh::ui::ResolutionMode::Low800x600,
                              &ui_error)) {
            fail_with("CharMakeNewDlg.bin load failed: " + ui_error);
        } else {
            auto catalog = CharMakeOptionCatalog::load(
                *engine->playdh_root(), &ui_error);
            if (!catalog || !catalog->hasChinaBaseline()) {
                fail_with("CharMake_SelectOption.bin load failed: " + ui_error);
            } else {
                m_optionCatalog = std::move(*catalog);
                if (!m_formModel.initialize(*m_optionCatalog)) {
                    fail_with("character creation form initialization failed");
                }
                if (auto* edit = name_edit()) {
                    edit->InitEditbox(static_cast<std::uint16_t>(edit->width()),
                                      static_cast<std::uint16_t>(kMaxNameLength + 1));
                }
                for (const auto& group : m_optionCatalog->groups()) {
                    refresh_option_text(group.category);
                }
                refresh_sex_visibility();
            }
        }
    }
    if (m_useHsel) {
        m_hsel = std::make_unique<mxh::crypto::HselStreamCipher>();
    }
    if (m_pEngine && m_pEngine->has_pending_transfer()) {
        auto v = m_pEngine->TakePendingTransfer();
        if (auto* login = std::get_if<LoginResult>(&v)) {
            m_login = std::move(*login);
            MLOG_DEBUG("CCharMake: pulled LoginResult from engine transfer slot "
                       "(agent=%s:%u, user_idx=%u)",
                       m_login.agent_addr.c_str(),
                       static_cast<unsigned>(m_login.agent_port),
                       static_cast<unsigned>(m_login.user_idx));
        }
    }
    if (m_started) return;
    m_started = true;
    if (!m_pEngine) {
        MLOG_WARN("CCharMake: no engine bound; waiting in disconnected state");
        return;
    }
    if (m_login.agent_addr.empty() || m_login.agent_port == 0) {
        fail_with("Start() called before SetLoginResult() with valid agent address");
        return;
    }
    MLOG_INFO("CCharMake connecting to %s:%u (user_idx=%u)",
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
        m_connectAcked = true;
    }
}

mxh::net::IEncryptor* CCharMake::encryptor_for(mxh::net::ConnectionId) {
    return m_hsel ? m_hsel.get() : nullptr;
}

void CCharMake::Release() {
    MLOG_DEBUG("CCharMake::Release");
    m_uiRuntime.clear();
    m_optionCatalog.reset();
    m_formModel = CharacterMakeFormModel{};
    m_nameAvailable.reset();
    m_checkedName.clear();
    m_nameCheckPending = false;
    m_cancelSent = false;
    m_pending      = CharacterMakeParams{};
    m_started      = false;
    m_connectAcked = false;
    m_makeSent     = false;
    m_failed       = false;
    m_failureReason.clear();
    setInitialized(false);
}

void CCharMake::Process() {
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

bool CCharMake::is_connected() const noexcept {
    return m_pEngine && m_pEngine->agent_session().is_connected();
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
        case UserConnProtocol::CharacterNameCheckAck: {
            if (m_nameCheckPending) {
                m_nameCheckPending = false;
                m_nameAvailable = true;
                MLOG_INFO("CCharMake: name '%s' is available",
                          m_checkedName.c_str());
            }
            break;
        }
        case UserConnProtocol::CharacterNameCheckNack: {
            if (m_nameCheckPending) {
                m_nameCheckPending = false;
                m_nameAvailable = false;
                MLOG_INFO("CCharMake: name '%s' is unavailable",
                          m_checkedName.c_str());
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
    if (!is_connected()) {
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

mxh::ui::cEditBox* CCharMake::name_edit() const noexcept {
    return dynamic_cast<mxh::ui::cEditBox*>(
        m_uiRuntime.findWindowByLegacyId("CMID_IDEDITBOX"));
}

void CCharMake::invalidate_name_check() noexcept {
    m_nameAvailable.reset();
    m_nameCheckPending = false;
    m_checkedName.clear();
    if (m_failed && m_failureReason.find("CharacterMakeNack") !=
                        std::string::npos) {
        m_failed = false;
        m_makeSent = false;
        m_failureReason.clear();
    }
}

void CCharMake::refresh_option_text(CharMakeOptionCategory category) {
    if (!m_optionCatalog) return;
    const auto* group = m_optionCatalog->find(category);
    const auto* option = m_formModel.selectedOption(category);
    if (!group || !option) return;
    if (auto* label = dynamic_cast<mxh::ui::cStatic*>(
            m_uiRuntime.findWindowByLegacyId(group->legacy_control_id))) {
        label->SetStaticText(option->label_bytes);
    }
}

void CCharMake::refresh_sex_visibility() {
    const bool male = m_formModel.params().sex_type == 0;
    for (const auto id : {"CMID_ManHairType", "CMID_ManFaceType"}) {
        if (auto* window = m_uiRuntime.findWindowByLegacyId(id)) {
            window->SetVisible(male);
        }
    }
    for (const auto id : {"CMID_WomanHairType", "CMID_WomanFaceType"}) {
        if (auto* window = m_uiRuntime.findWindowByLegacyId(id)) {
            window->SetVisible(!male);
        }
    }
}

bool CCharMake::CheckCurrentName() {
    auto* edit = name_edit();
    if (!edit || !is_connected() || m_nameCheckPending) return false;
    const auto& name = edit->editText();
    if (name.size() < 4 || name.size() > kMaxNameLength) return false;

    mxh::net::Message out;
    out.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterNameCheckSyn);
    out.header.object_id = m_login.user_idx;
    out.payload = legacy_character_name_check_payload(name);
    const auto error = m_pEngine->agent_session().send(out);
    if (error != mxh::net::NetError::Ok) return false;
    m_checkedName = name;
    m_nameAvailable.reset();
    m_nameCheckPending = true;
    return true;
}

bool CCharMake::SubmitCurrentForm() {
    auto* edit = name_edit();
    if (!edit) return false;
    CharacterMakeParams params = m_formModel.params();
    params.name = edit->editText();
    if (params.name.size() < 4 || params.name.size() > kMaxNameLength) {
        MLOG_WARN("CCharMake: character name must contain 4-16 bytes");
        return false;
    }
    return SubmitCharacter(params);
}

bool CCharMake::CancelCreation() {
    if (!m_pEngine || !is_connected() || m_cancelSent) return false;
    mxh::net::Message out;
    out.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::DirectCharacterListSyn);
    out.header.object_id = m_login.user_idx;
    if (m_pEngine->agent_session().send(out) != mxh::net::NetError::Ok) {
        return false;
    }
    m_cancelSent = true;
    return true;
}

bool CCharMake::handle_ui_activation(
    const ClientUiActivation& activation) {
    auto command = resolve_char_make_ui_command(activation);
    if (command.kind == CharMakeUiCommandKind::Rotate) {
        if (command.category == CharMakeOptionCategory::MaleHair &&
            m_formModel.params().sex_type != 0) {
            command.category = CharMakeOptionCategory::FemaleHair;
        } else if (command.category == CharMakeOptionCategory::MaleFace &&
                   m_formModel.params().sex_type != 0) {
            command.category = CharMakeOptionCategory::FemaleFace;
        }
        if (!m_formModel.rotate(command.category, command.direction)) return false;
        refresh_option_text(command.category);
        if (command.category == CharMakeOptionCategory::Sex) {
            refresh_option_text(CharMakeOptionCategory::MaleHair);
            refresh_option_text(CharMakeOptionCategory::FemaleHair);
            refresh_option_text(CharMakeOptionCategory::MaleFace);
            refresh_option_text(CharMakeOptionCategory::FemaleFace);
            refresh_option_text(CharMakeOptionCategory::Cloth);
            refresh_option_text(CharMakeOptionCategory::Boot);
            refresh_sex_visibility();
        }
        return true;
    }
    switch (command.kind) {
        case CharMakeUiCommandKind::CheckName: return CheckCurrentName();
        case CharMakeUiCommandKind::Submit: return SubmitCurrentForm();
        case CharMakeUiCommandKind::Cancel: return CancelCreation();
        case CharMakeUiCommandKind::None:
        case CharMakeUiCommandKind::Rotate:
        default: return false;
    }
}

bool CCharMake::OnMouseButton(bool left, bool down,
                              std::int32_t x, std::int32_t y) {
    auto result = m_uiRuntime.onMouseButton(left, down, x, y);
    if (result.activation) handle_ui_activation(*result.activation);
    return result.consumed;
}

bool CCharMake::OnMouseMove(std::int32_t x, std::int32_t y) {
    return m_uiRuntime.onMouseMove(x, y);
}

bool CCharMake::OnKeyEvent(bool down, std::uint32_t key) {
    const bool consumed = m_uiRuntime.onKey(down, static_cast<std::int32_t>(key));
    if (!down) return consumed;
    if (key == 0x08u) invalidate_name_check(); // VK_BACK
    if (key == 0x0Du) return SubmitCurrentForm() || consumed; // VK_RETURN
    if (key == 0x1Bu) {                                      // VK_ESCAPE
        CancelCreation();
        return true;
    }
    return consumed;
}

bool CCharMake::OnChar(std::uint32_t ch) {
    const bool consumed = m_uiRuntime.onChar(static_cast<std::int32_t>(ch));
    if (consumed) invalidate_name_check();
    return consumed;
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
    const auto e = m_pEngine->agent_session().send(out);
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
