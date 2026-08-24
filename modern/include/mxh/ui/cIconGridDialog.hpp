// cIconGridDialog.hpp — modern port of 墨香 cIconGridDialog
// (2D icon grid container with drag-and-drop, the basis for
// inventory, equipment slots, loot grids, shop cells, etc.).
//
// 1:1 port of legacy `cIconGridDialog` from
//   `墨香【源码】\[Client]MH\Interface\cIconGridDialog.{h,cpp}`
// (19 KB of legacy code; the modern port keeps the data model + state
// machine and stubs render / drag-dispatch with no-ops until the 6.6+
// cImage seam is wired in).
//
// The legacy widget extends cDialog directly (NOT cIconDialog — the
// 1D-cell cIconDialog and the 2D-grid cIconGridDialog are siblings
// under cDialog). The grid is sized by `m_nRow` and `m_nCol`; each
// cell can hold one cIcon. Cells are addressed linearly:
//   pos = cellY * m_nCol + cellX
// which is the legacy `GetPositionForCell` convention.
//
// Modern port scope (this commit):
//   - InitDialog / InitGrid (2D layout setup, with explicit cell width /
//     height / border X / border Y).
//   - AddIcon / DeleteIcon / MoveIcon (linear + 2D-coord overloads).
//   - GetCellPosition / GetPositionForXYRef / GetPositionForCell /
//     GetCellAbsPos (hit-test math, ported verbatim from legacy).
//   - SetAbsXY cascade to non-link icons (the legacy's "depend" path).
//   - SetDisable / SetAlpha / SetActive cascade to dependent icons.
//   - PtInCell / IsAddable / GetCellNum (query surface).
//   - GetCurSelCellPos / SetCurSelCellPos / SetShowGrid /
//     IsDragOverDraw / SetDragOverIconType (selection + drag prep).
//   - m_acceptableIconType (per-instance icon-type bitmask).
//
// Render-side hooks (SetIconCellBGImage / SetDragOverBGImage) and
// Render() are inert no-ops so 1:1 UI integration tests don't have to
// wire images. The cImage seam arrives in 6.6 alongside the rest of
// the 6.x render family.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this is a
// Tier 1.5 subcontrol port (alongside cListDialogEx in 0.13.13,
// cGuagen in 0.13.12). It unblocks Tier 2 dialogs that need
// multi-cell icon grids: PKLootingDialog (0.13.46), InventoryDialog
// (Tier 1.5, future), ShopDialog (Tier 2, future), etc.

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

// Forward declaration; the modern cIconGridDialog accepts any
// cWindow-derived pointer as the icon payload (we do not dereference
// it during testing).
class cIcon;

struct cIconGridCell {
    cIcon* icon  = nullptr;   // cWindow* payload (1:1 with legacy)
    bool   inUse = false;     // legacy BOOL use (NOTUSE / USE)
};

// Modern port of legacy `cIconGridDialog` (extends cDialog directly,
// 2D grid layout). The legacy cbWindowFunc dispatch is replaced with
// a no-op so the dialog is testable in isolation; the 6.6 dispatcher
// integration is the follow-up seam.
class cIconGridDialog : public cDialog {
public:
    cIconGridDialog();
    ~cIconGridDialog() override;

    // -------------------------------------------------------------------------
    // Init: position, size, basic image, grid dims (col x row), id.
    // The 8-param signature mirrors legacy cIconGridDialog::InitDialog
    // (x, y, wid, hei, basicImage, col, row, id=0) — the only deviation
    // is renaming InitDialog → Init for the modern cDialog family
    // convention.
    // -------------------------------------------------------------------------
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
              std::uint16_t hei, void* basicImage,
              std::uint16_t col, std::uint16_t row,
              std::int32_t id = 0);

    // InitGrid: layout the cells (relative position, per-cell size,
    // per-cell border). Must be called after Init() (so the cell
    // array exists). Mirrors legacy cIconGridDialog::InitGrid.
    void InitGrid(std::int32_t gridX, std::int32_t gridY,
                  std::uint16_t cellWid, std::uint16_t cellHei,
                  std::uint16_t borderX,  std::uint16_t borderY);

    // Render placeholder. The real draw (selected-bg, drag-over-bg,
    // per-cell icon sprites) lands with the 6.6 cImage seam.
    void Render() override {}

    // ActionEvent: row hit-test + selection update + drag dispatch.
    // Returns WE_LBTNCLICK on a successful cell click, otherwise the
    // WE_* code from cDialog's child dispatch. Modern port stubs the
    // legacy `cbWindowFunc` dispatch with a no-op flag; the 6.6
    // dispatcher integration is the follow-up seam.
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    // -------------------------------------------------------------------------
    // Layout cascade.
    // -------------------------------------------------------------------------

    // SetAbsXY cascades to dependent icons (legacy IsDepend path:
    // m_pIconGridCell[i].icon->IsDepend()). 1:1 with legacy
    // cIconGridDialog::SetAbsXY. Must be `noexcept` (R-12 polymorphic
    // virtual under /permissive- requires C2694 exception-spec match).
    void SetAbsXY(std::int32_t x, std::int32_t y) noexcept override;

    // SetActiveRecursive: cascading active-state to dependent icons.
    // 1:1 with legacy cIconGridDialog::SetActive (which calls
    // cDialog::SetActiveRecursive under the hood, then iterates
    // dependent icons). Modern cDialog::SetActiveRecursive already
    // cascades through children, so this iterates the icon grid cells.
    void SetActive(bool val) noexcept override;

    // SetDisable cascades to dependent icons.
    void SetDisable(bool val) noexcept override;

    // SetAlpha cascades to dependent icons (1:1 with legacy).
    void SetAlpha(std::uint8_t al) noexcept;

    // -------------------------------------------------------------------------
    // Cell hit-test math. Ported verbatim from the legacy.
    // -------------------------------------------------------------------------
    bool PtInCell(std::int32_t x, std::int32_t y) const noexcept;
    bool GetCellPosition(std::int32_t mouseX, std::int32_t mouseY,
                         std::uint16_t& cellX, std::uint16_t& cellY) const noexcept;
    bool GetPositionForXYRef(std::int32_t mouseX, std::int32_t mouseY,
                             std::uint16_t& pos) const noexcept;
    std::uint16_t GetPositionForCell(std::uint16_t cellX, std::uint16_t cellY) const noexcept;
    bool GetCellAbsPos(std::uint16_t pos, int& outAbsX, int& outAbsY) const noexcept;

    // -------------------------------------------------------------------------
    // AddIcon / DeleteIcon / MoveIcon. 1:1 with legacy.
    // -------------------------------------------------------------------------

    // Linear: pos = cellY * m_nCol + cellX.
    bool AddIcon(std::uint16_t pos, cIcon* icon);
    bool DeleteIcon(std::uint16_t pos, cIcon** icon);

    // 2D coords.
    bool AddIcon(std::uint16_t cellX, std::uint16_t cellY, cIcon* icon);
    bool DeleteIcon(std::uint16_t cellX, std::uint16_t cellY, cIcon** iconOut);
    bool DeleteIcon(cIcon* icon);

    // MoveIcon: 1:1 with legacy. Removes from the previous grid and
    // adds to (cellX, cellY). Returns FALSE if the destination is
    // already occupied (and the source keeps the icon).
    bool MoveIcon(std::uint16_t cellX, std::uint16_t cellY, cIcon* icon);

    // -------------------------------------------------------------------------
    // Query.
    // -------------------------------------------------------------------------
    std::uint16_t GetCellNum() const noexcept   { return static_cast<std::uint16_t>(m_nRow) * m_nCol; }
    cIcon*        GetIconForIdx(std::uint16_t idx) const;
    bool          IsAddable(std::uint16_t idx) const noexcept;
    bool          IsAddable(std::uint16_t cellX, std::uint16_t cellY, cIcon* pIcon) const noexcept;

    // -------------------------------------------------------------------------
    // Selection / drag state.
    // -------------------------------------------------------------------------
    std::int32_t GetCurSelCellPos() const noexcept  { return m_lCurSelCellPos; }
    void         SetCurSelCellPos(std::int32_t pos) noexcept { m_lCurSelCellPos = pos; }

    // -------------------------------------------------------------------------
    // Render-side hooks the legacy exposed; here they're inert no-ops
    // so the 1:1 UI integration tests don't have to wire images. The
    // cImage seam arrives in 6.6.
    // -------------------------------------------------------------------------
    void SetIconCellBGImage(void* /*img*/) noexcept {}
    void SetDragOverBGImage(void* /*img*/) noexcept {}

    void SetShowGrid(bool val) noexcept         { m_bShowGrid = val; }
    bool IsShowGrid() const noexcept           { return m_bShowGrid; }

    void SetDragOverIconType(int nIconType) noexcept { m_nIconType = nIconType; }
    int  GetDragOverIconType() const noexcept       { return m_nIconType; }

    // IsDragOverDraw: 1:1 with legacy. Returns true if a window is
    // currently being dragged AND that window's type matches the
    // grid's m_nIconType. Modern port returns false unconditionally
    // (the cWindowManager drag integration lands with 6.6).
    bool IsDragOverDraw() const noexcept { return false; }

    // -------------------------------------------------------------------------
    // Acceptable icon type bitmask (legacy m_acceptableIconType).
    // cIcon::GetIconType() & m_acceptableIconType must be non-zero
    // for AddIcon to accept the icon.
    // -------------------------------------------------------------------------
    void          SetAcceptableIconType(std::uint32_t type) noexcept { m_acceptableIconType = type; }
    std::uint32_t GetAcceptableIconType() const noexcept            { return m_acceptableIconType; }

    // -------------------------------------------------------------------------
    // Cell rect (legacy SetCellRect). The legacy exposes the cell rect
    // for outside-of-grid placements (e.g. when an external cDialog
    // hosts a cIconGridDialog and wants to know where to put the
    // parent's children). Modern port stores the rect verbatim.
    // -------------------------------------------------------------------------
    struct CellRect { std::int32_t left, top, right, bottom; };
    void  SetCellRect(std::int32_t l, std::int32_t t, std::int32_t r, std::int32_t b) noexcept;
    CellRect GetCellRect() const noexcept;

    // Constants from the legacy {NOTUSE=0, USE=1} enum.
    static constexpr int NOTUSE = 0;
    static constexpr int USE    = 1;

    // Cell default size (legacy DEFAULT_CELLSIZE). Used by GetCellPosition
    // for the hit-test math, which the legacy wrote with this constant
    // (not m_wCellWidth / m_wCellHeight — looks like a legacy bug we
    // preserve 1:1; see tests for the corresponding expectations).
    static constexpr std::uint16_t DEFAULT_CELLSIZE = 40;
    static constexpr std::uint16_t DEFAULT_CELLBORDER = 4;

    // Read-only access for tests / inspectors.
    std::uint16_t row() const noexcept  { return m_nRow; }
    std::uint16_t col() const noexcept  { return m_nCol; }

private:
    cIconGridCell*   m_pIconGridCell = nullptr;
    std::uint16_t    m_nRow          = 0;
    std::uint16_t    m_nCol          = 0;

    // Grid layout (set by InitGrid).
    std::int32_t     m_gridX         = 0;
    std::int32_t     m_gridY         = 0;
    std::uint16_t    m_wCellWidth    = DEFAULT_CELLSIZE;
    std::uint16_t    m_wCellHeight   = DEFAULT_CELLSIZE;
    std::uint16_t    m_wCellBorderX  = DEFAULT_CELLBORDER;
    std::uint16_t    m_wCellBorderY  = DEFAULT_CELLBORDER;

    CellRect         m_cellRect      {0, 0, 0, 0};

    // Selection + drag state.
    std::int32_t     m_lCurSelCellPos   = -1;
    std::int32_t     m_lCurDragOverPos  = -1;
    bool             m_bItemDraged      = false;
    bool             m_bShowGrid        = false;
    int              m_nIconType        = 0;

    // Acceptable icon type bitmask.
    std::uint32_t    m_acceptableIconType = 0xFFFFFFFFu;

    // Helpers.
    bool cellInBounds(std::uint16_t cellX, std::uint16_t cellY) const noexcept;
    bool cellInBounds(std::uint16_t pos) const noexcept;
    void computeCellRect() noexcept;
    bool isIconDepend(cIcon* icon) const noexcept;  // 1:1 with legacy IsDepend()
};

} // namespace mxh::ui
