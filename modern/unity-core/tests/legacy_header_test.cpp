#include "client/CCharSelectState.hpp"
#include "client/CInGameState.hpp"
#include "client/CLoginState.hpp"

#include <type_traits>

static_assert(std::is_same_v<
    decltype(&mxh::client::parse_legacy_login_ack),
    std::optional<mxh::client::LegacyLoginAck> (*)(
        std::span<const std::uint8_t>)>);
static_assert(std::is_same_v<
    decltype(&mxh::client::parse_legacy_character_list_ack),
    std::optional<std::vector<mxh::client::CharacterSlot>> (*)(
        std::span<const std::uint8_t>)>);
static_assert(std::is_same_v<
    decltype(&mxh::client::parse_legacy_gamein_ack),
    std::optional<mxh::client::GameInInfo> (*)(
        std::span<const std::uint8_t>)>);

int main() {
    const auto login = mxh::client::legacy_request_login_payload(1, "u", "p");
    const auto select = mxh::client::legacy_character_select_syn_payload(0);
    const auto game_in = mxh::client::make_legacy_gamein_syn_message(7);
    return login.size() == 38 && select.size() == 2 &&
           game_in.header.object_id == 7 ? 0 : 1;
}
