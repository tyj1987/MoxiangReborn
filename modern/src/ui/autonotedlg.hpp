// autonotedlg.hpp — modern port of 墨香
// CAutoNoteDlg (auto note / auto reply dialog:
// 1 cTextArea + 1 cButton + 1 cListDialog).
//
// 1:1 port of legacy `CAutoNoteDlg` from
//   `墨香【源码】\[Client]MH\AutoNoteDlg.h` and
//   `墨香【源码】\[Client]MH\AutoNoteDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_pTextAreaManual=NULL; m_pBtnAsk=NULL;
//     m_pListAuto=NULL.
//   - Dtor: if m_pListAuto, m_pListAuto->RemoveAll().
//   - Linking: resolve 1 cTextArea
//     (m_pTextAreaManual by AND_TEXTAREA_MANUAL)
//     + 1 cButton (m_pBtnAsk by AND_BTN_ASK) +
//     1 cListDialog (m_pListAuto by AND_LIST_AUTO);
//     SetScriptText on m_pTextAreaManual with
//     CHATMGR->GetChatMsg(1721); SetTextColor.
//   - OnActionEvent: WE_BTNCLICK & lId == AND_BTN_ASK
//     → resolve pObject via OBJECTMGR; check
//     pObject + eObjectKind_Player + pObject != HERO
//     (in #ifndef _GMTOOL_); call
//     AUTONOTEMGR->AskToAutoUser.
//   - AddAutoList(strName, strDate): sprintf
//     "%-16s %s" into buf; m_pListAuto->AddItem(buf).
//   - SetActiveTestClient: SetActive(TRUE); 35-loop
//     sprintf "%d %-16s %s" → m_pListAuto->AddItem.
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: null-init via default
//     member init).
//   - Dtor: empty (m_pListAuto unique_ptr auto-destroys
//     and cListDialog destructor calls RemoveAll
//     automatically; modern cListDialog doesn't have
//     an explicit RemoveAll in dtor but the modern
//     port stores it as a cDialog child so dtor chain
//     handles cleanup).
//   - Linking: REAL — resolve 3 children by id,
//     SetScriptText with kAutoNoteManualText
//     placeholder for CHATMGR msg 1721, SetTextColor
//     (gray = RGB_HALF(128,128,128)).
//   - OnActionEvent: REAL through optional host
//     callbacks, preserving selection/player/self
//     gates and AskToAutoUser random payload.
//   - AddAutoList: REAL — sprintf "%-16s %s" + AddItem.
//   - SetActiveTestClient: REAL — 35-loop sprintf +
//     AddItem + SetActive(true).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 45th **Tier 2** dialog port (after
// cUnionNoteDlg). The dialog has 1 cTextArea +
// 1 cButton + 1 cListDialog. Legacy singleton
// operations are supplied through host callbacks.

#pragma once

#include "cdialog.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

class cTextArea;
class cButton;
class cListDialog;

class cAutoNoteDlg : public cDialog {
public:
    cAutoNoteDlg();
    ~cAutoNoteDlg() override;

    // ----- 1:1 with legacy CAutoNoteDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 1 cTextArea
    // (m_pTextAreaManual by kIdTextAreaManual) +
    // 1 cButton (m_pBtnAsk by kIdBtnAsk) +
    // 1 cListDialog (m_pListAuto by kIdListAuto).
    // Calls SetScriptText on m_pTextAreaManual with
    // kAutoNoteManualText placeholder for CHATMGR
    // msg 1721 + SetTextColor (gray).
    void Linking();

    // ----- 1:1 with legacy CAutoNoteDlg::OnActionEvent -----

    using GetSelectedObjectFn = void* (*)(void* userData);
    using GetObjectKindFn = std::int32_t (*)(void* object, void* userData);
    using GetObjectIdFn = std::uint32_t (*)(void* object, void* userData);
    using IsHeroObjectFn = bool (*)(void* object, void* userData);
    using AddSystemMessageFn = void (*)(std::int32_t messageId,
                                        void* userData);
    using GetRandomPercentFn = std::uint32_t (*)(void* userData);
    using AskToAutoUserFn = void (*)(std::uint32_t objectId,
                                     std::uint32_t randomValue,
                                     void* userData);

    void SetCallbacks(GetSelectedObjectFn getSelectedObject,
                      GetObjectKindFn getObjectKind,
                      GetObjectIdFn getObjectId,
                      IsHeroObjectFn isHeroObject,
                      AddSystemMessageFn addSystemMessage,
                      GetRandomPercentFn getRandomPercent,
                      AskToAutoUserFn askToAutoUser,
                      void* userData = nullptr) noexcept;

    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- 1:1 with legacy CAutoNoteDlg::AddAutoList -----

    // 1:1 with legacy AddAutoList(strName, strDate).
    // sprintf "%-16s %s" into buf + AddItem.
    void AddAutoList(const char* strName, const char* strDate);

    // ----- 1:1 with legacy CAutoNoteDlg::SetActiveTestClient -----

    // 1:1 with legacy SetActiveTestClient. SetActive
    // (TRUE) + 35-loop sprintf "%d %-16s %s" + AddItem.
    void SetActiveTestClient();

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h AND_TEXTAREA_MANUAL
    // / AND_BTN_ASK / AND_LIST_AUTO (630-632). Local
    // 630-632 — distinct from 200-624 used by previous
    // Tier 2 dialogs.
    static constexpr std::int32_t kIdTextAreaManual = 630;
    static constexpr std::int32_t kIdBtnAsk         = 631;
    static constexpr std::int32_t kIdListAuto       = 632;

    // 1:1 with legacy CHATMGR->GetChatMsg(1721) for
    // auto note manual text. Modern port uses literal
    // placeholder until CHATMGR is ported.
    static constexpr const char* kAutoNoteManualText =
        "AUTO_NOTE_MANUAL_TEXT";  // CHATMGR msg 1721

    // 1:1 with legacy SetActiveTestClient 35-loop.
    static constexpr int kTestClientLoopCount = 35;

    // 1:1 with legacy RGB_HALF(128, 128, 128) for
    // the auto note text color (gray). ARGB = 0xFF808080.
    static constexpr std::uint32_t kAutoNoteTextColor = 0xFF808080u;
    static constexpr std::uint32_t kWeBtnClick = 0x0001u;
    static constexpr std::int32_t kPlayerObjectKind = 1;
    static constexpr std::int32_t kSelectPlayerMessageId = 1704;
    static constexpr std::int32_t kManualMessageId = 1721;

private:
    GetSelectedObjectFn m_getSelectedObject = nullptr;
    GetObjectKindFn m_getObjectKind = nullptr;
    GetObjectIdFn m_getObjectId = nullptr;
    IsHeroObjectFn m_isHeroObject = nullptr;
    AddSystemMessageFn m_addSystemMessage = nullptr;
    GetRandomPercentFn m_getRandomPercent = nullptr;
    AskToAutoUserFn m_askToAutoUser = nullptr;
    void* m_callbackUserData = nullptr;

    // 1:1 with legacy m_pTextAreaManual (resolved in
    // Linking by AND_TEXTAREA_MANUAL id).
    cTextArea* m_pTextAreaManual = nullptr;

    // 1:1 with legacy m_pBtnAsk (resolved in
    // Linking by AND_BTN_ASK id).
    cButton* m_pBtnAsk = nullptr;

    // 1:1 with legacy m_pListAuto (resolved in
    // Linking by AND_LIST_AUTO id).
    cListDialog* m_pListAuto = nullptr;
};

}  // namespace mxh::ui
