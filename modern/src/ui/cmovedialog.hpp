#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace mxh::ui {
struct MovePoint { std::uint32_t db_id{}; std::string name; bool town{}; };
class cMoveDialog final : public cDialog {
public:
 using MoveCallback=std::function<bool(const MovePoint&)>;
 void AddMoveInfo(MovePoint point); bool DeleteMoveInfo(std::uint32_t db_id); bool UpdateMoveInfo(MovePoint point);
 void SetTownMoveView(bool town) noexcept {m_town_view=town; m_selected=-1;}
 bool IsTownMove()const noexcept{return m_town_view;}
 bool SelectMoveIdx(std::size_t index) noexcept;
 bool MapMoveOK(); void ResetMoveIdx()noexcept{m_selected=-1;}
 int SelectedIndex()const noexcept{return m_selected;}
 const std::vector<MovePoint>& Points()const noexcept{return m_points;}
 void SetMoveCallback(MoveCallback cb){m_callback=std::move(cb);}
private: std::vector<MovePoint> m_points; int m_selected{-1}; bool m_town_view{}; MoveCallback m_callback;
};
}
