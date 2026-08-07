// agent_bobusang.cpp - TU anchor for the MP_BOBUSANG data plane.

#include "mxh/server/agent_bobusang.hpp"

namespace mxh::server {

// MP_BOBUSANGUserMsgParser / MP_BOBUSANGServerMsgParser routing per
// legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5191-5234. The header
// hosts the inline classify_bobusang_user / classify_bobusang_server;
// this TU exists so modern/src/server participates in the mxh_server
// link unit and the parser can be referenced from CMake.

}  // namespace mxh::server

[[maybe_unused]] constexpr int agent_bobusang_translation_unit_anchor = 0;
