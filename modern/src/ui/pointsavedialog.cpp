// pointsavedialog.cpp — 1:1 port of 墨香
// CPointSaveDialog (map save-point name editor).
// See pointsavedialog.hpp for the data-model
// rationale + 1:1 quirks.

#include "pointsavedialog.hpp"
#include "ceditbox.hpp"
#include "ctextarea.hpp"

namespace mxh::ui {

cPointSaveDialog::cPointSaveDialog() {
    // 1:1 with legacy CPointSaveDialog ctor:
    //   m_bNewPoint = TRUE;
    //   m_ItemIdx = 0;
    //   m_ItemPos = 0;
    //
    // 1:1 quirk: modern bool uses default member
    // init (m_bNewPoint = true in header). The
    // ctor body is empty.
}

cPointSaveDialog::~cPointSaveDialog() = default;

void cPointSaveDialog::Linking() {
    // 1:1 with legacy CPointSaveDialog::Linking.
    // The legacy is:
    //   m_pNameEdtBox = (cEditBox*)GetWindowForID(CHA_NAMEEDITBOX);
    //   m_pNameEdtBox->SetValidCheck(VCM_CHARNAME);
    m_pNameEdtBox =
        static_cast<cEditBox*>(findWindowById(kIdNameEditBox));
    if (m_pNameEdtBox) {
        // 1:1 with legacy SetValidCheck(VCM_CHARNAME).
        m_pNameEdtBox->SetValidCheck(kVcmCharName);
    }
}

void cPointSaveDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CPointSaveDialog::SetActive
    // override. The legacy is:
    //   cDialog::SetActive(val);
    //   m_pNameEdtBox->SetFocusEdit(val);
    //   if (val) m_pNameEdtBox->SetEditText("");
    cDialog::SetActive(val);
    if (m_pNameEdtBox) {
        m_pNameEdtBox->SetFocusEdit(val);
        if (val) {
            m_pNameEdtBox->SetEditText("");
        }
    }
}

void cPointSaveDialog::SetItemToMapServer(std::uint32_t itemIdx,
                                         std::uint32_t itemPos) noexcept {
    // 1:1 with legacy SetItemToMapServer(DWORD, DWORD)
    // inline setter.
    m_ItemIdx = itemIdx;
    m_ItemPos = itemPos;
}

void cPointSaveDialog::SetCallbacks(
    CheckSameNameFn              checkSameName,
    GetMoveDialogSelectedDBIdxFn getMoveDialogSelectedDBIdx,
    GetHeroObjectIdFn            getHeroObjectId,
    GetHeroPositionFn            getHeroPosition,
    GetHeroStateFn               getHeroState,
    GetMapNumFn                  getMapNum,
    GetChatMessageFn             getChatMessage,
    AddSystemMessageFn           addSystemMessage,
    EnableItemTableFn            enableItemTable,
    EndObjectStateFn             endObjectState,
    SendSavepointAddSynFn        sendSavepointAddSyn,
    SendSavepointUpdateSynFn     sendSavepointUpdateSyn,
    void*                        userData) noexcept {
    m_checkSameName              = checkSameName;
    m_getMoveDialogSelectedDBIdx = getMoveDialogSelectedDBIdx;
    m_getHeroObjectId            = getHeroObjectId;
    m_getHeroPosition            = getHeroPosition;
    m_getHeroState               = getHeroState;
    m_getMapNum                  = getMapNum;
    m_getChatMessage             = getChatMessage;
    m_addSystemMessage           = addSystemMessage;
    m_enableItemTable            = enableItemTable;
    m_endObjectState             = endObjectState;
    m_sendSavepointAddSyn        = sendSavepointAddSyn;
    m_sendSavepointUpdateSyn     = sendSavepointUpdateSyn;
    m_callbackUserData           = userData;
}

namespace {

// Re-enables the 4 item-table dialogs gated
// by the dialog (1:1 with legacy
// ITEMMGR->SetDisableDialog(FALSE, eItemTable_*)
// x4 block). A null m_enableItemTable falls
// through to the legacy no-op path.
void PS_EnableAllItemTables(cPointSaveDialog& dlg, void* userData) {
    if (dlg.GetEnableItemTableForTest() != nullptr) {
        dlg.GetEnableItemTableForTest()(cPointSaveDialog::kItemTableInventory, userData);
        dlg.GetEnableItemTableForTest()(cPointSaveDialog::kItemTablePyoguk, userData);
        dlg.GetEnableItemTableForTest()(cPointSaveDialog::kItemTableMunpaWarehouse, userData);
        dlg.GetEnableItemTableForTest()(cPointSaveDialog::kItemTableShop, userData);
    }
}

// Truncates a name buffer to MAX_SAVEDMOVE_NAME
// - 1 = 20 chars (1:1 with legacy
//   strncpy(msg.Data.Name, name, MAX_SAVEDMOVE_NAME-1))
// When name is nullptr, returns an empty
// string.
const char* PS_TruncateSavedMoveName(const char* name) {
    static thread_local char buf[cPointSaveDialog::kMaxSavedMoveName];
    std::memset(buf, 0, sizeof(buf));
    if (!name) return buf;
    std::strncpy(buf, name, cPointSaveDialog::kMaxSavedMoveNameTrunc);
    return buf;
}

// Returns the chat-message text for the given
// legacy id (used by ChangePointName) via the
// host GetChatMessage callback; falls back to
// a static placeholder when the callback is
// null.
const char* PS_ResolveChatMessage(
    std::int32_t msgId,
    const cPointSaveDialog& dlg) {
    if (dlg.GetChatMessageForTest() != nullptr) {
        return dlg.GetChatMessageForTest()(msgId, dlg.GetCallbackUserDataForTest());
    }
    switch (msgId) {
        case cPointSaveDialog::kSysmsgDuplicateName:
            return "SAVEPOINT_NAME_DUPLICATE";
        default:
            return "SAVEPOINT_MSG";
    }
}

}  // namespace

void cPointSaveDialog::ChangePointName() {
    // 1:1 with legacy CPointSaveDialog::ChangePointName.
    // The legacy is a 6-branch dispatch on
    // (empty / CheckSameName / m_bNewPoint).
    // Modern port preserves the order 1:1.
    // All 7 singletons (ITEMMGR + GAMEIN +
    // HERO + CHATMGR + MAP + NETWORK +
    // OBJECTSTATEMGR) are replaced with
    // OPTIONAL host callbacks; a null
    // callback falls through to the legacy
    // no-op path.
    std::string editNameStr = m_pNameEdtBox ? m_pNameEdtBox->editText() : std::string();
    const char* editName = editNameStr.c_str();
    // Use lowercase editText() (modern
    // cEditBox API returns std::string).
    if (editName && std::strlen(editName) == 0u) {
        // 1:1 with legacy empty-name branch:
        //   ITEMMGR->SetDisableDialog(FALSE,
        //     eItemTable_*) x4
        //   m_pNameEdtBox->SetEditText("")
        //   SetActive(FALSE); return;
        PS_EnableAllItemTables(*this, m_callbackUserData);
        if (m_pNameEdtBox) m_pNameEdtBox->SetEditText("");
        SetActive(false);
        return;
    }
    if (m_checkSameName &&
        m_checkSameName(editName, m_callbackUserData)) {
        // 1:1 with legacy CheckSameName branch:
        //   ITEMMGR->SetDisableDialog x4
        //   if (HERO->GetState() ==
        //       eObjectState_Deal)
        //     OBJECTSTATEMGR->EndObjectState(HERO,
        //       eObjectState_Deal)
        //   CHATMGR->AddMsg(CTC_SYSMSG,
        //     GetChatMsg(784)); return;
        PS_EnableAllItemTables(*this, m_callbackUserData);
        std::uint32_t heroObjectId = 0u;
        std::int32_t  heroState = 0;
        if (m_getHeroObjectId) heroObjectId = m_getHeroObjectId(m_callbackUserData);
        if (m_getHeroState)    heroState    = m_getHeroState(m_callbackUserData);
        if (heroObjectId != 0u &&
            m_endObjectState &&
            heroState == kObjectStateDeal) {
            m_endObjectState(heroObjectId, kObjectStateDeal, m_callbackUserData);
        }
        if (m_addSystemMessage) {
            m_addSystemMessage(
                PS_ResolveChatMessage(kSysmsgDuplicateName, *this),
                m_callbackUserData);
        }
        return;
    }
    if (m_bNewPoint) {
        // 1:1 with legacy m_bNewPoint==TRUE
        // branch: sends SEND_MOVEDATA_WITHITEM.
        // Reads HERO position via the host
        // callback (legacy reads VECTOR3.x/z).
        const std::uint32_t heroObjectId =
            m_getHeroObjectId
                ? m_getHeroObjectId(m_callbackUserData) : 0u;
        const std::uint32_t dbIdx =
            m_getMoveDialogSelectedDBIdx
                ? m_getMoveDialogSelectedDBIdx(m_callbackUserData) : 0u;
        const std::uint16_t mapNum =
            m_getMapNum
                ? m_getMapNum(m_callbackUserData) : 0u;
        std::uint16_t posX = 0u, posZ = 0u;
        if (m_getHeroPosition) {
            m_getHeroPosition(&posX, &posZ, m_callbackUserData);
        }
        const char* name = PS_TruncateSavedMoveName(editName);
        // 1:1 quirk: legacy casts (WORD)m_ItemIdx
        // and (POSTYPE)m_ItemPos -- both are
        // already std::uint16_t in modern port,
        // so no cast is needed here.
        if (m_sendSavepointAddSyn) {
            m_sendSavepointAddSyn(heroObjectId, dbIdx, mapNum,
                                   name, posX, posZ,
                                   static_cast<std::uint16_t>(m_ItemIdx),
                                   static_cast<std::uint16_t>(m_ItemPos),
                                   m_callbackUserData);
        }
    } else {
        // 1:1 with legacy m_bNewPoint==FALSE
        // branch: sends SEND_MOVEDATA_SIMPLE.
        const std::uint32_t heroObjectId =
            m_getHeroObjectId
                ? m_getHeroObjectId(m_callbackUserData) : 0u;
        const std::uint32_t dbIdx =
            m_getMoveDialogSelectedDBIdx
                ? m_getMoveDialogSelectedDBIdx(m_callbackUserData) : 0u;
        const char* name = PS_TruncateSavedMoveName(editName);
        if (m_sendSavepointUpdateSyn) {
            m_sendSavepointUpdateSyn(heroObjectId, dbIdx, name, m_callbackUserData);
        }
    }
    // 1:1 with legacy tail: clear edit box +
    // close the dialog after a successful
    // send.
    if (m_pNameEdtBox) m_pNameEdtBox->SetEditText("");
    SetActive(false);
}

void cPointSaveDialog::CancelPointName() {
    // 1:1 with legacy CPointSaveDialog::CancelPointName.
    // The legacy is:
    //   ITEMMGR->SetDisableDialog(FALSE,
    //     eItemTable_*) x4
    //   // HERO + OBJECTSTATEMGR block is
    //   // COMMENTED OUT in legacy (1:1
    //   // quirk: developer comment in Korean
    //   // says "when using return scroll or
    //   // teleport, after cancelling return
    //   // to moveDlg window and restrict
    //   // player movement")
    //   SetActive(FALSE);
    PS_EnableAllItemTables(*this, m_callbackUserData);
    SetActive(false);
}


}  // namespace mxh::ui
