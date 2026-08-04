#pragma once

// ai_system.hpp - Phase 6.2 AISystem 1:1 port (subset).
//
// Source-of-truth: legacy [Server]Map/AISystem.h + .cpp.
//
// Small structural port: AI state machine enum (7 states mirror
// legacy DoStand/WalkAround/Pursuit/Attack/RunAway/Rest + Dead),
// AISystem singleton that tracks subordinated objects and owns a
// monotonic monster ID generator.  The full state-machine dispatch
// in legacy AISystem.cpp (state-driven functions, ~17 KB of code)
// is deferred to a later commit; here we expose the framework
// hooks + the 7-state enum so other modules (Monster, AIGroup)
// can build against it.
//
// State machine semantics mirror legacy CStateMachinen:
//
//   Stand        -> idle, scans for new target
//   WalkAround   -> roams within group radius
//   Pursuit      -> chases current target
//   Attack       -> engaged in combat
//   RunAway      -> fleeing from low-HP threshold
//   Rest         -> recovering after combat
//   Dead         -> waiting for regen
//
// Transition graph (legacy) pinned by AiSystemTransitionsArePinned.

#include <cstdint>
#include <filesystem>
#include <vector>

#include "mxh/server/ai_define.hpp"
#include "mxh/server/ai_group_loader.hpp"

namespace mxh::server {

class Object;

// ---- 7-state AI machine (mirror legacy CStateMachinen states) ----
enum class AiState : std::uint8_t {
    Stand       = 0,
    WalkAround  = 1,
    Pursuit     = 2,
    Attack      = 3,
    RunAway     = 4,
    Rest        = 5,
    Dead        = 6,
    Max         = 7,
};

// ---- Sub-event payload (mirror legacy eStateEvent argument) ----
struct AiEvent {
    StateEvent     code     = StateEvent::Null;
    std::uint32_t  src_id   = 0;
    std::uint32_t  dest_id  = 0;
    std::uint32_t  delay    = 0;
    std::uint32_t  flag     = 0;
    std::uint16_t  msg_kind = 0;
};

// ---- AISystem singleton (mirror legacy CAISystem) ----
class AISystem {
public:
    AISystem() = default;
    ~AISystem() = default;

    AISystem(const AISystem&) = default;
    AISystem& operator=(const AISystem&) = default;

    // ---- Object registration ----
    // Returns true if obj was added (false if it was already tracked).
    bool add_object(Object* obj);

    // Returns the removed object, or nullptr if id was unknown.
    Object* remove_object(std::uint32_t id);

    // ---- Periodic dispatch ----
    // Iterates every tracked object and invokes its StateProcess
    // hook (legacy periodic_process).  Caller passes the current
    // game-time clock; the value is not consumed by the framework
    // today (subclasses drive timing via their own AIPARAM).
    void process(std::uint32_t cur_time_ms);

    // ---- State transitions ----
    // Legacy SetTransition forwards to CStateMachinen::SetState(obj,
    // newState).  Modern records the most-recent requested state
    // so tests can pin the dispatch invariant.
    void set_transition(Object* obj, AiState new_state);

    // ---- Monster ID generator (mirror CIndexGenerator) ----
    std::uint32_t generate_monster_id();
    void release_monster_id(std::uint32_t id);

    // ---- State change broadcast (legacy SendMsg) ----
    // Stub: dispatches a (kind, src, dest, delay, flag) tuple
    // through the registered handler if any.  Returns true if a
    // handler was invoked.
    void send_msg(std::uint16_t msg_kind, std::uint32_t src,
                  std::uint32_t dest, std::uint32_t delay,
                  std::uint32_t flag);

    // ---- Group list lifecycle (mirror LoadAIGroupList / RemoveAllList) ----
    // Modern stubs: LoadAIGroupList clears state and is invoked by
    // the agent server on map boot; RemoveAllList drops every
    // tracked object.
    void load_ai_group_list();
    bool load_ai_group_list(const std::filesystem::path& path);
    void remove_all_list();

    // ---- Inspection helpers (test only) ----
    std::size_t tracked_count() const { return objects_.size(); }
    bool is_tracked(Object* obj) const;
    AiState last_transition_for(Object* obj) const;
    std::uint32_t next_monster_id() const { return next_monster_id_; }
    const AiGroupList& group_list() const noexcept { return group_list_; }

    // ---- Singleton ----
    static AISystem& instance();

private:
    std::vector<Object*>       objects_{};
    std::uint32_t              next_monster_id_ = 1;
    std::vector<std::uint32_t> released_ids_{};
    AiGroupList group_list_{};

    // Last transition record per-object (sparse: index aligned with objects_).
    std::vector<AiState>       last_transitions_{};
};

}  // namespace mxh::server
