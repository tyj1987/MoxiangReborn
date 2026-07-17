// skinselectdialog.hpp — modern port of 墨香 CSkinSelectDialog
// (skin-select dialog: lets the player pick a costume-skin from
// the catalog, with a 3-cell preview of the equipped items).
//
// 1:1 port of legacy `CSkinSelectDialog` from
//   `墨香【源码】\[Client]MH\SkinSelectDialog.{h,cpp}` (~200 lines
//   legacy code; the modern port keeps the data model + state
//   machine + 1:1 quirks and stubs the engine-side singletons +
//   the CItemShow render path with no-ops until Phase 13+ real
//   impl lands).
//
// Modern port scope (this commit):
//   - 2 children: 1 cListDialog (skin list) + 1 cIconDialog
//     (3-cell preview, holds the equipped-items preview).
//   - State:
//       * m_dwSelectIdx (the selected skin index, 1-based per
//         legacy quirk: legacy uses Idx+1 when storing because
//         index 0 means "no selection"; the modern port
//         preserves the +1 offset for 1:1 parity).
//       * m_dwSkinDelayTime (skin-change cooldown)
//       * m_bSkinDelayResult (whether the cooldown is currently
//         satisfied)
//   - Linking: 1:1 with legacy — GetWindowForID(SKIN_SELECTLIST)
//     + GetWindowForID(SKIN_SELECT_ITEMVIEW) + SetShowSelect(TRUE).
//   - SetActive(BOOL) override: 1:1 — cDialog::SetActive + if FALSE
//     clear list / icon / idx, else call SkinItemListInfo().
//   - ActionEvent(CMouse*): 1:1 — cDialog::ActionEvent + list
//     hit-test + on click, populate the 3-cell preview with
//     placeholder cIcon entries (the real cIcon* comes from the
//     engine via CItemShow::Init in the legacy; modern port uses
//     placeholder pointers).
//   - OnActionEvent(lId, p, we) handler for close-window +
//     SKIN_SELECT_OK / CANCEL / RECOVERY. The OK + RECOVERY paths
//     trigger engine-side network send; modern port flips
//     m_bSkinDelayResult to reflect the cooldown.
//   - SkinItemListInfo() — populates the cListDialog from
//     GameResourceManager->GetNomalClothesSkinListCountNum().
//     Engine-side stubbed to 0 in the modern port (no skin data
//     available); the data-side loop + AddItem + color-from-level
//     logic is preserved 1:1.
//
// Modern-port simplifications (all documented in the .cpp file
// header):
// 1. CItemShow (engine-side class) is opaque in modern; the
//    m_NomalSkinView[3] array is declared as `void*` (no real
//    array — the placeholder pointers are stored inline).
// 2. Engine singletons (GAMERESRCMNGR / HERO / CHATMGR /
//    OBJECTMGR / ITEMMGR / NETWORK / WINDOWMGR) are stubbed to
//    no-op. The data-side state (select-idx / delay-flag /
//    list-population) is preserved 1:1.
// 3. InitSkinDelayTime / StartSkinDelayTime / CheckDelay are
//    legacy helper methods that are commented-out in the legacy
//    header; modern port drops them (they were never called in
//    the legacy).
// 4. The legacy dtor calls m_pNomalSkinListDlg->RemoveAll()
//    without a NULL check (1:1 quirk — if Linking() was never
//    called, this would crash). Modern port preserves the 1:1
//    behavior; the tests always call Linking first.
// 5. ActionEvent / OnActionEvent / Render are mostly engine-side
//    stubs (CMouse / real network / real CItemShow). The state-
//    side effects are preserved; the engine-binder layer
//    (Phase 14+) will replace the stubs.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this is a
// Tier 2 dialog port (0.13.47). It unblocks any Tier 2 dialog
// that needs a skin-selector preview (e.g. future Costume dialog
// port, since legacy has COSTUME_SKIN_SELECT_DLG too).

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cListDialog;
class cIconDialog;

class cSkinSelectDialog : public cDialog {
public:
    // 1:1 with legacy SKINITEM_LIST_MAX. The legacy uses this as
    // the size of m_NomalSkinView[] and as the upper bound of the
    // "for i = 0; i < SKINITEM_LIST_MAX; ++i" preview loop.
    static constexpr std::uint16_t SKINITEM_LIST_MAX = 3;

    // Local id range (1:1 with legacy PLI_*-style ids, but using
    // a small local offset since the modern port doesn't share
    // the global legacy id space). The id range 0-5 covers:
    //   0: SKIN_SELECT_DLG (the dialog itself)
    //   1: SKIN_SELECT_ITEMVIEW (the 3-cell preview cIconDialog)
    //   2: SKIN_SELECTLIST (the cListDialog)
    //   3: SKIN_SELECT_OK
    //   4: SKIN_SELECT_CANCEL
    //   5: SKIN_SELECT_RECOVERY
    static constexpr std::int32_t ID_DLG        = 0;
    static constexpr std::int32_t ID_ITEMVIEW   = 1;
    static constexpr std::int32_t ID_LIST       = 2;
    static constexpr std::int32_t ID_OK         = 3;
    static constexpr std::int32_t ID_CANCEL     = 4;
    static constexpr std::int32_t ID_RECOVERY   = 5;

    cSkinSelectDialog();
    ~cSkinSelectDialog() override;

    cSkinSelectDialog(const cSkinSelectDialog&) = delete;
    cSkinSelectDialog& operator=(const cSkinSelectDialog&) = delete;

    // -------------------------------------------------------------------------
    // Linking: 1:1 with legacy. Resolves 2 children by id
    // (legacy GetWindowForID equivalents via modern findWindowById).
    // The legacy also sets SetShowSelect(TRUE) on the list dialog
    // (default is FALSE in the modern cListDialog; the legacy
    // override is preserved for 1:1 behavior).
    // -------------------------------------------------------------------------
    void Linking();

    // SetActive(BOOL) override: 1:1 with legacy. val==FALSE clears
    // the list / icon / select-idx; val==TRUE calls
    // SkinItemListInfo().
    void SetActive(bool val) noexcept override;

    // ActionEvent: 1:1 with legacy. Drives the click → preview
    // populate flow. The modern port is a no-op for the engine-
    // side effects (CMouse / real cIcon* populate); only the
    // select-idx + list-clear side effects are preserved.
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    // OnActionEvent: handles WE_CLOSEWINDOW + the 3 button ids
    // (OK / CANCEL / RECOVERY). Engine-side network-send is
    // stubbed; the data-side state is preserved.
    bool OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // SkinItemListInfo: populates the cListDialog from
    // GameResourceManager (engine-side stubbed to 0 entries in
    // the modern port). Color-from-level is preserved 1:1.
    void SkinItemListInfo();

    // -------------------------------------------------------------------------
    // Test accessors.
    // -------------------------------------------------------------------------
    std::uint32_t GetSelectIdx() const noexcept        { return m_dwSelectIdx; }
    std::uint32_t GetSkinDelayTime() const noexcept   { return m_dwSkinDelayTime; }
    bool          IsSkinDelayResult() const noexcept   { return m_bSkinDelayResult; }

    // 1:1 with the legacy 1-based select-idx convention: the
    // modern port stores m_dwSelectIdx as Idx+1 (so 0 means "no
    // selection"). Test helper to set it directly.
    void          SetSelectIdx(std::uint32_t idx) noexcept { m_dwSelectIdx = idx; }
    void          SetSkinDelayResult(bool v) noexcept      { m_bSkinDelayResult = v; }

private:
    // 2 children. Stored as std::unique_ptr (created in Linking);
    // non-owning raw pointers for legacy m_pNomalSkinListDlg /
    // m_pNomalSkinIconDlg accessors.
    std::unique_ptr<cListDialog> m_upSkinList;
    std::unique_ptr<cIconDialog> m_upSkinIcon;
    cListDialog*  m_pNomalSkinListDlg = nullptr;
    cIconDialog*  m_pNomalSkinIconDlg = nullptr;

    // State.
    std::uint32_t m_dwSelectIdx       = 0;   // 1-based per legacy quirk
    std::uint32_t m_dwSkinDelayTime   = 0;
    bool          m_bSkinDelayResult  = false;

    // Helper: 1:1 with legacy, populates the 3-cell preview with
    // placeholder cIcon* entries.
    void populatePreview();
};

} // namespace mxh::ui
