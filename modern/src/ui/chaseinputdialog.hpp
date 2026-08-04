// chaseinputdialog.hpp — modern port of 墨香 CChaseinputDialog
// (chase input dialog: enter target player name for wanted
// chase item).
//
// 1:1 port of legacy `CChaseinputDialog` from
//   `墨香【源码】\[Client]MH\ChaseinputDialog.h` (497 B) and
//   `墨香【源码】\[Client]MH\ChaseinputDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_CHASEINPUT_DLG (legacy cWindow
//     type tag; modern cWindow / cDialog don't have
//     m_type, so modern port drops the ctor body).
//     m_LastChktime = 0.
//   - Linking: resolve cEditBox m_pEditName by id +
//     SetValidCheck(VCM_CHARNAME) (1:1 quirk: legacy
//     uses cIMEex VCM_CHARNAME; modern cEditBox supports
//     0/1/2/3 modes, closest is mode 2 = alpha only).
//   - SetActive override: 1:1 with base noexcept. Body:
//     base SetActive + if val clear edit text + reset
//     m_dwItemIdx to 0.
//   - SetItemIdx: 1:1 wrapper that sets m_dwItemIdx.
//   - WantedChaseSyn: rate-limit, validate the target name,
//     apply the GM filter and tracking-jin wanted-list gate,
//     send MP_ITEM_SHOPITEM_CHASE_SYN, then deactivate and
//     stamp m_LastChktime. Modern globals are host callbacks.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 14th **Tier 2** dialog port. The dialog is
// the simplest Tier 2 in P2-12 (1 child, 4 methods,
// 0 cTextArea dependencies — only cEditBox).
//
// 1:1 quirks preserved:
//   - Ctor drops m_type = WT_CHASEINPUT_DLG (legacy
//     cWindow type tag removed in Phase 6).
//   - Linking calls SetValidCheck(VCM_CHARNAME alias = 2)
//     — closest modern equivalent for the legacy
//     cIMEex character-name validator (same as
//     cMiniFriendDialog).
//   - SetActive matches base noexcept (R-12 polymorphic
//     virtual required).
//   - SetItemIdx is a 1:1 wrapper.
//   - WantedChaseSyn preserves the legacy branch order and
//     updates m_LastChktime only after the send dispatch.

#pragma once

#include "cdialog.hpp"

#include <cstddef>
#include <cstdint>

namespace mxh::ui {

class cEditBox;

class cChaseInputDialog : public cDialog {
public:
    // ----- Host-injected callbacks (legacy: gCurTime + CHATMGR + HERO + FILTERTABLE + WANTEDMGR + NETWORK) -----

    using GetCurrentTimeFn = std::uint32_t (*)(void* userData);

    // The host resolves the legacy CHATMGR message id to text and
    // adds it to CTC_SYSMSG.
    using AddSystemMessageFn = void (*)(std::int32_t chatMsgId,
                                        void* userData);

    using GetHeroObjectIdFn = std::uint32_t (*)(void* userData);
    using GetHeroObjectNameFn = const char* (*)(void* userData);

    // The input is the uppercase copy used by legacy FILTERTABLE.
    using FilterWordFn = bool (*)(const char* uppercaseName,
                                  void* userData);

    // Used only for eIncantation_Tracking_Jin.
    using IsWantedNameFn = bool (*)(const char* wantedName,
                                    void* userData);

    // The host serializes MSGBASE + SEND_CHASEBASE using the legacy
    // category/protocol bytes and pushes it to NETWORK. The return
    // value is intentionally ignored, matching legacy Send behavior.
    using SendChaseSynFn = bool (*)(std::uint32_t objectId,
                                    const char* wantedName,
                                    std::uint32_t itemIdx,
                                    void* userData);

    cChaseInputDialog();
    ~cChaseInputDialog() override;

    // ----- 1:1 with legacy CChaseinputDialog::Linking -----

    // Resolves cEditBox m_pEditName by id (kEditNameId=300)
    // + SetValidCheck(VCM_CHARNAME alias = 2).
    void Linking();

    // ----- 1:1 with legacy CChaseinputDialog::SetActive -----

    // 1:1 override: calls base SetActive + if val
    // clears the edit text + resets m_dwItemIdx.
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CChaseinputDialog::SetItemIdx -----

    // 1:1 wrapper that sets m_dwItemIdx.
    void SetItemIdx(std::uint32_t dwItem) noexcept {
        m_dwItemIdx = dwItem;
    }

    // ----- 1:1 with legacy CChaseinputDialog::WantedChaseSyn -----

    // Dispatch the chase syn. The legacy branch order is
    // preserved. Missing host callbacks make the corresponding
    // branch a safe no-op.
    void WantedChaseSyn();

    // Replace the legacy globals with host callbacks. All seven
    // callbacks are optional; null means that dependent branch is
    // silently skipped.
    void SetCallbacks(GetCurrentTimeFn getCurrentTime,
                      AddSystemMessageFn addSystemMessage,
                      GetHeroObjectIdFn getHeroObjectId,
                      GetHeroObjectNameFn getHeroObjectName,
                      FilterWordFn filterWord,
                      IsWantedNameFn isWantedName,
                      SendChaseSynFn sendChaseSyn,
                      void* userData = nullptr) noexcept;

    // ----- Accessors (used by tests) -----

    cEditBox* GetEditName() const noexcept { return m_pEditName; }
    std::uint32_t GetItemIdx() const noexcept { return m_dwItemIdx; }
    std::uint32_t GetLastChktime() const noexcept { return m_LastChktime; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kEditNameId = 300;  // was CHASE_EDITBOX

    // 1:1 quirk: legacy VCM_CHARNAME (from cIMEex.h) is
    // a character-name validator. The modern cEditBox
    // supports 0=none, 1=digits only, 2=alpha only,
    // 3=alnum. The closest modern equivalent for
    // VCM_CHARNAME is mode 2 (alpha only).
    static constexpr int kVcmCharnameAlias = 2;

    // Legacy CommonGameDefine.h / Protocol.h values.
    static constexpr std::size_t kMaxNameLength = 16;
    static constexpr std::size_t kMaxNameBufferLength = kMaxNameLength + 1;
    static constexpr std::uint32_t kRateLimitMilliseconds = 30000;
    static constexpr std::uint32_t kTrackingJinItemIdx = 55387;
    static constexpr std::int32_t kChatMsgRateLimited = 909;
    static constexpr std::int32_t kChatMsgSelfTarget = 911;
    static constexpr std::int32_t kChatMsgFilteredTarget = 919;
    static constexpr std::uint8_t kItemCategory = 5;
    static constexpr std::uint8_t kShopItemChaseSynProtocol = 154;

private:
    cEditBox*    m_pEditName  = nullptr;
    std::uint32_t m_LastChktime = 0;  // 1:1 with legacy
    std::uint32_t m_dwItemIdx   = 0;

    GetCurrentTimeFn     m_getCurrentTimeFn = nullptr;
    AddSystemMessageFn   m_addSystemMessageFn = nullptr;
    GetHeroObjectIdFn    m_getHeroObjectIdFn = nullptr;
    GetHeroObjectNameFn  m_getHeroObjectNameFn = nullptr;
    FilterWordFn         m_filterWordFn = nullptr;
    IsWantedNameFn       m_isWantedNameFn = nullptr;
    SendChaseSynFn       m_sendChaseSynFn = nullptr;
    void*                m_callbackUserData = nullptr;
};

}  // namespace mxh::ui
