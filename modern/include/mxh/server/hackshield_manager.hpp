// hackshield_manager.hpp - Phase 6.3 AgentServer 1:1 port of legacy
// [Server]Agent/HackShieldManager.h + HackShieldManager.cpp.
//
// The proprietary AntiCpSvr library is unavailable, so this port keeps the
// AgentServer state machine and wire constants while accepting vendor-call
// success/failure as explicit inputs. This preserves the original manager
// behavior without changing any legacy HackShield signatures.
//
// Locked invariants (1:1 with legacy):
//   - Message sizes are GUID request/ack=20, request=160, ack=56 bytes.
//   - HackShield category is 67 and protocols are GUID_REQ=0, GUID_ACK=1,
//     REQ=2, ACK=3, DISCONNECT=4.
//   - Only users with UserLevel >= eUSERLEVEL_SUPERUSER (5) are checked.
//   - m_bHSCheck uses 0=idle, 2=just sent/grace, 1=waiting. SendReq turns
//     2 into 1 without sending, then disconnects if called again at 1.
//   - A GUID ack clears the check flag before analysis; success sends an
//     ANTICPSVR_CHECK_ALL request and sets 2. A normal ack clears the flag.
//   - Analysis failure sends DISCONNECT and disconnects the connection.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace mxh::server {

inline constexpr std::size_t HACKSHIELD_REQ_SIZE = 160u;
inline constexpr std::size_t HACKSHIELD_ACK_SIZE = 56u;
inline constexpr std::size_t HACKSHIELD_GUID_REQ_SIZE = 20u;
inline constexpr std::size_t HACKSHIELD_GUID_ACK_SIZE = 20u;
inline constexpr std::uint8_t HACKSHIELD_CATEGORY = 67u;
inline constexpr std::uint8_t HACKSHIELD_SUPERUSER_LEVEL = 5u;
inline constexpr std::uint32_t ANTICPSVR_CHECK_GAME_MEMORY = 0x1u;
inline constexpr std::uint32_t ANTICPSVR_CHECK_HACKSHIELD_FILE = 0x2u;
inline constexpr std::uint32_t ANTICPSVR_CHECK_GAME_FILE = 0x4u;
inline constexpr std::uint32_t ANTICPSVR_CHECK_ALL = 0x7u;

enum class HackShieldProtocol : std::uint8_t {
    GuidReq = 0,
    GuidAck = 1,
    Req = 2,
    Ack = 3,
    Disconnect = 4,
};

enum class HackShieldActionKind : std::uint8_t {
    None = 0,
    Send = 1,
    Disconnect = 2,
};

struct HackShieldPacket {
    std::uint8_t Category = HACKSHIELD_CATEGORY;
    HackShieldProtocol Protocol = HackShieldProtocol::GuidReq;
    std::array<std::uint8_t, HACKSHIELD_REQ_SIZE> Payload{};
    std::size_t PayloadSize = 0;
};

struct HackShieldAction {
    HackShieldActionKind Kind = HackShieldActionKind::None;
    HackShieldPacket Packet{};
    std::uint32_t CheckOption = 0;
};

struct HackShieldUserState {
    std::uint32_t dwConnectionIndex = 0;
    std::uint8_t UserLevel = 0;
    std::uint8_t m_bHSCheck = 0;
};

inline HackShieldPacket make_hackshield_packet(HackShieldProtocol protocol,
                                                std::size_t payload_size = 0) {
    HackShieldPacket packet;
    packet.Protocol = protocol;
    packet.PayloadSize = payload_size;
    return packet;
}

inline HackShieldAction hackshield_none() {
    return HackShieldAction{};
}

inline HackShieldAction hackshield_send(HackShieldProtocol protocol,
                                         std::size_t payload_size,
                                         std::uint32_t check_option = 0) {
    HackShieldAction action;
    action.Kind = HackShieldActionKind::Send;
    action.Packet = make_hackshield_packet(protocol, payload_size);
    action.CheckOption = check_option;
    return action;
}

inline HackShieldAction hackshield_disconnect() {
    HackShieldAction action;
    action.Kind = HackShieldActionKind::Disconnect;
    action.Packet = make_hackshield_packet(HackShieldProtocol::Disconnect);
    return action;
}

inline HackShieldAction send_guid_req(HackShieldUserState& user,
                                       bool make_guid_req_succeeded) {
    if (user.UserLevel < HACKSHIELD_SUPERUSER_LEVEL) return hackshield_none();
    if (!make_guid_req_succeeded) return hackshield_none();

    user.m_bHSCheck = 2;
    return hackshield_send(HackShieldProtocol::GuidReq,
                           HACKSHIELD_GUID_REQ_SIZE);
}

inline HackShieldAction send_hackshield_req(HackShieldUserState& user,
                                             bool make_req_succeeded) {
    if (user.UserLevel < HACKSHIELD_SUPERUSER_LEVEL) return hackshield_none();

    if (user.m_bHSCheck == 2) {
        user.m_bHSCheck = 1;
        return hackshield_none();
    }
    if (user.m_bHSCheck == 1) return hackshield_disconnect();
    if (!make_req_succeeded) return hackshield_none();

    user.m_bHSCheck = 1;
    return hackshield_send(HackShieldProtocol::Req,
                           HACKSHIELD_REQ_SIZE,
                           ANTICPSVR_CHECK_GAME_MEMORY);
}

inline HackShieldAction parse_hackshield_message(
    HackShieldUserState* user,
    HackShieldProtocol protocol,
    bool analyze_succeeded,
    bool make_req_succeeded = true) {
    if (user == nullptr) return hackshield_none();

    if (protocol == HackShieldProtocol::GuidAck) {
        user->m_bHSCheck = 0;
        if (!analyze_succeeded) return hackshield_disconnect();
        if (!make_req_succeeded) return hackshield_none();

        user->m_bHSCheck = 2;
        return hackshield_send(HackShieldProtocol::Req,
                               HACKSHIELD_REQ_SIZE,
                               ANTICPSVR_CHECK_ALL);
    }

    if (protocol == HackShieldProtocol::Ack) {
        user->m_bHSCheck = 0;
        if (!analyze_succeeded) return hackshield_disconnect();
    }
    return hackshield_none();
}

} // namespace mxh::server