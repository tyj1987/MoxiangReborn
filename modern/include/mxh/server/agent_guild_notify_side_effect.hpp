#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

inline constexpr std::uint8_t LEGACY_EGUILDERR_CREATE_NAME = 5u;
inline constexpr std::uint8_t LEGACY_EGUILDERR_NICK_FILTER = 4u;

enum class AgentGuildNotifyAction : std::uint8_t {
    MunpaJoinSyn,
    MunhaNameChangeOrOtherJoinSyn,
    MunpaDeleteUserAlram,
    GuildCreateSyn,
    GuildGiveNicknameSyn,
};

enum class AgentGuildNotifyOutcome : std::uint8_t {
    ForwardedToMap = 0,
    NotedUser = 1,
    AlarmedMaster = 2,
    CreateNackName = 2,
    NickNackFilter = 3,
    Filtered = 4,
    NoUser = 5,
};

struct AgentGuildNotifyValidationInput final {
    AgentGuildNotifyAction action = AgentGuildNotifyAction::MunpaJoinSyn;
    bool user_found = false;
    bool master_found = false;
    bool filter_passed = false;
    bool usable_name_passed = false;
    bool no_quote_chars = false;
};

inline AgentGuildNotifyOutcome classify_agent_guild_notify_outcome(
    const AgentGuildNotifyValidationInput& in) noexcept {
    switch (in.action) {
        case AgentGuildNotifyAction::MunpaJoinSyn:
            if (!in.user_found) return AgentGuildNotifyOutcome::NoUser;
            if (!in.filter_passed) return AgentGuildNotifyOutcome::Filtered;
            return AgentGuildNotifyOutcome::NotedUser;
        case AgentGuildNotifyAction::MunhaNameChangeOrOtherJoinSyn:
            if (!in.user_found) return AgentGuildNotifyOutcome::NoUser;
            return AgentGuildNotifyOutcome::AlarmedMaster;
        case AgentGuildNotifyAction::MunpaDeleteUserAlram:
            if (!in.user_found) return AgentGuildNotifyOutcome::NoUser;
            if (!in.filter_passed) return AgentGuildNotifyOutcome::Filtered;
            return AgentGuildNotifyOutcome::NotedUser;
        case AgentGuildNotifyAction::GuildCreateSyn:
            if (!in.user_found) return AgentGuildNotifyOutcome::NoUser;
            if (!in.filter_passed) return AgentGuildNotifyOutcome::Filtered;
            if (!in.usable_name_passed) return AgentGuildNotifyOutcome::CreateNackName;
            return AgentGuildNotifyOutcome::ForwardedToMap;
        case AgentGuildNotifyAction::GuildGiveNicknameSyn:
            if (!in.user_found) return AgentGuildNotifyOutcome::NoUser;
            if (!in.usable_name_passed || !in.no_quote_chars) {
                return AgentGuildNotifyOutcome::NickNackFilter;
            }
            return AgentGuildNotifyOutcome::ForwardedToMap;
    }
    return AgentGuildNotifyOutcome::NoUser;
}

enum class AgentGuildNotifySideEffectKind : std::uint8_t {
    CopyNoteBuffers = 0,
    FilterCheckGuildName = 1,
    NoteServerSendtoPlayer = 2,
    SendJoinMasterAlram = 3,
    SendMunhaMasterAlram = 4,
    SendCreateNack = 5,
    SendNickNack = 6,
    ForwardToMapServer = 7,
};

struct AgentGuildNotifySideEffect final {
    AgentGuildNotifySideEffectKind kind =
        AgentGuildNotifySideEffectKind::CopyNoteBuffers;
    std::uint32_t object_id = 0;
    std::uint32_t master_id = 0;
    std::uint8_t nack_code = 0;
};

struct AgentGuildNotifySideEffectPlan final {
    std::vector<AgentGuildNotifySideEffect> effects;
    bool dispatched = false;
    bool forward_to_map = false;
    bool send_nack = false;
};

inline AgentGuildNotifySideEffectPlan agent_guild_notify_side_effect_plan(
    const AgentGuildNotifyValidationInput& in,
    std::uint32_t object_id,
    std::uint32_t master_id) {
    AgentGuildNotifySideEffectPlan plan;
    const AgentGuildNotifyOutcome outcome =
        classify_agent_guild_notify_outcome(in);

    switch (in.action) {
        case AgentGuildNotifyAction::MunpaJoinSyn:
            if (outcome != AgentGuildNotifyOutcome::NotedUser) return plan;
            plan.dispatched = true;
            plan.effects.reserve(in.master_found ? 4u : 3u);
            {
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::CopyNoteBuffers;
                effect.object_id = object_id;
                plan.effects.push_back(effect);
            }
            {
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::FilterCheckGuildName;
                effect.object_id = object_id;
                plan.effects.push_back(effect);
            }
            {
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::NoteServerSendtoPlayer;
                effect.object_id = object_id;
                plan.effects.push_back(effect);
            }
            if (in.master_found) {
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::SendJoinMasterAlram;
                effect.object_id = object_id;
                effect.master_id = master_id;
                plan.effects.push_back(effect);
            }
            return plan;
        case AgentGuildNotifyAction::MunhaNameChangeOrOtherJoinSyn:
            if (outcome != AgentGuildNotifyOutcome::AlarmedMaster) return plan;
            plan.dispatched = true;
            plan.effects.reserve(1u);
            {
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::SendMunhaMasterAlram;
                effect.object_id = object_id;
                plan.effects.push_back(effect);
            }
            return plan;
        case AgentGuildNotifyAction::MunpaDeleteUserAlram:
            if (outcome != AgentGuildNotifyOutcome::NotedUser) return plan;
            plan.dispatched = true;
            plan.effects.reserve(3u);
            {
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::CopyNoteBuffers;
                effect.object_id = object_id;
                plan.effects.push_back(effect);
            }
            {
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::FilterCheckGuildName;
                effect.object_id = object_id;
                plan.effects.push_back(effect);
            }
            {
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::NoteServerSendtoPlayer;
                effect.object_id = object_id;
                plan.effects.push_back(effect);
            }
            return plan;
        case AgentGuildNotifyAction::GuildCreateSyn:
            if (outcome == AgentGuildNotifyOutcome::ForwardedToMap) {
                plan.dispatched = true;
                plan.forward_to_map = true;
                plan.effects.reserve(1u);
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::ForwardToMapServer;
                effect.object_id = object_id;
                plan.effects.push_back(effect);
                return plan;
            }
            if (outcome == AgentGuildNotifyOutcome::CreateNackName) {
                plan.send_nack = true;
                plan.effects.reserve(1u);
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::SendCreateNack;
                effect.object_id = object_id;
                effect.nack_code = LEGACY_EGUILDERR_CREATE_NAME;
                plan.effects.push_back(effect);
            }
            return plan;
        case AgentGuildNotifyAction::GuildGiveNicknameSyn:
            if (outcome == AgentGuildNotifyOutcome::ForwardedToMap) {
                plan.dispatched = true;
                plan.forward_to_map = true;
                plan.effects.reserve(1u);
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::ForwardToMapServer;
                effect.object_id = object_id;
                plan.effects.push_back(effect);
                return plan;
            }
            if (outcome == AgentGuildNotifyOutcome::NickNackFilter) {
                plan.send_nack = true;
                plan.effects.reserve(1u);
                AgentGuildNotifySideEffect effect{};
                effect.kind = AgentGuildNotifySideEffectKind::SendNickNack;
                effect.object_id = object_id;
                effect.nack_code = LEGACY_EGUILDERR_NICK_FILTER;
                plan.effects.push_back(effect);
            }
            return plan;
    }
    return plan;
}

}  // namespace mxh::server
