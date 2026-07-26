#pragma once
#include <cstdint>
namespace mxh::server {
// MP_CATEGORY byte for MP_STREETSTALL (MP_STREETSTALL=29).
inline constexpr std::uint8_t streetstall_category=29;
enum class StreetStallUserActionKind : std::uint8_t { forward_to_map, drop_no_user, drop_object_mismatch };
struct StreetStallUserRequest { std::uint32_t connection_index=0; std::uint32_t object_id=0; std::uint32_t character_id=0; bool user_found=true; };
struct StreetStallUserAction { StreetStallUserActionKind kind=StreetStallUserActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t connection_index=0; };
StreetStallUserAction classify_streetstall_user(const StreetStallUserRequest&);
// Same auth pattern for MP_EXCHANGE (category 28). All MP_EXCHANGEUserMsgParser protocols flow through this single guard.
inline constexpr std::uint8_t exchange_category=28;
enum class ExchangeUserActionKind : std::uint8_t { forward_to_map, drop_no_user, drop_object_mismatch };
struct ExchangeUserRequest { std::uint32_t connection_index=0; std::uint32_t object_id=0; std::uint32_t character_id=0; bool user_found=true; };
struct ExchangeUserAction { ExchangeUserActionKind kind=ExchangeUserActionKind::forward_to_map; std::uint8_t protocol=0; std::uint32_t connection_index=0; };
ExchangeUserAction classify_exchange_user(const ExchangeUserRequest&);
}
