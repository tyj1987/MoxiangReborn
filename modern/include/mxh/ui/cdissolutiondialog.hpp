#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {struct DissolutionItem{std::uint32_t item_id{};std::uint32_t material_id{};std::uint16_t material_count{};};class cDissolutionDialog final:public cDialog{public:using ConfirmCallback=std::function<bool(const DissolutionItem&)>;bool SetItem(DissolutionItem item);void ClearItem()noexcept{m_item.reset();m_confirmed=false;}bool Confirm();void SetConfirmCallback(ConfirmCallback cb){m_callback=std::move(cb);}const std::optional<DissolutionItem>& Item()const noexcept{return m_item;}bool IsConfirmed()const noexcept{return m_confirmed;}private:std::optional<DissolutionItem>m_item;bool m_confirmed{};ConfirmCallback m_callback;};}
