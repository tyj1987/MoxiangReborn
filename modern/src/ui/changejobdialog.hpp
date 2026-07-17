// changejobdialog.hpp — modern port of 墨香 CChangeJobDialog
// (job-change item dialog: 2 state field + 2 method
// dispatch).
//
// 1:1 port of legacy `CChangeJobDialog` from
//   `墨香【源码】\[Client]MH\ChangeJobDialog.h` (920 B) and
//   `墨香【源码】\[Client]MH\ChangeJobDialog.cpp`.
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
//     eObjectState_Deal → OBJECTSTATEMGR->
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
//   - ChangeJobSyn: TODO (4-singleton: HERO +
//     NETWORK + SetProtocol + ITEMMGR not ported,
//     R-12.x deferred). Modern port is a no-op
//     while singletons are unported. When ported,
//     the body becomes the legacy code.
//   - CancelChangeJob: TODO (4-singleton: HERO +
//     OBJECTSTATEMGR + ITEMMGR not ported, R-12.x
//     deferred). Modern port is a no-op while
//     singletons are unported.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 33rd **Tier 2** dialog port (after
// cNameChangeDialog). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — all state lives in 4 global
// singletons (HERO + NETWORK + OBJECTSTATEMGR +
// ITEMMGR, R-12.x deferred).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cChangeJobDialog : public cDialog {
public:
    cChangeJobDialog();
    ~cChangeJobDialog() override;

    // ----- 1:1 with legacy CChangeJobDialog::SetItemInfo / GetItemPos / GetItemDBIdx -----

    // 1:1 with legacy SetItemInfo (inline setter).
    void SetItemInfo(std::uint32_t itemPos, std::uint32_t itemDBIdx) noexcept;

    // 1:1 with legacy GetItemPos (inline getter).
    std::uint32_t GetItemPos() const noexcept     { return m_ItemPos; }

    // 1:1 with legacy GetItemDBIdx (inline getter).
    std::uint32_t GetItemDBIdx() const noexcept   { return m_ItemDBIdx; }

    // ----- 1:1 with legacy CChangeJobDialog::ChangeJobSyn -----

    // 1:1 with legacy ChangeJobSyn. The whole
    // method is TODO (4-singleton: HERO + NETWORK
    // + SetProtocol + ITEMMGR not ported, R-12.x
    // deferred). Modern port is a no-op while
    // singletons are unported. When ported, the
    // body becomes the legacy code.
    void ChangeJobSyn();

    // ----- 1:1 with legacy CChangeJobDialog::CancelChangeJob -----

    // 1:1 with legacy CancelChangeJob. The whole
    // method is TODO (4-singleton: HERO +
    // OBJECTSTATEMGR + ITEMMGR not ported, R-12.x
    // deferred). Modern port is a no-op while
    // singletons are unported. When ported, the
    // body becomes the legacy code.
    void CancelChangeJob();

private:
    // 1:1 with legacy m_ItemPos + m_ItemDBIdx
    // (item context for the job change).
    std::uint32_t m_ItemPos   = 0;
    std::uint32_t m_ItemDBIdx = 0;
};

}  // namespace mxh::ui
