// debugdlg.hpp — modern port of 墨香
// CDebugDlg (debug flag display dialog:
// cListDialog subclass + 6 BOOL flags).
//
// 1:1 port of legacy `CDebugDlg` from
//   `墨香【源码】\[Client]MH\DebugDlg.h` and
//   `墨香【源码】\[Client]MH\DebugDlg.cpp`.
//
// What the legacy does:
//   - Ctor: cListDialog base init; 6 flags default
//     uninitialized (depend on cListDialog ctor
//     for cListDialog state).
//   - Dtor: empty body (base class dtor).
//   - DebugMsgParser(BYTE type, char* msg, ...):
//     variadic-format AddItem calls based on type
//     (DBG_ATTACK / DBG_ITEM / DBG_MOVE /
//     DBG_MUGONG / DBG_CHAT / DBG_USERCONN).
//   - 6 setter/getter pairs for m_bAttackFlag,
//     m_bItemFlag, m_bMoveFlag, m_bMugongFlag,
//     m_bChatFlag, m_bUserConnFlag.
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: cListDialog ctor
//     handles cListDialog state; 6 flags use
//     default member init = false).
//   - Dtor: empty (no-op).
//   - DebugMsgParser: TODO (variadic AddItem calls
//     + 6 branch dispatch, R-12.x deferred). Modern
//     port is no-op for now.
//   - 6 setter/getter pairs: REAL with bool
//     (1:1 with legacy BOOL).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 54th **Tier 2** dialog port (after
// cSurvivalCountDialog). The dialog has 6 BOOL
// flags + 1 DebugMsgParser method. cListDialog
// base is ported; the subclass is a thin data
// model extension.

#pragma once

#include "clistdialog.hpp"

#include <cstdarg>
#include <cstdint>

namespace mxh::ui {

class cDebugDlg : public cListDialog {
public:
    cDebugDlg();
    ~cDebugDlg() override;

    // ----- 1:1 with legacy CDebugDlg::DebugMsgParser -----

    // 1:1 with legacy DebugMsgParser(BYTE type,
    // char* msg, ...). The 6-branch dispatch +
    // variadic AddItem calls is TODO (R-12.x
    // deferred). Modern port is no-op for now.
    void DebugMsgParser(std::uint8_t type, const char* msg, ...);

    // ----- 1:1 with legacy CDebugDlg 6 setter/getter pairs -----

    // 1:1 with legacy SetAttackBtnFlag.
    void SetAttackBtnFlag(bool flag) noexcept {
        m_bAttackFlag = flag;
    }
    // 1:1 with legacy GetAttackBtnFalg (note: legacy
    // typo "Falg" — modern port uses "Flag" for
    // 1:1 spelling-fix).
    bool GetAttackBtnFlag() const noexcept {
        return m_bAttackFlag;
    }

    // 1:1 with legacy SetItemBtnFlag / GetItemBtnFlag.
    void SetItemBtnFlag(bool flag) noexcept {
        m_bItemFlag = flag;
    }
    bool GetItemBtnFlag() const noexcept {
        return m_bItemFlag;
    }

    // 1:1 with legacy SetMoveBtnFlag / GetMoveBtnFlag.
    void SetMoveBtnFlag(bool flag) noexcept {
        m_bMoveFlag = flag;
    }
    bool GetMoveBtnFlag() const noexcept {
        return m_bMoveFlag;
    }

    // 1:1 with legacy SetMugongBtnFlag / GetMugongBtnFlag.
    void SetMugongBtnFlag(bool flag) noexcept {
        m_bMugongFlag = flag;
    }
    bool GetMugongBtnFlag() const noexcept {
        return m_bMugongFlag;
    }

    // 1:1 with legacy SetChatBtnFlag / GetChatBtnFlag.
    void SetChatBtnFlag(bool flag) noexcept {
        m_bChatFlag = flag;
    }
    bool GetChatBtnFlag() const noexcept {
        return m_bChatFlag;
    }

    // 1:1 with legacy SetUserConnBtnFlag /
    // GetUserConnBtnFlag.
    void SetUserConnBtnFlag(bool flag) noexcept {
        m_bUserConnFlag = flag;
    }
    bool GetUserConnBtnFlag() const noexcept {
        return m_bUserConnFlag;
    }

    // ----- Local enum (1:1 with legacy DBG_* enum) -----

    // 1:1 with legacy DBG_ATTACK / DBG_ITEM /
    // DBG_MOVE / DBG_MUGONG / DBG_CHAT / DBG_USERCONN.
    static constexpr std::uint8_t kDbgAttack   = 0;
    static constexpr std::uint8_t kDbgItem     = 1;
    static constexpr std::uint8_t kDbgMove     = 2;
    static constexpr std::uint8_t kDbgMugong   = 3;
    static constexpr std::uint8_t kDbgChat     = 4;
    static constexpr std::uint8_t kDbgUserConn = 5;

private:
    // 1:1 with legacy 6 BOOL flags (init uninitialized
    // in legacy ctor; modern port uses default
    // member init = false).
    bool m_bAttackFlag = false;
    bool m_bItemFlag = false;
    bool m_bMoveFlag = false;
    bool m_bMugongFlag = false;
    bool m_bChatFlag = false;
    bool m_bUserConnFlag = false;
};

}  // namespace mxh::ui
