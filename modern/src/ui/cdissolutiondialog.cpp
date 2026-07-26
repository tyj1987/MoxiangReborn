#include "cdissolutiondialog.hpp"
namespace mxh::ui{bool cDissolutionDialog::SetItem(DissolutionItem i){if(i.item_id==0||i.material_id==0||i.material_count==0)return false;m_item=i;m_confirmed=false;return true;}bool cDissolutionDialog::Confirm(){if(!m_item||m_confirmed)return false;if(m_callback&&!m_callback(*m_item))return false;m_confirmed=true;return true;}}
