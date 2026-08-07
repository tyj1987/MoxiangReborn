// agent_exchange.cpp - TU anchor for the MP_EXCHANGE data plane.

#include "mxh/server/agent_exchange.hpp"

namespace mxh::server {

// MP_EXCHANGEUserMsgParser routing per legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 5095-5104. The header hosts
// the inline classify_exchange_user; this TU exists so modern/src/server
// participates in the mxh_server link unit and the parser can be
// referenced from CMake.

}  // namespace mxh::server

[[maybe_unused]] constexpr int agent_exchange_translation_unit_anchor = 0;
