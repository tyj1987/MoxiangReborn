#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_QUEST (MP_QUEST=38 in MP_CATEGORY, 1-based position 38).
inline constexpr std::uint8_t quest_category=38;
// Sub-protocols within MP_PROTOCOL_QUEST (offset 0..29 from MP_PROTOCOL_QUEST enum).
inline constexpr std::uint8_t quest_totalinfo=0;
inline constexpr std::uint8_t quest_changestate=1;
inline constexpr std::uint8_t quest_remove_notify=2;
inline constexpr std::uint8_t quest_maindata_load=3,quest_subdata_load=4,quest_item_load=5;
inline constexpr std::uint8_t quest_delete_syn=6,quest_delete_ack=7,quest_delete_nack=8;
inline constexpr std::uint8_t quest_start_syn=9,quest_start_ack=10,quest_start_nack=11;
inline constexpr std::uint8_t quest_end_syn=12,quest_end_ack=13,quest_end_nack=14;
inline constexpr std::uint8_t quest_takeitem_ack=15;
inline constexpr std::uint8_t quest_takemoney_ack=16;
inline constexpr std::uint8_t quest_takeexp_ack=17,quest_takesexp_ack=18;
inline constexpr std::uint8_t quest_giveitem_ack=19;
inline constexpr std::uint8_t quest_givemoney_ack=20;
inline constexpr std::uint8_t quest_delete_confirm_syn=21,quest_delete_confirm_ack=22;
inline constexpr std::uint8_t quest_regist_checktime=23,quest_unregist_checktime=24;
inline constexpr std::uint8_t quest_time_limit=25;
inline constexpr std::uint8_t quest_execute_error=26,quest_full=27;
enum class QuestActionKind : std::uint8_t { forward_to_map, send_nack_no_quest };
struct QuestRequest { std::uint8_t protocol=0; std::uint32_t object_id=0; bool user_has_quest=true; };
struct QuestAction { QuestActionKind kind=QuestActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t object_id=0; std::uint8_t error_code=0; };
QuestAction classify_quest(const QuestRequest&);
}
