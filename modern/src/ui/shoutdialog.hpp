// shoutdialog.hpp — modern port of 墨香 CShoutDialog
// (shout message sender dialog: 1 cEditBox + 1 SEND
// button + item info state).
//
// 1:1 port of legacy `CShoutDialog` from
//   `墨香【源码】\[Client]MH\ShoutDialog.h` (832 B) and
//   `墨香【源码】\[Client]MH\ShoutDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_SHOUT_DLG (legacy cWindow
//     type tag); m_dwItemIdx = m_dwItemPos = 0.
//   - Dtor: empty body.
//   - Linking: resolve 1 cEditBox child (m_pMsgBox
//     by CHA_MSG id).
//   - SetItemInfo(DWORD, DWORD): inline setter for
//     m_dwItemIdx + m_dwItemPos.
//   - SendShoutMsgSyn: get edit text; if empty →
//     CHATMGR->AddMsg(903) + return FALSE; else
//     clear edit + FILTERTABLE->FilterChat check
//     (if filtered → CHATMGR->AddMsg(27) + return
//     FALSE); else sprintf SHOUTBASE_ITEMINFO msg
//     with hero name + buf, send via NETWORK, set
//     active false, reset m_dwItemIdx +
//     m_dwItemPos to 0; return TRUE.
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type = WT_SHOUT_DLG
//     drop, modern cWindow does not have m_type).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve cEditBox child by id.
//   - SetItemInfo: REAL inline setter.
//   - SendShoutMsgSyn: TODO (4-singleton: CHATMGR
//     + FILTERTABLE + HERO + NETWORK not ported,
//     R-12.x deferred). The 1:1 contract is
//     preserved: returns bool, early return on
//     empty/filtered, sends network message on
//     success.
//   - 2 state fields: m_dwItemIdx + m_dwItemPos
//     (1:1 with legacy).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 27th **Tier 2** dialog port (after
// cGuildNickNameDialog). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — all state lives in 4 global
// singletons (CHATMGR + FILTERTABLE + HERO + NETWORK).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cEditBox;

class cShoutDialog : public cDialog {
public:
    cShoutDialog();
    ~cShoutDialog() override;

    // ----- 1:1 with legacy CShoutDialog::Linking -----

    // 1:1 with legacy Linking. Resolve cEditBox
    // child (m_pMsgBox by kIdMsgBox) by id.
    void Linking();

    // ----- 1:1 with legacy CShoutDialog::SetItemInfo -----

    // 1:1 with legacy SetItemInfo (inline setter
    // for the item idx + pos state).
    void SetItemInfo(std::uint32_t itemIdx, std::uint32_t itemPos) noexcept;

    // ----- 1:1 with legacy CShoutDialog::SendShoutMsgSyn -----

    // 1:1 with legacy SendShoutMsgSyn. Returns
    // false on empty message or filtered message,
    // true on success. The whole method is TODO
    // (4-singleton: CHATMGR + FILTERTABLE + HERO +
    // NETWORK not ported, R-12.x deferred). When
    // the singletons are ported, the body becomes
    // the legacy code.
    bool SendShoutMsgSyn();

    // ----- 1:1 with legacy CShoutDialog::GetItemIdx -----

    std::uint32_t GetItemIdx() const noexcept { return m_dwItemIdx; }

    // ----- 1:1 with legacy CShoutDialog::GetItemPos -----

    std::uint32_t GetItemPos() const noexcept { return m_dwItemPos; }

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID
    // (CHA_MSG). Local 410 — distinct from 200-401
    // used by previous Tier 2 dialogs.
    static constexpr std::int32_t kIdMsgBox = 410;

private:
    // 1:1 with legacy m_pMsgBox (resolved in
    // Linking by CHA_MSG id).
    cEditBox* m_pMsgBox = nullptr;

    // 1:1 with legacy m_dwItemIdx + m_dwItemPos
    // (item context for the shout message).
    std::uint32_t m_dwItemIdx = 0;
    std::uint32_t m_dwItemPos = 0;
};

}  // namespace mxh::ui
