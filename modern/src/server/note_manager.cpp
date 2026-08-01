// note_manager.cpp

#include "mxh/server/note_manager.hpp"
#include <algorithm>
#include <cstring>

namespace mxh::server {

bool NoteManager::send(std::uint32_t sender_id, std::uint32_t recv_id,
                          const std::string& body, std::uint32_t now_ms) noexcept {
    if (recv_id == 0 || inbox_count(recv_id) >= MXH_NOTE_PER_PLAYER) return false;
    NoteEntry n{};
    n.note_id = next_note_id_++;
    n.sender_id = sender_id;
    n.recv_id   = recv_id;
    n.send_ms   = now_ms;
    n.expire_ms = now_ms + 14ULL * 24ULL * 3600000ULL;  // 14 days like legacy
    n.kind      = 0;
    // Truncate body to 100 chars + NUL.
    std::size_t k = std::min<std::size_t>(body.size(), 100);
    std::memcpy(n.body, body.data(), k);
    n.body[k] = '\0';
    notes_.push_back(n);
    return true;
}

std::size_t NoteManager::inbox_count(std::uint32_t recv_id) const noexcept {
    std::size_t n = 0;
    for (const auto& x : notes_) if (x.recv_id == recv_id && !x.acknowledged) ++n;
    return n;
}

std::vector<NoteEntry> NoteManager::read(std::uint32_t recv_id) const {
    std::vector<NoteEntry> out;
    for (const auto& x : notes_) {
        if (x.recv_id == recv_id && !x.acknowledged) out.push_back(x);
    }
    return out;
}

bool NoteManager::acknowledge(std::uint32_t recv_id, std::uint32_t note_id) noexcept {
    for (auto& x : notes_) {
        if (x.recv_id == recv_id && x.note_id == note_id && !x.acknowledged) {
            x.acknowledged = 1;
            return true;
        }
    }
    return false;
}

// -------- AutoNoteManager --------

bool AutoNoteManager::enqueue(AutoNoteEntry e) noexcept {
    if (entries_.size() >= MXH_AUTONOTE_LIMIT) return false;
    if (e.note_id == 0) e.note_id = next_note_id_++;
    entries_.push_back(e);
    return true;
}

bool AutoNoteManager::acknowledge(std::uint32_t note_id) noexcept {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->note_id == note_id) {
            entries_.erase(it);
            return true;
        }
    }
    return false;
}

// -------- AutoNoteRoom --------

void AutoNoteRoom::add_player(std::uint32_t player_id) noexcept {
    if (player_id == 0) return;
    if (has(player_id)) return;
    players_.push_back(player_id);
}

bool AutoNoteRoom::remove_player(std::uint32_t player_id) noexcept {
    auto it = std::find(players_.begin(), players_.end(), player_id);
    if (it == players_.end()) return false;
    players_.erase(it);
    return true;
}

bool AutoNoteRoom::has(std::uint32_t player_id) const noexcept {
    for (auto p : players_) if (p == player_id) return true;
    return false;
}

}  // namespace mxh::server

