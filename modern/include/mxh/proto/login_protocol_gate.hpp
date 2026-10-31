#pragma once

// Item protocol hash gate for the modern login handshake.
//
// The hash travels on modern CheckVersion (protocol 200,
// modern/include/mxh/proto/protocol.hpp). That number sits above the
// original UserConn range. The last original UserConn enumerator is
// 墨香【源码】/[CC]Header/Protocol.h:689. RequestLogin stays protocol 1
// at Protocol.h:522. CharacterSelectSyn stays at Protocol.h:544.
// Neither original message gains a field.
//
// CheckVersion body, modern only:
//   [client_version:u16 LE] [name_len:u8] [name] [hash_len:u8] [hash ASCII]
// A body that stops after the name has no hash. Missing and unequal
// hashes share VersionRejectReason::ProtocolMismatch (reason byte 3 on
// NotifyVersionNack, protocol 202). Reasons 0, 1, and 2 stay TooOld,
// TooNew, and ServerFull (protocol.hpp VersionRejectReason).
//
// The compared digest is kItemProtocolSha256 from the CMake-generated
// protocol_hash.hpp. This header does not generate or embed that value.

#include "mxh/proto/protocol.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::proto {

inline constexpr std::uint8_t kLoginProtocolMismatchReason = 3;

inline constexpr char kItemProtocolMismatchUserText[] =
    "Item protocol mismatch. The client and server Item protocols differ.";

static_assert(static_cast<std::uint8_t>(VersionRejectReason::TooOld) == 0);
static_assert(static_cast<std::uint8_t>(VersionRejectReason::TooNew) == 1);
static_assert(static_cast<std::uint8_t>(VersionRejectReason::ServerFull) == 2);
static_assert(static_cast<std::uint8_t>(VersionRejectReason::ProtocolMismatch)
              == kLoginProtocolMismatchReason);
static_assert(kModernCheckVersion == 200);
static_assert(kModernNotifyVersionNack == 202);
static_assert(static_cast<std::uint8_t>(UserConnProtocol::RequestLogin) == 1);

struct LoginItemProtocolDecision {
    bool accepted = false;
    std::uint8_t reason = 0;
    std::string log_line;
    std::string user_message;
};

inline bool is_item_protocol_hash_token(std::string_view hash) {
    if (hash.size() != 64) return false;
    for (unsigned char c : hash) {
        const bool digit = c >= '0' && c <= '9';
        const bool lower = c >= 'a' && c <= 'f';
        if (!digit && !lower) return false;
    }
    return true;
}

inline const char* login_protocol_user_message(std::uint8_t reason) {
    switch (reason) {
    case 0: return "Client version is too old.";
    case 1: return "Client version is too new.";
    case 2: return "Server is full.";
    case kLoginProtocolMismatchReason: return kItemProtocolMismatchUserText;
    default: return kItemProtocolMismatchUserText;
    }
}

inline LoginItemProtocolDecision evaluate_login_item_protocol(
    std::string_view client_hash,
    std::string_view server_hash) {
    LoginItemProtocolDecision decision;
    const bool client_ok = is_item_protocol_hash_token(client_hash);
    const bool server_ok = is_item_protocol_hash_token(server_hash);
    if (client_ok && server_ok && client_hash == server_hash) {
        decision.accepted = true;
        decision.reason = static_cast<std::uint8_t>(VersionRejectReason::TooOld);
        return decision;
    }
    decision.accepted = false;
    decision.reason = kLoginProtocolMismatchReason;
    const std::string client_token = client_ok ? std::string(client_hash) : "missing";
    decision.log_line = "PROTOCOL_MISMATCH item=" + client_token + "/"
                      + std::string(server_hash);
    decision.user_message = login_protocol_user_message(kLoginProtocolMismatchReason);
    return decision;
}

inline std::string item_hash_from_check_version(const std::uint8_t* data,
                                                std::size_t size) {
    if (data == nullptr || size < 3) return {};
    const std::size_t name_len = data[2];
    const std::size_t name_end = 3u + name_len;
    if (size < name_end + 1u) return {};
    const std::size_t hash_len = data[name_end];
    if (hash_len == 0 || size < name_end + 1u + hash_len) return {};
    return std::string(reinterpret_cast<const char*>(data + name_end + 1u), hash_len);
}

inline void append_item_hash_to_check_version(std::vector<std::uint8_t>& payload,
                                              std::string_view hash) {
    if (hash.size() > 255u) return;
    payload.push_back(static_cast<std::uint8_t>(hash.size()));
    payload.insert(payload.end(), hash.begin(), hash.end());
}

inline std::vector<std::uint8_t> make_check_version_payload(
    std::uint16_t client_version,
    std::string_view client_name,
    std::string_view item_hash) {
    std::vector<std::uint8_t> payload;
    payload.push_back(static_cast<std::uint8_t>(client_version & 0xFF));
    payload.push_back(static_cast<std::uint8_t>((client_version >> 8) & 0xFF));
    const std::size_t name_len = client_name.size() > 255u ? 255u : client_name.size();
    payload.push_back(static_cast<std::uint8_t>(name_len));
    payload.insert(payload.end(), client_name.begin(),
                   client_name.begin() + static_cast<std::ptrdiff_t>(name_len));
    append_item_hash_to_check_version(payload, item_hash);
    return payload;
}

inline void write_version_nack_payload(std::uint8_t* out,
                                       std::uint16_t server_version,
                                       std::uint8_t reason) {
    out[0] = static_cast<std::uint8_t>(server_version & 0xFF);
    out[1] = static_cast<std::uint8_t>((server_version >> 8) & 0xFF);
    out[2] = reason;
}

inline std::uint8_t version_nack_reason(const std::uint8_t* payload, std::size_t size) {
    if (payload == nullptr || size < 3) return kLoginProtocolMismatchReason;
    return payload[2];
}

}  // namespace mxh::proto
