// changejobdialog.cpp — 1:1 port of 墨香
// CChangeJobDialog (job-change item dialog). See
// changejobdialog.hpp for the data-model rationale
// + 1:1 quirks.

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

void cChangeJobDialog::SetItemInfo(std::uint32_t itemPos, std::uint32_t itemDBIdx) noexcept {
    // 1:1 with legacy CChangeJobDialog::SetItemInfo
    // (inline setter in the header).
    m_ItemPos = itemPos;
    m_ItemDBIdx = itemDBIdx;
}

void cChangeJobDialog::ChangeJobSyn() {
    // 1:1 with legacy CChangeJobDialog::ChangeJobSyn.
    // The legacy is:
    //   MSG_DWORD2 msg;
    //   SetProtocol(&msg, MP_ITEM, MP_ITEM_SHOPITEM_JOBCHANGE_SYN);
    //   msg.dwObjectID = HEROID;
    //   msg.dwData1 = m_ItemPos;
    //   msg.dwData2 = m_ItemDBIdx;
    //   NETWORK->Send(&msg, sizeof(msg));
    //   SetActive(FALSE);
    //
    // The modern port: the whole method is TODO
    // (4-singleton: HERO + NETWORK + SetProtocol +
    // ITEMMGR not ported, R-12.x deferred). Modern
    // port is a no-op (does not call SetActive, no
    // state change) while singletons are unported.
    // When ported, the body becomes the legacy code.
    // TODO: 4-singleton dispatch (R-12.x deferred).
}

void cChangeJobDialog::CancelChangeJob() {
    // 1:1 with legacy CChangeJobDialog::CancelChangeJob.
    // The legacy is:
    //   if (HERO->GetState() == eObjectState_Deal)
    //     OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    //   ITEMMGR->SetDisableDialog(FALSE, eItemTable_Inventory);
    //   ITEMMGR->SetDisableDialog(FALSE, eItemTable_Pyoguk);
    //   ITEMMGR->SetDisableDialog(FALSE, eItemTable_GuildWarehouse);
    //   ITEMMGR->SetDisableDialog(FALSE, eItemTable_Shop);
    //   SetActive(FALSE);
    //
    // The modern port: the whole method is TODO
    // (4-singleton: HERO + OBJECTSTATEMGR + ITEMMGR
    // not ported, R-12.x deferred). Modern port is
    // a no-op while singletons are unported. When
    // ported, the body becomes the legacy code.
    // TODO: 4-singleton dispatch (R-12.x deferred).
}

}  // namespace mxh::ui
