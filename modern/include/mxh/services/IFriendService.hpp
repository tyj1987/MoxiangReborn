// mxh/services/IFriendService.hpp
// Phase 13.3 service interface for friend list dialog (cFriendDialog).
//
// Tier 3 dialog (cFriendDialog, cMiniFriendDialog) currently reads
// the friend roster via legacy singletons (FRIENDMGR / CHATMGR).
// This service is the modern replacement: dialog code takes an
// IFriendService* and queries the friend roster through it.
//
// The interface is read-only for the roster (the roster itself is
// driven by MP_FRIENDLIST_SYN/ACK on the wire) and exposes the
// presence query needed by cFriendDialog::WhisperSelected (the dialog
// needs to verify the friend is online before opening a private chat
// channel).
//
// Write paths (addFriend, removeFriend, status broadcasts) are out
// of scope; they belong to the network layer that mediates with the
// agent server.
//
// Usage pattern (from a future cFriendDialog::Refresh):
//   void cFriendDialog::Refresh() {
//     const auto* svc = m_friendService;
//     if (!svc) return;
//     for (std::size_t i = 0; i < svc->friendCount(); ++i) {
//       auto entry = svc->getFriend(i);
//       if (!entry) continue;
//       // ... render row with status indicator
//     }
//   }

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace mxh::services {

// One friend roster entry. Mirrors the legacy FriendInfo layout
// (player dwID + strName + byStatus). The struct is intentionally
// trivial; the dialog renders the row and the service answers
// roster queries.
enum class FriendStatus : std::uint8_t {
    Offline = 0,
    Online  = 1,
    Busy    = 2,
};

struct FriendEntry {
    std::uint32_t  id     = 0;
    std::string    name;
    FriendStatus   status = FriendStatus::Offline;
};

class IFriendService {
public:
    virtual ~IFriendService() = default;

    // ----- Roster (read) -----

    // Number of friends in the local roster. Must be in sync with
    // getFriend() (returning a non-nullopt for any i in [0, friendCount())).
    virtual std::size_t friendCount() const noexcept = 0;

    // Return the roster entry at index i, or std::nullopt if
    // i is out of range.
    virtual std::optional<FriendEntry> getFriend(std::size_t i) const noexcept = 0;

    // True if `id` is in the local roster. Useful for guarding
    // MP_FRIEND_DEL_SYN so the dialog does not dispatch a delete
    // for a friend that has already been removed.
    virtual bool isFriend(std::uint32_t id) const noexcept = 0;

    // True if `name` is in the local roster (case-sensitive
    // substring lookup is a dialog-side concern; the service
    // answers exact-match).
    virtual bool isFriendByName(const std::string& name) const noexcept = 0;

    // ----- Presence (read) -----

    // Return the presence status of `id`, or std::nullopt if
    // `id` is not in the roster.
    virtual std::optional<FriendStatus> getStatus(std::uint32_t id) const noexcept = 0;
};

}  // namespace mxh::services
