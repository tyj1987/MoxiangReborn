// pklootingdialog.hpp — modern port of 墨香 cPKLootingDialog
// (PK loot dialog: when a player kills another, the loser can roll
// for items from the corpse before a 30-second timer runs out).
//
// 1:1 port of legacy `CPKLootingDialog` from
//   `墨香【源码】\[Client]MH\PKLootingDialog.{h,cpp}` (8.7 KB legacy
//   code; the modern port keeps the data model + state machine +
//   timer/delay logic and stubs the engine-side dispatch with
//   no-ops until Phase 13+ real impl lands).
//
// Modern port scope (this commit):
//   - 7 cStatic children (bad-fame / time / chance / target-name /
//     item-count / end-text / none-text).
//   - 1 cIconGridDialog child (the loot grid itself; 12 cells, one
//     per item slot).
//   - State fields:
//       * m_dwDiePlayerIdx (the dead player's object id)
//       * m_nTime (remaining seconds)
//       * m_dwStartTime (timer start, ms)
//       * m_nChance (remaining pick chances)
//       * m_nLootItemNum (remaining item count)
//       * m_bSelected[12] (per-cell "already picked" flag)
//       * m_bLootingEnd / m_bMsgSync (lifecycle flags)
//       * m_dwCreateTime + m_bShow (delayed-show machinery)
//   - InitPKLootDlg / Linking (1:1 quirk: 7 cStatic + 1 cIconGridDialog
//     captured via id-based resolve, same as legacy).
//   - ActionEvent override (m_bShow delay + per-second timer
//     countdown; decrements m_nTime and on expiry calls the
//     engine-side CloseLootingDialog — the modern port's
//     SetLootingEnd + m_bShow flip is the side effect).
//   - OnActionEvent (handles loot-cell click; decrements chance +
//     disables grid when chance runs out; sets m_bMsgSync for the
//     network round-trip; engine-side Send / ObjectManager calls
//     are stubbed).
//   - ReleaseAllIcon / ChangeIconImage / AddLootingItemNum (the
//     data-side helpers used by the engine's loot / drop / take
//     flow).
//   - SetMsgSync (the engine-side SetDisable-Dialog cross-disable
//     is reduced to a state flag in the modern port; the actual
//     inventory / warehouse SetDisable is the engine's job).
//
// Modern-port simplifications (all documented in the .cpp file header):
// 1. Engine singleton dependencies (PKMGR, HERO, OBJECTMGR, ITEMMGR,
//    CHATMGR, NETWORK) are stubbed to no-op. The data-side state
//    (chance / item-count / end-flag / msg-sync) is preserved 1:1;
//    the network-send / object-lookup / item-image-fetch side
//    effects are deferred to the engine-binder layer (Phase 14+).
// 2. cIcon grid contents (m_pIGDItem) are cIcon* opaque; the
//    modern port calls m_pIGDItem->AddIcon / DeleteIcon / GetIconForIdx
//    but the cIcon* itself is never dereferenced.
// 3. Render is a no-op (real draw + sprite bind lands with the 6.6
//    cImage seam).
// 4. The 30-second timer / 1-second delay constants are exposed as
//    public constexpr so tests can override them if needed.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this is a
// Tier 2 dialog port (0.13.46). It unblocks any Tier 2 dialog that
// needs a timed-loot / timed-pick flow.

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;
class cIconGridDialog;

class cPKLootingDialog : public cDialog {
public:
    // Constants (1:1 with legacy values).
    // PKLOOTING_ITEM_NUM: the legacy array size; 12 cells visible
    // in the loot grid (the grid is laid out as 4 cols x 3 rows
    // when InitGrid is called with these values).
    static constexpr std::uint16_t PKLOOTING_ITEM_NUM    = 12;
    // PKLOOTING_LIMIT_TIME: the legacy 30-second timer (ms).
    static constexpr std::uint32_t PKLOOTING_LIMIT_TIME  = 30000;
    // PKLOOTING_DLG_DELAY_TIME: the legacy 1-second delay between
    // Init and the dialog actually becoming visible (used to give
    // the death animation a beat before the loot dialog pops up).
    static constexpr std::uint32_t PKLOOTING_DLG_DELAY_TIME = 1000;

    // Item kind enum (1:1 with legacy eLOOTINGITEM_KIND).
    enum class LootItemKind : std::int32_t {
        Item = 0,
        Money = 1,
        Exp = 2,
        None = 3,
    };

    cPKLootingDialog();
    ~cPKLootingDialog() override;

    cPKLootingDialog(const cPKLootingDialog&) = delete;
    cPKLootingDialog& operator=(const cPKLootingDialog&) = delete;

    // -------------------------------------------------------------------------
    // Init: id + position + the dead player's object index. Mirrors legacy
    // CPKLootingDialog::InitPKLootDlg(dwID, x, y, dwDiePlayerIdx).
    // -------------------------------------------------------------------------
    void InitPKLootDlg(std::int32_t dwID, std::int32_t x, std::int32_t y,
                       std::uint32_t dwDiePlayerIdx);

    // Linking: 1:1 with legacy. Resolves 7 cStatic + 1 cIconGridDialog
    // by id (the legacy uses GetWindowForID). After Linking(), the
    // cPKLootingDialog has non-owning raw pointers to its children
    // and can route OnActionEvent to them.
    void Linking();

    // Render placeholder (1:1 with legacy: cDialog::Render only when
    // m_bShow is true; the modern port is a no-op until the cImage
    // seam).
    void Render() override;

    // ActionEvent: handles the m_bShow delay + per-second timer
    // countdown. The legacy decrements m_nTime every second and on
    // expiry calls PKMGR->CloseLootingDialog. Modern port mirrors
    // the state-side effects and returns 0 from the engine call
    // (the engine-binder layer wires the actual close).
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    // OnActionEvent: routes a child-window event to the right
    // handler. Currently handles:
    //   - PLI_BTN_CLOSE + WE_BTNCLICK → SetLootingEnd(true)
    //     (engine-side CloseLootingDialog is stubbed).
    //   - PLI_IGD_ITEM + WE_LBTNCLICK → loot-cell pick (decrements
    //     chance, disables grid when chance runs out, sets
    //     m_bMsgSync for the network round-trip; engine-side
    //     distance check + Send is stubbed).
    //   - WE_CLOSEWINDOW → SetLootingEnd(true).
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // -------------------------------------------------------------------------
    // State accessors.
    // -------------------------------------------------------------------------
    std::uint32_t GetDiePlayerIdx() const noexcept { return m_dwDiePlayerIdx; }

    bool IsLootingEnd() const noexcept    { return m_bLootingEnd != 0; }
    void SetLootingEnd(bool bEnd) noexcept { m_bLootingEnd = bEnd ? 1 : 0; }

    void SetMsgSync(bool bSync) noexcept   { m_bMsgSync = bSync ? 1 : 0; }
    bool IsMsgSync() const noexcept        { return m_bMsgSync != 0; }

    // -------------------------------------------------------------------------
    // Data-side helpers (used by the engine's loot / drop / take flow).
    // -------------------------------------------------------------------------

    // ReleaseAllIcon: clears the loot grid (modern port's cIconGridDialog
    // owns the cIconGridCell array; the engine is responsible for
    // deleting the cIcon* payloads).
    void ReleaseAllIcon();

    // ChangeIconImage: switch the icon at the given cell to a different
    // kind (item / money / exp / none). The modern port records the
    // kind; the actual cImage fetch is the engine's job (deferred
    // to the cImage seam).
    void ChangeIconImage(std::uint16_t pos, LootItemKind nKind,
                         std::uint16_t ItemIdx = 0);

    // AddLootingItemNum: the server-push back handler. Decrements
    // m_nLootItemNum; if it hits 0, the grid is disabled and the
    // end-text is set. 1:1 with legacy CPKLootingDialog::AddLootingItemNum.
    void AddLootingItemNum();

    // -------------------------------------------------------------------------
    // Test accessors.
    // -------------------------------------------------------------------------
    int  GetTime() const noexcept       { return m_nTime; }
    int  GetChance() const noexcept     { return m_nChance; }
    int  GetLootItemNum() const noexcept { return m_nLootItemNum; }
    std::uint32_t GetCreateTime() const noexcept { return m_dwCreateTime; }
    bool IsShow() const noexcept        { return m_bShow != 0; }
    bool IsSelected(std::uint16_t idx) const noexcept;

    // WindowID constants (1:1 with legacy PLI_*).
    static constexpr std::int32_t ID_BTN_CLOSE     = 0;   // legacy PLI_BTN_CLOSE
    static constexpr std::int32_t ID_STC_BADFAME   = 1;   // legacy PLI_STC_BADFAME
    static constexpr std::int32_t ID_STC_TIME      = 2;   // legacy PLI_STC_TIME
    static constexpr std::int32_t ID_STC_CHANCE    = 3;   // legacy PLI_STC_CHANCE
    static constexpr std::int32_t ID_STC_TARGETNAME = 4;  // legacy PLI_STC_TARGETNAME
    static constexpr std::int32_t ID_STC_ITEM      = 5;   // legacy PLI_STC_ITEM
    static constexpr std::int32_t ID_STC_END       = 6;   // legacy PLI_STC_END
    static constexpr std::int32_t ID_STC_NONE      = 7;   // legacy PLI_STC_NONE
    static constexpr std::int32_t ID_IGD_ITEM      = 8;   // legacy PLI_IGD_ITEM

    // Test-only: override the clock used by ActionEvent's m_bShow
    // delay and timer countdown. Pass nullptr to restore the
    // default (returns 0, so the timer never elapses).
    static void SetClockForTesting(std::uint32_t (*fn)());

private:
    // Non-owning raw pointers (legacy m_pStc* / m_pIGDItem accessors).
    // Children are created in Linking() (per cAlertDlg pattern) and
    // owned by the dialog via the cWindow children list.
    cStatic*         m_pStcBadFame = nullptr;
    cStatic*         m_pStcTime    = nullptr;
    cStatic*         m_pStcChance  = nullptr;
    cStatic*         m_pStcTarget  = nullptr;
    cStatic*         m_pStcItem    = nullptr;
    cStatic*         m_pStcEnd     = nullptr;
    cStatic*         m_pStcNone    = nullptr;
    cIconGridDialog* m_pIGDItem    = nullptr;

    // State.
    std::uint32_t m_dwDiePlayerIdx = 0;
    int           m_nTime          = 0;     // remaining seconds
    std::uint32_t m_dwStartTime    = 0;     // timer start (ms)
    int           m_nChance        = 0;     // remaining pick chances
    int           m_nLootItemNum   = 0;     // remaining item count

    // Per-cell "already picked" flag.
    bool          m_bSelected[PKLOOTING_ITEM_NUM] = {};

    // Lifecycle flags.
    int           m_bLootingEnd = 0;
    int           m_bMsgSync    = 0;

    // Delayed-show machinery.
    std::uint32_t m_dwCreateTime = 0;
    int           m_bShow        = 0;

    // Per-cell current kind (1:1 with the eLIK_* enum).
    LootItemKind  m_nItemKind[PKLOOTING_ITEM_NUM] = {};

    // Helpers.
    void clearSelection();
    void setEndState();
};

} // namespace mxh::ui
