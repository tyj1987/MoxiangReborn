// agent_streetstall.cpp - TU anchor for the MP_STREETSTALL data plane.

#include "mxh/server/agent_streetstall.hpp"

namespace mxh::server {

// MP_STREETSTALLUserMsgParser routing per legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 5083-5092. The header hosts
// the inline classify_streetstall_user; this TU exists so modern/src/server
// participates in the mxh_server link unit and the parser can be
// referenced from CMake.

}  // namespace mxh::server

[[maybe_unused]] constexpr int agent_streetstall_translation_unit_anchor = 0;
