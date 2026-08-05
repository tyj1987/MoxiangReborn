// citemshopgriddialog.cpp - modern 1:1 port of Moxiang CItemShopGridDialog.
//
// Body methods mirror the legacy math. The host callbacks (move-syn send,
// chat message, pet-cur-summon gating) replace the legacy ITEMMGR / CHATMGR
// / PETMGR / NETWORK singletons; a null callback falls through safely.

#include "citemshopgriddialog.hpp"
#include "cIconGridDialog.hpp"

namespace mxh::ui {

cItemShopGridDialog::cItemShopGridDialog() = default;
cItemShopGridDialog::~cItemShopGridDialog() = default;

void cItemShopGridDialog::Init(std::int32_t x, std::int32_t y,
                              std::uint16_t wid, std::uint16_t hei,
                              void* basicImage,
                              std::int32_t id) {
    cIconGridDialog::Init(x, y, wid, hei, basicImage,
                            /*col*/kShopTabCount,
                            /*row*/kShopCellPerTab, id);
    m_TabNumber = 0;
    SetAcceptableIconType(0xFFFFFFFFu);
    SetDragOverIconType(0);
}

std::uint16_t cItemShopGridDialog::GetRelativePosition(std::uint16_t absPos) noexcept {
    // 1:1 with legacy CItemShopGridDialog::GetRelativePosition:
    //     return ( absPos - TP_SHOPITEM_START ) % TABCELL_SHOPITEM_NUM;
    constexpr std::uint16_t kShopItemStart = mxh::game::TP_SHOPITEM_START;
    if (absPos < kShopItemStart) return 0;
    return static_cast<std::uint16_t>((absPos - kShopItemStart) % kShopCellPerTab);
}

cIcon* cItemShopGridDialog::GetItemForPos(std::uint16_t absPos) {
    constexpr std::uint16_t kShopItemStart = mxh::game::TP_SHOPITEM_START;
    constexpr std::uint16_t kShopItemEnd   = mxh::game::TP_SHOPITEM_END;
    if (absPos < kShopItemStart || absPos >= kShopItemEnd) return nullptr;
    const std::uint16_t pageOffset = static_cast<std::uint16_t>(kShopCellPerTab * m_TabNumber);
    const std::uint16_t relPos = static_cast<std::uint16_t>(absPos - (kShopItemStart + pageOffset));
    if (relPos >= GetCellNum()) return nullptr;
    return GetIconForIdx(relPos);
}

bool cItemShopGridDialog::AddItem(cIcon* pIcon) {
    if (!pIcon) return false;
    if (GetCellNum() == 0) return false;
    for (std::uint16_t i = 0; i < GetCellNum(); ++i) {
        if (IsAddable(i)) {
            return AddIcon(i, pIcon);
        }
    }
    return false;
}

bool cItemShopGridDialog::DeleteItem(std::uint16_t pos, cIcon** ppIcon) {
    return DeleteIcon(pos, ppIcon);
}

bool cItemShopGridDialog::CanBeMoved(cIcon* pIcon, std::uint16_t pos) noexcept {
    (void)pIcon;
    constexpr std::uint16_t kShopItemStart = mxh::game::TP_SHOPITEM_START;
    constexpr std::uint16_t kShopInvenEnd  = mxh::game::TP_SHOPINVEN_END;
    if (pos < kShopItemStart || pos >= kShopInvenEnd) return false;
    return true;
}

bool cItemShopGridDialog::FakeGeneralItemMove(std::uint16_t toPos,
                                              cIcon* pFromIcon, cIcon* pToIcon) {
    if (!CanBeMoved(pFromIcon, toPos)) return false;
    (void)pToIcon;
    if (!m_sendMoveSyn) return false;
    ItemShopGridMovePayload payload{};
    return m_sendMoveSyn(payload);
}

bool cItemShopGridDialog::FakeMoveItem(std::int32_t mouseX, std::int32_t mouseY,
                                       cIcon* pSrcIcon) {
    if (!pSrcIcon) return false;
    std::uint16_t toPos = 0;
    if (!GetPositionForXYRef(mouseX, mouseY, toPos)) return false;
    const std::uint16_t absTo = static_cast<std::uint16_t>(
        toPos + mxh::game::TP_SHOPITEM_START + (kShopCellPerTab * m_TabNumber));
    return FakeGeneralItemMove(absTo, pSrcIcon, GetItemForPos(absTo));
}

}  // namespace mxh::ui
