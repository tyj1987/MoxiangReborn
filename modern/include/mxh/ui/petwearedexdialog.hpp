// petwearedexdialog.hpp — modern port of 墨香 CPetWearedExDialog
// (pet equipment slot dialog: 3 pet equipment slots).
//
// 1:1 port of legacy `CPetWearedExDialog` from
//   `墨香【源码】\[Client]MH\PetWearedExDialog.h` (445 B) and
//   `墨香【源码】\[Client]MH\PetWearedExDialog.cpp`.
//
// What the legacy does:
//   - Ctor / dtor: empty bodies (no state init).
//   - AddItem(relPos, cIcon* InIcon): cast cIcon* to CItem*,
//     call cIconDialog::AddIcon(relPos, InIcon); on success
//     return TRUE; on failure return FALSE. (1:1 quirk:
//     legacy has a Korean comment "!!!복사본 옵션 적용" /
//     "copy option apply" but no actual code in the
//     method — the comment is just a TODO marker from the
//     2008-era developer.)
//   - DeleteItem(relPos, cIcon** outIcon): same as AddItem
//     but calls cIconDialog::DeleteIcon. Same Korean
//     comment, same behavior.
//   - GetBlankPositionRestrictRef(WORD& absPos): scan all
//     3 pet equip cells, return TRUE with absPos =
//     TP_PETWEAR_START + i where i is the first addable
//     cell. Return FALSE if all cells occupied. This is
//     the legacy's helper for ItemManager to find the
//     first empty pet-equip slot and convert it to an
//     absolute tab-position index (TP_PETWEAR_START is
//     defined in [CC]Header/CommonGameDefine.h as
//     `TP_PETINVEN_END`, currently 490 in the legacy
//     enum; SLOT_PETWEAR_NUM = 3).
//   - CheckDuplication(DWORD ItemIdx): scan all 3 pet
//     equip cells, return TRUE if any cell holds a
//     CItem whose GetItemIdx() == ItemIdx (i.e. the same
//     item type is already equipped on the pet). Return
//     FALSE otherwise. (1:1 quirk: legacy casts
//     GetIconForIdx(i) to CItem* even if the cell is
//     empty — if the cell is empty, the CItem* is null
//     and the if(pItem) guard skips it.)
//
// The modern port covers:
//   - Ctor / dtor: empty (no-op) — 1:1 with legacy.
//   - AddItem: REAL wrap of cIconDialog::AddIcon. Returns
//     true if base AddIcon succeeds, else false. The
//     Korean "!!!복사본 옵션 적용" comment is preserved as
//     a doc-only no-op (the legacy had no code there
//     either, just a TODO marker).
//   - DeleteItem: REAL wrap of cIconDialog::DeleteIcon.
//     Same as AddItem.
//   - GetBlankPositionRestrictRef: REAL — scan cells
//     [0, kSlotPetWearNum) using cIconDialog::IsAddable,
//     return true + set absPos = kTpPetWearStart + i
//     when the first addable cell is found. The
//     kSlotPetWearNum=3 / kTpPetWearStart=490 constants
//     are taken from legacy [CC]Header/CommonGameDefine.h
//     (enum at line 1215/1269/1321/1376). The modern port
//     inlines them as class-level constexpr — we do not
//     include the shared header (which would break the
//     1:1 shared-header constraint per AGENTS.md).
//   - CheckDuplication: TODO — cItem is not yet ported
//     (R-12.x deferred, same constraint as
//     cWearedExDialog's Titan-vs-normal branch).
//     The modern method scans the cells, but instead of
//     dereferencing the icon (which is forward-declared
//     and could be a non-cItem subclass), it just checks
//     whether the cell is in use. When cItem is ported,
//     this becomes: `auto* item = static_cast<CItem*>(cell.icon);
//     if (item && item->GetItemIdx() == ItemIdx) return true;`
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 17th **Tier 2** dialog port (after cBailDialog).
// The dialog has no service dependency on the modern
// service interface (Phase 13) — all state lives in
// cIconDialog's cell array (3 cells).

#pragma once

#include "cIconDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cIcon;

class cPetWearedExDialog : public cIconDialog {
public:
    cPetWearedExDialog();
    ~cPetWearedExDialog() override;

    // ----- 1:1 with legacy CPetWearedExDialog::AddItem -----

    // 1:1 with legacy AddItem(WORD relPos, cIcon* InIcon).
    // Cast InIcon to CItem*, call cIconDialog::AddIcon.
    // Returns true if AddIcon succeeded, else false. The
    // legacy Korean "!!!복사본 옵션 적용" comment is
    // preserved as a doc-only TODO marker (no code in
    // the legacy either).
    bool AddItem(std::uint16_t relPos, cIcon* inIcon);

    // ----- 1:1 with legacy CPetWearedExDialog::DeleteItem -----

    // 1:1 with legacy DeleteItem(WORD relPos, cIcon**
    // outIcon). Cast *outIcon to CItem*, call
    // cIconDialog::DeleteIcon. Returns true if
    // DeleteIcon succeeded, else false.
    bool DeleteItem(std::uint16_t relPos, cIcon** outIcon);

    // ----- 1:1 with legacy CPetWearedExDialog::GetBlankPositionRestrictRef -----

    // 1:1 with legacy GetBlankPositionRestrictRef(WORD&
    // absPos). Scan cells [0, kSlotPetWearNum) using
    // cIconDialog::IsAddable. Return true with
    // absPos = kTpPetWearStart + i where i is the first
    // addable cell. Return false if all cells are
    // occupied. The kTpPetWearStart constant mirrors the
    // legacy TP_PETWEAR_START enum value (= 490,
    // defined as TP_PETINVEN_END in
    // [CC]Header/CommonGameDefine.h).
    bool GetBlankPositionRestrictRef(std::uint16_t& absPos);

    // ----- 1:1 with legacy CPetWearedExDialog::CheckDuplication -----

    // 1:1 with legacy CheckDuplication(DWORD ItemIdx).
    // Scan cells [0, kSlotPetWearNum) and check whether
    // any cell holds a CItem whose GetItemIdx() == ItemIdx.
    // The cItem class is not yet ported (R-12.x deferred,
    // same constraint as cWearedExDialog). The modern port
    // delegates the icon-to-item-index extraction to an
    // OPTIONAL host callback so the dialog stays decoupled
    // from the cItem port:
    //   for (int i = 0; i < kSlotPetWearNum; ++i) {
    //     auto* icon = GetIconForIdx(i);
    //     if (!icon) continue;
    //     if (!m_getItemIdxFn) return false;
    //     if (m_getItemIdxFn(icon, userData) == ItemIdx) return true;
    //   }
    //   return false;
    bool CheckDuplication(std::uint32_t itemIdx);

    using GetItemIdxFn = std::uint32_t (*)(cIcon* icon, void* userData);

    // Replaces legacy cItem::GetItemIdx() calls. The host
    // receives each in-use cIcon* and returns the item idx
    // stored in it. OPTIONAL: when no callback is registered
    // CheckDuplication returns false (1:1 with the prior
    // cItem-not-ported TODO state).
    void SetItemIdxCallback(GetItemIdxFn getItemIdx,
                            void* userData = nullptr) noexcept;

    // ----- Constants from legacy [CC]Header/CommonGameDefine.h -----

    // SLOT_PETWEAR_NUM = 3 (the number of pet equipment
    // slots). 1:1 with the legacy enum.
    static constexpr std::uint16_t kSlotPetWearNum = 3;

    // TP_PETWEAR_START = 490 (the absolute tab-position
    // index where pet equipment slots begin, defined as
    // TP_PETINVEN_END in the legacy enum). 1:1 with the
    // legacy enum. The modern port inlines this constant
    // (rather than including the shared header) to keep
    // the 1:1 shared-header constraint per AGENTS.md.
    static constexpr std::uint16_t kTpPetWearStart = 490;

    GetItemIdxFn m_getItemIdxFn = nullptr;
    void* m_callbackUserData = nullptr;
};

}  // namespace mxh::ui
