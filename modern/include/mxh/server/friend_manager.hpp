// friend_manager.hpp - Phase D5 1:1 port of legacy [Server]Map/FriendManager.h.
// The legacy CFriendManager is a singleton wrapper around UserLogOut; the
// friend list itself lives on the player object. This 1:1 port keeps the
// singleton's surface area minimal: it records that a user has logged out
// so dependents can invalidate any cached references.

#pragma once

#include <cstdint>
#include <unordered_set>

namespace mxh::server {

// Mirrors legacy CFriendManager state (singleton).
struct FriendManagerState {
    std::unordered_set<std::uint32_t> logged_out_users;
};

// Singleton-style manager state. Use one global instance.
inline FriendManagerState& friend_manager_singleton() {
    static FriendManagerState s;
    return s;
}

// Mark user logged-out. legacy CFriendManager::UserLogOut() removed cached
// entries; here we just record the event.
inline void friend_user_logout(std::uint32_t player_id) {
    friend_manager_singleton().logged_out_users.insert(player_id);
}

inline bool friend_is_user_logged_out(std::uint32_t player_id) {
    return friend_manager_singleton().logged_out_users.count(player_id) > 0u;
}

inline void friend_clear_logged_out(std::uint32_t player_id) {
    friend_manager_singleton().logged_out_users.erase(player_id);
}

} // namespace mxh::server
