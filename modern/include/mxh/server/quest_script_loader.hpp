#pragma once

#include "mxh/server/quest_subquest_block.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace mxh::server {

// One quest script definition built from a single `$QUEST id { ... }`
// stanza in the legacy [Server]Map/QuestManager.cpp::LoadQuestScript()
// file.  The block may contain zero or more `$SUBQUEST idx { ... }`
// stanzas; each one yields a QuestSubquestEntry.  `end_param` is the
// subquest index captured from the first *ENDQUEST execute across all
// sub-quests (legacy CQuestInfo::SetEndParam semantics).
struct QuestScriptDefinition final {
    std::uint32_t quest_idx = 0;
    std::vector<QuestSubquestEntry> subquests;
    std::uint32_t end_param = 0;
    bool end_param_set = false;
};

// Top-level parse result for an entire QuestScript.bin / .txt file.
struct QuestScriptParseResult final {
    std::vector<QuestScriptDefinition> quests;
    std::unordered_map<std::uint32_t, std::size_t> quest_index_by_id;

    std::uint32_t rows_seen = 0;
    std::uint32_t rows_parsed = 0;
    std::uint32_t parse_errors = 0;
    std::uint32_t file_type = 0;
    std::uint8_t  decoded_crc = 0;
    std::uint8_t  stored_crc = 0;
    std::string   error_message;

    const QuestScriptDefinition* find_quest(std::uint32_t quest_idx) const noexcept;
};

// Pure-function parsers.  parse_quest_script_bytes() runs the
// MHFile header/CRC/XOR pipeline + tokenization + quest-stanza
// extraction; load_quest_script() wraps the disk read.
QuestScriptParseResult parse_quest_script_bytes(
    std::span<const std::uint8_t> raw);
QuestScriptParseResult load_quest_script(
    const std::filesystem::path& path);

// Parse a legacy plain-text QuestScript.txt (or an already-decoded
// QuestScript.bin payload).  Same stanza grammar as the .bin loader;
// the .bin path runs MHFile decode first then forwards the decoded
// text into this function.
QuestScriptParseResult parse_quest_script_text(
    std::string_view text) noexcept;

// Parse one `$QUEST id { ... }` stanza.  Exposed for tests and the
// .txt fallback path; the binary loader feeds each quest stanza
// extracted from the decoded text into this entry point.
std::optional<QuestScriptDefinition> parse_quest_stanza(
    std::string_view stanza) noexcept;

}  // namespace mxh::server
