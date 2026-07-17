// petwearedexdialog.cpp — 1:1 port of 墨香 CPetWearedExDialog
// (pet equipment slot dialog: 3 pet equipment slots).
// See petwearedexdialog.hpp for the data-model rationale +
// 1:1 quirks.

#include "petwearedexdialog.hpp"
// cIcon class is forward-declared in cIconDialog.hpp
// (no modern port yet — R-12.x deferred). The methods
// in petwearedexdialog.cpp don't dereference cIcon*, so
// we don't need to include a cIcon header.

namespace mxh::ui {

cPetWearedExDialog::cPetWearedExDialog() {
    // 1:1 with legacy CPetWearedExDialog::CPetWearedExDialog:
    //   empty body, no state init.
}

cPetWearedExDialog::~cPetWearedExDialog() = default;

bool cPetWearedExDialog::AddItem(std::uint16_t relPos, cIcon* inIcon) {
    // 1:1 with legacy CPetWearedExDialog::AddItem. The
    // legacy is:
    //   CItem* item = (CItem*) InIcon;
    //   if (AddIcon(relPos, InIcon)) {
    //       //!!!복사본 옵션 적용
    //       return TRUE;
    //   } else
    //       return FALSE;
    //
    // The Korean "!!!복사본 옵션 적용" / "copy option
    // apply" comment is a 2008-era TODO marker with no
    // code attached. The modern port preserves it as a
    // doc-only no-op and returns the base AddIcon
    // result (REAL).
    if (!cIconDialog::AddIcon(relPos, inIcon)) {
        return false;
    }
    // TODO(legacy): "!!!복사본 옵션 적용" / copy option
    //               apply. No code in the legacy either.
    return true;
}

bool cPetWearedExDialog::DeleteItem(std::uint16_t relPos, cIcon** outIcon) {
    // 1:1 with legacy CPetWearedExDialog::DeleteItem.
    // The legacy is:
    //   if (DeleteIcon(relPos, outIcon)) {
    //       //!!!복사본 옵션 적용
    //       return TRUE;
    //   } else
    //       return FALSE;
    //
    // Same Korean comment, same behavior.
    if (!cIconDialog::DeleteIcon(relPos, outIcon)) {
        return false;
    }
    // TODO(legacy): "!!!복사본 옵션 적용" / copy option
    //               apply. No code in the legacy either.
    return true;
}

bool cPetWearedExDialog::GetBlankPositionRestrictRef(std::uint16_t& absPos) {
    // 1:1 with legacy CPetWearedExDialog::GetBlankPositionRestrictRef.
    // The legacy is:
    //   for (int i = 0; i < SLOT_PETWEAR_NUM; ++i) {
    //       if (IsAddable(i)) {
    //           absPos = TP_PETWEAR_START + i;
    //           return TRUE;
    //       }
    //   }
    //   return FALSE;
    //
    // The modern port uses cIconDialog::IsAddable (REAL)
    // and the inlined kSlotPetWearNum / kTpPetWearStart
    // constants (which mirror the legacy enum values).
    for (std::uint16_t i = 0; i < kSlotPetWearNum; ++i) {
        if (cIconDialog::IsAddable(i)) {
            absPos = static_cast<std::uint16_t>(kTpPetWearStart + i);
            return true;
        }
    }
    return false;
}

bool cPetWearedExDialog::CheckDuplication(std::uint32_t itemIdx) {
    // 1:1 with legacy CPetWearedExDialog::CheckDuplication.
    // The legacy is:
    //   for (int i = 0; i < SLOT_PETWEAR_NUM; ++i) {
    //       CItem* pItem = (CItem*) (GetIconForIdx(i));
    //       if (pItem) {
    //           if (pItem->GetItemIdx() == ItemIdx)
    //               return TRUE;
    //       }
    //   }
    //   return FALSE;
    //
    // The modern port:
    //   - Iterates cells [0, kSlotPetWearNum).
    //   - For each in-use cell, the icon is a cIcon*
    //     (forward-declared; modern cItem is not ported
    //     yet — R-12.x deferred).
    //   - We cannot safely static_cast<cItem*>(icon) and
    //     dereference it (the test passes opaque
    //     reinterpret_cast<cIcon*>(0x1) pointers that
    //     would crash on deref). So the modern port
    //     returns false unconditionally with a TODO
    //     marker for the cItem port.
    //
    // When cItem is ported, replace the body with:
    //   for (std::uint16_t i = 0; i < kSlotPetWearNum; ++i) {
    //     cIcon* icon = cIconDialog::GetIconForIdx(i);
    //     if (!icon) continue;
    //     auto* item = static_cast<cItem*>(icon);
    //     if (item && item->GetItemIdx() == itemIdx) {
    //       return true;
    //     }
    //   }
    //   return false;
    (void)itemIdx;
    // TODO: cItem not ported (R-12.x deferred). When
    //       ported, scan cells and compare GetItemIdx().
    return false;
}

}  // namespace mxh::ui
