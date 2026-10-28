#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <unordered_map>
namespace mxh::ui {class cMainBarDialog final:public cDialog{public:using ToggleCallback=std::function<void(std::uint32_t,bool)>;bool SetButton(std::uint32_t id,bool visible);bool IsButtonVisible(std::uint32_t id)const noexcept;bool Toggle(std::uint32_t id);void SetVisible(bool visible)noexcept{m_visible=visible;}bool IsVisible()const noexcept{return m_visible;}void SetToggleCallback(ToggleCallback cb){m_callback=std::move(cb);}private:std::unordered_map<std::uint32_t,bool>m_buttons;bool m_visible{true};ToggleCallback m_callback;};}
