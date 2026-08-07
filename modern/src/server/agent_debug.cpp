// agent_debug.cpp - Translation-unit anchor for AgentDebug.

#include "mxh/server/agent_debug.hpp"

namespace mxh::server {

// MP_DebugMsgParser routing per legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 2837-2853. The
// header hosts the inline classify_agent_debug; this TU exists so
// modern/src/server participates in the mxh_server link unit.

}  // namespace mxh::server

[[maybe_unused]] constexpr int agent_debug_translation_unit_anchor = 0;
