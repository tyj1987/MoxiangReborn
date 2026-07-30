// note_manager.hpp - 1:1 port of legacy NoteManager + AutoNoteManager + AutoNoteRoom.
//
// The legacy [Server]Map has three note subsystems:
//   1. NoteManager: in-game mail (player-to-player text). LIMIT_DCOUNT 10.
//   2. AutoNoteManager: per-server auto-events memo queue (e.g. level-up notice).
//   3. AutoNoteRoom: per-map distribution list; legacy uses an idx into CNoteRoom.
//
// Modern port keeps them as one module under note_manager (the legacy
// AgentSide stubs are independent files but the wire layout is identical).

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mxh::server {

// Max notes per player (legacy NOTE_MAX per player LIMIT_DCOUNT).
inline constexpr std::uint8_t MXH_NOTE_PER_PLAYER = 10;

// Max auto-notes per server (legacy COUNT_AUTONOTE).
inline constexpr std::uint16_t MXH_AUTONOTE_LIMIT = 1000;

// 1:1 with legacy NOTE struct.
struct NoteEntry final {
    std::uint32_t note_id   = 0;
    std::uint32_t sender_id = 0;
    std::uint32_t recv_id   = 0;
    std::uint32_t send_ms   = 0;
    std::uint32_t expire_ms = 0;
    char          sender_name[17] = {};
    char          recv_name[17]   = {};
    char          body[101] = {};
    std::uint8_t  acknowledged = 0;  // legacy bDeleted flag; 0=active, 1=read/closed
    std::uint8_t  kind         = 0;
    std::uint8_t  reserved0    = 0;
    std::uint16_t reserved1    = 0;
};

// Auto note (legacy AutoNote/auAutoNote).
struct AutoNoteEntry final {
    std::uint32_t note_id       = 0;
    std::uint32_t target_player = 0;
    std::uint32_t ack_player    = 0;
    std::uint32_t send_ms       = 0;
    std::uint8_t  kind          = 0;
    std::uint8_t  reserved0     = 0;
    std::uint16_t reserved1     = 0;
    char          text[128]     = {};
};

// NoteManager — per-server in-game mail router.
class NoteManager final {
public:
    bool send(std::uint32_t sender_id, std::uint32_t recv_id, const std::string& body,
               std::uint32_t now_ms) noexcept;
    std::size_t inbox_count(std::uint32_t recv_id) const noexcept;
    std::vector<NoteEntry> read(std::uint32_t recv_id) const;
    bool acknowledge(std::uint32_t recv_id, std::uint32_t note_id) noexcept;
    std::size_t total() const noexcept { return notes_.size(); }
private:
    std::vector<NoteEntry> notes_;
};

// AutoNoteManager — server-wide auto event note queue.
class AutoNoteManager final {
public:
    bool enqueue(AutoNoteEntry e) noexcept;
    std::size_t size() const noexcept { return entries_.size(); }
    std::vector<AutoNoteEntry> snapshot() const noexcept { return entries_; }
    bool acknowledge(std::uint32_t note_id) noexcept;
private:
    std::vector<AutoNoteEntry> entries_;
};

// AutoNoteRoom — per-map distribution group (legacy CNoteRoom).
class AutoNoteRoom final {
public:
    void add_player(std::uint32_t player_id) noexcept;
    bool remove_player(std::uint32_t player_id) noexcept;
    std::size_t size() const noexcept { return players_.size(); }
    bool has(std::uint32_t player_id) const noexcept;
    const std::vector<std::uint32_t>& players() const noexcept { return players_; }
private:
    std::vector<std::uint32_t> players_;
};

}  // namespace mxh::server

