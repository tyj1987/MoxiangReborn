// clistitem.hpp — modern port of 墨香 cListItem (helper base
// class for widgets that hold a list of ITEMs with a max-line
// cap).
//
// 1:1 port of legacy `cListItem` from
//   `墨香【源码】\[Client]MH\Interface\cListItem.h`. The legacy
//   cListItem is a thin helper base that provides:
//   - AddItem(ITEM* pItem)        -- tail-add with head-drop on overflow
//   - AddItem(ITEM* pItem, LONG idx) -- insert at idx with head-drop
//   - RemoveAll()
//   - RemoveItem(LONG idx) / RemoveItem(char* str)
//   - GetItemCount() / SetMaxLine / GetMaxLine
//
// The legacy uses cPtrList (engine-side linked list) for storage
// and ITEM (engine-side struct with a char[64] string + DWORD
// rgb + WORD wType). The modern port uses std::vector<ComboItem>
// (defined in this header to break the ccombobox.hpp ↔
// clistitem.hpp circular include) and a private max-line cap.
//
// The modern cListItem extends cWindow so that derived widgets
// (cComboBox) get absX/absY/SetAbsXY for free. The legacy uses
// multiple inheritance (cListItem is *not* a cWindow); the
// modern port uses single-inheritance + cListItem-as-cWindow
// for simplicity (the cListItem's own UI geometry is moot —
// it has no Init/Render of its own).
//
// Modern port scope:
//   - m_items (std::vector<ComboItem>)
//   - m_maxLine (WORD cap; 0 = unlimited)
//   - AddItem(ComboItem) + AddItem(ComboItem, std::size_t)
//   - RemoveAll() + RemoveItem(std::size_t)
//   - GetItemCount() + SetMaxLine / GetMaxLine
//
// 1:1 quirks preserved:
//   - When at max-line cap, the head item is dropped on AddItem
//     (legacy FIFO eviction).
//   - AddItem(idx) with idx > GetItemCount() is silently dropped
//     (the legacy FindIndex returns null, InsertAfter no-ops).

#pragma once

#include "cWindow.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::ui {

// ComboItem is the modern replacement for the legacy ITEM
// struct (char[64] string + DWORD rgb + WORD wType). Defined
// here (not in ccombobox.hpp) to break the ccombobox.hpp ↔
// clistitem.hpp circular include.
struct ComboItem {
    std::string   text;
    std::uint32_t rgb     = 0xFFFFFFFFu;  // ARGB
    std::uint16_t type    = 0;
};

class cListItem : public cWindow {
public:
    cListItem() = default;
    ~cListItem() override = default;

    cListItem(const cListItem&) = delete;
    cListItem& operator=(const cListItem&) = delete;

    // -------------------------------------------------------------------------
    // AddItem: tail-add. If m_maxLine > 0 and the list is at
    // cap, the head item is dropped (1:1 with legacy FIFO
    // eviction).
    // -------------------------------------------------------------------------
    void AddItem(const ComboItem& item) {
        if (m_maxLine < 1) return;
        if (m_maxLine <= m_items.size()) {
            m_items.erase(m_items.begin());
        }
        m_items.push_back(item);
    }

    // AddItem at index. If m_maxLine > 0 and the list is at cap,
    // the head item is dropped first. If idx > size, the insert
    // is silently dropped (legacy FindIndex returns null).
    void AddItem(const ComboItem& item, std::size_t idx) {
        if (m_maxLine < 1) return;
        if (m_maxLine <= m_items.size()) {
            m_items.erase(m_items.begin());
        }
        if (idx >= m_items.size()) return;
        m_items.insert(m_items.begin() + idx, item);
    }

    void RemoveAll() noexcept { m_items.clear(); }
    void RemoveItem(std::size_t idx) {
        if (idx >= m_items.size()) return;
        m_items.erase(m_items.begin() + idx);
    }

    std::size_t GetItemCount() const noexcept { return m_items.size(); }

    void          SetMaxLine(std::uint16_t maxLine) noexcept { m_maxLine = maxLine; }
    std::uint16_t GetMaxLine() const noexcept               { return m_maxLine; }

    // Read-only access for tests / inspectors.
    const std::vector<ComboItem>& Items() const noexcept { return m_items; }
    // Read-write access (used by derived classes that need to
    // mutate item contents without owning the vector).
    std::vector<ComboItem>& ItemsMutable() noexcept { return m_items; }

protected:
    std::uint16_t          m_maxLine = 0;
    std::vector<ComboItem> m_items;
};

} // namespace mxh::ui

