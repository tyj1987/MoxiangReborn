#include "citemshopdialog.hpp"
#include <limits>
namespace mxh::ui {
std::uint32_t cItemShopDialog::TotalPrice(std::size_t i,std::uint16_t q)const noexcept{if(i>=m_entries.size()||q==0)return 0;auto p=static_cast<std::uint64_t>(m_entries[i].price)*q;return p>std::numeric_limits<std::uint32_t>::max()?std::numeric_limits<std::uint32_t>::max():static_cast<std::uint32_t>(p);}
bool cItemShopDialog::Buy(std::size_t i,std::uint16_t q){if(i>=m_entries.size()||q==0)return false;auto total=TotalPrice(i,q);if(total>m_money)return false;if(m_purchase&&!m_purchase(m_entries[i],q))return false;m_money-=total;return true;}
}
