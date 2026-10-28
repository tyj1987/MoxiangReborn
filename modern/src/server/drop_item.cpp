// drop_item.cpp

#include "mxh/server/drop_item.hpp"

#include "mxh/compat/mh_file_ex.hpp"

#include <charconv>
#include <string>
#include <string_view>

namespace mxh::server {

void DropTableRegistry::add(const DropTable& t) noexcept {
    tables_.push_back(t);
}

const DropTable* DropTableRegistry::find(std::uint32_t monster_kind, std::uint32_t drop_id) const noexcept {
    for (const auto& t : tables_) {
        if ((t.monster_kind == monster_kind || t.monster_kind == 0) &&
            t.drop_id == drop_id) return &t;
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

namespace {

bool next_token(std::string_view text, std::size_t& cursor,
                std::string_view& token) noexcept {
    while (cursor < text.size() &&
           (text[cursor] == ' ' || text[cursor] == '\t' ||
            text[cursor] == '\r' || text[cursor] == '\n')) {
        ++cursor;
    }
    if (cursor == text.size()) return false;
    const auto begin = cursor;
    while (cursor < text.size() && text[cursor] != ' ' &&
           text[cursor] != '\t' && text[cursor] != '\r' &&
           text[cursor] != '\n') {
        ++cursor;
    }
    token = text.substr(begin, cursor - begin);
    return true;
}

template <typename T>
bool parse_number(std::string_view token, T& value) noexcept {
    if (token.empty()) return false;
    const auto result = std::from_chars(token.data(),
                                        token.data() + token.size(), value);
    return result.ec == std::errc{} &&
           result.ptr == token.data() + token.size();
}

}  // namespace

std::vector<DropTable> load_drop_item_tables(
    const std::filesystem::path& path,
    std::string_view resource_profile_id,
    std::string* error) {
    const auto decoded = mxh::compat::read_server_mh_bin(
        path, resource_profile_id);
    if (!decoded.ok()) {
        if (error) *error = "read failed (error=" +
            std::to_string(static_cast<int>(decoded.error)) + ")";
        return {};
    }
    const auto text = std::string_view(
        reinterpret_cast<const char*>(decoded.value.data.data()),
        decoded.value.data.size());
    std::vector<DropTable> tables;
    std::size_t cursor = 0;
    while (cursor < text.size()) {
        std::string_view token;
        if (!next_token(text, cursor, token)) break;
        DropTable table;
        if (!parse_number(token, table.drop_id)) {
            if (error) *error = "invalid drop index token: " +
                std::string(token);
            return {};
        }
        for (std::size_t index = 0; index < MAX_DROP_PER_MONSTER; ++index) {
            std::string_view name;
            std::string_view item_token;
            std::string_view ratio_token;
            if (!next_token(text, cursor, name) ||
                !next_token(text, cursor, item_token) ||
                !next_token(text, cursor, ratio_token)) {
                if (error) *error = "truncated drop record at index " +
                    std::to_string(table.drop_id);
                return {};
            }
            DropItemEntry entry;
            if (!parse_number(item_token, entry.item_id) ||
                !parse_number(ratio_token, entry.ratio)) {
                if (error) *error = "invalid drop entry at index " +
                    std::to_string(table.drop_id);
                return {};
            }
            if (entry.item_id != 0 && entry.ratio != 0) {
                table.entries.push_back(entry);
            }
        }
        tables.push_back(std::move(table));
    }
    if (tables.empty() && error) *error = "no drop records";
    return tables;
}

}  // namespace mxh::server
