// unionnotedlg.hpp — modern port of 墨香
// CUnionNoteDialog (guild union note sender dialog:
// 1 cEditBox + 1 cTextArea + 1 CItem state +
// m_bUse flag).
//
// 1:1 port of legacy `CUnionNoteDlg` from
//   `墨香【源码】\[Client]MH\UnionNoteDlg.h` and
//   `墨香【源码】\[Client]MH\UnionNoteDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_bUse = FALSE.
//   - Dtor: empty body.
//   - Linking: resolve 1 cTextArea (m_pNoteText
//     by AN_TEXTREA); SetEnterAllow(FALSE);
//     SetScriptText("").
//   - Show(CItem* pItem): checks HERO guild idx +
//     rank + union idx + pItem + m_bUse (4-singleton
//     dispatch via CHATMGR); m_pItem = pItem;
//     SetActive(TRUE).
//   - Use(): m_bUse = FALSE; m_pNoteText->SetScriptText("");
//     send MSG_ITEM_USE_SYN via NETWORK; ITEMMGR++.
//   - OnActionEvnet (typo): 2 button case —
//     AN_SENDOKBTN → send MSG_GUILD_SEND_NOTE via
//     NETWORK; SetActive(FALSE). AN_CANCELBTN →
//     SetActive(FALSE).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_bUse default
//     = false via member init).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve cTextArea by id,
//     SetEnterAllow(false), SetScriptText("").
//   - Show / Use / OnActionEvent: REAL through host
//     callbacks, preserving all guild gates, item-use
//     fields/count, union-note fields, and close paths.
//   - 1:1 quirk: legacy ctor m_bUse = FALSE;
//     modern uses default member init (m_bUse = false).
//   - 1:1 quirk: m_pTitleEdit is declared in legacy
//     header but never used in cpp; modern port
//     preserves the declaration for 1:1 parity.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 44th **Tier 2** dialog port (after
// cMPGuageDialog). The dialog has 1 cTextArea
// (m_pNoteText) + 1 cEditBox (m_pTitleEdit,
// unused) + 1 CItem (forward-declared) +
// m_bUse flag. Legacy singleton operations are
// supplied through optional host callbacks.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cEditBox;
class cTextArea;

class cUnionNoteDlg : public cDialog {
public:
    cUnionNoteDlg();
    ~cUnionNoteDlg() override;

    // ----- 1:1 with legacy CUnionNoteDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 1 cTextArea
    // (m_pNoteText by kIdNoteText), call
    // SetEnterAllow(false), SetScriptText("").
    void Linking();

    // ----- 1:1 with legacy CUnionNoteDlg::Show -----

    using AddSystemMessageFn = void (*)(std::int32_t messageId,
                                          void* userData);
    using GetHeroDwordFn = std::uint32_t (*)(void* userData);
    using GetHeroRankFn = std::int32_t (*)(void* userData);
    using GetHeroNameFn = const char* (*)(void* userData);
    using GetItemWordFn = std::uint16_t (*)(void* item, void* userData);
    using GetItemPositionFn = std::uint32_t (*)(void* item, void* userData);
    using SendItemUseFn = void (*)(std::uint32_t objectId,
                                   std::uint16_t itemIdx,
                                   std::uint32_t position,
                                   void* userData);
    using SendUnionNoteFn = void (*)(std::uint32_t objectId,
                                     std::uint32_t unionId,
                                     const char* fromName,
                                     const char* note,
                                     void* userData);
    using IncrementItemUseCountFn = void (*)(void* userData);

    void SetCallbacks(AddSystemMessageFn addSystemMessage,
                      GetHeroDwordFn getGuildIdx,
                      GetHeroRankFn getGuildMemberRank,
                      GetHeroDwordFn getGuildUnionIdx,
                      GetHeroDwordFn getHeroObjectId,
                      GetHeroNameFn getHeroName,
                      GetItemWordFn getItemIdx,
                      GetItemPositionFn getItemPosition,
                      SendItemUseFn sendItemUse,
                      SendUnionNoteFn sendUnionNote,
                      IncrementItemUseCountFn incrementItemUseCount,
                      void* userData = nullptr) noexcept;

    void Show(void* pItem);

    // ----- 1:1 with legacy CUnionNoteDlg::Use -----

    // Clears use/text state, sends the item-use
    // fields, then increments the debug use count.
    void Use();

    // ----- 1:1 with legacy CUnionNoteDlg::OnActionEvnet -----

    // 1:1 with legacy OnActionEvnet (typo'd):
    // SEND sends union-note fields and falls through
    // to the same close path as CANCEL.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- 1:1 with legacy m_bUse getter -----

    // 1:1 with legacy m_bUse access (used by tests
    // to verify the flag transitions).
    bool IsUse() const noexcept { return m_bUse; }

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h AN_TEXTREA (620).
    // Local 620 — distinct from 200-620 used by
    // previous Tier 2 dialogs.
    static constexpr std::int32_t kIdNoteText = 620;

    // 1:1 with legacy AN_TITLEEDIT (declared in
    // header but unused in cpp). Local 621.
    static constexpr std::int32_t kIdTitleEdit = 621;

    // 1:1 with legacy AN_SENDOKBTN + AN_CANCELBTN.
    static constexpr std::int32_t kIdSendOkBtn   = 622;
    static constexpr std::int32_t kIdCancelBtn   = 623;
    static constexpr std::int32_t kGuildMaster = 50;
    static constexpr std::int32_t kGuildViceMaster = 40;
    static constexpr std::int32_t kNoGuildMessageId = 35;
    static constexpr std::int32_t kInvalidRankMessageId = 1100;
    static constexpr std::int32_t kNoUnionMessageId = 1103;
    static constexpr std::int32_t kInvalidItemMessageId = 786;
    static constexpr std::int32_t kAlreadyUsingMessageId = 752;
    static constexpr std::uint32_t kWeBtnClick = 0x0001u;

private:
    AddSystemMessageFn m_addSystemMessage = nullptr;
    GetHeroDwordFn m_getGuildIdx = nullptr;
    GetHeroRankFn m_getGuildMemberRank = nullptr;
    GetHeroDwordFn m_getGuildUnionIdx = nullptr;
    GetHeroDwordFn m_getHeroObjectId = nullptr;
    GetHeroNameFn m_getHeroName = nullptr;
    GetItemWordFn m_getItemIdx = nullptr;
    GetItemPositionFn m_getItemPosition = nullptr;
    SendItemUseFn m_sendItemUse = nullptr;
    SendUnionNoteFn m_sendUnionNote = nullptr;
    IncrementItemUseCountFn m_incrementItemUseCount = nullptr;
    void* m_callbackUserData = nullptr;

    // 1:1 with legacy m_pTitleEdit (declared in
    // header but never used in cpp). Modern port
    // preserves the field for 1:1 parity.
    cEditBox* m_pTitleEdit = nullptr;

    // 1:1 with legacy m_pNoteText (resolved in
    // Linking by AN_TEXTREA id).
    cTextArea* m_pNoteText = nullptr;

    // 1:1 with legacy m_pItem. Item properties are
    // extracted by host callbacks to avoid coupling.
    void* m_pItem = nullptr;

    // 1:1 with legacy m_bUse (BOOL; init FALSE
    // in ctor). Modern uses bool (default member
    // init = false).
    bool m_bUse = false;
};

}  // namespace mxh::ui
