#include "citemshopdialog.hpp"
#include <limits>
namespace mxh::ui {
// GetMoney prefers the shop service when one is bound so the dialog renders
// the live economy rather than a stale snapshot. Falls back to m_money when
// no service is bound (legacy NPC types + unit tests without a service).
std::uint32_t cItemShopDialog::GetMoney() const noexcept {
 if (m_shop_service) return m_shop_service->playerMoney();
 return m_money;
}
std::uint32_t cItemShopDialog::TotalPrice(std::size_t i,std::uint16_t q)const noexcept{
 if (m_shop_service) {
  auto svc_entry = m_shop_service->getShopEntry(i);
  if (!svc_entry || q == 0) return 0;
  auto p=static_cast<std::uint64_t>(svc_entry->price)*q;
  return p>std::numeric_limits<std::uint32_t>::max()?std::numeric_limits<std::uint32_t>::max():static_cast<std::uint32_t>(p);
 }
 if (i >= m_entries.size() || q == 0) return 0;
 auto p=static_cast<std::uint64_t>(m_entries[i].price)*q;
 return p>std::numeric_limits<std::uint32_t>::max()?std::numeric_limits<std::uint32_t>::max():static_cast<std::uint32_t>(p);
}
bool cItemShopDialog::Buy(std::size_t i,std::uint16_t q){
 if (q == 0) return false;
 // Resolve entry + total via the service when bound, otherwise via local snapshot.
 ShopEntry entry{};
 std::uint32_t total = 0;
 bool have_entry = false;
 if (m_shop_service) {
  auto svc_entry = m_shop_service->getShopEntry(i);
  if (!svc_entry) return false;
  entry = *svc_entry; have_entry = true;
  auto p = static_cast<std::uint64_t>(entry.price) * q;
  total = p > std::numeric_limits<std::uint32_t>::max() ? std::numeric_limits<std::uint32_t>::max() : static_cast<std::uint32_t>(p);
  if (!m_shop_service->hasEnoughMoney(total)) return false;
 } else {
  if (i >= m_entries.size()) return false;
  entry = m_entries[i]; have_entry = true;
  total = TotalPrice(i, q);
  if (total > m_money) return false;
 }
 if (!have_entry) return false;
 if (m_purchase && !m_purchase(entry, q)) return false;
 // Service-bound dialogs let the service mutate its own money snapshot
 // (typically the MapHandler reacts to MP_ITEM_BUY_ACK and updates the
 // player economy). Local-snapshot mode still deducts from m_money.
 if (!m_shop_service) m_money -= total;
 return true;
}
}