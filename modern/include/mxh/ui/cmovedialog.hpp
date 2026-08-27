#pragma once
#include "mxh/ui/cDialog.hpp"
#include "mxh/services/IMoveService.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace mxh::ui {
using MovePoint = mxh::services::MovePoint;
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
 // IMoveService is the modern catalog + town/saved discriminator source of truth.
 // When set, MapMoveOK() gates the dispatch on service->isKnownPoint(db_id) so a
 // teleport request to an unlocked point never reaches the wire, and the town/saved
 // view discrimination in SelectMoveIdx() consults service->hasTownPoint() /
 // hasSavedPoint(). The local m_points snapshot remains a fallback for unit tests
 // + legacy types not yet wired.
 void SetMoveService(mxh::services::IMoveService* service) noexcept {m_move_service=service;}
 mxh::services::IMoveService* GetMoveService() const noexcept {return m_move_service;}
 std::size_t PointCount() const noexcept;
private: std::vector<MovePoint> m_points; int m_selected{-1}; bool m_town_view{}; MoveCallback m_callback; mxh::services::IMoveService* m_move_service{};
};
}
