// titanrepairdlg.hpp — modern port of 墨香 CTitanRepairDlg (titan repair).
//
// 1:1 port of legacy `CTitanRepairDlg` from
//   `墨香【源码】\[Client]MH\TitanRepairDlg.{h,cpp}`.
//
// CTitanRepairDlg is the "titan repair" dialog: clicking the
// repair-part button toggles a cursor mode (eCURSOR_TITANREPAIR)
// for selecting a single part to repair, and the repair-all
// button sends MSG_TITAN_REPAIR_TOTAL_EQUIPITEM_SYN to the
// server after showing a WINDOWMGR confirm msgbox with the
// total cost. On dialog close (val==FALSE), the dialog cascades
// the eObjectState_Deal state-end + cursor reset, and on the
// WE_CLOSEWINDOW event it ends the deal state and closes
// inventory + titan inventory dialogs.
//
// 1:1 contract preserved:
//   - Linking() — empty body in legacy. Modern port preserves
//     empty body verbatim.
//   - SetActive(BOOL val) override — calls base cDialog::SetActive,
//     then on val==FALSE: ends eObjectState_Deal if active, and
//     resets cursor to eCURSOR_DEFAULT if it's currently
//     eCURSOR_TITANREPAIR. Modern port: HERO/OBJECTSTATEMGR/
//     CURSOR singletons stubbed no-op per Phase 6 pattern.
//   - OnActionEvent(lId, p, we) — branches on WE_CLOSEWINDOW
//     (end eObjectState_Deal + close inventory + titan-inventory
//     dialogs via GAMEIN), then on TITAN_REPAIR_PART (toggle
//     cursor between eCURSOR_TITANREPAIR and eCURSOR_DEFAULT),
//     and on TITAN_REPAIR_ALL (compute total repair cost via
//     TITANMGR + WINDOWMGR confirm msgbox via CHATMGR). All
//     singletons stubbed no-op.
//
// 1:1 quirks preserved:
//   - 1:1 quirk: legacy class name in comment is `CTitanPartsChangeDlg`
//     but the .h/.cpp filenames are `TitanRepairDlg` — legacy
//     copy-paste residue. Modern port uses `cTitanRepairDlg` to
//     match the filenames (and the existing P2-12 roadmap).
//   - 1:1 quirk: legacy ctor + dtor have empty bodies. Modern
//     port preserves the empty bodies via `= default`.
//   - 1:1 quirk: legacy Linking() body is empty (the dialog
//     has no child widgets to wire). Modern port preserves
//     the empty body verbatim.
//   - 1:1 quirk: legacy SetActive override is `virtual`. Modern
//     port: `void SetActive(bool val) noexcept override` (R-12
//     fix: virtual SetActive is noexcept in cDialog base).
//   - 1:1 quirk: legacy OnActionEvent has a 1:1 comment residue
//     "GAMEIN->GetTitanRepairDlg()->SetActive(FALSE);" — that
//     call is commented out (it would self-close the dialog
//     we're handling, which would infinite-loop). Modern port
//     preserves the commented-out call as a 1:1 quirk note.
//   - 1:1 quirk: legacy OnActionEvent has Korean comments
//     (e.g. "��������" = "individual repair", "��ü����" =
//     "all repair"). Modern port preserves the comments in the
//     1:1 quirks section of the .cpp (CJK characters would
//     be mojibake in some encoders).
//   - 1:1 quirk: legacy OnActionEvent first switch uses `we` as
//     the discriminator (not `we & WE_BTNCLICK` like most
//     dialogs). Modern port preserves the exact `we` switch —
//     WE_CLOSEWINDOW is the only branch — and the second
//     switch uses `lId` (per legacy).
//   - 1:1 quirk: legacy `OnActionEvent` returns TRUE for both
//     branches. Modern port preserves this verbatim (modern
//     port returns true).
//   - 1:1 quirk: legacy WearedExDialog.h / InventoryExDialog.h
//     / TitanInventoryDlg.h includes are not used in the
//     dialog body (only via GAMEIN->GetXxx()). Modern port
//     omits those legacy includes.
//   - 1:1 quirk: legacy MHAudioManager.h / MouseCursor.h are
//     pulled in for the global CURSOR + audio singletons.
//     Modern port uses a `StubCursor` helper that mimics the
//     legacy CURSOR state machine (eCURSOR_DEFAULT vs
//     eCURSOR_TITANREPAIR), with state read/written via
//     test-injectable accessors.
//   - 1:1 quirk: legacy 6-singleton dispatch (HERO/
//     OBJECTSTATEMGR / CURSOR / GAMEIN / CHATMGR / WINDOWMGR /
//     TITANMGR) — all stubbed no-op per Phase 6 pattern.
//   - 1:1 quirk: legacy TITAN_REPAIR_PART / TITAN_REPAIR_ALL
//     come from `WindowIDEnum.h`. Modern port uses a local
//     id range 2100-2101 to avoid coupling to the legacy
//     header.
//   - 1:1 quirk: legacy ChatManager message ids 1582 (no
//     items to repair) and 1543 (repair confirm) are unused
//     in the modern port (CHATMGR stubbed). They are
//     documented as constants for production wiring.
//   - 1:1 quirk: legacy MBI_TITAN_TOTAL_REPAIR / MBT_YESNO
//     are unused in the modern port (WINDOWMGR stubbed).
//     They are documented as constants for production wiring.
//   - 1:1 quirk: legacy `MSG_TITAN_REPAIR_TOTAL_EQUIPITEM_SYN`
//     struct (from [CC]Header/CommonStruct.h) is replaced by
//     a stub `TITANMGR->GetTitanEnduranceTotalInfo(&msg, TRUE)`
//     returning 0 (modern port returns 0 → "no items to
//     repair" branch). Production code would link a real
//     TITANMGR + CommonStruct.h.

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

// 1:1 with legacy WindowIDEnum.h. Local id range to avoid
// coupling to the legacy header.
constexpr int kIdTitanRepairPart = 2100;
constexpr int kIdTitanRepairAll  = 2101;

// 1:1 with legacy WE_CLOSEWINDOW=1 (per legacy cWindowDef.h
// enum WINDOW_EVENT). Modern cWindow::WindowEvent does not
// yet define a CloseWindow value (deferred to Phase 6.1.x
// per cWindow.hpp), so the modern port uses a local constant
// matching the legacy value. Production code would route
// close-window events through cWindow::WindowEvent after the
// Phase 6.1.x enum extension.
constexpr std::uint32_t kWeCloseWindow = 1u;

// 1:1 with legacy CHATMGR message ids (from ChatManager.h).
constexpr int kChatMsgNoItemsToRepair = 1582;
constexpr int kChatMsgRepairConfirm   = 1543;

// 1:1 with legacy WINDOWMGR / cMsgBox enums.
constexpr int kMbiTitanTotalRepair = 1543;

// 1:1 with legacy cursor state (from MouseCursor.h).
enum class ECursorState : std::int32_t {
    Default       = 0,  // eCURSOR_DEFAULT
    TitanRepair   = 1,  // eCURSOR_TITANREPAIR
};

class cTitanRepairDlg : public cDialog {
public:
    cTitanRepairDlg();
    ~cTitanRepairDlg() override;

    cTitanRepairDlg(const cTitanRepairDlg&) = delete;
    cTitanRepairDlg& operator=(const cTitanRepairDlg&) = delete;

    void Linking();
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy OnActionEvent — returns true on both
    // branches (WE_CLOSEWINDOW and the per-id switch).
    bool OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // Test accessors.
    ECursorState cursorState() const noexcept { return s_cursor; }
    bool objectStateDealEnded() const noexcept { return s_objectStateDealEnded; }
    std::uint32_t inventoryDialogCloseCount() const noexcept { return s_inventoryDialogCloseCount; }
    std::uint32_t titanInventoryDialogCloseCount() const noexcept { return s_titanInventoryDialogCloseCount; }
    std::uint32_t chatMsgNoItemsToRepairCount() const noexcept { return s_chatMsgNoItemsCount; }
    std::uint32_t chatMsgRepairConfirmCount() const noexcept { return s_chatMsgRepairConfirmCount; }
    std::uint32_t windowMgrMsgBoxCount() const noexcept { return s_windowMgrMsgBoxCount; }
    std::uint32_t titanMgrRepairCallCount() const noexcept { return s_titanMgrRepairCallCount; }

    // Test-injectable TITANMGR stub: legacy GetTitanEnduranceTotalInfo
    // returns a DWORD cost (0 = no items to repair). Modern port
    // tests inject a non-zero cost to verify the WINDOWMGR
    // confirm msgbox path. Default is 0 (no items).
    static void SetTitanRepairCostForTesting(std::uint32_t cost) noexcept {
        s_titanRepairCost = cost;
    }
    static std::uint32_t titanRepairCostForTesting() noexcept {
        return s_titanRepairCost;
    }
    static void ClearTestInjections() noexcept;

private:
    // Test-injectable singletons (replaces the legacy globals: HERO,
    // OBJECTSTATEMGR, CURSOR, GAMEIN, CHATMGR, WINDOWMGR, TITANMGR).
    static inline ECursorState s_cursor = ECursorState::Default;
    static inline bool s_objectStateDealEnded = false;
    static inline std::uint32_t s_inventoryDialogCloseCount = 0;
    static inline std::uint32_t s_titanInventoryDialogCloseCount = 0;
    static inline std::uint32_t s_chatMsgNoItemsCount = 0;
    static inline std::uint32_t s_chatMsgRepairConfirmCount = 0;
    static inline std::uint32_t s_windowMgrMsgBoxCount = 0;
    static inline std::uint32_t s_titanMgrRepairCallCount = 0;
    static inline std::uint32_t s_titanRepairCost = 0;
};

} // namespace mxh::ui
