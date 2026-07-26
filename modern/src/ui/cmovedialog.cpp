#include "cmovedialog.hpp"
#include <algorithm>
namespace mxh::ui {
void cMoveDialog::AddMoveInfo(MovePoint p){if(p.db_id==0||p.name.empty())return;if(std::none_of(m_points.begin(),m_points.end(),[&](const auto&x){return x.db_id==p.db_id;}))m_points.push_back(std::move(p));}
bool cMoveDialog::DeleteMoveInfo(std::uint32_t id){auto it=std::find_if(m_points.begin(),m_points.end(),[&](const auto&p){return p.db_id==id;});if(it==m_points.end())return false;m_points.erase(it);m_selected=-1;return true;}
bool cMoveDialog::UpdateMoveInfo(MovePoint p){for(auto& x:m_points)if(x.db_id==p.db_id){x=std::move(p);return true;}return false;}
bool cMoveDialog::SelectMoveIdx(std::size_t i)noexcept{if(i>=m_points.size()||m_points[i].town!=m_town_view)return false;m_selected=static_cast<int>(i);return true;}
bool cMoveDialog::MapMoveOK(){if(m_selected<0||static_cast<std::size_t>(m_selected)>=m_points.size())return false;return !m_callback||m_callback(m_points[static_cast<std::size_t>(m_selected)]);}
}
