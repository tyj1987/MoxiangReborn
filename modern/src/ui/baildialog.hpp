// baildialog.hpp - modern port of legacy CBailDialog
// (bail dialog: enter amount of bad fame to pay off +
// show the bail cost + minimum required).
//
// 1:1 port of legacy CBailDialog from
//   legacy [Client]MH/BailDialog.h (497 B) and
//   legacy [Client]MH/BailDialog.cpp.
//
// What the legacy does:
//   - Ctor: 2 children null + m_BadFame = 0.
//   - Linking: resolve cEditBox m_pBailEdtBox by id +
//     SetValidCheck(VCM_NUMBER) + SetAlign(TXT_RIGHT).
//     Resolve cTextArea m_pBailText by id + set its
//     script text to a formatted string (CHATMGR msg
//     644 + the bail cost formatted with AddComma).
//   - Open: if HERO->GetBadFame() > MIN_BADFAME_FOR_BAIL,
//     set the edit text to "0" + activate the dialog.
//     Else: show a WINDOWMGR msg box with CHATMGR msg
//     659. 1:1 quirk: legacy has SetActive(TRUE) (not
//     SetActive(true)) - the modern SetActive
//     override matches the base noexcept spec.
//   - Close: SetDisable(FALSE) + SetActive(FALSE) +
//     OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal).
//   - SetFame: read the edit text as a number (with
//     AddComma handling) + 4-singleton check +
//     WINDOWMGR msg box. Returns void.
//   - SetBadFrameSync: send MSG_FAME network message +
//     Close.
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_pBailEdtBox = NULL +
//     m_pBailText = NULL + m_BadFame = 0 - modern port
//     uses nullptr + default member init).
//   - Dtor: empty (no-op).
//   - Linking: REAL 1:1 port via cWindow::findWindowById
//     + SetValidCheck(1) = VCM_NUMBER + SetAlign(Right)
//     + SetScriptText (placeholder text until CHATMGR
//     is ported).
//   - Open: REAL 1:1 port via host-injected
//     GetHeroBadFameFn + ShowMsgBoxFn. If the heros
//     bad fame exceeds kMinBadFameForBail, the dialog
//     sets its edit box text to "0" + activates itself.
//     Otherwise it shows the failure msg box and stays
//     hidden. If callbacks are not wired, the dialog
//     remains hidden (safe no-op).
//   - Close: REAL 1:1 port - SetDisable(FALSE) +
//     SetActive(FALSE) + EndDealStateFn() (replaces
//     OBJECTSTATEMGR->EndObjectState). The state-end
//     branch is silently skipped if EndDealStateFn is
//     null.
//   - SetFame: REAL 1:1 port via ParseAmountFn +
//     GetHeroBadFameFn + GetHeroMoneyFn + ShowMsgBoxFn
//     + SetDisable(TRUE) on each failure + the final
//     confirmation msg box. Returns early if amount is 0.
//   - SetBadFrameSync: REAL 1:1 port via GetHeroIdFn +
//     SendBadFameFn + Close() after dispatch. Returns
//     early if m_BadFame <= 0.
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 16th Tier 2 dialog port. The dialog
// exercises cTextArea (already ported in 0.13.23) +
// cEditBox (already ported in 6.1).
//
// 1:1 quirks preserved:
//   - Ctor initializes m_pBailEdtBox = m_pBailText = NULL
//     + m_BadFame = 0 (modern port uses nullptr +
//     default member init).
//   - Linking calls SetValidCheck(VCM_NUMBER) +
//     SetAlign(TXT_RIGHT) on the cEditBox. The
//     SetScriptText call uses placeholder text
//     "BAIL_TEXT_PLACEHOLDER" (legacy uses
//     CHATMGR->GetChatMsg(644) + AddComma-formatted
//     bail cost).
//   - Open: legacy has if (HERO->GetBadFame() > ...)
//     with a 3-singleton dispatch. Modern port
//     dispatches via host callbacks.
//   - Close: legacy has 3-singleton dispatch. Modern
//     port dispatches via SetDisable + SetActive
//     (direct base calls) + EndDealStateFn.
//   - SetFame: 4-singleton dispatch. Modern port
//     dispatches via host callbacks.
//   - SetBadFrameSync: 3-singleton dispatch. Modern
//     port dispatches via host callbacks.
//   - 1:1 quirk: Open does not call SetDisable.
//   - 1:1 quirk: SetFame returns void (legacy: the
//     function does not return a value; the bail flow
//     continues asynchronously after the user clicks
//     the YES button in the MBI_BAILNOTICEMSG msg box).
//   - 1:1 quirk: SetBadFrameSync does not return a
//     value; failure to send is not propagated to the
//     caller.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cEditBox;
class cTextArea;

class cBailDialog : public cDialog {
public:
    // ----- Host-injected callbacks (legacy: HERO + WINDOWMGR + CHATMGR + NETWORK + OBJECTSTATEMGR + AddComma globals) -----

    // 1:1 with legacy HERO->GetBadFame(). Used by Open
    // (compare against kMinBadFameForBail) + SetFame
    // (compare against m_BadFame + kMinBadFameForBail).
    using GetHeroBadFameFn = std::uint32_t (*)(void* userData);

    // 1:1 with legacy HERO->GetMoney(). Used by SetFame
    // (compare against m_BadFame * kBailPrice).
    using GetHeroMoneyFn = std::uint32_t (*)(void* userData);

    // 1:1 with legacy HERO->GetID(). Used by
    // SetBadFrameSync to stamp MSG_FAME.dwObjectID.
    using GetHeroIdFn = std::uint32_t (*)(void* userData);

    // 1:1 with legacy atoi(RemoveComma(text)). Parses
    // the bail amount the user typed in the edit box
    // (stripping comma thousands separators first).
    using ParseAmountFn = std::uint32_t (*)(const char* text,
                                            void* userData);

    // 1:1 with legacy WINDOWMGR->MsgBox(windowId, btnType,
    // chatMsgId, ...). The host is responsible for
    // formatting the chat message + AddComma-formatting
    // the numeric parameters + creating the modal msg box
    // and wiring its OK/YES-NO callback. param1 / param2
    // are zero when the legacy call site passes no
    // numeric params (e.g. CHATMGR->GetChatMsg(117)).
    // Returns true if the msg box was created (the legacy
    // code uses the pointer truthiness).
    using ShowMsgBoxFn = bool (*)(std::int32_t windowId,
                                  std::int32_t buttonType,
                                  std::int32_t msgId,
                                  std::uint32_t param1,
                                  std::uint32_t param2,
                                  void* userData);

    // 1:1 with legacy OBJECTSTATEMGR->EndObjectState(HERO,
    // eObjectState_Deal). Used by Close.
    using EndDealStateFn = void (*)(void* userData);

    // 1:1 with legacy NETWORK->Send(&msg, sizeof(msg)) for
    // the MP_CHAR / MP_CHAR_BADFAME_SYN packet (Category
    // = Character = 3, sub-protocol = 45). The host is
    // responsible for serializing the MSG_FAME wire
    // payload {dwObjectID, Fame} and pushing onto the
    // network queue. The return value is intentionally ignored so Close() is not gated by the send result.
    using SendBadFameFn = bool (*)(std::uint32_t objectId,
                                  std::uint32_t fame,
                                  void* userData);

    // ----- 1:1 with legacy CommonGameDefine.h + WindowIDs.h + cMsgBox.h enum values -----

    // MIN_BADFAME_FOR_BAIL = 100 (1:1 with legacy
    // [CC]Header/CommonGameDefine.h).
    static constexpr std::uint32_t kMinBadFameForBail = 100;

    // BAIL_PRICE = 10000 (1:1 with legacy
    // [CC]Header/CommonGameDefine.h).
    static constexpr std::uint32_t kBailPrice = 10000;

    // MBI_BAILNOTICEMSG = 4036 (1:1 with legacy
    // [Client]MH/WindowIDs.h WINDOW_ID sequence).
    static constexpr std::int32_t kMbiBailNoticeMsg = 4036;

    // MBI_BAILNOTICEERR = 4037 (1:1).
    static constexpr std::int32_t kMbiBailNoticeErr = 4037;

    // MBI_BAILLOWBADFAME = 4038 (1:1).
    static constexpr std::int32_t kMbiBailLowBadFame = 4038;

    // MBT_OK = 1 (1:1 with legacy
    // [Client]MH/cMsgBox.h enum eMBType).
    static constexpr std::int32_t kMbtOk = 1;

    // MBT_YESNO = 2 (1:1).
    static constexpr std::int32_t kMbtYesNo = 2;

    // Chat message IDs (1:1 with legacy CHATMGR msg
    // table; documented for the host to format with
    // CHATMGR->GetChatMsg(id)).
    static constexpr std::int32_t kChatMsgLowBadFame = 659;
    static constexpr std::int32_t kChatMsgNoticeBadFame = 648;
    static constexpr std::int32_t kChatMsgNoticeMoney = 117;
    static constexpr std::int32_t kChatMsgConfirmBail = 645;
    static constexpr std::int32_t kChatMsgBailText = 644;

    cBailDialog();
    ~cBailDialog() override;

    // ----- 1:1 with legacy CBailDialog::Linking -----

    // Resolves 2 children by id (kBailEditBoxId=320,
    // kBailTextId=321) + SetValidCheck(VCM_NUMBER) +
    // SetAlign(TXT_RIGHT) on the cEditBox +
    // SetScriptText on the cTextArea (placeholder text
    // until CHATMGR is ported).
    void Linking();

    // ----- 1:1 with legacy CBailDialog::Open / Close -----

    // Open: 1:1 wrapper. If GetHeroBadFameFn() > kMinBadFameForBail,
    // set the edit text to "0" + activate the dialog.
    // Otherwise show the failure msg box (MBI_BAILLOWBADFAME,
    // kChatMsgLowBadFame, AddComma(kMinBadFameForBail)).
    // No-op safe when GetHeroBadFameFn is null.
    void Open();

    // Close: 1:1 wrapper. SetDisable(FALSE) +
    // SetActive(FALSE) + EndDealStateFn(). The state-end
    // branch is silently skipped when EndDealStateFn is
    // null.
    void Close();

    // ----- 1:1 with legacy CBailDialog::SetFame / SetBadFrameSync -----

    // SetFame: 1:1 wrapper. Parses the edit text via
    // ParseAmountFn, validates against the heros bad fame
    // + money, and shows the appropriate msg box (error
    // for insufficient fame, error for insufficient money,
    // confirmation for success). Sets the dialog disabled
    // on each failure so the user cannot re-click until
    // the msg box is dismissed. No-op safe when
    // ParseAmountFn is null (the edit text parse fails).
    void SetFame();

    // SetBadFrameSync: 1:1 wrapper. Sends the bad-fame
    // resolution request via SendBadFameFn(GetHeroIdFn(),
    // m_BadFame) + Close() after dispatch. Returns early
    // when m_BadFame <= 0. No-op safe when SendBadFameFn
    // or GetHeroIdFn is null.
    void SetBadFrameSync();

    // ----- Host callback wiring (test seam + production wiring share the same API) -----

    // Replace the legacy HERO / WINDOWMGR / CHATMGR /
    // NETWORK / OBJECTSTATEMGR / AddComma globals with
    // host-injected function pointers + opaque userData.
    // All seven pointers are optional; null means the
    // corresponding branch is silently skipped.
    void SetCallbacks(GetHeroBadFameFn getHeroBadFame,
                      GetHeroMoneyFn getHeroMoney,
                      GetHeroIdFn getHeroId,
                      ParseAmountFn parseAmount,
                      ShowMsgBoxFn showMsgBox,
                      EndDealStateFn endDealState,
                      SendBadFameFn sendBadFame,
                      void* userData = nullptr) noexcept;

    // ----- Accessors (used by tests) -----

    cEditBox*    GetBailEditBox() const noexcept { return m_pBailEdtBox; }
    cTextArea*   GetBailText()    const noexcept { return m_pBailText;   }
    std::uint32_t GetBadFame()    const noexcept { return m_BadFame;     }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kBailEditBoxId = 320;  // was BAIL_BAILEDITBOX
    static constexpr std::int32_t kBailTextId    = 321;  // was BAIL_TEXTAREA

private:
    cEditBox*     m_pBailEdtBox = nullptr;
    cTextArea*    m_pBailText   = nullptr;
    std::uint32_t m_BadFame     = 0;

    // Host-injected callbacks (see SetCallbacks doc).
    GetHeroBadFameFn m_getHeroBadFameFn = nullptr;
    GetHeroMoneyFn   m_getHeroMoneyFn   = nullptr;
    GetHeroIdFn      m_getHeroIdFn      = nullptr;
    ParseAmountFn    m_parseAmountFn    = nullptr;
    ShowMsgBoxFn     m_showMsgBoxFn     = nullptr;
    EndDealStateFn   m_endDealStateFn   = nullptr;
    SendBadFameFn    m_sendBadFameFn    = nullptr;
    void*            m_callbackUserData = nullptr;
};

}  // namespace mxh::ui
