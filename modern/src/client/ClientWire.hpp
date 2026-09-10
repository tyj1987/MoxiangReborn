#pragma once

#include "mxh/game/item_types.hpp"
#include "mxh/net/net.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::client {

struct LegacyLoginAck {
    std::string agent_addr;
    std::uint16_t agent_port = 0;
    std::uint32_t user_idx = 0;
    std::uint8_t user_level = 0;
};

struct CharacterSlot {
    std::uint32_t chrid = 0;
    std::string name;
    bool valid = false;
    std::uint8_t gender = 0;
    std::uint8_t face_type = 0;
    std::uint8_t hair_type = 0;
    std::uint16_t level = 0;
    std::uint16_t map_num = 0;
    std::array<std::uint16_t, 10> weared_item_idx{};
};

struct LegacyCharacterMakeParams {
    std::string wire_name;
    std::uint8_t sex_type = 0;
    std::uint8_t body_type = 0;
    std::uint8_t hair_type = 0;
    std::uint8_t face_type = 0;
    std::uint8_t start_area = 17;
    std::array<std::uint16_t, 10> worn_item_index{};
    std::uint8_t standing_array_num = 0xff;
    float height = 1.0f;
    float width = 1.0f;
};

struct MugongInfo {
    std::uint32_t db_idx = 0;
    std::uint16_t icon_idx = 0;
    std::uint16_t position = 0;
    std::uint32_t exp = 0;
    std::uint8_t sung = 0;
    std::uint8_t wear = 0;
    std::uint16_t quick_position = 0;
    std::uint16_t option_idx = 0;
};

inline constexpr std::size_t kMugongSlotCount = 25;
inline constexpr std::size_t kQuickSlotCount = 8;

struct GameInInfo {
    std::uint32_t player_id = 0;
    std::uint32_t user_id = 0;
    std::string name;
    std::uint16_t level = 0;
    std::uint16_t map_num = 0;
    std::uint32_t life = 0;
    std::uint32_t max_life = 0;
    std::uint32_t mp = 0;
    std::uint32_t max_mp = 0;
    // Legacy EXPTYPE is 64-bit (the production Map DB path uses _atoi64 and
    // %I64d), and HERO_TOTALINFO carries all eight bytes on the wire.
    std::uint64_t exp = 0;
    std::uint32_t money = 0;
    std::uint8_t gender = 0;
    std::uint8_t face_type = 0;
    std::uint8_t hair_type = 0;
    std::uint16_t gen_gol = 0;
    std::uint16_t min_chub = 0;
    std::uint16_t che_ryuk = 0;
    std::uint16_t sim_mek = 0;
    std::array<std::uint16_t, 10> weared_item_idx{};
    std::uint16_t position_x = 0;
    std::uint16_t position_z = 0;
    std::uint16_t server_year = 0;
    std::uint16_t server_month = 0;
    std::uint16_t server_day = 0;
    std::uint16_t server_hour = 0;
    std::array<MugongInfo, kMugongSlotCount> mugong{};
    mxh::game::ItemTotalInfo items{};
};

std::vector<std::uint8_t> legacy_request_login_payload(
    std::uint32_t auth_key, const std::string& user_id,
    const std::string& password);
std::optional<LegacyLoginAck> parse_legacy_login_ack(
    std::span<const std::uint8_t> payload);

bool is_listed_character(std::span<const CharacterSlot> slots,
                         std::uint32_t chrid) noexcept;
std::vector<std::uint8_t> legacy_character_list_syn_payload(
    std::uint32_t user_id, std::uint32_t dist_auth_key);
std::vector<std::uint8_t> legacy_character_select_syn_payload(
    std::uint16_t channel);
bool valid_legacy_character_name_bytes(std::string_view name) noexcept;
std::optional<LegacyCharacterMakeParams> legacy_china_character_make_params(
    std::string wire_name, std::uint8_t sex_type, std::uint8_t hair_type,
    std::uint8_t face_type, std::uint8_t cloth_option,
    std::uint8_t boot_option, std::uint8_t weapon_option);
std::optional<std::vector<std::uint8_t>> legacy_character_make_syn_payload(
    const LegacyCharacterMakeParams& params, std::uint32_t user_id);
std::vector<std::uint8_t> legacy_character_remove_syn_payload(
    std::uint32_t character_id);
mxh::net::Message legacy_character_disconnect_syn_message();
std::optional<std::vector<CharacterSlot>> parse_legacy_character_list_ack(
    std::span<const std::uint8_t> payload);
std::optional<std::uint16_t> parse_legacy_character_select_ack(
    std::span<const std::uint8_t> payload);

std::optional<GameInInfo> parse_legacy_gamein_ack(
    std::span<const std::uint8_t> payload);
std::array<MugongInfo, kMugongSlotCount> parse_legacy_mugong_total(
    std::span<const std::uint8_t> payload);
mxh::game::ItemTotalInfo parse_legacy_item_total(
    std::span<const std::uint8_t> payload);
mxh::net::Message make_legacy_gamein_syn_message(std::uint32_t player_id);
mxh::net::Message make_legacy_gameout_syn_message(std::uint32_t player_id);

}  // namespace mxh::client
