#include "cfrienddialog.hpp"
#include <algorithm>
namespace mxh::ui{
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