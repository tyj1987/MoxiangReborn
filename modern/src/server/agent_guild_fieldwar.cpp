// agent_guild_fieldwar.cpp - TU anchor for the MP_GUILD_FIELDWAR data plane.

#include "mxh/server/agent_guild_fieldwar.hpp"

namespace mxh::server {

// MP_GUILD_FIELDWARUserMsgParser / MP_GUILD_FIELDWARServerMsgParser routing per
// legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 4014-4090. The header
// hosts the inline classify_guild_fieldwar_user / classify_guild_fieldwar_server;
// this TU exists so modern/src/server participates in the mxh_server link unit
// and the parser can be referenced from CMake.

}  // namespace mxh::server

[[maybe_unused]] constexpr int agent_guild_fieldwar_translation_unit_anchor = 0;
