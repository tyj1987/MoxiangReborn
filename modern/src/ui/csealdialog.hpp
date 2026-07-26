#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {enum class SealMode:std::uint8_t{Seal,Unseal};struct SealItem{std::uint32_t item_id{};bool sealed{};};class cSealDialog final:public cDialog{public:using ActionCallback=std::function<bool(const SealItem&,SealMode)>;void SetMode(SealMode mode)noexcept{m_mode=mode;m_done=false;}bool SetItem(SealItem item);bool Execute();void Clear()noexcept{m_item.reset();m_done=false;}const std::optional<SealItem>& Item()const noexcept{return m_item;}bool Done()const noexcept{return m_done;}void SetCallback(ActionCallback cb){m_callback=std::move(cb);}private:SealMode m_mode{SealMode::Seal};std::optional<SealItem>m_item;bool m_done{};ActionCallback m_callback;};}
