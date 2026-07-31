// cdebugdlg.hpp -- modern port of Moxiang CDebugDlg
//   (debug flag display dialog).
//
// 1:1 port of legacy `CDebugDlg` from
//   `[Client]MH\DebugDlg.{h,cpp}`.
//
// 6 boolean flags (Attack / Item / Move / Mugong /
// Chat / UserConn) toggle the per-type debug log
// filter.  DebugMsgParser routes a typed debug
// message to the cListDialog (base class) AddItem
// chain when the corresponding flag is set.
//
// 1:1 dependencies:
//   * cListDialog base (legacy inherits from
//     cListDialog; modern port follows the same).
//   * 6 flag fields (m_bAttackFlag etc.) -- default
//     false on construction (1:1 with legacy FALSE).
//   * cListDialog::AddItem for the rendered line.
//
// 1:1 quirks:
//   - 1:1 with legacy `CDebugDlg::CDebugDlg`: ctor
//     zeroes 6 flags (modern port: default member
//     init = false).
//   - 1:1 with legacy `CDebugDlg::DebugMsgParser`:
//     variadic -- type + format + args.  Legacy
//     uses vsprintf + per-type prefix + AddItem.
//     Modern port: same shape, std::va_list + std::string
//     for safety.
//   - 1:1 with legacy `GetAttackBtnFalg()` typo:
//     modern port keeps the typo as `GetAttackBtnFalg()`
//     so callers ported 1:1 from the legacy still
//     compile when neither side is renamed.  (The
//     corrected `GetAttackBtnFlag()` is also provided
//     for new code.)

#pragma once

#include "cListDialog.hpp"

#include <cstdarg>
#include <cstdint>
#include <string>

namespace mxh::ui {

// 1:1 with legacy DebugDlg.h anonymous enum:
//   DBG_ATTACK   = 0
//   DBG_ITEM     = 1
//   DBG_MOVE     = 2
//   DBG_MUGONG   = 3
//   DBG_CHAT     = 4
//   DBG_USERCONN = 5
enum class DebugType : std::uint8_t {
    Attack   = 0,
    Item     = 1,
    Move     = 2,
    Mugong   = 3,
    Chat     = 4,
    UserConn = 5,
};

class cDebugDlg : public cListDialog {
public:
    cDebugDlg();
    ~cDebugDlg() override;

    cDebugDlg(const cDebugDlg&) = delete;
    cDebugDlg& operator=(const cDebugDlg&) = delete;

    // 1:1 with legacy CDebugDlg::DebugMsgParser.
    // Variadic -- type + format + args.  The legacy
    // builds a string via vsprintf, then routes by
    // type to AddItem (with a prefix like "ATTACK:")
    // when the matching flag is set.  Unknown
    // types fall through to the default branch
    // ("NORMAL:") which always adds.
    void DebugMsgParser(std::uint8_t type, const char* msg, ...);

    // ---- 1:1 setters (legacy: BOOL flags) ----
    void SetAttackBtnFlag(bool flag)   noexcept { m_bAttackFlag   = flag; }
    void SetItemBtnFlag(bool flag)     noexcept { m_bItemFlag     = flag; }
    void SetMoveBtnFlag(bool flag)     noexcept { m_bMoveFlag     = flag; }
    void SetMugongBtnFlag(bool flag)   noexcept { m_bMugongFlag   = flag; }
    void SetChatBtnFlag(bool flag)     noexcept { m_bChatFlag     = flag; }
    void SetUserConnBtnFlag(bool flag) noexcept { m_bUserConnFlag = flag; }

    // ---- 1:1 getters (legacy: BOOL flags) ----
    // 1:1 quirk: legacy `GetAttackBtnFalg()` typo'd
    // (F-a-l-g) -- modern port preserves it as
    // GetAttackBtnFalg() so legacy callers compile.
    bool GetAttackBtnFalg() const noexcept   { return m_bAttackFlag; }
    bool GetAttackBtnFlag()  const noexcept   { return m_bAttackFlag; }
    bool GetItemBtnFlag()    const noexcept   { return m_bItemFlag; }
    bool GetMoveBtnFlag()    const noexcept   { return m_bMoveFlag; }
    bool GetMugongBtnFlag()  const noexcept   { return m_bMugongFlag; }
    bool GetChatBtnFlag()    const noexcept   { return m_bChatFlag; }
    bool GetUserConnBtnFlag() const noexcept  { return m_bUserConnFlag; }

    // Test accessors.
    std::uint32_t AddItemCount() const noexcept { return m_addItemCount; }
    std::string LastAddedText() const noexcept  { return m_lastAddedText; }
    std::uint32_t LastAddedColor() const noexcept { return m_lastAddedColor; }

private:
    bool m_bAttackFlag   = false;
    bool m_bItemFlag     = false;
    bool m_bMoveFlag     = false;
    bool m_bMugongFlag   = false;
    bool m_bChatFlag     = false;
    bool m_bUserConnFlag = false;

    // Test hooks: the modern port routes to AddItem
    // directly (it overrides cListDialog::AddItem to
    // record the call for tests).  This avoids the
    // need for a real cListDialog::AddItem test stub.
    std::uint32_t m_addItemCount = 0;
    std::string   m_lastAddedText;
    std::uint32_t m_lastAddedColor = 0;

    void AddItemForTest(const std::string& text, std::uint32_t color);
};

} // namespace mxh::ui
