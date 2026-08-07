// agent_debug.hpp - AgentDebug data plane (category=40, MP_DEBUG).
//
// 1:1 port of MP_DebugMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 2837-2853.
//
// The legacy handler accepts exactly one protocol (MP_DEBUG_CLIENTASSERT)
// and logs the assertion. There is no network response, no DB write,
// no map forwarding; the only side effect is a console log line. (For
// historical reasons the legacy log line is the literal text
// `\tcoffee tools attacking.\t` regardless of the client payload.)
//
// We preserve this 1-protocol semantic and surface the dispatch as
// a pure decision: either route the assert to the logger or drop it
// when the assert payload is missing.

#pragma once

#include <cstdint>
#include <string_view>

namespace mxh::server {

// MP_CATEGORY byte for MP_DEBUG (40 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t debug_category = 40u;

// Legacy sub-protocol offsets (MP_PROTOCOL_DEBUG is single-entry).
inline constexpr std::uint8_t debug_clientassert = 0u;

// Legacy log text emitted when MP_DEBUG_CLIENTASSERT arrives. The
// text is invariant for ALL client asserts and was a Moxian
// developer watermark (the original author team called themselves
// the "coffee tools").
inline constexpr std::string_view legacy_debug_assert_log =
    "\tcoffee tools attacking.\t";

enum class AgentDebugOutcome : std::uint8_t {
    Logged,   // legacy: WriteAssertMsg line emitted
    Dropped,  // legacy: unknown protocol / missing payload -> silent
};

struct AgentDebugRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool payload_present = true;
};

// 1:1 with legacy MP_DebugMsgParser: only MP_DEBUG_CLIENTASSERT
// produces a log line; everything else is silently dropped.
inline AgentDebugOutcome classify_agent_debug(
    const AgentDebugRequest& r) noexcept {
    if (r.protocol != debug_clientassert) {
        return AgentDebugOutcome::Dropped;
    }
    if (!r.payload_present) {
        return AgentDebugOutcome::Dropped;
    }
    return AgentDebugOutcome::Logged;
}

}  // namespace mxh::server
