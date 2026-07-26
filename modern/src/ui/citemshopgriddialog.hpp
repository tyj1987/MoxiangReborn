#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <optional>
#include <vector>
namespace mxh::ui {struct ShopGridItem{std::uint32_t id{};std::uint32_t price{};std::uint16_t stock{};};class cItemShopGridDialog final:public cDialog{public:static constexpr std::size_t kColumns=5,kRows=4,kPageSize=kColumns*kRows;void SetItems(std::vector<ShopGridItem> items){m_items=std::move(items);m_page=0;m_selected=static_cast<std::size_t>(-1);}void SelectPage(std::size_t page)noexcept{if(page<PageCount())m_page=page;m_selected=static_cast<std::size_t>(-1);}std::size_t PageCount()const noexcept{return (m_items.size()+kPageSize-1)/kPageSize;}bool Select(std::size_t cell)noexcept;std::optional<ShopGridItem> Selected()const;bool CanBuy(std::uint16_t quantity=1)const noexcept;const std::vector<ShopGridItem>& Items()const noexcept{return m_items;}private:std::vector<ShopGridItem>m_items;std::size_t m_page{},m_selected{static_cast<std::size_t>(-1)};};}
