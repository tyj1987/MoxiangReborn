// drop_item.cpp

#include "mxh/server/drop_item.hpp"

namespace mxh::server {

void DropTableRegistry::add(const DropTable& t) noexcept {
    tables_.push_back(t);
}

const DropTable* DropTableRegistry::find(std::uint32_t monster_kind, std::uint32_t drop_id) const noexcept {
    for (const auto& t : tables_) {
        if (t.monster_kind == monster_kind && t.drop_id == drop_id) return &t;
    }
    return nullptr;
}

std::uint32_t DropTableRegistry::roll(std::uint32_t monster_kind, std::uint32_t drop_id,
                                        std::uint32_t rng_value) const noexcept {
    const auto* t = find(monster_kind, drop_id);
    if (!t) return 0;
    // Sum ratios; the roll is u32 modulo total.
    std::uint32_t total = 0;
    for (const auto& e : t->entries) total += e.ratio;
    if (total == 0) return 0;
    std::uint32_t pick = rng_value % total;
    std::uint32_t accum = 0;
    for (const auto& e : t->entries) {
        accum += e.ratio;
        if (pick < accum) return e.item_id;
    }
    return t->entries.back().item_id;  // fallback
}

}  // namespace mxh::server