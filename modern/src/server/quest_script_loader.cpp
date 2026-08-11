#include "mxh/server/quest_script_loader.hpp"

#include "mxh/compat/detail/text_parse.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <cstring>
#include <fstream>

namespace mxh::server {
namespace {

bool is_space(char value) noexcept {
    return value == ' ' || value == '\t' || value == '\n' || value == '\r';
}

std::string_view trim(std::string_view value) noexcept {
    while (!value.empty() && is_space(value.front())) value.remove_prefix(1);
    while (!value.empty() && is_space(value.back())) value.remove_suffix(1);
    return value;
}

bool next_token(std::string_view text, std::size_t& cursor,
                std::string_view& token) noexcept {
    while (cursor < text.size() && is_space(text[cursor])) ++cursor;
    if (cursor == text.size()) return false;
    const auto start = cursor;
    while (cursor < text.size() && !is_space(text[cursor])) ++cursor;
    token = text.substr(start, cursor - start);
    return true;
}

bool parse_u32(std::string_view token, std::uint32_t& value) noexcept {
    if (token.empty()) return false;
    const auto result = std::from_chars(
        token.data(), token.data() + token.size(), value);
    return result.ec == std::errc{} &&
           result.ptr == token.data() + token.size();
}

void absorb_end_param(QuestScriptDefinition& def,
                      const std::vector<QuestExecuteSpec>& executes) {
    if (def.end_param_set) return;
    for (const auto& spec : executes) {
        if (spec.kind == QuestExecuteKind::EndQuest) {
            def.end_param = spec.subquest_idx;
            def.end_param_set = true;
            return;
        }
    }
}

}  // namespace

const QuestScriptDefinition* QuestScriptParseResult::find_quest(
    std::uint32_t quest_idx) const noexcept {
    const auto it = quest_index_by_id.find(quest_idx);
    if (it == quest_index_by_id.end()) return nullptr;
    return &quests[it->second];
}

std::optional<QuestScriptDefinition> parse_quest_stanza(
    std::string_view stanza) noexcept {
    std::size_t cursor = 0;
    std::string_view token;
    if (!next_token(stanza, cursor, token) || token != "$QUEST") {
        return std::nullopt;
    }
    std::string_view quest_token;
    std::uint32_t quest_idx = 0;
    if (!next_token(stanza, cursor, quest_token) ||
        !parse_u32(quest_token, quest_idx)) {
        return std::nullopt;
    }
    if (!next_token(stanza, cursor, token) || token != "{") {
        return std::nullopt;
    }
    // Walk the rest of the stanza counting '{' / '}' so the body
    // we pass to parse_quest_subquest_block() is the exact
    // substring between the $QUEST braces (inner $SUBQUEST blocks
    // have their own braces that confuse a simple "body.back()"
    // check).
    int depth = 1;
    std::size_t end = std::string_view::npos;
    for (std::size_t i = cursor; i < stanza.size(); ++i) {
        if (stanza[i] == '{') ++depth;
        else if (stanza[i] == '}') {
            --depth;
            if (depth == 0) { end = i; break; }
        }
    }
    if (end == std::string_view::npos) return std::nullopt;
    auto body = trim(stanza.substr(cursor, end - cursor));

    QuestScriptDefinition def;
    def.quest_idx = quest_idx;

    // Walk the body token-by-token, slicing out each
    // $SUBQUEST id { ... } sub-stanza and forwarding to
    // parse_quest_subquest_stanza().  Each sub-stanza may live
    // on its own physical line or several $SUBQUEST blocks
    // may be packed onto a single line.
    std::size_t body_cursor = 0;
    while (body_cursor < body.size()) {
        std::size_t dollar = body.find('$', body_cursor);
        if (dollar == std::string_view::npos) break;
        // Find the end of this $SUBQUEST sub-stanza by counting
        // braces from the opening '{' that follows "$SUBQUEST id".
        int sub_depth = 0;
        bool seen_open = false;
        std::size_t sub_end = std::string_view::npos;
        for (std::size_t i = dollar; i < body.size(); ++i) {
            if (body[i] == '{') {
                ++sub_depth;
                seen_open = true;
            } else if (body[i] == '}') {
                --sub_depth;
                if (seen_open && sub_depth == 0) { sub_end = i + 1; break; }
            }
        }
        if (sub_end == std::string_view::npos) return std::nullopt;
        auto sub_text = trim(body.substr(dollar, sub_end - dollar));
        auto sub = parse_quest_subquest_stanza(sub_text, quest_idx);
        if (!sub.has_value()) return std::nullopt;
        for (const auto& trigger : sub->triggers) {
            absorb_end_param(def, trigger.executes);
        }
        def.subquests.push_back(std::move(*sub));
        body_cursor = sub_end;
    }
    return def;
}

QuestScriptParseResult parse_quest_script_text(
    std::string_view text) noexcept {
    QuestScriptParseResult result;
    std::size_t cursor = 0;
    std::string stanza;
    bool in_stanza = false;
    bool stanza_open = false;
    int depth = 0;

    auto flush_stanza = [&]() {
        if (stanza.empty()) return;
        ++result.rows_seen;
        auto def = parse_quest_stanza(stanza);
        if (!def.has_value()) {
            ++result.parse_errors;
        } else {
            result.quest_index_by_id.emplace(
                def->quest_idx, result.quests.size());
            result.quests.push_back(std::move(*def));
            ++result.rows_parsed;
        }
        stanza.clear();
        in_stanza = false;
        stanza_open = false;
        depth = 0;
    };

    while (cursor < text.size()) {
        const auto line_end = text.find('\n', cursor);
        const auto count = line_end == std::string_view::npos
            ? text.size() - cursor
            : line_end - cursor;
        auto line = text.substr(cursor, count);
        cursor = line_end == std::string_view::npos
            ? text.size()
            : line_end + 1;

        if (!in_stanza) {
            const auto trimmed = trim(line);
            if (trimmed.empty()) continue;
            std::size_t token_cursor = 0;
            std::string_view first;
            if (next_token(trimmed, token_cursor, first) && first == "$QUEST") {
                in_stanza = true;
                stanza.assign(line);
                stanza.push_back('\n');
                for (const char ch : line) {
                    if (ch == '{') ++depth;
                    else if (ch == '}') --depth;
                }
                stanza_open = stanza_open || line.find('{') != std::string_view::npos;
                if (stanza_open && depth <= 0) flush_stanza();
                continue;
            }
            ++result.rows_seen;
            ++result.parse_errors;
            continue;
        }

        // Inside a $QUEST stanza: accumulate until brace depth returns to 0.
        stanza.append(line);
        stanza.push_back('\n');
        for (const char ch : line) {
            if (ch == '{') ++depth;
            else if (ch == '}') --depth;
        }
        stanza_open = stanza_open || line.find('{') != std::string_view::npos;
        if (stanza_open && depth <= 0) flush_stanza();
    }
    flush_stanza();
    return result;
}

QuestScriptParseResult parse_quest_script_bytes(
    std::span<const std::uint8_t> raw) {
    QuestScriptParseResult result;
    if (raw.size() < sizeof(mxh::compat::MhFileHeader) + 2) {
        result.error_message = "QuestScript.bin is shorter than its header + CRC bytes";
        return result;
    }
    mxh::compat::MhFileHeader header{};
    std::memcpy(&header, raw.data(), sizeof(header));
    result.file_type = header.type;
    const std::size_t payload_offset = sizeof(header) + 1;
    if (header.file_size > raw.size() - payload_offset) {
        result.error_message = "QuestScript.bin payload exceeds file size";
        return result;
    }
    result.stored_crc = raw[payload_offset - 1];
    std::vector<std::uint8_t> payload(
        raw.begin() + static_cast<std::ptrdiff_t>(payload_offset),
        raw.begin() + static_cast<std::ptrdiff_t>(payload_offset + header.file_size));
    result.decoded_crc = mxh::compat::detail::decode_mhfile_text_payload(
        header.type, payload);
    const auto text = std::string_view(
        reinterpret_cast<const char*>(payload.data()), payload.size());
    auto parsed = parse_quest_script_text(text);
    parsed.file_type = result.file_type;
    parsed.decoded_crc = result.decoded_crc;
    parsed.stored_crc = result.stored_crc;
    if (!result.error_message.empty()) {
        parsed.error_message = std::move(result.error_message);
    }
    return parsed;
}

QuestScriptParseResult load_quest_script(
    const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        QuestScriptParseResult result;
        result.error_message = "file open failed";
        return result;
    }
    const auto size = file.tellg();
    if (size < 0) {
        QuestScriptParseResult result;
        result.error_message = "file size failed";
        return result;
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    return parse_quest_script_bytes(bytes);
}

}  // namespace mxh::server
