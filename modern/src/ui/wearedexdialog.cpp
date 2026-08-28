// wearedexdialog.cpp — 1:1 port of 墨香 CWearedExDialog
// (equipment slot dialog). See wearedexdialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "wearedexdialog.hpp"
#include "mxh/ui/cDialogLoader.hpp"
#include "mxh/ui/cResourceManager.hpp"

#include <array>
// cIcon class is forward-declared in cIconDialog.hpp
// (no modern port yet — R-12.x deferred). The ctor
// in wearedexdialog.cpp doesn't dereference cIcon*, so
// we don't need to include a cIcon header.

namespace mxh::ui {

cWearedExDialog::cWearedExDialog() {
    // 1:1 with legacy CWearedExDialog::CWearedExDialog:
    //   m_type = WT_WEAREDDIALOG;
    //   m_nIconType = WT_ITEM;
    //
    // The modern cWindow / cIconDialog doesn't have
    // m_type / m_nIconType fields (1:1 quirk: legacy
    // cWindow type tags were removed in Phase 6 when
    // cWindow was modernized). The modern port drops
    // the ctor body — the type tags are no longer
    // needed (the modern cIconDialog::AddIcon uses
    // only the cell layout, not a type tag).
}

cWearedExDialog::~cWearedExDialog() = default;

void cWearedExDialog::RefreshFromInventoryService() {
    if (!m_inventory) return;

    // The ten rectangles are the live IN_WEAREDDLG layout also used by the
    // gameplay hit-test (hat, weapon, dress, shoes, rings, cape, necklace,
    // armlet and belt).
    static constexpr std::array<std::array<std::int32_t, 2>, 10> kCells{{
        {{53, 28}}, {{8, 73}}, {{98, 73}}, {{53, 118}},
        {{143, 28}}, {{188, 28}}, {{143, 73}}, {{188, 73}},
        {{143, 118}}, {{188, 118}}}};
    SetCellNum(static_cast<std::uint16_t>(kCells.size()));
    for (const auto& cell : kCells) AddIconCell(cell[0], cell[1], 42, 42);
    m_owned_icons.clear();

    for (std::size_t slot = 0; slot < kCells.size(); ++slot) {
        const auto* item = m_inventory->getWearedItem(
            static_cast<std::uint8_t>(slot));
        if (!item || item->dwDBIdx == 0 || item->wIconIdx == 0) continue;
        auto* image = cDialogLoader::LoadLegacyPathImage(
            item->wIconIdx, PathFileType::ItemPath);
        if (!image) continue;
        auto icon = std::make_unique<cIcon>();
        icon->InitIcon(0, 0, 42, 42, image, 0,
                       static_cast<std::int32_t>(slot));
        auto* icon_ptr = icon.get();
        if (AddIcon(static_cast<std::uint16_t>(slot), icon_ptr)) {
            m_owned_icons.push_back(std::move(icon));
        }
    }
}

bool cWearedExDialog::AddItem(std::uint16_t relPos, cIcon* inIcon) {
    // 1:1 with legacy CWearedExDialog::AddItem. The
    // legacy is:
    //   CItem* item = (CItem*) InIcon;
    //   CHero* pHero = OBJECTMGR->GetHero();
    //   if (AddIcon(relPos, InIcon)) {
    //       if (item->GetItemKind() & eTITAN_EQUIPITEM) {
    //           // ... 7-singleton Titan branch ...
    //       } else {
    //           // ... 7-singleton normal branch ...
    //       }
    //       return TRUE;
    //   } else {
    //       return FALSE;
    //   }
    //
    // The modern port calls cIconDialog::AddIcon (REAL)
    // and on success, the singleton dispatch is TODO.
    // The return value matches the legacy contract
    // (true if AddIcon succeeded, else false).
    if (!cIconDialog::AddIcon(relPos, inIcon)) {
        return false;
    }
    // TODO: dispatch to OBJECTMGR + APPEARANCEMGR +
    //       ITEMMGR + STATSMGR + MUGONGMGR + GAMEIN +
    //       TITANMGR once those singletons are ported.
    //       The branch is:
    //         if (item->GetItemKind() & eTITAN_EQUIPITEM) {
    //             // Titan: pAprInfo->WearedItemIdx[relPos] = idx
    //             //         + pHero->SetBaseMotion()
    //             //         + APPEARANCEMGR->AddCharacterPartChange(...)
    //             //         + STATSMGR->CalcTitanStats(...)
    //             //         + MUGONGMGR->RefreshMugong()
    //             //         + GAMEIN->GetQuickDialog()->RefreshIcon()
    //         } else {
    //             // Normal: pHero->SetWearedItemIdx(relPos, idx)
    //             //         + pHero->SetCurComboNum(SKILL_COMBO_NUM)
    //             //           (1:1 quirk: weapon swap resets combo)
    //             //         + APPEARANCEMGR->AddCharacterPartChange(...)
    //             //         + STATSMGR->CalcItemStats(HERO)
    //             //         + 8x GAMEIN->GetCharacterDialog()->SetXxx() +
    //             //           UpdateData (5 core stats + attack/defense/critical)
    //             //         + MUGONGMGR->RefreshMugong()
    //             //         + GAMEIN->GetQuickDialog()->RefreshIcon()
    //         }
    return true;
}

bool cWearedExDialog::DeleteItem(std::uint16_t relPos, cIcon** outIcon) {
    // 1:1 with legacy CWearedExDialog::DeleteItem. The
    // legacy is:
    //   CHero* pHero = OBJECTMGR->GetHero();
    //   if (DeleteIcon(relPos, outIcon)) {
    //       CItem* item = (CItem*) *outIcon;
    //       if (item->GetItemKind() & eTITAN_EQUIPITEM) {
    //           // ... same 7-singleton Titan branch ...
    //       } else {
    //           // ... same 7-singleton normal branch ...
    //       }
    //       return TRUE;
    //   } else {
    //       return FALSE;
    //   }
    //
    // The modern port calls cIconDialog::DeleteIcon
    // (REAL) and on success, the singleton dispatch is
    // TODO. The return value matches the legacy
    // contract.
    if (!cIconDialog::DeleteIcon(relPos, outIcon)) {
        return false;
    }
    // TODO: dispatch to OBJECTMGR + APPEARANCEMGR +
    //       ITEMMGR + STATSMGR + MUGONGMGR + GAMEIN +
    //       TITANMGR once those singletons are ported.
    //       The branch is the same as AddItem's TODO
    //       (Titan vs normal based on
    //       item->GetItemKind() & eTITAN_EQUIPITEM).
    return true;
}

}  // namespace mxh::ui
