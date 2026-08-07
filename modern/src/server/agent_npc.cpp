// agent_npc.cpp - Translation-unit anchor for AgentNpc.

#include "mxh/server/agent_npc.hpp"

namespace mxh::server {

// The header hosts the inline classify_agent_npc; this TU exists
// so modern/src/server participates in the mxh_server link unit and
// the data plane can be referenced from CMake.

}  // namespace mxh::server

[[maybe_unused]] constexpr int agent_npc_translation_unit_anchor = 0;
