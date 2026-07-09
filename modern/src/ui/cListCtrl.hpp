// mxh/ui/cListCtrl.hpp
// Phase 6.5 — modern C++ cListCtrl widget. Multi-column scrollable list
// (the inventory / character / friend list workhorse). Builds on cWindow
// (6.0) + cImage (6.4) for the head/body/over/select images.
//
// Scope (this phase):
//   - Column model: configurable column count, width per column, header text
//   - Row model: data rows (text + per-column tint color) + selection
//   - Hit test: row index from (mouseX, mouseY), bounds-clamped
//   - Selection: selected row index (latched on click)
//   - TopItem: top-of-viewport row (scroll offset)
//   - 4 images: head (header strip), body (row background), over (hover),
//     select (selected-row highlight) — all cImage* via the 6.4 adapter
//   - Per-row click + double-click callbacks (modern std::function)
//   - Margin / column text color
//
// Deferred:
//   - Real cReportItem text rendering (the legacy engine has its own
//     multi-line text + report layout; that's a separate module)
//   - Scrollbar widget (the legacy cListCtrl delegates scrolling to a
//     sibling cScrollBar window; we'll add a tiny scroll interface that
//     a sibling widget can drive)
//   - Sortable columns
//   - Drag-and-drop reordering
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "cWindow.hpp"

namespace mxh::ui {

class cListCtrl : public cWindow {
public:
    // Click / dblclick callbacks. The callback receives the list, the
    // clicked row index (-1 if outside the data), and userdata.
    using RowCallback = std::function<void(cListCtrl& self, std::int32_t rowIdx,
                                            void* userdata)>;

    // One column's metadata.
    struct Column {
        std::int32_t width   = 0;     // pixel width
        std::string  header;          // header text
        std::uint32_t headerColor = 0xFF000000u;  // 0xAARRGGBB
    };

    // One row of cell data. cells.size() must equal the column count.
    struct Row {
        std::vector<std::string>  texts;
        std::vector<std::uint32_t> colors;
    };

    cListCtrl() = default;
    ~cListCtrl() override = default;

    cListCtrl(const cListCtrl&) = delete;
    cListCtrl& operator=(const cListCtrl&) = delete;

    // -------------------------------------------------------------------------
    // Init: position, size, basic image, id. Use SetColumns + SetImages
    // afterwards to configure the list shape.
    // -------------------------------------------------------------------------
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
              std::uint16_t hei, void* basicImage, std::int32_t id = 0);

    // InitListCtrl: legacy cListCtrl::InitListCtrl(wMaxColumns, wLinePerPage).
    // In the legacy engine this was called separately from Init; we keep
    // the same two-step pattern so the engine-facing code doesn't have
    // to change. `wLinePerPage` is the visible row count.
    void InitListCtrl(std::uint16_t wMaxColumns, std::uint16_t wLinePerPage);

    // InitListCtrlImage: legacy cListCtrl::InitListCtrlImage. Wires the
    // head / body / over images. cImage* here are the modern cImage
    // pointers (from 6.4) so the render path is the same as the rest of
    // the modern framework.
    void InitListCtrlImage(void* headImage, std::uint8_t headLineHeight,
                           void* bodyImage, std::uint8_t bodyLineHeight,
                           void* overImage);

    // Render placeholder (real GPU draw in 6.6 / MoxianRenderDemo smoke).
    void Render() override {}

    // ActionEvent: row hit-test + selection update + click callbacks.
    // Returns WE_LBTNCLICK on a successful row click, WE_LBTNDBLCLICK on
    // dblclick, otherwise the WE_* code from cWindow's child dispatch.
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    // -------------------------------------------------------------------------
    // Column model.
    // -------------------------------------------------------------------------
    void SetColumns(std::vector<Column> cols);
    std::size_t columnCount() const noexcept { return m_columns.size(); }
    const Column& columnAt(std::size_t i) const { return m_columns.at(i); }

    // -------------------------------------------------------------------------
    // Row model. AddRow appends; RemoveRowAt removes; RemoveAll clears.
    // -------------------------------------------------------------------------
    void AddRow(Row row);
    void RemoveRowAt(std::size_t idx);
    void RemoveAll() noexcept;
    std::size_t rowCount() const noexcept { return m_rows.size(); }
    const Row& rowAt(std::size_t i) const { return m_rows.at(i); }

    // -------------------------------------------------------------------------
    // Selection.
    // -------------------------------------------------------------------------
    void  SetSelectedRowIdx(std::int32_t idx) noexcept { m_selectedRowIdx = idx; }
    std::int32_t selectedRowIdx() const noexcept       { return m_selectedRowIdx; }
    void  ClearSelection() noexcept                     { m_selectedRowIdx = -1; }

    // -------------------------------------------------------------------------
    // Viewport: top-of-viewport row index.
    // -------------------------------------------------------------------------
    void  SetTopItemIdx(std::int32_t idx) noexcept;
    std::int32_t topItemIdx() const noexcept { return m_topItemIdx; }

    // -------------------------------------------------------------------------
    // Over row (the row under the cursor). Updated by ActionEvent.
    // -------------------------------------------------------------------------
    void         SetOverRowIdx(std::int32_t idx) noexcept { m_overRowIdx = idx; }
    std::int32_t overRowIdx() const noexcept              { return m_overRowIdx; }
    void         ClearOver() noexcept                     { m_overRowIdx = -1; }

    // -------------------------------------------------------------------------
    // SetMargin (legacy: a RECT with left/top insets, right/bottom unused).
    // -------------------------------------------------------------------------
    void SetMargin(std::int32_t left, std::int32_t top) noexcept;
    std::int32_t marginLeft() const noexcept { return m_marginLeft; }
    std::int32_t marginTop()  const noexcept { return m_marginTop;  }

    // -------------------------------------------------------------------------
    // Selection option (legacy m_wSelectOption). 0 = over (hover highlight),
    // 1 = select (latched selected-row highlight).
    // -------------------------------------------------------------------------
    void     SetSelectOption(std::uint16_t opt) noexcept { m_selectOption = opt; }
    std::uint16_t selectOption() const noexcept         { return m_selectOption; }

    // -------------------------------------------------------------------------
    // Callbacks.
    // -------------------------------------------------------------------------
    void SetClickFunc(RowCallback cb)      { m_onClick  = std::move(cb); }
    void SetDblClickFunc(RowCallback cb)   { m_onDblClick = std::move(cb); }
    void SetUserdata(void* u)              { m_userdata = u; }

    // -------------------------------------------------------------------------
    // Image accessors.
    // -------------------------------------------------------------------------
    void* headImage()   const noexcept { return m_headImage; }
    void* bodyImage()   const noexcept { return m_bodyImage; }
    void* overImage()   const noexcept { return m_overImage; }
    std::uint8_t headLineHeight() const noexcept { return m_headLineHeight; }
    std::uint8_t bodyLineHeight() const noexcept { return m_bodyLineHeight; }

    // -------------------------------------------------------------------------
    // Row hit test. Returns the row index under (x, y) (0-based, body
    // area only — header is not part of the row space). Returns
    // linePerPage+1 (= 0xFFFF sentinel) if outside.
    // -------------------------------------------------------------------------
    std::uint16_t PtIdxInRow(std::int32_t x, std::int32_t y) const noexcept;

    // Get the visible-row count (linePerPage).
    std::uint16_t linePerPage() const noexcept { return m_linePerPage; }

private:
    // Images (cImage* via 6.4 adapter).
    void* m_headImage = nullptr;
    void* m_bodyImage = nullptr;
    void* m_overImage = nullptr;
    std::uint8_t m_headLineHeight = 0;
    std::uint8_t m_bodyLineHeight = 0;

    // Column + row model.
    std::vector<Column> m_columns;
    std::vector<Row>    m_rows;

    // Selection / viewport / hover.
    std::int32_t  m_selectedRowIdx = -1;
    std::int32_t  m_topItemIdx     = 0;
    std::int32_t  m_overRowIdx     = -1;

    // Layout.
    std::uint16_t m_linePerPage = 0;
    std::uint16_t m_maxColumns  = 0;
    std::int32_t  m_marginLeft  = 3;
    std::int32_t  m_marginTop   = 4;
    std::uint16_t m_selectOption = 0;

    // Callbacks.
    RowCallback   m_onClick;
    RowCallback   m_onDblClick;
    void*         m_userdata = nullptr;
};

} // namespace mxh::ui
