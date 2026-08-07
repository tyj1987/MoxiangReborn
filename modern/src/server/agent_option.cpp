// agent_option.cpp - Translation-unit anchor for AgentOption.

#include "mxh/server/agent_option.hpp"

namespace mxh::server {

// MP_OPTIONUserMsgParser routing per legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 2707-2730. The
// header hosts the inline classify_option_user; this TU exists so
// modern/src/server participates in the mxh_server link unit and
// the parser can be referenced from CMake.

}  // namespace mxh::server

[[maybe_unused]] constexpr int agent_option_translation_unit_anchor = 0;
