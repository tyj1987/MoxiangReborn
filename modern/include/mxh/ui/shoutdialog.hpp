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
//   - SendShoutMsgSyn: REAL through optional host
//     callbacks, preserving edit clearing, filtering,
//     message formatting, WORD casts, send, close,
//     item-state reset, and return values.
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

#include <cstddef>
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

    using AddSystemMessageFn = void (*)(std::int32_t messageId,
                                          void* userData);
    using FilterChatFn = bool (*)(const char* message, void* userData);
    using GetHeroNameFn = const char* (*)(void* userData);
    using GetHeroObjectIdFn = std::uint32_t (*)(void* userData);
    using SendShoutFn = void (*)(std::uint32_t objectId,
                                 std::uint16_t itemIdx,
                                 std::uint16_t itemPos,
                                 const char* message,
                                 void* userData);

    void SetCallbacks(AddSystemMessageFn addSystemMessage,
                      FilterChatFn filterChat,
                      GetHeroNameFn getHeroName,
                      GetHeroObjectIdFn getHeroObjectId,
                      SendShoutFn sendShout,
                      void* userData = nullptr) noexcept;

    // Returns false on empty/filtered/missing-host
    // paths and true after sending and closing.
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
    static constexpr std::size_t kMaxShoutLength = 60;
    static constexpr std::int32_t kEmptyMessageId = 903;
    static constexpr std::int32_t kFilteredMessageId = 27;

private:
    AddSystemMessageFn m_addSystemMessage = nullptr;
    FilterChatFn m_filterChat = nullptr;
    GetHeroNameFn m_getHeroName = nullptr;
    GetHeroObjectIdFn m_getHeroObjectId = nullptr;
    SendShoutFn m_sendShout = nullptr;
    void* m_callbackUserData = nullptr;

    // 1:1 with legacy m_pMsgBox (resolved in
    // Linking by CHA_MSG id).
    cEditBox* m_pMsgBox = nullptr;

    // 1:1 with legacy m_dwItemIdx + m_dwItemPos
    // (item context for the shout message).
    std::uint32_t m_dwItemIdx = 0;
    std::uint32_t m_dwItemPos = 0;
};

}  // namespace mxh::ui
