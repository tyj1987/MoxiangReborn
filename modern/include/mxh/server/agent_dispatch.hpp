// agent_dispatch.hpp
//
// D4.R1 -- Agent side-effect dispatcher scaffolding.
//
// Bridges the agent_*_side_effect_plan.hpp headers (data plane) to the
// actual wire layer. For now it only covers the simplest Drop /
// ForwardToUser pattern shared by all 23+ "agent no handler"
// categories added in D4.173-195. The dispatcher is intentionally
// generic so adding a new category is a 1-line include + a few lines
// of dispatch code.
//
// Wire layer is injected via the IAgentWireSink interface; production
// wires this to the modern TcpServer.Send + Send2User + Broadcast
// implementations, while tests wire it to a mock that records the
// dispatch sequence for assertions.
//
// This header is the runtime counterpart of every agent_*_side_effect_plan.hpp
// and the foundation that the future D4 shop-item runtime orchestrator will
// extend with DB-touching side effects (ShopItemParamUpdateToDb, etc).

#pragma once

#include <cstdint>
#include <vector>

#include "mxh/server/agent_npc.hpp"
#include "mxh/server/agent_npc_side_effect_plan.hpp"
#include "mxh/server/agent_pk.hpp"
#include "mxh/server/agent_pk_side_effect_plan.hpp"
#include "mxh/server/agent_journal.hpp"
#include "mxh/server/agent_journal_side_effect_plan.hpp"
#include "mxh/server/agent_suryun.hpp"
#include "mxh/server/agent_suryun_side_effect_plan.hpp"
#include "mxh/server/agent_societyact.hpp"
#include "mxh/server/agent_societyact_side_effect_plan.hpp"
#include "mxh/server/agent_partywar.hpp"
#include "mxh/server/agent_partywar_side_effect_plan.hpp"
#include "mxh/server/agent_titan.hpp"
#include "mxh/server/agent_titan_side_effect_plan.hpp"
#include "mxh/server/agent_itemext.hpp"
#include "mxh/server/agent_itemext_side_effect_plan.hpp"
#include "mxh/server/agent_kyunggong.hpp"
#include "mxh/server/agent_kyunggong_side_effect_plan.hpp"
#include "mxh/server/agent_simbub.hpp"
#include "mxh/server/agent_simbub_side_effect_plan.hpp"
#include "mxh/server/agent_pyoguk.hpp"
#include "mxh/server/agent_pyoguk_side_effect_plan.hpp"
#include "mxh/server/agent_charrevive.hpp"
#include "mxh/server/agent_charrevive_side_effect_plan.hpp"
#include "mxh/server/agent_bossmonster.hpp"
#include "mxh/server/agent_bossmonster_side_effect_plan.hpp"
#include "mxh/server/agent_monster.hpp"
#include "mxh/server/agent_monster_side_effect_plan.hpp"
#include "mxh/server/agent_char.hpp"
#include "mxh/server/agent_char_side_effect_plan.hpp"
#include "mxh/server/agent_auctionboard.hpp"
#include "mxh/server/agent_auctionboard_side_effect_plan.hpp"
#include "mxh/server/agent_quick.hpp"
#include "mxh/server/agent_quick_side_effect_plan.hpp"
#include "mxh/server/agent_peacewarmode.hpp"
#include "mxh/server/agent_peacewarmode_side_effect_plan.hpp"
#include "mxh/server/agent_ungijosik.hpp"
#include "mxh/server/agent_ungijosik_side_effect_plan.hpp"
#include "mxh/server/agent_auction.hpp"
#include "mxh/server/agent_auction_side_effect_plan.hpp"
#include "mxh/server/agent_autopatch.hpp"
#include "mxh/server/agent_autopatch_side_effect_plan.hpp"
#include "mxh/server/agent_signal.hpp"
#include "mxh/server/agent_signal_side_effect_plan.hpp"
#include "mxh/server/agent_tactic.hpp"
#include "mxh/server/agent_tactic_side_effect_plan.hpp"

namespace mxh::server {

// Wire-layer abstraction. Production wires this to the modern net library.
// Tests wire this to a mock recorder. The dispatcher itself is pure: it
// iterates effects and calls sink->send2user / sink->drop depending on kind.
struct IAgentWireSink {
    virtual ~IAgentWireSink() = default;
    virtual void send2user(std::uint32_t connection_index, std::uint8_t protocol, std::uint32_t object_id) = 0;
    virtual void drop(std::uint8_t protocol, std::uint32_t object_id) = 0;
};

// ===== Per-category dispatchers =====
// Each takes the side-effect plan + the wire sink and walks the effects.
// The dispatch order is preserved; the plan itself is the source of truth.

inline std::size_t dispatch_agent_npc_plan(
    const AgentNpcSideEffectPlan& plan, IAgentWireSink* sink) {
    if (!sink) return 0;
    std::size_t n = 0;
    for (const auto& e : plan.effects) {
        switch (e.kind) {
            case AgentNpcSideEffectKind::ForwardToUser:
                sink->send2user(e.connection_index, e.reply_protocol, e.object_id);
                ++n;
                break;
            case AgentNpcSideEffectKind::Drop:
                sink->drop(e.reply_protocol, e.object_id);
                ++n;
                break;
        }
    }
    return n;
}

inline std::size_t dispatch_agent_pk_plan(
    const AgentPkSideEffectPlan& plan, IAgentWireSink* sink) {
    if (!sink) return 0;
    std::size_t n = 0;
    for (const auto& e : plan.effects) {
        switch (e.kind) {
            case AgentPkSideEffectKind::ForwardToUser:
                sink->send2user(e.connection_index, e.reply_protocol, e.object_id);
                ++n;
                break;
            case AgentPkSideEffectKind::Drop:
                sink->drop(e.reply_protocol, e.object_id);
                ++n;
                break;
        }
    }
    return n;
}

// Generic dispatcher template: any side-effect plan that uses the
// common "Drop / ForwardToUser" pattern with reply_protocol/connection_index/object_id
// fields can plug in here.
template <typename Plan, typename Kind, Kind K_Drop, Kind K_Fwd>
inline std::size_t dispatch_forward_drop_plan(
    const Plan& plan, IAgentWireSink* sink,
    Kind (*kind_of)(const typename std::decay_t<decltype(plan.effects)>::value_type&)) {
    if (!sink) return 0;
    std::size_t n = 0;
    for (const auto& e : plan.effects) {
        switch (kind_of(e)) {
            case K_Fwd:
                sink->send2user(e.connection_index, e.reply_protocol, e.object_id);
                ++n;
                break;
            case K_Drop:
                sink->drop(e.reply_protocol, e.object_id);
                ++n;
                break;
        }
    }
    return n;
}

// Per-effect kind extractors used by the generic dispatcher.
inline AgentJournalSideEffectKind journal_kind_of(const AgentJournalSideEffect& e) { return e.kind; }
inline AgentSuryunSideEffectKind suryun_kind_of(const AgentSuryunSideEffect& e) { return e.kind; }
inline AgentSocietyActSideEffectKind societyact_kind_of(const AgentSocietyActSideEffect& e) { return e.kind; }
inline AgentPartyWarSideEffectKind partywar_kind_of(const AgentPartyWarSideEffect& e) { return e.kind; }
inline AgentTitanSideEffectKind titan_kind_of(const AgentTitanSideEffect& e) { return e.kind; }
inline AgentItemExtSideEffectKind itemext_kind_of(const AgentItemExtSideEffect& e) { return e.kind; }
inline AgentKyungGongSideEffectKind kyunggong_kind_of(const AgentKyungGongSideEffect& e) { return e.kind; }
inline AgentSimBubSideEffectKind simbub_kind_of(const AgentSimBubSideEffect& e) { return e.kind; }
inline AgentPyogukSideEffectKind pyoguk_kind_of(const AgentPyogukSideEffect& e) { return e.kind; }
inline AgentCharReviveSideEffectKind charrevive_kind_of(const AgentCharReviveSideEffect& e) { return e.kind; }
inline AgentBossMonsterSideEffectKind bossmonster_kind_of(const AgentBossMonsterSideEffect& e) { return e.kind; }
inline AgentMonsterSideEffectKind monster_kind_of(const AgentMonsterSideEffect& e) { return e.kind; }
inline AgentCharSideEffectKind char_kind_of(const AgentCharSideEffect& e) { return e.kind; }
inline AgentAuctionBoardSideEffectKind auctionboard_kind_of(const AgentAuctionBoardSideEffect& e) { return e.kind; }
inline AgentQuickSideEffectKind quick_kind_of(const AgentQuickSideEffect& e) { return e.kind; }
inline AgentPeaceWarModeSideEffectKind peacewarmode_kind_of(const AgentPeaceWarModeSideEffect& e) { return e.kind; }
inline AgentUngiJosikSideEffectKind ungijosik_kind_of(const AgentUngiJosikSideEffect& e) { return e.kind; }
inline AgentAuctionSideEffectKind auction_kind_of(const AgentAuctionSideEffect& e) { return e.kind; }
inline AgentAutoPatchSideEffectKind autopatch_kind_of(const AgentAutoPatchSideEffect& e) { return e.kind; }
inline AgentSignalSideEffectKind signal_kind_of(const AgentSignalSideEffect& e) { return e.kind; }
inline AgentTacticSideEffectKind tactic_kind_of(const AgentTacticSideEffect& e) { return e.kind; }

// ===== Generic dispatch helpers for each "agent no handler" category =====
inline std::size_t dispatch_agent_journal_plan(const AgentJournalSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentJournalSideEffectPlan, AgentJournalSideEffectKind,
        AgentJournalSideEffectKind::Drop, AgentJournalSideEffectKind::ForwardToUser>(p, s, journal_kind_of);
}
inline std::size_t dispatch_agent_suryun_plan(const AgentSuryunSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentSuryunSideEffectPlan, AgentSuryunSideEffectKind,
        AgentSuryunSideEffectKind::Drop, AgentSuryunSideEffectKind::ForwardToUser>(p, s, suryun_kind_of);
}
inline std::size_t dispatch_agent_societyact_plan(const AgentSocietyActSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentSocietyActSideEffectPlan, AgentSocietyActSideEffectKind,
        AgentSocietyActSideEffectKind::Drop, AgentSocietyActSideEffectKind::ForwardToUser>(p, s, societyact_kind_of);
}
inline std::size_t dispatch_agent_partywar_plan(const AgentPartyWarSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentPartyWarSideEffectPlan, AgentPartyWarSideEffectKind,
        AgentPartyWarSideEffectKind::Drop, AgentPartyWarSideEffectKind::ForwardToUser>(p, s, partywar_kind_of);
}
inline std::size_t dispatch_agent_titan_plan(const AgentTitanSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentTitanSideEffectPlan, AgentTitanSideEffectKind,
        AgentTitanSideEffectKind::Drop, AgentTitanSideEffectKind::ForwardToUser>(p, s, titan_kind_of);
}
inline std::size_t dispatch_agent_itemext_plan(const AgentItemExtSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentItemExtSideEffectPlan, AgentItemExtSideEffectKind,
        AgentItemExtSideEffectKind::Drop, AgentItemExtSideEffectKind::ForwardToUser>(p, s, itemext_kind_of);
}
inline std::size_t dispatch_agent_kyunggong_plan(const AgentKyungGongSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentKyungGongSideEffectPlan, AgentKyungGongSideEffectKind,
        AgentKyungGongSideEffectKind::Drop, AgentKyungGongSideEffectKind::ForwardToUser>(p, s, kyunggong_kind_of);
}
inline std::size_t dispatch_agent_simbub_plan(const AgentSimBubSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentSimBubSideEffectPlan, AgentSimBubSideEffectKind,
        AgentSimBubSideEffectKind::Drop, AgentSimBubSideEffectKind::ForwardToUser>(p, s, simbub_kind_of);
}
inline std::size_t dispatch_agent_pyoguk_plan(const AgentPyogukSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentPyogukSideEffectPlan, AgentPyogukSideEffectKind,
        AgentPyogukSideEffectKind::Drop, AgentPyogukSideEffectKind::ForwardToUser>(p, s, pyoguk_kind_of);
}
inline std::size_t dispatch_agent_charrevive_plan(const AgentCharReviveSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentCharReviveSideEffectPlan, AgentCharReviveSideEffectKind,
        AgentCharReviveSideEffectKind::Drop, AgentCharReviveSideEffectKind::ForwardToUser>(p, s, charrevive_kind_of);
}
inline std::size_t dispatch_agent_bossmonster_plan(const AgentBossMonsterSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentBossMonsterSideEffectPlan, AgentBossMonsterSideEffectKind,
        AgentBossMonsterSideEffectKind::Drop, AgentBossMonsterSideEffectKind::ForwardToUser>(p, s, bossmonster_kind_of);
}
inline std::size_t dispatch_agent_monster_plan(const AgentMonsterSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentMonsterSideEffectPlan, AgentMonsterSideEffectKind,
        AgentMonsterSideEffectKind::Drop, AgentMonsterSideEffectKind::ForwardToUser>(p, s, monster_kind_of);
}
inline std::size_t dispatch_agent_char_plan(const AgentCharSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentCharSideEffectPlan, AgentCharSideEffectKind,
        AgentCharSideEffectKind::Drop, AgentCharSideEffectKind::ForwardToUser>(p, s, char_kind_of);
}
inline std::size_t dispatch_agent_auctionboard_plan(const AgentAuctionBoardSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentAuctionBoardSideEffectPlan, AgentAuctionBoardSideEffectKind,
        AgentAuctionBoardSideEffectKind::Drop, AgentAuctionBoardSideEffectKind::ForwardToUser>(p, s, auctionboard_kind_of);
}
inline std::size_t dispatch_agent_quick_plan(const AgentQuickSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentQuickSideEffectPlan, AgentQuickSideEffectKind,
        AgentQuickSideEffectKind::Drop, AgentQuickSideEffectKind::ForwardToUser>(p, s, quick_kind_of);
}
inline std::size_t dispatch_agent_peacewarmode_plan(const AgentPeaceWarModeSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentPeaceWarModeSideEffectPlan, AgentPeaceWarModeSideEffectKind,
        AgentPeaceWarModeSideEffectKind::Drop, AgentPeaceWarModeSideEffectKind::ForwardToUser>(p, s, peacewarmode_kind_of);
}
inline std::size_t dispatch_agent_ungijosik_plan(const AgentUngiJosikSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentUngiJosikSideEffectPlan, AgentUngiJosikSideEffectKind,
        AgentUngiJosikSideEffectKind::Drop, AgentUngiJosikSideEffectKind::ForwardToUser>(p, s, ungijosik_kind_of);
}
inline std::size_t dispatch_agent_auction_plan(const AgentAuctionSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentAuctionSideEffectPlan, AgentAuctionSideEffectKind,
        AgentAuctionSideEffectKind::Drop, AgentAuctionSideEffectKind::ForwardToUser>(p, s, auction_kind_of);
}
inline std::size_t dispatch_agent_autopatch_plan(const AgentAutoPatchSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentAutoPatchSideEffectPlan, AgentAutoPatchSideEffectKind,
        AgentAutoPatchSideEffectKind::Drop, AgentAutoPatchSideEffectKind::ForwardToUser>(p, s, autopatch_kind_of);
}
inline std::size_t dispatch_agent_signal_plan(const AgentSignalSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentSignalSideEffectPlan, AgentSignalSideEffectKind,
        AgentSignalSideEffectKind::Drop, AgentSignalSideEffectKind::ForwardToUser>(p, s, signal_kind_of);
}
inline std::size_t dispatch_agent_tactic_plan(const AgentTacticSideEffectPlan& p, IAgentWireSink* s) {
    return dispatch_forward_drop_plan<AgentTacticSideEffectPlan, AgentTacticSideEffectKind,
        AgentTacticSideEffectKind::Drop, AgentTacticSideEffectKind::ForwardToUser>(p, s, tactic_kind_of);
}

}  // namespace mxh::server
