#pragma once
#include "mxh/server/quest_manager.hpp"
#include "mxh/server/quest_script_loader.hpp"
namespace mxh::server {
QuestDefinition make_runtime_quest_definition(const QuestScriptDefinition& script);
} // namespace mxh::server
