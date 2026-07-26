
#pragma once
#include <cstdint>
#include <optional>
namespace mxh::server {
inline constexpr std::uint8_t murimnet_category=35;
inline constexpr std::uint8_t murimnet_changetomurimnet_syn=1,murimnet_connect_syn=2,murimnet_reconnect_syn=3;
inline constexpr std::uint8_t murimnet_changetomurimnet_ack=4,murimnet_changetomurimnet_nack=5,murimnet_pr_start=6,murimnet_disconnect_ack=7,murimnet_returntomurimnet_ack=8,murimnet_returntomurimnet_nack=9;
inline constexpr std::uint16_t murimnet_default_server_num=99;
enum class MurimNetActionKind : std::uint8_t { forward_to_map, send_ack, send_nack };
struct MurimNetServerLookup { std::uint16_t server_num=murimnet_default_server_num; std::optional<std::uint16_t> port; std::optional<std::uint32_t> connection_index; };
struct MurimNetUserRequest { std::uint8_t protocol=0; std::uint32_t character_id=0,unique_idx=0,data=0; MurimNetServerLookup lookup; };
struct MurimNetServerRequest { std::uint8_t protocol=0; std::uint32_t character_id=0,data=0; MurimNetServerLookup lookup; };
struct MurimNetAction { MurimNetActionKind kind=MurimNetActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t character_id=0; };
MurimNetAction classify_murimnet_user(const MurimNetUserRequest&);
MurimNetAction classify_murimnet_server(const MurimNetServerRequest&);
}
