#pragma once

#include "cIconGridDialog.hpp"

#include "mxh/game/item_types.hpp"

#include <cstdint>
#include <functional>

namespace mxh::ui {

class cIcon;

struct ItemShopGridMovePayload {
    std::uint16_t fromPos      = 0;
    std::uint16_t wFromItemIdx = 0;
    std::uint16_t toPos        = 0;
    std::uint16_t wToItemIdx   = 0;
};
using SendMoveSynFn = std::function<bool(const ItemShopGridMovePayload&)>;

class cItemShopGridDialog : public cIconGridDialog {
public:
    static constexpr std::uint16_t kShopTabCount   = 5;
    static constexpr std::uint16_t kShopCellPerTab = 30;
    static constexpr std::uint32_t IG_SHOPITEM_MAXINDEX = 999;

    cItemShopGridDialog();
    ~cItemShopGridDialog() override;

    cItemShopGridDialog(const cItemShopGridDialog&)            = delete;
    cItemShopGridDialog& operator=(const cItemShopGridDialog&) = delete;

    void Init(std::int32_t x = 0, std::int32_t y = 0,
              std::uint16_t wid = 0, std::uint16_t hei = 0,
              void* basicImage = nullptr,
              std::int32_t id = 0);

    bool AddItem(cIcon* pIcon);
    bool DeleteItem(std::uint16_t pos, cIcon** ppIcon);

    void ShopItemDelete(std::uint32_t, std::uint16_t,
                        std::uint32_t) noexcept {}

    static std::uint16_t GetRelativePosition(std::uint16_t absPos) noexcept;

    cIcon* GetItemForPos(std::uint16_t absPos);

    bool FakeMoveItem(std::int32_t mouseX, std::int32_t mouseY,
                      cIcon* pSrcIcon);
    bool FakeGeneralItemMove(std::uint16_t toPos,
                             cIcon* pFromIcon, cIcon* pToIcon);
    bool CanBeMoved(cIcon* pIcon, std::uint16_t pos) noexcept;

    void SetTabNumber(std::uint32_t number) noexcept { m_TabNumber = number; }
    std::uint32_t GetTabNumber() const noexcept { return m_TabNumber; }

    void SetSendMoveSynFn(SendMoveSynFn fn) noexcept { m_sendMoveSyn = std::move(fn); }

    std::uint16_t CellCount() const noexcept { return GetCellNum(); }

private:
    std::uint32_t m_TabNumber = 0;
    SendMoveSynFn m_sendMoveSyn;

    static bool IsShopAbsoluteRange(std::uint16_t absPos) noexcept;
};

}  // namespace mxh::ui
