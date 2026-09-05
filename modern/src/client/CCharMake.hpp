// mxh/client/CCharMake.hpp
// Phase B.4 - character-creation state (replaces CCharMake stub).
//
// Wires the eGS_CHARMAKE (GameStateId::CharMake = 5) slot to the modern
// mxh::net::TcpClient and drives the legacy 4DyuchiNET character-creation
// handshake against MoxianAgentServer:
//
//   client.Connect(agent_addr:port) -> recv AgentConnectSuccess (proto=8)
//                                    ->  [host fills the creation form]
//                                    ->  send CharacterMakeSyn (proto=22,
//                                        CHARACTERMAKEINFO 59B payload)
//                                    ->  recv CharacterListAck (proto=12)
//                                        = success (server re-sends the
//                                        refreshed list) -> CharSelect
//                                    ->  recv CharacterMakeNack (proto=24)
//                                        = failure (stay, host may retry)
//
// 1:1 with the legacy MHClient CharMake.cpp + GlobalEventFunc.cpp flow
// (CM_CharMakeBtnFunc) and agent_handler.cpp
// (handle_legacy_character_make):
//   * The legacy client memcpy's its CHARACTERMAKEINFO, sets
//     Category/Protocol/StandingArrayNum=-1 and the name, then sends
//     sizeof(msg) (59 bytes after MSGBASE).  The agent ignores
//     bDuplCheck/WearedItemIdx and overwrites UserID server-side.
//   * On success the legacy client receives the refreshed
//     MP_USERCONN_CHARACTERLIST_ACK and switches to eGAMESTATE_CHARSELECT;
//     on MP_USERCONN_CHARACTER_MAKE_NACK it stays in the create state.
//   * CharacterMakeAck (proto=23) is emitted by the modern agent as a
//     no-op marker; the state transition is driven by ListAck exactly
//     like the legacy client.
//
// Modern port notes (same conventions as CCharSelectState):
//   * The agent address/port + auth keys are bridged via LoginResult
//     (defined in CCharSelectState.hpp).
//   * The 59-byte encode is a free function (legacy_character_make_syn_payload)
//     so unit tests can lock the binary shape without a TcpClient.
//   * The state owns its TcpClient (unique_ptr) and disconnects in
//     Release(), matching the legacy per-state network ownership.

#pragma once

#include "CharMakeOptions.hpp"
#include "ClientUiRuntime.hpp"

#include "CGameState.hpp"
#include "CCharSelectState.hpp"  // LoginResult

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mxh/net/net.hpp"
#include "mxh/crypto/hsel_encryptor.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cDialogLoader.hpp"

namespace mxh::ui {
class cEditBox;
}

namespace mxh::client {

class CEngine;

// Character-creation options the host collects from the creation UI.
// 1:1 field set with the legacy CHARACTERMAKEINFO (CommonStruct.h:734)
// minus the fields the server overwrites / ignores.
struct CharacterMakeParams {
    std::string name;       // truncated to MAX_NAME_LENGTH (16) on the wire
    std::uint8_t sex_type   = 0;
    std::uint8_t body_type  = 0;
    std::uint8_t hair_type  = 0;
    std::uint8_t face_type  = 0;
    std::uint8_t start_area = 0;    // user-selected in the legacy client
    std::array<std::uint16_t, 10> weared_item_idx{};
    std::uint8_t standing_array_num = 0xFF;
    float        height     = 1.0f;
    float        width      = 1.0f;
};

class CharacterMakeFormModel {
public:
    bool initialize(const CharMakeOptionCatalog& catalog) noexcept;
    bool rotate(CharMakeOptionCategory category, int direction) noexcept;
    std::size_t selectedIndex(CharMakeOptionCategory category) const noexcept;
    const CharMakeOption* selectedOption(
        CharMakeOptionCategory category) const noexcept;
    const CharacterMakeParams& params() const noexcept { return m_params; }
    CharacterMakeParams& params() noexcept { return m_params; }

private:
    bool apply(CharMakeOptionCategory category, std::size_t index) noexcept;

    const CharMakeOptionCatalog* m_catalog = nullptr;
    std::array<std::size_t, 10> m_indices{};
    CharacterMakeParams m_params;
};

enum class CharMakeUiCommandKind : std::uint8_t {
    None,
    Rotate,
    CheckName,
    Submit,
    Cancel,
};

struct CharMakeUiCommand {
    CharMakeUiCommandKind kind = CharMakeUiCommandKind::None;
    CharMakeOptionCategory category = CharMakeOptionCategory::Sex;
    int direction = 0;
};

CharMakeUiCommand resolve_char_make_ui_command(
    const ClientUiActivation& activation) noexcept;

std::vector<std::uint8_t> legacy_character_name_check_payload(
    std::string_view name);

// Build the 59-byte legacy CHARACTERMAKEINFO payload (after MSGBASE) that
// agent_handler.cpp::handle_legacy_character_make parses:
//   [0..17)  Name[17] (16 chars + NUL, truncated, zero-padded)
//   [17..21) UserID (u32 LE) - filled from LoginResult.user_idx
//   [21]     SexType (u8)
//   [22]     BodyType (u8)
//   [23]     HairType (u8)
//   [24]     FaceType (u8)
//   [25]     StartArea (u8)
//   [26..30) bDuplCheck (u32 LE) - 0 (legacy client sends FALSE)
//   [30..50) WearedItemIdx[10] (10 * u16 LE)
//   [50]     StandingArrayNum (u8)
//   [51..55) Height (f32 LE)
//   [55..59) Width (f32 LE)
std::vector<std::uint8_t>
legacy_character_make_syn_payload(const CharacterMakeParams& params,
                                  std::uint32_t user_id);

// -------------------------------------------------------------------------
// CCharMake - eGS_CHARMAKE state.
// -------------------------------------------------------------------------
class CCharMake final : public CGameState,
                        public mxh::net::IConnectionHandler {
public:
    CCharMake();
    ~CCharMake() override;

    CCharMake(const CCharMake&)            = delete;
    CCharMake& operator=(const CCharMake&) = delete;

    // CGameState
    void Init(void* pInitParam) override;
    void Release() override;
    void Process() override;

    // mxh::net::IConnectionHandler
    bool on_connect(mxh::net::ConnectionId id,
                    const std::string& remote_addr) override;
    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& msg) override;
    void on_disconnect(mxh::net::ConnectionId id,
                       mxh::net::NetError reason) override;
    mxh::net::IEncryptor* encryptor_for(mxh::net::ConnectionId id) override;

    // Host bridge: receive the LoginResult captured by CLoginState.
    // Must be called before Start().
    void SetLoginResult(LoginResult r) { m_login = std::move(r); }

    // Public start hook.  Begins TCP connect to AgentServer.
    // Idempotent (second call is a no-op).
    void Start(CEngine* engine, bool use_hsel = false);

    // ---- Phase 1 §7.3 protocol-burst test hooks ------------------------
    // Direct dispatch (skips network recv thread) so the test can slam
    // the state with synthetic NameCheck / MakeAck packets and verify
    // the create flow without spinning up a real AgentServer.  Mirrors
    // the CInGameState / CLoginState hook pattern (commits a8e194da /
    // d3804f82).  Production code never calls these.
    void SetDispatchForTest(bool enabled) noexcept { m_dispatchEnabledForTest = enabled; }
    void HandleMessageForTest(const mxh::net::Message& msg) {
        if (!m_dispatchEnabledForTest) return;
        on_message({}, msg);
    }

    // M-R7.1 (G3 bug fix 2026-08-20): host reads the loaded cDialog
    // tree to render the 1:1 UI (CharMakeNewDlg.bin — 49 children).
    const std::vector<std::unique_ptr<mxh::ui::cDialog>>& ui_dialogs() const noexcept {
        return m_uiRuntime.dialogs();
    }
    ClientUiRuntime& ui_runtime() noexcept { return m_uiRuntime; }

    // Submit the creation form: validates + builds the 59-byte
    // CHARACTERMAKEINFO and sends CharacterMakeSyn.  Safe to call once
    // the agent connection is up; returns false if not connected or an
    // identical submit is already in flight.
    bool SubmitCharacter(const CharacterMakeParams& params);
    bool SubmitCurrentForm();
    bool RotateAppearanceOption(CharMakeOptionCategory category, int direction);
    bool CheckCurrentName();
    bool CancelCreation();
    bool OnMouseButton(bool left, bool down, std::int32_t x, std::int32_t y);
    bool OnMouseMove(std::int32_t x, std::int32_t y);
    bool OnKeyEvent(bool down, std::uint32_t key);
    bool OnChar(std::uint32_t ch);

    // Inspectors.
    bool        is_connected() const noexcept;
    bool        is_submitted() const noexcept { return m_makeSent; }
    bool        is_failed() const noexcept { return m_failed; }
    const std::string& failure_reason() const noexcept { return m_failureReason; }
    const LoginResult& login_result() const noexcept { return m_login; }
    const CharacterMakeFormModel& form_model() const noexcept { return m_formModel; }
    std::optional<bool> name_available() const noexcept { return m_nameAvailable; }

private:
    void send_make_syn();
    bool handle_ui_activation(const ClientUiActivation& activation);
    void refresh_option_text(CharMakeOptionCategory category);
    void refresh_sex_visibility();
    mxh::ui::cEditBox* name_edit() const noexcept;
    void invalidate_name_check() noexcept;
    void fail_with(const std::string& reason);

    CEngine*                 m_pEngine = nullptr;  // not owned
    std::unique_ptr<mxh::net::TcpClient> m_client;
    LoginResult              m_login;
    bool                     m_useHsel = false;
    std::unique_ptr<mxh::crypto::HselStreamCipher> m_hsel;
    ClientUiRuntime          m_uiRuntime;
    std::optional<CharMakeOptionCatalog> m_optionCatalog;
    CharacterMakeFormModel  m_formModel;
    std::optional<bool>      m_nameAvailable;
    std::string              m_checkedName;
    bool                     m_nameCheckPending = false;
    bool                     m_cancelSent = false;

    CharacterMakeParams      m_pending;     // captured by SubmitCharacter
    bool                     m_started  = false;
    bool                     m_connectAcked = false;
    bool                     m_makeSent = false;
    bool                     m_failed   = false;
    bool                     m_releasing = true;
    // Phase 1 §7.3: opt-in flag for cchar_make_state_test to call
    // on_message() directly.  When false, HandleMessageForTest is a
    // no-op so a missing test setup cannot accidentally exercise the
    // dispatch path.  Defaults to false.
    bool                     m_dispatchEnabledForTest = false;
    std::string              m_failureReason;
};

} // namespace mxh::client
