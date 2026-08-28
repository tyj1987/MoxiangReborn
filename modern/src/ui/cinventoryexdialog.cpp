#include "cinventoryexdialog.hpp"

#include "mxh/ui/cDialogLoader.hpp"
#include "mxh/ui/cIcon.hpp"
#include "mxh/ui/cIconGridDialog.hpp"

#include <array>
#include <utility>

namespace mxh::ui {

namespace {
constexpr std::array<const char*, 4> kInventoryTabIds{{
    "IN_TABDLG1", "IN_TABDLG2", "IN_TABDLG3", "IN_TABDLG4"}};
constexpr std::size_t kSlotsPerTab = 20u;
}

cInventoryExDialog::cInventoryExDialog() : m_slots(kSlotCount) {}

bool cInventoryExDialog::AddItem(std::uint16_t id,
                                 std::int32_t durability) {
    for (auto& slot : m_slots) {
        if (!slot) {
            slot = InventoryItem{id, durability, false};
            RefreshVisibleIcons();
            return true;
        }
    }
    return false;
}

bool cInventoryExDialog::DeleteItem(std::size_t position) {
    if (position >= m_slots.size() || !m_slots[position]) return false;
    m_slots[position].reset();
    RefreshVisibleIcons();
    return true;
}

bool cInventoryExDialog::MoveItem(std::size_t from, std::size_t to) {
    if (from >= m_slots.size() || to >= m_slots.size() || from == to ||
        !m_slots[from] || m_slots[from]->locked ||
        (m_slots[to] && m_slots[to]->locked)) {
        return false;
    }
    std::swap(m_slots[from], m_slots[to]);
    RefreshVisibleIcons();
    return true;
}

bool cInventoryExDialog::IsExist(std::size_t position) const noexcept {
    return position < m_slots.size() && m_slots[position].has_value();
}

std::optional<InventoryItem>
cInventoryExDialog::GetItemForPos(std::size_t position) const {
    return position < m_slots.size() ? m_slots[position] : std::nullopt;
}

std::size_t cInventoryExDialog::GetBlankNum() const noexcept {
    std::size_t blank = 0;
    for (const auto& slot : m_slots) blank += !slot.has_value();
    return blank;
}

bool cInventoryExDialog::SetItemLocked(std::size_t position,
                                       bool locked) noexcept {
    if (position >= m_slots.size() || !m_slots[position]) return false;
    m_slots[position]->locked = locked;
    return true;
}

bool cInventoryExDialog::UpdateItemDurabilityAdd(std::size_t position,
                                                 int delta) {
    if (position >= m_slots.size() || !m_slots[position]) return false;
    m_slots[position]->durability += delta;
    return true;
}

void cInventoryExDialog::RefreshFromInventoryService() {
    if (!m_inventory) return;
    for (std::size_t position = 0; position < m_slots.size(); ++position) {
        const auto* item = m_inventory->getItem(
            static_cast<std::uint16_t>(position));
        if (!item || item->dwDBIdx == 0 || item->wIconIdx == 0) {
            m_slots[position].reset();
            continue;
        }
        m_slots[position] = InventoryItem{
            item->wIconIdx, static_cast<std::int32_t>(item->Durability), false};
    }
    RefreshVisibleIcons();
}

void cInventoryExDialog::RefreshVisibleIcons() {
    // InterfaceScript declares these as generic grids. The live inventory
    // contract is four pages of five columns by four rows (80 wire slots).
    for (const auto* tab_id : kInventoryTabIds) {
        auto* grid = dynamic_cast<cIconGridDialog*>(
            findWindowByLegacyId(tab_id));
        if (!grid) continue;
        grid->ClearIcons();
        grid->EnsureGridDimensions(5, 4);
        grid->InitGrid(0, 0, cIconGridDialog::DEFAULT_CELLSIZE,
                       cIconGridDialog::DEFAULT_CELLSIZE,
                       cIconGridDialog::DEFAULT_CELLBORDER,
                       cIconGridDialog::DEFAULT_CELLBORDER);
    }

    // cIcon borrows the cached cImage; keep icon instances alive for the
    // lifetime of the current rendered inventory contents.
    m_owned_icons.clear();
    for (std::size_t position = 0; position < m_slots.size(); ++position) {
        if (!m_slots[position]) continue;
        const auto tab = position / kSlotsPerTab;
        const auto cell = position % kSlotsPerTab;
        if (tab >= kInventoryTabIds.size()) break;
        auto* grid = dynamic_cast<cIconGridDialog*>(
            findWindowByLegacyId(kInventoryTabIds[tab]));
        if (!grid) continue;

        auto* image = cDialogLoader::LoadLegacyPathImage(
            m_slots[position]->item_id, PathFileType::ItemPath);
        // A missing item-path entry is deliberately left empty. Release
        // builds must never turn an unresolved asset into a debug quad.
        if (!image) continue;

        auto icon = std::make_unique<cIcon>();
        icon->InitIcon(0, 0, cIconGridDialog::DEFAULT_CELLSIZE,
                       cIconGridDialog::DEFAULT_CELLSIZE, image, 0,
                       static_cast<std::int32_t>(position));
        auto* icon_ptr = icon.get();
        if (grid->AddIcon(static_cast<std::uint16_t>(cell), icon_ptr)) {
            m_owned_icons.push_back(std::move(icon));
        }
    }
}

void cInventoryExDialog::ReleaseInventory() noexcept {
    for (auto& slot : m_slots) slot.reset();
    m_owned_icons.clear();
    m_money = 0;
    m_state = InventoryState::Default;
    RefreshVisibleIcons();
}

}  // namespace mxh::ui
