// changejobdialog.cpp - 1:1 port of legacy CChangeJobDialog
// (job-change item dialog). See changejobdialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "changejobdialog.hpp"

namespace mxh::ui {

cChangeJobDialog::cChangeJobDialog() {
    // 1:1 with legacy CChangeJobDialog ctor:
    //   m_type = WT_ITEM_CHANGEJOB_DLG;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
}

cChangeJobDialog::~cChangeJobDialog() = default;

void cChangeJobDialog::SetItemInfo(std::uint32_t itemPos,
                                   std::uint32_t itemDbIdx) noexcept {
    // 1:1 with legacy CChangeJobDialog::SetItemInfo
    // (inline setter in the header).
    m_ItemPos = itemPos;
    m_ItemDBIdx = itemDbIdx;
}

void cChangeJobDialog::SetCallbacks(GetHeroObjectIdFn getHeroObjectId,
                                   SendChangeJobSynFn sendSyn,
                                   IsHeroInDealStateFn isHeroInDeal,
                                   EndDealStateFn endDeal,
                                   SetItemTableDisabledFn setItemTableDisabled,
                                   void* userData) noexcept {
    m_getHeroObjectIdFn       = getHeroObjectId;
    m_sendSynFn               = sendSyn;
    m_isHeroInDealFn          = isHeroInDeal;
    m_endDealFn               = endDeal;
    m_setItemTableDisabledFn  = setItemTableDisabled;
    m_callbackUserData        = userData;
}

void cChangeJobDialog::ChangeJobSyn() {
    // 1:1 with legacy CChangeJobDialog::ChangeJobSyn:
    //   MSG_DWORD2 msg;
    //   SetProtocol(&msg, MP_ITEM, MP_ITEM_SHOPITEM_JOBCHANGE_SYN);
    //   msg.dwObjectID = HEROID;
    //   msg.dwData1 = m_ItemPos;
    //   msg.dwData2 = m_ItemDBIdx;
    //   NETWORK->Send(&msg, sizeof(msg));
    //   SetActive(FALSE);
    //
    // Modern port (1:1):
    //   - Build the ChangeJobRequest triple {objectId, itemPos,
    //     itemDbIdx}. The host GetHeroObjectIdFn callback replaces
    //     the legacy HEROID macro / HERO->GetID(). If the callback
    //     is null, objectId defaults to 0 (the wire byte is
    //     irrelevant when nothing is sent).
    //   - Hand the request to the host SendChangeJobSynFn callback
    //     (replaces NETWORK->Send). The host serializes the legacy
    //     MSG_DWORD2 + MP_ITEM / MP_ITEM_SHOPITEM_JOBCHANGE_SYN
    //     protocol envelope + pushes onto the network queue. If the
    //     callback is null, the send branch is silently skipped.
    //   - Always SetActive(FALSE) last (legacy quirk: dialog hides
    //     itself regardless of network send result).
    if (m_sendSynFn) {
        const std::uint32_t objectId = m_getHeroObjectIdFn
            ? m_getHeroObjectIdFn(m_callbackUserData)
            : 0u;
        (void)m_sendSynFn(objectId, m_ItemPos, m_ItemDBIdx,
                          m_callbackUserData);
    }
    SetActive(false);
}

void cChangeJobDialog::CancelChangeJob() {
    // 1:1 with legacy CChangeJobDialog::CancelChangeJob:
    //   if (HERO->GetState() == eObjectState_Deal)
    //       OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    //   ITEMMGR->SetDisableDialog(FALSE, eItemTable_Inventory);
    //   ITEMMGR->SetDisableDialog(FALSE, eItemTable_Pyoguk);
    //   ITEMMGR->SetDisableDialog(FALSE, eItemTable_GuildWarehouse);
    //   ITEMMGR->SetDisableDialog(FALSE, eItemTable_Shop);
    //   SetActive(FALSE);
    //
    // Modern port (1:1):
    //   - IsHeroInDealStateFn() replaces HERO->GetState() ==
    //     eObjectState_Deal. If true (and EndDealStateFn is wired),
    //     call EndDealStateFn() (replaces
    //     OBJECTSTATEMGR->EndObjectState).
    //   - 4x SetItemTableDisabledFn(FALSE, tableId) calls in the
    //     legacy order: Inventory / Pyoguk / GuildWarehouse / Shop.
    //     If SetItemTableDisabledFn is null, the 4 calls are silently
    //     skipped.
    //   - Always SetActive(FALSE) last (legacy quirk: dialog hides
    //     itself regardless of state-end result).
    if (m_isHeroInDealFn && m_isHeroInDealFn(m_callbackUserData)
        && m_endDealFn) {
        m_endDealFn(m_callbackUserData);
    }
    if (m_setItemTableDisabledFn) {
        m_setItemTableDisabledFn(false, kItemTableInventory,
                                 m_callbackUserData);
        m_setItemTableDisabledFn(false, kItemTablePyoguk,
                                 m_callbackUserData);
        m_setItemTableDisabledFn(false, kItemTableGuildWarehouse,
                                 m_callbackUserData);
        m_setItemTableDisabledFn(false, kItemTableShop,
                                 m_callbackUserData);
    }
    SetActive(false);
}

}  // namespace mxh::ui
