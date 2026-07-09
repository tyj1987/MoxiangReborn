// cListDialog.hpp — modern port of 墨香 cListDialog (text list with selection).
//
// 1:1 port of legacy `cListDialog` from
//   `墨香【源码】\[Client]MH\interface\cListDialog.h`.
//
// A cListDialog is a scrollable list of text lines. The legacy uses it
// everywhere a player sees a vertical text list: chat, guild members,
// quest log, item shop, etc. The key state is:
//   - a vector of (text, color) rows
//   - selected row index (-1 = none)
//   - top index (first visible row) + line height
//   - max line count (clamped)
//
// Render + scroll buttons + gauge bar are out of scope for Phase 6.12;
// the modern port covers the data model + selection + scrolling, which
// is what every consumer (cGuildDialog, cChatDialog, etc.) needs.

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace mxh::ui {

class cListDialog : public cDialog {
public:
    cListDialog();
    ~cListDialog() override;

    using Row = std::pair<std::string, std::uint32_t>;  // text + color

    // Configuration ----------------------------------------------------------
    void InitList(std::uint16_t maxLines, std::int32_t clipX, std::int32_t clipY,
                  std::int32_t clipW, std::int32_t clipH);
    void SetLineHeight(std::int32_t h) noexcept { m_lineHeight = h; }
    void SetMaxLine(std::uint16_t n) noexcept    { m_maxLine = n; }
    std::uint16_t GetMaxLine() const noexcept    { return m_maxLine; }
    void SetShowSelect(bool v) noexcept          { m_showSelect = v; }
    bool IsShowSelect() const noexcept           { return m_showSelect; }

    // Row operations --------------------------------------------------------
    void AddItem(std::string text, std::uint32_t color = 0xFF000000, int line = -1);
    void RemoveAll() noexcept { m_rows.clear(); m_selectedRow = -1; m_topRow = 0; }
    std::size_t RowCount() const noexcept         { return m_rows.size(); }
    bool IsMaxLineOver() const noexcept           { return m_rows.size() > m_maxLine; }

    // Selection / scroll ----------------------------------------------------
    int  GetCurSelectedRowIdx() const noexcept    { return m_selectedRow; }
    void SetCurSelectedRowIdx(int idx) noexcept   { m_selectedRow = idx; }
    int  GetSelectRowIdx() const noexcept         { return m_selectedRow; }
    int  GetTopListItemIdx() const noexcept       { return m_topRow; }
    void SetTopListItemIdx(int idx) noexcept;

    void OnUpwardItem();
    void OnDownwardItem();

    // Hit test: returns the row index under (x, y) or -1.
    int PtIdxInRow(std::int32_t x, std::int32_t y) const noexcept;

    // Render placeholder.
    void Render() override {}

    void SetAutoScroll(bool v) noexcept           { m_autoScroll = v; }

private:
    std::vector<Row> m_rows;
    int              m_selectedRow = -1;
    int              m_topRow      = 0;
    std::uint16_t    m_maxLine     = 0;
    std::int32_t     m_lineHeight  = 14;
    bool             m_showSelect  = true;
    bool             m_autoScroll  = false;
    std::int32_t     m_clipX       = 0;
    std::int32_t     m_clipY       = 0;
    std::int32_t     m_clipW       = 0;
    std::int32_t     m_clipH       = 0;
};

} // namespace mxh::ui
