#pragma once
#include <cstdint>
#include <optional>
namespace mxh::server {
// MP_CATEGORY byte for MP_ITEM (1-based, MP_ITEM=5 after MP_SERVER=1).
inline constexpr std::uint8_t item_category=5;
// Sub-protocols used by Agent item routing (subset of MP_PROTOCOL_ITEM 0..251).
inline constexpr std::uint8_t item_shopitem_changemap_syn=116,item_shopitem_changemap_ack=117,item_shopitem_changemap_nack=118;
inline constexpr std::uint8_t item_shopitem_chase_syn=154,item_shopitem_chase_ack=155,item_shopitem_chase_nack=156;
inline constexpr std::uint8_t item_shopitem_nchange_syn=161,item_shopitem_nchange_nack=163;
inline constexpr std::uint8_t item_shopitem_shout_ack=189,item_shopitem_shout_nack=190;
inline constexpr std::uint8_t item_shopitem_shout_sendserver=191;
// Error code 6 used in NACK for invalid name length / bad content.
inline constexpr std::uint32_t item_name_nack_code=6;
// Chase-NACK rewrites dwData to 2 before sending to the user.
inline constexpr std::uint32_t item_chase_nack_data=2;
enum class ItemUserActionKind : std::uint8_t { forward_to_map, forward_to_map_if_name_valid, send_nack_to_user, send_chase_lookup };
enum class ItemServerActionKind : std::uint8_t { forward_to_user, send_chase_nack_to_user, shout_ack_with_broadcast, shout_add_only, forward_to_client };
struct ItemUserRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t name_length=0; bool name_usable=true; bool name_has_invalid_char=false; bool user_found=false; };
struct ItemServerRequest { std::uint8_t protocol=0; std::uint32_t data=0; bool shout_buffer_full=false; };
struct ItemUserAction { ItemUserActionKind kind=ItemUserActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t error_code=0; bool drop_payload=false; };
struct ItemServerAction { ItemServerActionKind kind=ItemServerActionKind::forward_to_client; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint32_t alternate_data=0; bool broadcast_shout=false; };
ItemUserAction classify_item_user(const ItemUserRequest&);
ItemUserAction classify_item_user_ext(std::uint8_t protocol);
ItemServerAction classify_item_server(const ItemServerRequest&);
ItemServerAction classify_item_server_ext(std::uint8_t protocol);
}
