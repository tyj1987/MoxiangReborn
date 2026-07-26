#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::server {

inline constexpr std::uint32_t max_user_in_agent = 4000u;
inline constexpr std::uint32_t max_server_connection = 100u;
inline constexpr std::size_t max_point_num = 256u;
inline constexpr std::size_t event_max = 32u;
inline constexpr std::uint16_t mp_userconn = 6u;
inline constexpr std::uint16_t mp_userconn_connection_check = 10u;
inline constexpr std::uint16_t mp_hackcheck = 34u;
inline constexpr std::uint16_t mp_hackcheck_speedhack = 75u;

enum class Nation : std::uint8_t { korea = 0, china = 1 };

enum class ConnectionActionKind : std::uint8_t {
    send_speed_hack_check,
    send_connection_check,
    mark_disconnect,
    remove_user
};

struct ConnectionUserState {
    std::uint32_t user_id = 0;
    std::uint32_t connection_index = 0;
    std::uint32_t last_connection_check_time = 0;
    bool connection_check_failed = false;
    bool programmer = false;
    bool has_user_map = false;
};

struct ConnectionAction {
    ConnectionActionKind kind{};
    std::uint32_t user_id = 0;
    std::uint32_t connection_index = 0;
    std::uint32_t data = 0;
    std::uint16_t category = 0;
    std::uint16_t protocol = 0;
};

std::vector<ConnectionAction> compute_connection_actions(
    const ConnectionUserState& user, std::uint32_t now_ms, bool bill_tick);

struct ServerSystemTick {
    std::vector<std::function<void(std::uint32_t)>> hooks;
    void process(std::uint32_t now_ms) const;
};

struct MapChangeInfo {
    std::uint32_t kind = 0;
    std::string current_map_name;
    std::string object_name;
    std::uint16_t current_map_num = 0;
    std::uint16_t move_map_num = 0;
    float current_x = 0.0F;
    float current_z = 0.0F;
    float move_x = 0.0F;
    float move_z = 0.0F;
    std::uint16_t chx_num = 0;
};

struct EventNotifyState {
    std::string title;
    std::string context;
    bool enabled = false;
    std::array<bool, event_max> applied{};
};

void set_event_notify_strings(EventNotifyState& state, std::string_view title,
                              std::string_view context);
void set_event_notify_enabled(EventNotifyState& state, bool enabled);
void reset_applied_events(EventNotifyState& state);
bool set_applied_event(EventNotifyState& state, std::size_t event_index);

struct ServerSystemState {
    std::uint16_t server_num = 0;
    bool ready = false;
    bool crypt_enabled = false;
    bool test_server = false;
    Nation nation = Nation::korea;
    EventNotifyState event_notify;
    std::vector<MapChangeInfo> map_changes;
    std::uint32_t next_auth_key = 1000u;
};

std::uint32_t make_auth_key(ServerSystemState& state);
void release_auth_key(ServerSystemState&, std::uint32_t);
const MapChangeInfo* get_map_change_info(const ServerSystemState& state,
                                         std::uint32_t kind);

} // namespace mxh::server