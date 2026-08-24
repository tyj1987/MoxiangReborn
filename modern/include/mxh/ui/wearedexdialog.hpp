// wearedexdialog.hpp — modern port of 墨香 CWearedExDialog
// (equipment slot dialog: 10 equipment slots + 4 Titan
// equipment slots).
//
// 1:1 port of legacy `CWearedExDialog` from
//   `墨香【源码】\[Client]MH\WearedExDialog.h` (714 B) and
//   `墨香【源码】\[Client]MH\WearedExDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_WEAREDDIALOG; m_nIconType = WT_ITEM
//     (legacy cWindow type tags; the modern cWindow /
//     cIconDialog doesn't have m_type / m_nIconType
//     fields, so the modern port drops the ctor body).
//   - AddItem(relPos, cIcon*): cast cIcon* to CItem*,
//     call cIconDialog::AddIcon(relPos, InIcon); on
//     success:
//       - If item kind has eTITAN_EQUIPITEM:
//         * SetWearedItemIdx[relPos] on hero's
//           TITAN_APPEARANCEINFO
//         * hero->SetBaseMotion()
//         * APPEARANCEMGR->AddCharacterPartChange
//         * STATSMGR->CalcTitanStats(...)
//         * MUGONGMGR->RefreshMugong()
//         * GAMEIN->GetQuickDialog()->RefreshIcon()
//       - Else (normal equipment):
//         * hero->SetWearedItemIdx(relPos, idx)
//         * hero->SetCurComboNum(SKILL_COMBO_NUM)
//           (1:1 quirk: weapon swap resets combo to 0)
//         * APPEARANCEMGR->AddCharacterPartChange
//         * STATSMGR->CalcItemStats(HERO)
//         * 8x GAMEIN->GetCharacterDialog()->SetXxx() +
//           UpdateData (legacy 1:1: re-render all 5 core
//           stats + attack/defense/critical)
//         * MUGONGMGR->RefreshMugong()
//         * GAMEIN->GetQuickDialog()->RefreshIcon()
//     Return TRUE if AddIcon succeeded, else FALSE.
//   - DeleteItem(relPos, cIcon**): cast *outIcon to
//     CItem*, call cIconDialog::DeleteIcon(relPos, outIcon);
//     on success same singleton dispatch (Titan vs
//     normal). Return TRUE if DeleteIcon succeeded, else
//     FALSE.
//
// The modern port covers the ctor (no-op, m_type /
// m_nIconType fields don't exist) + the base AddIcon /
// DeleteIcon call (REAL, cIconDialog API). The singleton
// dispatch is no-op TODO until 7 singletons (OBJECTMGR /
// APPEARANCEMGR / ITEMMGR / STATSMGR / MUGONGMGR /
// GAMEIN / TITANMGR) are ported.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 7th **Tier 2** dialog port (after cExitDialog,
// cMacroDialog, cCharMakeDlg, cGuildJoinDialog,
// cCharStateDialog, cSOSDialog). The dialog has no
// service dependency on the modern service interface
// (Phase 13) — all state lives in 7 global singletons,
// none of which are ported yet. The ItemManager +
// StatsCalcManager + MugongManager ports are tracked as
// future Tier 3+ work items (StatsCalcManager depends on
// PlayerStatsService already in Phase 13.2).

#pragma once

#include "cIconDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cIcon;

class cWearedExDialog : public cIconDialog {
public:
    cWearedExDialog();
    ~cWearedExDialog() override;

    // ----- 1:1 with legacy CWearedExDialog::AddItem -----

    // 1:1 with legacy AddItem(WORD relPos, cIcon* InIcon).
    // Cast InIcon to CItem*, call cIconDialog::AddIcon,
    // then on success dispatch 7-singleton path
    // (Titan vs normal branch based on
    // item->GetItemKind() & eTITAN_EQUIPITEM). Modern
    // port calls base AddIcon (REAL) + TODO singleton
    // dispatch. Returns true if AddIcon succeeded, else
    // false.
    bool AddItem(std::uint16_t relPos, cIcon* inIcon);

    // ----- 1:1 with legacy CWearedExDialog::DeleteItem -----

    // 1:1 with legacy DeleteItem(WORD relPos, cIcon**
    // outIcon). Cast *outIcon to CItem*, call
    // cIconDialog::DeleteIcon, then on success dispatch
    // 7-singleton path. Modern port calls base
    // DeleteIcon (REAL) + TODO singleton dispatch.
    // Returns true if DeleteIcon succeeded, else false.
    bool DeleteItem(std::uint16_t relPos, cIcon** outIcon);
};

}  // namespace mxh::ui
