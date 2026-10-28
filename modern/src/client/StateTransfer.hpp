#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace mxh::client {

struct LoginResult {
    std::string agent_addr;
    std::uint16_t agent_port = 0;
    std::uint32_t user_idx = 0;
    std::uint8_t user_level = 0;
    std::uint32_t dist_auth_key = 0;
};

struct GameEntryRequest {
    std::uint32_t character_id = 0;
    std::uint16_t map_num = 0;
};

using StateTransfer = std::variant<std::monostate, LoginResult, GameEntryRequest>;

}  // namespace mxh::client
