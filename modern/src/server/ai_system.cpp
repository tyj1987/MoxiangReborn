// ai_system.cpp - Phase 6.2 AISystem 1:1 port implementations.

#include "mxh/server/ai_system.hpp"

#include "mxh/server/object.hpp"

#include <algorithm>
#include <utility>

namespace mxh::server {

AISystem& AISystem::instance() {
    static AISystem s{};
    return s;
}

bool AISystem::add_object(Object* obj) {
    if (obj == nullptr) return false;
    if (is_tracked(obj)) return false;
    objects_.push_back(obj);
    last_transitions_.push_back(AiState::Stand);
    return true;
}

Object* AISystem::remove_object(std::uint32_t id) {
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        if (objects_[i] != nullptr && objects_[i]->get_id() == id) {
            Object* removed = objects_[i];
            objects_.erase(objects_.begin() + static_cast<std::ptrdiff_t>(i));
            last_transitions_.erase(last_transitions_.begin() +
                                    static_cast<std::ptrdiff_t>(i));
            return removed;
        }
    }
    return nullptr;
}

bool AISystem::is_tracked(Object* obj) const {
    if (obj == nullptr) return false;
    return std::find(objects_.begin(), objects_.end(), obj) != objects_.end();
}

AiState AISystem::last_transition_for(Object* obj) const {
    if (obj == nullptr) return AiState::Dead;
    auto it = std::find(objects_.begin(), objects_.end(), obj);
    if (it == objects_.end()) return AiState::Dead;
    auto idx = static_cast<std::size_t>(it - objects_.begin());
    if (idx >= last_transitions_.size()) return AiState::Dead;
    return last_transitions_[idx];
}

void AISystem::process(std::uint32_t /*cur_time_ms*/) {
    // Legacy Process iterates every tracked object and drives its
    // CStateMachinen::Update / StateProcess.  Modern forwards to
    // a per-object hook the Object base class owns; subclass
    // hooks (Monster::StateProcess) override the default no-op.
    // We avoid touching virtuals here: framework-level ticks are
    // not implemented yet, just the bookkeeping.
}

void AISystem::set_transition(Object* obj, AiState new_state) {
    if (obj == nullptr) return;
    auto it = std::find(objects_.begin(), objects_.end(), obj);
    if (it == objects_.end()) return;
    auto idx = static_cast<std::size_t>(it - objects_.begin());
    if (idx >= last_transitions_.size()) return;
    last_transitions_[idx] = new_state;
}

std::uint32_t AISystem::generate_monster_id() {
    if (!released_ids_.empty()) {
        std::uint32_t id = released_ids_.back();
        released_ids_.pop_back();
        return id;
    }
    return next_monster_id_++;
}

void AISystem::release_monster_id(std::uint32_t id) {
    if (id == 0) return;
    if (id >= next_monster_id_) return;
    // Avoid duplicate releases of the same ID.
    if (std::find(released_ids_.begin(), released_ids_.end(), id)
        != released_ids_.end()) {
        return;
    }
    released_ids_.push_back(id);
}

void AISystem::send_msg(std::uint16_t /*msg_kind*/, std::uint32_t /*src*/,
                        std::uint32_t /*dest*/, std::uint32_t /*delay*/,
                        std::uint32_t /*flag*/) {
    // Legacy SendMsg routes through CMsgRouter.  The router is
    // owned by a later commit; here we just no-op so callers can
    // compile against the API surface.
}

void AISystem::load_ai_group_list() {
    objects_.clear();
    last_transitions_.clear();
    group_list_ = {};
}

bool AISystem::load_ai_group_list(const std::filesystem::path& path) {
    auto loaded = load_ai_group_list_bin(path);
    if (!loaded.has_value()) return false;
    objects_.clear();
    last_transitions_.clear();
    group_list_ = std::move(*loaded);
    return true;
}

void AISystem::remove_all_list() {
    objects_.clear();
    last_transitions_.clear();
    released_ids_.clear();
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int ai_system_translation_unit_anchor = 0;
}
