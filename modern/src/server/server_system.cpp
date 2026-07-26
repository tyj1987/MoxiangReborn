#include "mxh/server/server_system.hpp"

#include <algorithm>
#include <utility>

namespace mxh::server {

std::vector<ConnectionAction> compute_connection_actions(
    const ConnectionUserState& user, std::uint32_t now_ms, bool bill_tick) {
    std::vector<ConnectionAction> actions;
    constexpr std::uint32_t check_interval_ms = 60u * 1000u;
    if (user.connection_index == 0u) {
        if (now_ms - user.last_connection_check_time > check_interval_ms * 2u) {
            actions.push_back({ConnectionActionKind::remove_user, user.user_id, 0u, 0u, 0u, 0u});
        }
        return actions;
    }
    if (bill_tick && user.has_user_map) {
        actions.push_back({ConnectionActionKind::send_speed_hack_check, user.user_id,
                           user.connection_index, now_ms, mp_hackcheck, mp_hackcheck_speedhack});
    }
    if (user.programmer) return actions;
    if (now_ms - user.last_connection_check_time > check_interval_ms * 10u) {
        if (user.connection_check_failed) {
            actions.push_back({ConnectionActionKind::mark_disconnect, user.user_id,
                               user.connection_index, 0u, 0u, 0u});
        } else {
            actions.push_back({ConnectionActionKind::send_connection_check, user.user_id,
                               user.connection_index, now_ms, mp_userconn,
                               mp_userconn_connection_check});
        }
    }
    return actions;
}

void ServerSystemTick::process(std::uint32_t now_ms) const {
    for (const auto& hook : hooks) if (hook) hook(now_ms);
}

void set_event_notify_strings(EventNotifyState& state, std::string_view title,
                              std::string_view context) {
    state.title.assign(title.substr(0, 31));
    state.context.assign(context.substr(0, 127));
}

void set_event_notify_enabled(EventNotifyState& state, bool enabled) {
    state.enabled = enabled;
}

void reset_applied_events(EventNotifyState& state) {
    state.applied.fill(false);
}

bool set_applied_event(EventNotifyState& state, std::size_t event_index) {
    if (event_index >= state.applied.size()) return false;
    state.applied[event_index] = true;
    return true;
}

std::uint32_t make_auth_key(ServerSystemState& state) {
    if (state.next_auth_key == 0u) state.next_auth_key = 1u;
    return state.next_auth_key++;
}

void release_auth_key(ServerSystemState&, std::uint32_t) {}

const MapChangeInfo* get_map_change_info(const ServerSystemState& state,
                                         std::uint32_t kind) {
    const auto it = std::find_if(state.map_changes.begin(), state.map_changes.end(),
                                 [kind](const MapChangeInfo& info) { return info.kind == kind; });
    return it == state.map_changes.end() ? nullptr : std::addressof(*it);
}

} // namespace mxh::server

[[maybe_unused]] constexpr int server_system_translation_unit_anchor = 0;