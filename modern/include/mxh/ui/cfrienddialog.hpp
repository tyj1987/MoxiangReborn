#pragma once
#include "mxh/ui/cDialog.hpp"
#include "mxh/services/IFriendService.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace mxh::ui {class cListCtrl;using FriendStatus = mxh::services::FriendStatus;struct FriendEntry{std::uint32_t id{};std::string name;FriendStatus status{FriendStatus::Offline};};class cFriendDialog final:public cDialog{public:using WhisperCallback=std::function<void(const FriendEntry&)>;void AddFriend(FriendEntry f);bool RemoveFriend(std::uint32_t id);bool UpdateStatus(std::uint32_t id,FriendStatus s);bool Select(std::size_t i)noexcept;bool WhisperSelected();
 // Resolve the shipped Friend.bin controls after the dialog tree is built.
 // The list is then refreshed from the bound IFriendService (when present)
 // and remains usable with the local snapshot in unit/legacy mode.
 void Linking();
 void RefreshFriendList();
 cListCtrl* FriendList() const noexcept { return m_friendList; }
 std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                           std::uint32_t mouseFlags) override;
 void SetWhisperCallback(WhisperCallback cb){m_whisper=std::move(cb);}
 // IFriendService is the modern roster + presence source of truth. When set,
 // the dialog queries the service for the friend list (WhisperSelected guards
 // isFriend() before opening a private chat channel) and the local m_friends
 // snapshot remains a fallback for unit tests + legacy types not yet wired.
 void SetFriendService(mxh::services::IFriendService* service) noexcept {m_friend_service=service;}
 mxh::services::IFriendService* GetFriendService() const noexcept {return m_friend_service;}
 bool IsFriendOnline(std::uint32_t id) const noexcept;const FriendEntry* Selected()const noexcept;const std::vector<FriendEntry>& Friends()const noexcept{return m_friends;}private:std::vector<FriendEntry>m_friends;std::size_t m_selected{static_cast<std::size_t>(-1)};WhisperCallback m_whisper; mxh::services::IFriendService* m_friend_service{};cListCtrl* m_friendList{};};}
