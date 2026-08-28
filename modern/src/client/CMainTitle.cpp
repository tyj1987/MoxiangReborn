// mxh/client/CMainTitle.cpp
// Modern login-title state.  Owns the version check, logo/server-list
// presentation and the hand-off into the authenticated client flow while
// preserving the legacy public surface used by the surrounding state machine.
// Optional legacy-only decorations remain outside the playable path; the
// active IDDlg and network lifecycle are driven by the injected client context.

#include "CMainTitle.hpp"

#include "CEngine.hpp"

#include "mxh/log/mlog.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cEditBox.hpp"
#include "mxh/ui/cImage.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

namespace mxh::client {

namespace {
void clear_secret(std::string& value) noexcept {
    volatile char* bytes = value.empty() ? nullptr : value.data();
    for (std::size_t i = 0; bytes && i < value.size(); ++i) bytes[i] = '\0';
    value.clear();
}
}

namespace {

// 1:1 quirk: the legacy MHClient.cpp reads MHVerInfo.ver as a plain
// text file whose first line is the client version string (e.g.
// "NDSC08070301" for the Korean server).  The version is later
// included in MP_USERCONN_REQUEST_LOGIN.  We preserve the same
// format on the modern side; A.1.8 stores the string but the
// network layer (B.1) is what actually sends it.
constexpr const char* kMHVerInfoPath = "MHVerInfo.ver";

void readMHVerInfo(char out[32]) {
    std::ifstream in(kMHVerInfoPath);
    if (!in.is_open()) {
        // Fallback: the file may live next to the executable.  Try a
        // few common locations.
        for (const char* p : {"Resource/MHVerInfo.ver",
                              "Client/MHVerInfo.ver",
                              "MHVerInfo.bin"}) {
            std::ifstream in2(p);
            if (in2.is_open()) {
                in = std::move(in2);
                break;
            }
        }
    }
    if (!in.is_open()) {
        MLOG_WARN("CMainTitle: MHVerInfo.ver not found, using default version");
        std::strncpy(out, "MXRBN99999999", 32 - 1);
        out[32 - 1] = '\0';
        return;
    }
    std::string line;
    std::getline(in, line);
    if (line.empty()) {
        std::strncpy(out, "MXRBN99999999", 32 - 1);
    } else {
        std::strncpy(out, line.c_str(), 32 - 1);
    }
    out[32 - 1] = '\0';
    MLOG_INFO("CMainTitle: client version = %s", out);
}

constexpr std::uint16_t kLoginEditBytes = 17;  // MAX_NAME_LENGTH + 1
constexpr std::uint32_t kVkReturn = 0x0Du;
constexpr std::uint32_t kVkTab    = 0x09u;

void expand_dialog_to_content(mxh::ui::cDialog& dlg) {
    std::int32_t max_w = dlg.width();
    std::int32_t max_h = dlg.height();
    for (std::size_t i = 0; i < dlg.childCount(); ++i) {
        mxh::ui::cWindow* child = dlg.childAt(i);
        if (!child) continue;
        max_w = std::max(max_w, child->relX() +
                         static_cast<std::int32_t>(child->width()));
        max_h = std::max(max_h, child->relY() +
                         static_cast<std::int32_t>(child->height()));
    }
    if (auto* img = static_cast<mxh::ui::cImage*>(dlg.basicImage())) {
        const auto& rect = img->srcImageRect();
        if (!rect.isEmpty()) {
            max_w = std::max(max_w, rect.right - rect.left);
            max_h = std::max(max_h, rect.bottom - rect.top);
        }
    }
    if (max_w > dlg.width() || max_h > dlg.height()) {
        dlg.SetWH(max_w, max_h);
    }
}

} // namespace

LoginUiCommand resolve_login_ui_command(
    const ClientUiActivation& activation) noexcept {
    if (activation.legacy_id == "MT_OKBTN" ||
        activation.legacy_func == "MT_LogInOkBtnFunc") {
        return {LoginUiCommandKind::Submit};
    }
    if (activation.legacy_id == "MT_ENDBTN" ||
        activation.legacy_func == "MT_ExitBtnFunc") {
        return {LoginUiCommandKind::Exit};
    }
    return {};
}

CMainTitle::CMainTitle() = default;
CMainTitle::~CMainTitle() {
    clear_secret(m_password);
}

void CMainTitle::Init(void* /*pInitParam*/) {
    MLOG_INFO("CMainTitle::Init — booting into the login flow");
    readMHVerInfo(m_ClientVersion);

    // The legacy engine started the logo window + camera + intro
    // replay here.  A.1.8 just records the start time and marks
    // m_bInit so Process() can drive the state machine.
    m_dwStartTime = ::GetTickCount();
    m_bInit       = true;
    m_bServerList = false;

    setInitialized(true);
}

void CMainTitle::Release() {
    MLOG_INFO("CMainTitle::Release");
    clearPassword();
    m_uiRuntime.clear();
    m_submitRequested = false;
    m_pCamera         = nullptr;
    m_pLogoWindow     = nullptr;
    m_pAdvice         = nullptr;
    m_pServerListDlg  = nullptr;
    m_pIntroReplayDlg = nullptr;
    m_bInit           = false;
    setInitialized(false);
}

void CMainTitle::Process() {
    // Login/network ownership now lives in CLoginState.  The title state
    // remains responsible for presenting IDDlg and collecting user input;
    // this per-frame hook intentionally only maintains the active dialog.
    if (!m_bInit) return;
}

void CMainTitle::OnLoginError(std::uint32_t errorcode, std::uint32_t /*dwParam*/) {
    // The host transfers failed login attempts back to Title and presents
    // the server-provided reason through ClientUiRuntime::showMessage.
    // Keep this hook for legacy callers that report directly to the state.
    MLOG_WARN("CMainTitle::OnLoginError code=%u", errorcode);
    clearPassword();
    if (!m_bInit || !m_uiRuntime.isActive()) return;
    const std::string message = "登录失败（错误码 " +
        std::to_string(errorcode) + "），请检查账号和密码后重试。";
    if (!m_uiRuntime.showMessage(0x4D4C4552, message)) {
        MLOG_WARN("CMainTitle: unable to open login error dialog");
    }
}

void CMainTitle::OnDisconnect() {
    MLOG_INFO("CMainTitle::OnDisconnect");
    m_bDisconntinToDist   = true;
    m_dwDiconWaitTime     = ::GetTickCount();
    m_bWaitConnectToAgent = false;
    m_bServerList         = false;
}

void CMainTitle::Start(CEngine* engine,
                       std::string username,
                       std::string password) {
    if (!username.empty()) m_username = std::move(username);
    if (!password.empty()) m_password = std::move(password);
    m_submitRequested = false;

    if (engine && engine->playdh_root().has_value() && m_uiRuntime.empty()) {
        std::string ui_error;
        if (!m_uiRuntime.load(*engine->playdh_root(), "IDDlg.bin",
                              mxh::ui::ResolutionMode::Low800x600,
                              &ui_error)) {
            MLOG_WARN("CMainTitle: IDDlg.bin load failed: %s",
                      ui_error.c_str());
        } else {
            m_uiRuntime.activateAllLoadedDialogs();
            if (!m_uiRuntime.setDialogActive("MT_LOGINDLG", true)) {
                MLOG_WARN("CMainTitle: MT_LOGINDLG id not found after load");
            }
            if (auto* root = dynamic_cast<mxh::ui::cDialog*>(
                    m_uiRuntime.findWindowByLegacyId("MT_LOGINDLG"))) {
                expand_dialog_to_content(*root);
            }
            bind_login_edits();
        }
    } else if (!m_uiRuntime.empty()) {
        bind_login_edits();
    }
}

void CMainTitle::bind_login_edits() {
    auto configure = [](mxh::ui::cEditBox* edit, bool secret,
                        const std::string& text) {
        if (!edit) return;
        edit->InitEditbox(static_cast<std::uint16_t>(edit->width()),
                          kLoginEditBytes);
        edit->SetSecret(secret);
        if (!text.empty()) edit->SetEditText(text);
    };
    configure(id_edit(), false, m_username);
    configure(password_edit(), true, m_password);
    if (auto* id = id_edit()) {
        m_uiRuntime.onMouseButton(true, true, id->absX() + 1, id->absY() + 1);
        m_uiRuntime.onMouseButton(true, false, id->absX() + 1, id->absY() + 1);
    }
}

mxh::ui::cEditBox* CMainTitle::id_edit() const {
    return dynamic_cast<mxh::ui::cEditBox*>(
        m_uiRuntime.findWindowByLegacyId("MT_IDEDITBOX"));
}

mxh::ui::cEditBox* CMainTitle::password_edit() const {
    return dynamic_cast<mxh::ui::cEditBox*>(
        m_uiRuntime.findWindowByLegacyId("MT_PWDEDITBOX"));
}

void CMainTitle::sync_credentials_from_edits() {
    if (auto* id = id_edit()) m_username = id->editText();
    if (auto* pwd = password_edit()) m_password = pwd->editText();
}

bool CMainTitle::trySubmit() {
    sync_credentials_from_edits();
    if (m_username.empty() || m_password.empty()) {
        (void)m_uiRuntime.focusWindowByLegacyId(
            m_username.empty() ? "MT_IDEDITBOX" : "MT_PWDEDITBOX");
        (void)m_uiRuntime.showMessage(
            9202, "请输入账号和密码。");
        return false;
    }
    m_submitRequested = true;
    return true;
}

void CMainTitle::clearFields() {
    m_username.clear();
    clearPassword();
    m_submitRequested = false;
    if (auto* id = id_edit()) id->SetEditText("");
    if (auto* pwd = password_edit()) pwd->SetEditText("");
    if (auto* id = id_edit()) {
        m_uiRuntime.onMouseButton(true, true, id->absX() + 1, id->absY() + 1);
        m_uiRuntime.onMouseButton(true, false, id->absX() + 1, id->absY() + 1);
    }
}

void CMainTitle::clearPassword() {
    clear_secret(m_password);
    if (auto* pwd = password_edit()) pwd->ClearEditTextSecure();
}

bool CMainTitle::consumeSubmit() noexcept {
    if (!m_submitRequested) return false;
    m_submitRequested = false;
    return true;
}

bool CMainTitle::handle_ui_activation(const ClientUiActivation& activation) {
    const auto command = resolve_login_ui_command(activation);
    switch (command.kind) {
        case LoginUiCommandKind::Submit:
            return trySubmit();
        case LoginUiCommandKind::Exit:
            clearFields();
            return true;
        case LoginUiCommandKind::None:
        default:
            return false;
    }
}

bool CMainTitle::handle_edit_return() {
    auto* id = id_edit();
    auto* pwd = password_edit();
    if (id && id->hasFocus() && !id->editText().empty()) {
        m_uiRuntime.onKey(true, static_cast<std::int32_t>(kVkTab));
        return true;
    }
    if (pwd && pwd->hasFocus() && !pwd->editText().empty()) {
        return trySubmit();
    }
    return trySubmit();
}

bool CMainTitle::OnMouseButton(bool left, bool down,
                               std::int32_t x, std::int32_t y) {
    auto result = m_uiRuntime.onMouseButton(left, down, x, y);
    if (result.activation) handle_ui_activation(*result.activation);
    return result.consumed;
}

bool CMainTitle::OnMouseMove(std::int32_t x, std::int32_t y) {
    return m_uiRuntime.onMouseMove(x, y);
}

bool CMainTitle::OnKeyEvent(bool down, std::uint32_t key) {
    const bool consumed = m_uiRuntime.onKey(down, static_cast<std::int32_t>(key));
    bool activation_handled = false;
    if (auto activation = m_uiRuntime.consumeKeyActivation()) {
        handle_ui_activation(*activation);
        activation_handled = true;
    }
    if (!down) return consumed;
    if (key == kVkReturn && !activation_handled) return handle_edit_return() || consumed;
    return consumed;
}

bool CMainTitle::OnChar(std::uint32_t ch) {
    return m_uiRuntime.onChar(static_cast<std::int32_t>(ch));
}

} // namespace mxh::client
