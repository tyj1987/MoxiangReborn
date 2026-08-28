#include "cfrienddialog.hpp"
#include "mxh/ui/cListCtrl.hpp"
#include <algorithm>
#include <cstdio>

namespace mxh::ui{
namespace {
const char* status_text(FriendStatus status) noexcept {
 switch (status) {
  case FriendStatus::Online: return "Online";
  case FriendStatus::Busy: return "Busy";
  default: return "Offline";
 }
}
std::uint32_t status_color(FriendStatus status) noexcept {
 switch (status) {
  case FriendStatus::Online: return 0xFF2A9D49u;
  case FriendStatus::Busy: return 0xFFD68A00u;
  default: return 0xFF7A7A7Au;
 }
}
}

void cFriendDialog::Linking() {
 m_friendList = dynamic_cast<cListCtrl*>(findWindowByLegacyId("FRI_FRIENDLISTLCTL"));
 if (m_friendList) {
  m_friendList->SetColumns({cListCtrl::Column{103, "", 0xFFFFFFFFu}});
  m_friendList->InitListCtrl(1, 10);
 }
 RefreshFriendList();
}

void cFriendDialog::RefreshFriendList() {
 std::uint32_t selected_id = 0;
 if (const auto* selected = Selected()) selected_id = selected->id;
 if (m_friend_service) {
  std::vector<FriendEntry> live;
  live.reserve(m_friend_service->friendCount());
  for (std::size_t i = 0; i < m_friend_service->friendCount(); ++i) {
   const auto entry = m_friend_service->getFriend(i);
   if (entry && entry->id != 0 && !entry->name.empty()) {
    live.push_back({entry->id, entry->name, entry->status});
   }
  }
  m_friends = std::move(live);
 }
 m_selected = static_cast<std::size_t>(-1);
 if (selected_id != 0) {
  for (std::size_t i = 0; i < m_friends.size(); ++i) {
   if (m_friends[i].id == selected_id) { m_selected = i; break; }
  }
 }
 if (!m_friendList) return;
 m_friendList->RemoveAll();
 for (const auto& friend_entry : m_friends) {
  char row_text[256]{};
  std::snprintf(row_text, sizeof(row_text), "%-16s %s", friend_entry.name.c_str(),
                status_text(friend_entry.status));
  m_friendList->AddRow({{row_text}, {status_color(friend_entry.status)}});
 }
 m_friendList->SetSelectedRowIdx(
     m_selected < m_friends.size() ? static_cast<std::int32_t>(m_selected) : -1);
}

std::uint32_t cFriendDialog::ActionEvent(std::int32_t mouseX,
                                         std::int32_t mouseY,
                                         std::uint32_t mouseFlags) {
 const auto event = cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
 if (m_friendList) {
  const auto row = m_friendList->selectedRowIdx();
  if (row >= 0 && static_cast<std::size_t>(row) < m_friends.size()) {
   m_selected = static_cast<std::size_t>(row);
  }
 }
 return event;
}

// IsFriendOnline checks the service (if bound) for live presence;
// falls back to the local m_friends snapshot when no service is bound.
bool cFriendDialog::IsFriendOnline(std::uint32_t id) const noexcept {
 if (m_friend_service) {
  auto status = m_friend_service->getStatus(id);
  return status.has_value() && *status == mxh::services::FriendStatus::Online;
 }
 for (const auto& f : m_friends) if (f.id == id) return f.status == FriendStatus::Online;
 return false;
}
void cFriendDialog::AddFriend(FriendEntry f){if(f.id==0||f.name.empty())return;if(std::none_of(m_friends.begin(),m_friends.end(),[&](const auto&x){return x.id==f.id;}))m_friends.push_back(std::move(f));}
bool cFriendDialog::RemoveFriend(std::uint32_t id){auto it=std::find_if(m_friends.begin(),m_friends.end(),[&](const auto&x){return x.id==id;});if(it==m_friends.end())return false;m_friends.erase(it);m_selected=static_cast<std::size_t>(-1);return true;}
bool cFriendDialog::UpdateStatus(std::uint32_t id,FriendStatus s){for(auto&f:m_friends)if(f.id==id){f.status=s;return true;}return false;}
bool cFriendDialog::Select(std::size_t i)noexcept{if(i>=m_friends.size())return false;m_selected=i;return true;}
const FriendEntry* cFriendDialog::Selected()const noexcept{return m_selected<m_friends.size()?&m_friends[m_selected]:nullptr;}
bool cFriendDialog::WhisperSelected(){
 auto*f=Selected();if(!f)return false;
 // Service-bound dialogs gate the whisper on live presence so an offline
 // friend cannot receive a private chat. Local-snapshot mode skips the gate
 // to match legacy behavior (whisper regardless of presence; server decides).
 if (m_friend_service && !m_friend_service->isFriend(f->id)) return false;
 if (m_whisper) m_whisper(*f);
 return true;
}
}
