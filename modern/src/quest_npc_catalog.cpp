#include "mxh/compat/quest_npc_catalog.hpp"

#include "mxh/compat/mh_file_ex.hpp"

#include <charconv>
#include <cstdlib>

namespace mxh::compat {
namespace {
std::vector<std::string_view> split_tabs(std::string_view line) {
    std::vector<std::string_view> fields;
    std::size_t cursor = 0;
    while (cursor <= line.size()) {
        const auto end = line.find('\t', cursor);
        auto field = line.substr(cursor, end == std::string_view::npos ? line.size() - cursor : end - cursor);
        while (!field.empty() && (field.back() == '\r' || field.back() == ' ')) field.remove_suffix(1);
        fields.push_back(field);
        if (end == std::string_view::npos) break;
        cursor = end + 1;
    }
    return fields;
}

template <typename T> bool number(std::string_view value, T& out) {
    unsigned parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || parsed > 0xffffu) return false;
    out = static_cast<T>(parsed);
    return true;
}
}  // namespace

QuestNpcCatalog parse_quest_npc_text(std::string_view text) noexcept {
    QuestNpcCatalog result;
    std::size_t cursor = 0;
    while (cursor < text.size()) {
        const auto end = text.find('\n', cursor);
        const auto line = text.substr(cursor, end == std::string_view::npos ? text.size() - cursor : end - cursor);
        const auto fields = split_tabs(line);
        if (fields.size() >= 7 && !fields[0].empty()) {
            QuestNpcEntry entry;
            entry.name = std::string(fields[2]);
            unsigned x = 0, z = 0;
            const auto px = std::from_chars(fields[4].data(), fields[4].data() + fields[4].size(), x);
            const auto pz = std::from_chars(fields[5].data(), fields[5].data() + fields[5].size(), z);
            char* tail = nullptr;
            const std::string direction(fields[6]);
            entry.direction = std::strtof(direction.c_str(), &tail);
            if (number(fields[0], entry.map_num) && number(fields[1], entry.npc_kind) &&
                number(fields[3], entry.npc_index) && px.ec == std::errc{} && pz.ec == std::errc{} &&
                x <= 0xffffu && z <= 0xffffu && tail != direction.c_str()) {
                entry.position_x = static_cast<std::uint16_t>(x);
                entry.position_z = static_cast<std::uint16_t>(z);
                result.entries.push_back(std::move(entry));
            } else ++result.parse_errors;
        }
        if (end == std::string_view::npos) break;
        cursor = end + 1;
    }
    return result;
}

QuestNpcCatalog load_quest_npc_catalog(const std::filesystem::path& path) {
    const auto file = read_mh_bin(path);
    if (!file.ok()) return {{}, 0, "questnpclist.bin could not be decoded"};
    return parse_quest_npc_text(std::string_view(
        reinterpret_cast<const char*>(file.value.data.data()), file.value.data.size()));
}

std::size_t quest_npc_count_for_map(const QuestNpcCatalog& catalog,
                                    std::uint16_t map_num) noexcept {
    std::size_t count = 0;
    for (const auto& entry : catalog.entries) if (entry.map_num == map_num) ++count;
    return count;
}

}  // namespace mxh::compat
