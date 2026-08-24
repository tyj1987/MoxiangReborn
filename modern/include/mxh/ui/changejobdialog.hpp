// changejobdialog.hpp - modern port of legacy CChangeJobDialog
// (job-change item dialog: 2 state field + 2 method
// dispatch).
//
// 1:1 port of legacy CChangeJobDialog from
//   legacy [Client]MH/ChangeJobDialog.h (920 B) and
//   legacy [Client]MH/ChangeJobDialog.cpp.
//
// What the legacy does:
//   - Ctor: m_type = WT_ITEM_CHANGEJOB_DLG (legacy
//     cWindow type tag).
//   - Dtor: empty body.
//   - SetItemInfo(DWORD ItemPos, DWORD ItemDBIdx):
//     inline setter for m_ItemPos + m_ItemDBIdx.
//   - GetItemPos / GetItemDBIdx: inline getter
//     for m_ItemPos / m_ItemDBIdx.
//   - ChangeJobSyn: build MSG_DWORD2, set protocol
//     to MP_ITEM / MP_ITEM_SHOPITEM_JOBCHANGE_SYN,
//     set dwObjectID to HEROID, set dwData1 to
//     m_ItemPos + dwData2 to m_ItemDBIdx, send via
//     NETWORK, then SetActive(FALSE).
//   - CancelChangeJob: if HERO->GetState() ==
//     eObjectState_Deal then OBJECTSTATEMGR->
//     EndObjectState(HERO, eObjectState_Deal);
//     then 4x ITEMMGR->SetDisableDialog(FALSE, ...)
//     for Inventory / Pyoguk / GuildWarehouse /
//     Shop; then SetActive(FALSE).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type =
//     WT_ITEM_CHANGEJOB_DLG drop, modern cWindow
//     does not have m_type).
//   - Dtor: empty (no-op).
//   - SetItemInfo / GetItemPos / GetItemDBIdx:
//     REAL inline setter / getters.
//   - ChangeJobSyn: REAL 1:1 port via host-injected
//     callbacks (GetHeroObjectIdFn +
//     SendChangeJobSynFn). Build the MSG_DWORD2
//     payload {objectId, itemPos, itemDbIdx}, hand
//     it to the host send callback, then
//     SetActive(FALSE). When no callback is wired
//     (e.g. test scenarios or pre-integration), the
//     method becomes a pure SetActive(FALSE) no-op.
//   - CancelChangeJob: REAL 1:1 port via host-injected
//     callbacks (IsHeroInDealStateFn +
//     EndDealStateFn + SetItemTableDisabledFn).
//     If IsHeroInDealState() returns true, call
//     EndDealState(); then 4x
//     SetItemTableDisabled(FALSE, Inventory/Pyoguk/
//     GuildWarehouse/Shop); then SetActive(FALSE).
//     When no callbacks are wired, the method
//     becomes a pure SetActive(FALSE) no-op (the
//     4 item-table calls are silently skipped).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 33rd Tier 2 dialog port (after
// cNameChangeDialog). The dialog has no service
// dependency on the modern service interface
// (Phase 13) - all state lives in 4 global
// singletons (HERO + NETWORK + OBJECTSTATEMGR +
// ITEMMGR, R-12.x deferred).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cChangeJobDialog : public cDialog {
public:
    // ----- Host-injected callbacks (legacy: HERO + NETWORK + OBJECTSTATEMGR + ITEMMGR globals) -----

    // 1:1 with legacy HEROID macro / HERO->GetID(). The host returns
    // the player object id so the dialog can stamp it into the
    // MSG_DWORD2 payload as dwObjectID.
    using GetHeroObjectIdFn = std::uint32_t (*)(void* userData);

    // 1:1 with legacy NETWORK->Send(&msg, sizeof(msg)) for the
    // MP_ITEM / MP_ITEM_SHOPITEM_JOBCHANGE_SYN packet. The host is
    // responsible for serializing the {objectId, itemPos, itemDbIdx}
    // triple into the legacy MSG_DWORD2 wire format and pushing it
    // onto the network queue. Returns true on accept, false on drop.
    using SendChangeJobSynFn = bool (*)(std::uint32_t objectId,
                                        std::uint32_t itemPos,
                                        std::uint32_t itemDbIdx,
                                        void* userData);

    // 1:1 with legacy HERO->GetState() == eObjectState_Deal. The host
    // returns true when the local hero is currently inside a deal
    // (player-to-player trade) state.
    using IsHeroInDealStateFn = bool (*)(void* userData);

    // 1:1 with legacy OBJECTSTATEMGR->EndObjectState(HERO,
    // eObjectState_Deal). The host performs the state-end transition.
    using EndDealStateFn = void (*)(void* userData);

    // 1:1 with legacy ITEMMGR->SetDisableDialog(FALSE, tableId). The
    // host toggles a per-table enable/disable flag (FALSE = enabled).
    using SetItemTableDisabledFn = void (*)(bool disabled,
                                            std::int32_t tableId,
                                            void* userData);

    // ----- 1:1 with legacy CommonGameDefine.h enum values (locked for wire compatibility) -----

    // eObjectState_Deal = 6 (1:1 with legacy CommonGameDefine.h
    // enum eObjectState). Recorded here for documentation; not used
    // directly by the dialog (the host callback owns the comparison).
    static constexpr std::int32_t kObjectStateDeal = 6;

    // eItemTable_Inventory = 0 (1:1).
    static constexpr std::int32_t kItemTableInventory = 0;

    // eItemTable_Pyoguk = 2 (warehouse, 1:1).
    static constexpr std::int32_t kItemTablePyoguk = 2;

    // eItemTable_GuildWarehouse = eItemTable_MunpaWarehouse = 10
    // (1:1 with legacy #define alias inside the eItemTable enum).
    static constexpr std::int32_t kItemTableGuildWarehouse = 10;

    // eItemTable_Shop = 3 (1:1).
    static constexpr std::int32_t kItemTableShop = 3;

    // ----- Wire payload (1:1 with legacy MSG_DWORD2 {dwObjectID, dwData1, dwData2}) -----

    // 1:1 with legacy MSG_DWORD2 (CommonStruct.h): dwObjectID +
    // dwData1 + dwData2 = 12 bytes of payload beyond the MSGBASE
    // header. Host network layer encodes the full MSGBASE + payload.
    struct ChangeJobRequest {
        std::uint32_t objectId;   // 1:1 with dwObjectID (= HEROID)
        std::uint32_t itemPos;    // 1:1 with dwData1 (= m_ItemPos)
        std::uint32_t itemDbIdx;  // 1:1 with dwData2 (= m_ItemDBIdx)
    };

    cChangeJobDialog();
    ~cChangeJobDialog() override;

    // ----- 1:1 with legacy CChangeJobDialog::SetItemInfo / GetItemPos / GetItemDBIdx -----

    // 1:1 with legacy SetItemInfo (inline setter).
    void SetItemInfo(std::uint32_t itemPos, std::uint32_t itemDbIdx) noexcept;

    // 1:1 with legacy GetItemPos (inline getter).
    std::uint32_t GetItemPos() const noexcept     { return m_ItemPos; }

    // 1:1 with legacy GetItemDBIdx (inline getter).
    std::uint32_t GetItemDBIdx() const noexcept   { return m_ItemDBIdx; }

    // ----- 1:1 with legacy CChangeJobDialog::ChangeJobSyn -----

    // 1:1 with legacy ChangeJobSyn. Build a ChangeJobRequest, hand it
    // to the host SendChangeJobSynFn callback, then SetActive(FALSE).
    // If the callback is null (no host wired it yet), the method
    // degenerates to a pure SetActive(FALSE) no-op so the dialog
    // stays safe to call from a UI button before integration.
    void ChangeJobSyn();

    // ----- 1:1 with legacy CChangeJobDialog::CancelChangeJob -----

    // 1:1 with legacy CancelChangeJob. If IsHeroInDealStateFn
    // returns true, call EndDealStateFn; then dispatch
    // SetItemTableDisabledFn(FALSE, ...) to the 4 tables
    // (Inventory / Pyoguk / GuildWarehouse / Shop); then
    // SetActive(FALSE). If any callback is null, its branch is
    // silently skipped and the dialog stays safe to call.
    void CancelChangeJob();

    // ----- Host callback wiring (test seam + production wiring share the same API) -----

    // Replace the legacy HERO / NETWORK / OBJECTSTATEMGR / ITEMMGR
    // globals with host-injected function pointers + opaque
    // userData. All five pointers are optional; null means the
    // corresponding branch in ChangeJobSyn / CancelChangeJob is
    // silently skipped.
    void SetCallbacks(GetHeroObjectIdFn getHeroObjectId,
                      SendChangeJobSynFn sendSyn,
                      IsHeroInDealStateFn isHeroInDeal,
                      EndDealStateFn endDeal,
                      SetItemTableDisabledFn setItemTableDisabled,
                      void* userData = nullptr) noexcept;

private:
    // 1:1 with legacy m_ItemPos + m_ItemDBIdx (item context for the
    // job change).
    std::uint32_t m_ItemPos   = 0;
    std::uint32_t m_ItemDBIdx = 0;

    // Host-injected callbacks (see SetCallbacks doc).
    GetHeroObjectIdFn     m_getHeroObjectIdFn       = nullptr;
    SendChangeJobSynFn    m_sendSynFn               = nullptr;
    IsHeroInDealStateFn   m_isHeroInDealFn          = nullptr;
    EndDealStateFn        m_endDealFn               = nullptr;
    SetItemTableDisabledFn m_setItemTableDisabledFn = nullptr;
    void*                 m_callbackUserData        = nullptr;
};

}  // namespace mxh::ui
