#include "cmovedialog.hpp"
#include <algorithm>
namespace mxh::ui {
// PointCount consults the service (if bound) for the live list size;
// falls back to the local m_points snapshot when no service is bound.
std::size_t cMoveDialog::PointCount() const noexcept {
 if (m_move_service) return m_move_service->pointCount();
 return m_points.size();
}
void cMoveDialog::AddMoveInfo(MovePoint p){if(p.db_id==0||p.name.empty())return;if(std::none_of(m_points.begin(),m_points.end(),[&](const auto&x){return x.db_id==p.db_id;}))m_points.push_back(std::move(p));}
bool cMoveDialog::DeleteMoveInfo(std::uint32_t id){auto it=std::find_if(m_points.begin(),m_points.end(),[&](const auto&p){return p.db_id==id;});if(it==m_points.end())return false;m_points.erase(it);m_selected=-1;return true;}
bool cMoveDialog::UpdateMoveInfo(MovePoint p){for(auto& x:m_points)if(x.db_id==p.db_id){x=std::move(p);return true;}return false;}
bool cMoveDialog::SelectMoveIdx(std::size_t i)noexcept{
 // Service-bound dialogs also gate on town/saved presence so the town tab is
 // hidden when no town teleport is known, matching the legacy tab behavior.
 if (i >= m_points.size()) return false;
 if (m_move_service) {
  bool want_town = m_town_view;
  bool svc_has_town = m_move_service->hasTownPoint();
  bool svc_has_saved = m_move_service->hasSavedPoint();
  if (want_town && !svc_has_town) return false;
  if (!want_town && !svc_has_saved) return false;
  if (m_points[i].town != m_town_view) return false;
 } else {
  if (m_points[i].town != m_town_view) return false;
 }
 m_selected=static_cast<int>(i);
 return true;
}
bool cMoveDialog::MapMoveOK(){
 if (m_selected < 0 || static_cast<std::size_t>(m_selected) >= m_points.size()) return false;
 const auto& point = m_points[static_cast<std::size_t>(m_selected)];
 // Service-bound dialogs gate the teleport dispatch on the service known-point
 // list so an unknown point cannot reach the wire. Local-snapshot mode skips the
 // gate to match legacy behavior (teleport per local roster).
 if (m_move_service && !m_move_service->isKnownPoint(point.db_id)) return false;
 return !m_callback || m_callback(point);
}
}