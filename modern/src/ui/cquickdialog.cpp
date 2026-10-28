// cquickdialog.cpp -- 2026-09-07 visual polish: 1:1 port of cQuickDialog.
//
// Surface (legacy, 墨香【源码】\[Client]MH\QuickDlg.{h,cpp}):
//   - 3 pages of 12 slots = 36 quick-bar slots total
//   - Bind(page, slot, kind, id) sets a slot
//   - BindItemFromInventory / BindSkillFromService resolve the slot
//     id through the inventory / skill service at bind time
//   - Activate(slot) fires the slot and invokes m_cb with the
//     (kind, id) payload
//   - SelectPage(page) switches which 12 slots are visible
//
// Modern port keeps the legacy surface 1:1.  The actual cDialog
// render path is shared (handled by cWindow::Render) so this file
// only owns the page/slot bookkeeping + service injection.

#include "cquickdialog.hpp"

#include <utility>

namespace mxh::ui {

cQuickDialog::cQuickDialog() {
    m_slots.assign(kPageCount * kSlotsPerPage, QuickSlot{});
    m_page = 0;
    m_cb = nullptr;
    m_user = nullptr;
    m_inventory = nullptr;
    m_skills = nullptr;
}

bool cQuickDialog::Bind(std::size_t page, std::size_t slot, QuickKind kind, std::uint32_t id) {
    if (page >= kPageCount || slot >= kSlotsPerPage) return false;
    m_slots[page * kSlotsPerPage + slot] = QuickSlot{kind, id};
    return true;
}

bool cQuickDialog::BindItemFromInventory(std::size_t page, std::size_t slot, std::uint16_t iconIdx) {
    if (page >= kPageCount || slot >= kSlotsPerPage) return false;
    if (!m_inventory) return false;
    auto opt = m_inventory->findItemByIconIdx(iconIdx);
    if (!opt.has_value()) return false;
    return Bind(page, slot, QuickKind::Item, *opt);
}

bool cQuickDialog::BindSkillFromService(std::size_t page, std::size_t slot, std::uint32_t skillIdx) {
    if (page >= kPageCount || slot >= kSlotsPerPage) return false;
    if (!m_skills) return false;
    if (!m_skills->isLearned(skillIdx)) return false;
    return Bind(page, slot, QuickKind::Skill, skillIdx);
}

bool cQuickDialog::Remove(std::size_t page, std::size_t slot) {
    if (page >= kPageCount || slot >= kSlotsPerPage) return false;
    auto& s = m_slots[page * kSlotsPerPage + slot];
    if (s.kind == QuickKind::Empty) return false;
    s = QuickSlot{};
    return true;
}

std::optional<QuickSlot> cQuickDialog::Get(std::size_t page, std::size_t slot) const {
    if (page >= kPageCount || slot >= kSlotsPerPage) return std::nullopt;
    const auto& s = m_slots[page * kSlotsPerPage + slot];
    if (s.kind == QuickKind::Empty) return std::nullopt;
    return s;
}

bool cQuickDialog::Activate(std::size_t slot) {
    if (slot >= kSlotsPerPage) return false;
    if (m_page >= kPageCount) return false;
    const auto& s = m_slots[m_page * kSlotsPerPage + slot];
    if (s.kind == QuickKind::Empty) return false;
    if (m_cb) m_cb(s, m_user);
    return true;
}

}  // namespace mxh::ui
