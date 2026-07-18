// titanrepairdlg.cpp — modern port of 墨香 CTitanRepairDlg (titan repair).
//
// 1:1 port body. See legacy `TitanRepairDlg.cpp` for the original.

#include "titanrepairdlg.hpp"

#include "cwindow.hpp"

#include <cstdint>

namespace mxh::ui {

cTitanRepairDlg::cTitanRepairDlg() = default;
cTitanRepairDlg::~cTitanRepairDlg() = default;

void cTitanRepairDlg::ClearTestInjections() noexcept {
    s_cursor = ECursorState::Default;
    s_objectStateDealEnded = false;
    s_inventoryDialogCloseCount = 0;
    s_titanInventoryDialogCloseCount = 0;
    s_chatMsgNoItemsCount = 0;
    s_chatMsgRepairConfirmCount = 0;
    s_windowMgrMsgBoxCount = 0;
    s_titanMgrRepairCallCount = 0;
    s_titanRepairCost = 0;
}

void cTitanRepairDlg::Linking() {
    // 1:1 quirk: legacy Linking() body is empty (the dialog
    // has no child widgets to wire). Modern port preserves the
    // empty body verbatim.
}

void cTitanRepairDlg::SetActive(bool val) noexcept {
    // 1:1 with legacy SetActive(BOOL val):
    //   cDialog::SetActive(val);
    //   if(val == FALSE)
    //   {
    //       if(HERO->GetState() == eObjectState_Deal)
    //           OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    //       if(CURSOR->GetCursor() == eCURSOR_TITANREPAIR)
    //       {
    //           CURSOR->SetCursor(eCURSOR_DEFAULT);
    //       }
    //   }
    //
    // Modern port: HERO/OBJECTSTATEMGR/CURSOR singletons stubbed
    // no-op. The s_objectStateDealEnded + s_cursor state is
    // mutated for test inspection.
    cDialog::SetActive(val);
    if (!val) {
        // 1:1 quirk: legacy checks HERO->GetState() == eObjectState_Deal
        // before ending. Modern port: conservative "always end" for
        // test inspection (s_objectStateDealEnded = true). The
        // conditional guard is documented as a 1:1 quirk but
        // not enforced in the modern port (HERO not ported).
        s_objectStateDealEnded = true;
        if (s_cursor == ECursorState::TitanRepair) {
            s_cursor = ECursorState::Default;
        }
    }
}

bool cTitanRepairDlg::OnActionEvent(std::int32_t lId, void* /*p*/,
                                    std::uint32_t we) {
    // 1:1 with legacy OnActionEvent:
    //   switch(we)
    //   {
    //   case WE_CLOSEWINDOW:
    //       if(HERO->GetState() == eObjectState_Deal)
    //           OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    //       GAMEIN->GetInventoryDialog()->SetActive(FALSE);
    //       GAMEIN->GetTitanInventoryDlg()->SetActive(FALSE);
    //       //GAMEIN->GetTitanRepairDlg()->SetActive(FALSE);
    //       return TRUE;
    //   }
    //   switch(lId)
    //   {
    //   case TITAN_REPAIR_PART:
    //       if(CURSOR->GetCursor() == eCURSOR_TITANREPAIR)
    //           CURSOR->SetCursor(eCURSOR_DEFAULT);
    //       else if(CURSOR->GetCursor() == eCURSOR_DEFAULT)
    //           CURSOR->SetCursor(eCURSOR_TITANREPAIR);
    //   case TITAN_REPAIR_ALL:
    //       MSG_TITAN_REPAIR_TOTAL_EQUIPITEM_SYN msg;
    //       msg.Init();
    //       DWORD dwMoney = TITANMGR->GetTitanEnduranceTotalInfo(&msg, TRUE);
    //       if(dwMoney == 0)
    //           CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(1582));
    //       else
    //           WINDOWMGR->MsgBox(MBI_TITAN_TOTAL_REPAIR, MBT_YESNO,
    //                              CHATMGR->GetChatMsg(1543), dwMoney);
    //   }
    //   return TRUE;
    //
    // 1:1 quirks:
    //   - legacy uses `we` (not `we & WE_BTNCLICK`) as the
    //     discriminator for the close-window branch.
    //   - legacy `return TRUE` is preserved as `return true`.
    //   - legacy commented-out self-close is documented.
    //   - legacy TITAN_REPAIR_PART/ALL come from WindowIDEnum.h;
    //     modern port uses local kIdTitanRepairPart/kIdTitanRepairAll.
    //   - legacy TITAN_REPAIR_PART switch has no `break` (legacy
    //     c++17 compatibility); TITAN_REPAIR_ALL falls through
    //     from TITAN_REPAIR_PART. Modern port preserves this
    //     fall-through via 2 sequential branches (no break).
    //   - legacy TITANMGR.GetTitanEnduranceTotalInfo returns
    //     DWORD cost. Modern port: test-injectable s_titanRepairCost
    //     (default 0 → "no items" branch).
    //   - legacy CHATMGR msg 1582 (no items) and 1543 (confirm)
    //     are stubbed no-op; modern port increments
    //     s_chatMsgNoItemsCount / s_chatMsgRepairConfirmCount.
    //   - legacy WINDOWMGR MsgBox stubbed no-op; modern port
    //     increments s_windowMgrMsgBoxCount.

    switch (we) {
    case kWeCloseWindow: {
        // 1:1 quirk: legacy commented-out self-close
        //   //GAMEIN->GetTitanRepairDlg()->SetActive(FALSE);
        // preserved as a 1:1 quirk note. Modern port: do not
        // self-close (would infinite-loop).
        s_objectStateDealEnded = true;
        ++s_inventoryDialogCloseCount;
        ++s_titanInventoryDialogCloseCount;
        return true;
    }
    }  // 1:1 quirk: legacy switch has no `default` branch.

    // 1:1 quirk: legacy second switch has no break between
    // TITAN_REPAIR_PART and TITAN_REPAIR_ALL. Modern port
    // preserves the fall-through: PART's cursor toggle is
    // followed by ALL's repair-cost check (in the legacy
    // path, both branches execute on PART id — the cursor
    // toggle and the cost check). Modern port: match the
    // legacy fall-through by NOT breaking between the two
    // branches (sequential if/else if structure).
    if (lId == kIdTitanRepairPart) {
        if (s_cursor == ECursorState::TitanRepair) {
            s_cursor = ECursorState::Default;
        } else if (s_cursor == ECursorState::Default) {
            s_cursor = ECursorState::TitanRepair;
        }
    }
    if (lId == kIdTitanRepairAll || lId == kIdTitanRepairPart) {
        // 1:1 quirk: legacy TITANMGR call is unconditional on
        // TITAN_REPAIR_ALL (and on TITAN_REPAIR_PART via the
        // missing break). Modern port: same — call the stub
        // TITANMGR whether the user clicked ALL or PART
        // (preserves legacy fall-through).
        ++s_titanMgrRepairCallCount;
        const std::uint32_t dwMoney = s_titanRepairCost;
        if (dwMoney == 0) {
            ++s_chatMsgNoItemsCount;
        } else {
            ++s_chatMsgRepairConfirmCount;
            ++s_windowMgrMsgBoxCount;
        }
    }

    return true;  // 1:1 quirk: legacy `return TRUE` is preserved.
}

} // namespace mxh::ui
