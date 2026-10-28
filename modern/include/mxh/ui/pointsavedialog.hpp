// pointsavedialog.hpp — modern port of 墨香
// CPointSaveDialog (map save-point name editor
// dialog: 1 cEditBox + 1 cTextArea + ItemPos/ItemIdx
// state + m_bNewPoint flag).
//
// 1:1 port of legacy `CPointSaveDialog` from
//   `墨香【源码】\[Client]MH\PointSaveDialog.h`
//   and `墨香【源码】\[Client]MH\PointSaveDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_bNewPoint = TRUE; m_ItemIdx = 0;
//     m_ItemPos = 0.
//   - Dtor: empty body.
//   - Linking: resolve 1 cEditBox (m_pNameEdtBox by
//     CHA_NAMEEDITBOX); SetValidCheck(VCM_CHARNAME).
//   - SetActive(BOOL val) override: cDialog::SetActive
//     + m_pNameEdtBox->SetFocusEdit(val);
//     if (val) m_pNameEdtBox->SetEditText("").
//   - SetItemToMapServer: inline setter for m_ItemIdx
//     + m_ItemPos.
//   - ChangePointName: 4-singleton dispatch via
//     ITEMMGR + GAMEIN + HERO + CHATMGR + MAP +
//     NETWORK (sends SEND_MOVEDATA_WITHITEM or
//     SEND_MOVEDATA_SIMPLE).
//   - CancelPointName: 4-singleton dispatch via
//     ITEMMGR + HERO + OBJECTSTATEMGR.
//   - SetDialogStatus: inline setter for m_bNewPoint.
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_bNewPoint = true
//     + m_ItemIdx/m_ItemPos init via default
//     member init).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve cEditBox by id +
//     SetValidCheck(VCM_CHARNAME=2).
//   - SetActive override: REAL — cDialog::SetActive
//     + SetFocusEdit + SetEditText (if val).
//   - SetItemToMapServer: REAL inline setter.
//   - ChangePointName: TODO (5-singleton dispatch,
//     R-12.x deferred). Modern port is empty.
//   - CancelPointName: TODO (3-singleton dispatch).
//     Modern port is empty.
//   - SetDialogStatus: REAL inline setter.
//   - State accessors: IsNewPoint + GetItemPos +
//     GetItemIdx for tests.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cEditBox;
class cTextArea;

class cPointSaveDialog : public cDialog {
public:
    cPointSaveDialog();
    ~cPointSaveDialog() override;

    // ----- 1:1 with legacy CPointSaveDialog::Linking -----

    // 1:1 with legacy Linking. Resolve 1 cEditBox
    // (m_pNameEdtBox by kIdNameEditBox) +
    // SetValidCheck(VCM_CHARNAME=2).
    void Linking();

    // ----- 1:1 with legacy CPointSaveDialog::SetActive override -----

    // 1:1 with legacy SetActive override. Always
    // base + SetFocusEdit(val). If val, SetEditText("").
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CPointSaveDialog::SetItemToMapServer -----

    // 1:1 with legacy SetItemToMapServer(DWORD,
    // DWORD) inline setter.
    void SetItemToMapServer(std::uint32_t itemIdx,
                             std::uint32_t itemPos) noexcept;

    
    // ----- 1:1 with legacy CHATMGR / eItemTable /
    // eObjectState_Deal / MAX_SAVEDMOVE_NAME constants -----

    // 1:1 with legacy CHATMGR->GetChatMsg(784) --
    //   "save-point name already used" (the
    //   CheckSameName branch's error message).
    static constexpr std::int32_t kSysmsgDuplicateName = 784;

    // 1:1 with legacy eITEMTABLE enum values
    // (CommonGameDefine.h). The modern port uses
    // std::int8_t for the table id (matches the
    // legacy BYTE pass-through).
    static constexpr std::int8_t kItemTableInventory = 0;        // eItemTable_Inventory
    static constexpr std::int8_t kItemTablePyoguk = 2;           // eItemTable_Pyoguk
    static constexpr std::int8_t kItemTableShop = 3;            // eItemTable_Shop
    static constexpr std::int8_t kItemTableMunpaWarehouse = 10;  // eItemTable_MunpaWarehouse

    // 1:1 with legacy eObjectState_Deal (used by
    // CancelPointName + ChangePointName's
    // CheckSameName branch).
    static constexpr std::int32_t kObjectStateDeal = 6;

    // 1:1 with legacy MAX_SAVEDMOVE_NAME = 21
    // (CommonGameDefine.h). The legacy strncpy
    // copies MAX_SAVEDMOVE_NAME - 1 = 20 chars.
    static constexpr std::size_t kMaxSavedMoveName = 21;
    static constexpr std::size_t kMaxSavedMoveNameTrunc = 20;
    // ----- Host callback signatures -----

    // 1:1 with legacy GAMEIN->GetMoveDialog()
    //   ->CheckSameName(name)
    using CheckSameNameFn = bool (*)(const char* name, void* userData);

    // 1:1 with legacy GAMEIN->GetMoveDialog()
    //   ->GetSelectedDBIdx()
    using GetMoveDialogSelectedDBIdxFn = std::uint32_t (*)(void* userData);

    // 1:1 with legacy HERO->GetID().
    using GetHeroObjectIdFn = std::uint32_t (*)(void* userData);

    // 1:1 with legacy HERO->GetCurPosition() --
    // the modern host fills posX + posZ via
    // out-params (the legacy VECTOR3 is not
    // ported; we use the WORD-cast values
    // directly).
    using GetHeroPositionFn = void (*)(std::uint16_t* posX,
                                       std::uint16_t* posZ,
                                       void* userData);

    // 1:1 with legacy HERO->GetState().
    using GetHeroStateFn = std::int32_t (*)(void* userData);

    // 1:1 with legacy MAP->GetMapNum().
    using GetMapNumFn = std::uint16_t (*)(void* userData);

    // 1:1 with legacy CHATMGR->GetChatMsg(id).
    using GetChatMessageFn = const char* (*)(std::int32_t msgId, void* userData);

    // 1:1 with legacy CHATMGR->AddMsg(CTC_SYSMSG,
    // text). CTC_SYSMSG is folded into the host
    // (system-message tag).
    using AddSystemMessageFn = void (*)(const char* text, void* userData);

    // 1:1 with legacy ITEMMGR->SetDisableDialog
    //   (FALSE, eItemTable_*)
    // Always invoked with the `enable = FALSE`
    // direction, so the host signature drops the
    // bool (matches the legacy "re-enable all 4
    // tables" idiom).
    using EnableItemTableFn = void (*)(std::int8_t tableId, void* userData);

    // 1:1 with legacy OBJECTSTATEMGR->EndObjectState
    //   (HERO, eObjectState_Deal)
    using EndObjectStateFn = void (*)(std::uint32_t objectId,
                                      std::int32_t stateIdx,
                                      void* userData);

    // 1:1 with legacy NETWORK->Send for
    //   SEND_MOVEDATA_WITHITEM (m_bNewPoint==TRUE).
    // The host is responsible for packing the
    // wire format (Category=MP_ITEM, Protocol=
    // MP_ITEM_SHOPITEM_SAVEPOINT_ADD_SYN, etc.)
    // using the values passed in. The ItemPos
    // is a POSTYPE (cast to WORD in legacy).
    using SendSavepointAddSynFn = void (*)(
        std::uint32_t heroObjectId,
        std::uint32_t dbIdx,
        std::uint16_t mapNum,
        const char* name,
        std::uint16_t posX,
        std::uint16_t posZ,
        std::uint16_t itemIdx,
        std::uint16_t itemPos,
        void* userData);

    // 1:1 with legacy NETWORK->Send for
    //   SEND_MOVEDATA_SIMPLE (m_bNewPoint==FALSE).
    using SendSavepointUpdateSynFn = void (*)(
        std::uint32_t heroObjectId,
        std::uint32_t dbIdx,
        const char* name,
        void* userData);

    // Install host callbacks. Pass nullptr for
    // any callback to fall through to the legacy
    // no-op / "singleton not yet ported" path.
    void SetCallbacks(
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
        void*                        userData = nullptr) noexcept;

// ----- 1:1 with legacy CPointSaveDialog::ChangePointName -----

    // 1:1 with legacy ChangePointName. The 5-singleton
    // dispatch (ITEMMGR + GAMEIN + HERO + CHATMGR + MAP
    // + NETWORK) is now implemented via OPTIONAL host
    // callbacks (see SetCallbacks below).
    void ChangePointName();

    // ----- 1:1 with legacy CPointSaveDialog::CancelPointName -----

    // 1:1 with legacy CancelPointName. The 3-singleton
    // dispatch (ITEMMGR + HERO + OBJECTSTATEMGR) is now
    // implemented via OPTIONAL host callbacks (see
    // SetCallbacks below).
    void CancelPointName();

    // ----- 1:1 with legacy CPointSaveDialog::SetDialogStatus -----

    // 1:1 with legacy SetDialogStatus(BOOL bNewPoint)
    // inline setter.
    void SetDialogStatus(bool bNewPoint) noexcept {
        m_bNewPoint = bNewPoint;
    }

    // ----- 1:1 with legacy state accessors -----

    // 1:1 with legacy m_bNewPoint getter.
    bool IsNewPoint() const noexcept { return m_bNewPoint; }
    // 1:1 with legacy m_ItemIdx getter.
    std::uint32_t GetItemIdx() const noexcept { return m_ItemIdx; }
    // 1:1 with legacy m_ItemPos getter.
    std::uint32_t GetItemPos() const noexcept { return m_ItemPos; }

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h CHA_NAMEEDITBOX.
    // Local 710 — distinct from 200-700 used by
    // previous Tier 2 dialogs.
    static constexpr std::int32_t kIdNameEditBox = 710;

    // 1:1 with legacy VCM_CHARNAME = 2 (char-name
    // valid check).
    static constexpr int kVcmCharName = 2;
    // ----- Test-only accessors -----

    // Returns the m_enableItemTable callback
    // pointer (used by the cpp-internal
    // PS_EnableAllItemTables helper).
    EnableItemTableFn GetEnableItemTableForTest() const noexcept {
        return m_enableItemTable;
    }

    // Returns the m_getChatMessage callback
    // pointer (used by PS_ResolveChatMessage).
    GetChatMessageFn GetChatMessageForTest() const noexcept {
        return m_getChatMessage;
    }

    // Returns the m_callbackUserData pointer so
    // the cpp helpers can pass it to the host
    // callbacks.
    void* GetCallbackUserDataForTest() const noexcept {
        return m_callbackUserData;
    }





private:
    // 1:1 with legacy m_pNameEdtBox (resolved in
    // Linking by CHA_NAMEEDITBOX id).
    cEditBox* m_pNameEdtBox = nullptr;

    // 1:1 with legacy m_pText (declared in header
    // but never used in legacy cpp body; modern
    // port preserves for 1:1 parity).
    cTextArea* m_pText = nullptr;

    // 1:1 with legacy m_bNewPoint (BOOL; init TRUE).
    bool m_bNewPoint = true;

    // 1:1 with legacy m_ItemPos (DWORD; init 0).
    std::uint32_t m_ItemPos = 0;

    // 1:1 with legacy m_ItemIdx (DWORD; init 0).
    std::uint32_t m_ItemIdx = 0;

    // 1:1 host callback pointers (replaces
    // ITEMMGR + GAMEIN + HERO + CHATMGR + MAP +
    // NETWORK + OBJECTSTATEMGR globals,
    // R-12.x deferred).
    CheckSameNameFn              m_checkSameName              = nullptr;
    GetMoveDialogSelectedDBIdxFn m_getMoveDialogSelectedDBIdx = nullptr;
    GetHeroObjectIdFn            m_getHeroObjectId            = nullptr;
    GetHeroPositionFn            m_getHeroPosition            = nullptr;
    GetHeroStateFn               m_getHeroState               = nullptr;
    GetMapNumFn                  m_getMapNum                  = nullptr;
    GetChatMessageFn             m_getChatMessage             = nullptr;
    AddSystemMessageFn           m_addSystemMessage           = nullptr;
    EnableItemTableFn            m_enableItemTable            = nullptr;
    EndObjectStateFn             m_endObjectState             = nullptr;
    SendSavepointAddSynFn        m_sendSavepointAddSyn        = nullptr;
    SendSavepointUpdateSynFn     m_sendSavepointUpdateSyn     = nullptr;
    void*                        m_callbackUserData           = nullptr;
};

}  // namespace mxh::ui
