// cIconDialog.hpp — modern port of 墨香 cIconDialog (icon grid container).
//
// 1:1 port of the data model behind legacy `cIconDialog` from
//   `墨香【源码】\[Client]MH\interface\cIconDialog.{h,cpp}`.
//
// The legacy widget backs every icon-grid in the game: inventory, equip
// slots, quick-bar, mini-map icons, etc. It owns a fixed-size array of
// `cIconCell` slots; each slot may hold one `cIcon` (a sprite + drag
// state) and a rectangle (relative to the dialog origin) describing
// where the slot lives. The container dispatches clicks via cDialog +
// per-cell hit-test (`PtInCell` / `GetPositionForXYRef`), manages
// selection (`m_lCurSelCellPos`), and supports type filtering
// (`m_acceptableIconType`).
//
// Modern port scope:
//   - Cell layout + AddIconCell/SetCellNum/AddIcon/DeleteIcon state.
//   - PtInCell / GetPositionForXYRef (used by inventory hit-tests).
//   - Selection / accessibility flags (acceptableIconType).
//   - SetAbsXY delegation that drags linked icons with the dialog
//     (the legacy's `bOnlyLink=FALSE` path; `bOnlyLink=TRUE` slots
//     keep their world-anchored position).
//
// Drag-and-drop + render side effects are out of scope here — they need
// the legacy `cIcon` class (which is a cWindow-derived sprite) plus
// the dispatcher integration. The Phase 6 series keeps the data model
// 1:1 so a follow-up Phase 7 / 8 task can wire the cIcon sprite on top
// without breaking cell semantics.

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <vector>

namespace mxh::ui {

// Forward declaration; the modern cIconDialog accepts any cWindow-derived
// pointer as the icon payload (we do not dereference it during testing).
class cIcon;

struct cIconCell {
    cIcon*     icon     = nullptr;
    std::int32_t relX    = 0;        // cell rect top-left (relative)
    std::int32_t relY    = 0;
    std::int32_t relW    = 0;        // cell rect width/height
    std::int32_t relH    = 0;
    bool          inUse    = false;     // cell has a real icon (vs empty slot)
    bool          onlyLink = false;     // legacy bOnlyLink: world-anchored
};

class cIconDialog : public cDialog {
public:
    cIconDialog();
    ~cIconDialog() override;

    // Cell bookkeeping -------------------------------------------------------
    void  SetCellNum(std::uint16_t num);
    std::uint16_t GetCellNum() const noexcept  { return static_cast<std::uint16_t>(m_cells.size()); }
    void  AddIconCell(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h);

    // Query ------------------------------------------------------------------
    bool   PtInCell(std::int32_t x, std::int32_t y) const noexcept;
    bool   GetPositionForXYRef(std::int32_t x, std::int32_t y, std::uint16_t& pos) const noexcept;
    bool   IsAddable(std::uint16_t idx) const noexcept;
    bool   IsAcceptable(std::uint32_t type) const noexcept;
    std::uint32_t GetAcceptableIconType() const noexcept { return m_acceptableIconType; }
    void   SetAcceptableIconType(std::uint32_t t) noexcept { m_acceptableIconType = t; }

    // AddIcon / DeleteIcon: the legacy supported adding by cell index or by
    // screen xy (via PtInCell). We support both. Returning false means the
    // cell was already in use (for AddIcon) or absent (for DeleteIcon).
    bool   AddIcon(std::uint16_t cellIdx, cIcon* icon, bool onlyLink = false);
    bool   DeleteIcon(std::uint16_t cellIdx, cIcon** outIcon = nullptr);
    void   DeleteIconAll() noexcept;

    cIcon* GetIconForIdx(std::uint16_t idx) const;

    // Selection --------------------------------------------------------------
    std::int32_t GetCurSelCellPos() const noexcept { return m_curSelCellPos; }
    void         SetCurSelCellPos(std::int32_t pos) noexcept { m_curSelCellPos = pos; }

    // Layout -----------------------------------------------------------------
    // SetAbsXY cascades to non-link icons (legacy behavior).
    void SetAbsXY(std::int32_t x, std::int32_t y) noexcept override;

    // Render-side hooks the legacy exposed; here they're inert no-ops so the
    // 1:1 UI integration tests don't have to wire images.
    void SetIconCellBGImage(void* /*img*/)        noexcept {}
    void SetDragOverBGImage(void* /*img*/)        noexcept {}

    // Constants from the legacy {NOTUSE=0, USE=1} enum.
    static constexpr int NOTUSE = 0;
    static constexpr int USE    = 1;

private:
    std::vector<cIconCell> m_cells;
    std::uint32_t          m_acceptableIconType = 0xFFFFFFFFu;
    std::int32_t           m_curSelCellPos      = -1;
};

} // namespace mxh::ui
