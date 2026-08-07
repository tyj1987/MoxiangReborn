// pet_inven_info_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// pet_inven_info_side_effect_plan(). The data plane returns an empty
// plan (no player / no pet summoned) or a single
// FirePetInvenDbQuery entry; this header walks the plan and
// dispatches the entry to a virtual PetInvenInfoSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_PETINVEN_INFO_SYN from
// [Server]Map/ItemManager.cpp:4941-4952):
//   - FindUser returns null: handler returns (empty plan).
//   - GetCurSummonPet() returns null: handler returns (empty plan,
//     no pet summoned, silent drop).
//   - Both pass: handler fires PetInvenItemOptionInfo -- the DB query
//     that loads the pet-inventory item info for the currently-
//     summoned pet (slot range TP_PETINVEN_START=80..END=100).
//   - The handler sends NO ACK/NACK to the client; the data arrives
//     later via MP_ITEM broadcasts from the DB callback.
//
// Pattern mirrors shop_item_mpinfo_side_effect_runtime.hpp (D4.65)
// and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/pet_inven_info_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the PetInvenInfo side-effect chain.
class PetInvenInfoSideEffectSink {
public:
    virtual ~PetInvenInfoSideEffectSink() = default;

    // Legacy: PetInvenItemOptionInfo(...) -- fires the DB query that
    // loads the pet-inventory item info (slot range 80..100).
    virtual void fire_pet_inven_db_query(std::uint32_t object_id,
                                         std::uint32_t user_id,
                                         std::uint16_t start_pos,
                                         std::uint16_t end_pos) = 0;
};

struct PetInvenInfoRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t db_queries      = 0;
    bool trigger_db_flag_consumed = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline PetInvenInfoRuntimeOutcome apply_pet_inven_info_side_effects(
    const PetInvenInfoSideEffectPlan& plan,
    PetInvenInfoSideEffectSink& sink) {
    PetInvenInfoRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case PetInvenInfoSideEffectKind::FirePetInvenDbQuery:
            sink.fire_pet_inven_db_query(
                effect.object_id, effect.user_id,
                effect.start_pos, effect.end_pos);
            ++out.db_queries;
            ++out.effects_applied;
            break;
        }
    }
    out.trigger_db_flag_consumed = plan.trigger_db;
    return out;
}

}  // namespace mxh::server
