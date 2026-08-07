// agent_jackpot.cpp - TU anchor for the MP_JACKPOT data plane.

#include "mxh/server/agent_jackpot.hpp"

namespace mxh::server {

// MP_JACKPOTUserMsgParser / MP_JACKPOTServerMsgParser routing per
// legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 4614-4673. The
// header hosts the inline classify_jackpot_user / classify_jackpot_server;
// this TU exists so modern/src/server participates in the mxh_server
// link unit and the parser can be referenced from CMake.

}  // namespace mxh::server

[[maybe_unused]] constexpr int agent_jackpot_translation_unit_anchor = 0;
