#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace mxh::ui {struct NpcOption{std::uint32_t action{};std::string text;};class cNpcScriptDialog final:public cDialog{public:using ActionCallback=std::function<bool(std::uint32_t)>;void SetText(std::string text){m_text=std::move(text);}void SetOptions(std::vector<NpcOption> options){m_options=std::move(options);m_selected=-1;}bool Select(std::size_t index)noexcept;bool ExecuteSelected();void SetActionCallback(ActionCallback cb){m_action=std::move(cb);}const std::string& Text()const noexcept{return m_text;}int SelectedIndex()const noexcept{return m_selected;}const std::vector<NpcOption>& Options()const noexcept{return m_options;}private:std::string m_text;std::vector<NpcOption>m_options;int m_selected{-1};ActionCallback m_action;};}
