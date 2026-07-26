#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace mxh::ui {enum class FriendStatus:std::uint8_t{Offline,Online,Busy};struct FriendEntry{std::uint32_t id{};std::string name;FriendStatus status{FriendStatus::Offline};};class cFriendDialog final:public cDialog{public:using WhisperCallback=std::function<void(const FriendEntry&)>;void AddFriend(FriendEntry f);bool RemoveFriend(std::uint32_t id);bool UpdateStatus(std::uint32_t id,FriendStatus s);bool Select(std::size_t i)noexcept;bool WhisperSelected();void SetWhisperCallback(WhisperCallback cb){m_whisper=std::move(cb);}const FriendEntry* Selected()const noexcept;const std::vector<FriendEntry>& Friends()const noexcept{return m_friends;}private:std::vector<FriendEntry>m_friends;std::size_t m_selected{static_cast<std::size_t>(-1)};WhisperCallback m_whisper;};}
