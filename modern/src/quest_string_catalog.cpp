#include "mxh/compat/quest_string_catalog.hpp"

#include "mxh/compat/mh_file_ex.hpp"

#include <algorithm>
#include <charconv>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mxh::compat {
namespace {
std::string_view trim(std::string_view value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' ||
                              value.front() == '\r' || value.front() == '\n')) value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' ||
                              value.back() == '\r' || value.back() == '\n')) value.remove_suffix(1);
    return value;
}

bool parse_id(std::string_view value, std::uint16_t& result) {
    unsigned parsed = 0;
    const auto converted = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (converted.ec != std::errc{} || parsed > 0xffffu) return false;
    result = static_cast<std::uint16_t>(parsed);
    return true;
}
}  // namespace

const QuestStringEntry* QuestStringCatalog::find(
    std::uint16_t quest_id, std::uint16_t subquest_id) const noexcept {
    const auto it = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) {
        return entry.quest_id == quest_id && entry.subquest_id == subquest_id;
    });
    return it == entries.end() ? nullptr : &*it;
}

std::vector<const QuestStringEntry*> QuestStringCatalog::main_quests() const {
    std::vector<const QuestStringEntry*> result;
    for (const auto& entry : entries) if (entry.subquest_id == 0) result.push_back(&entry);
    return result;
}

QuestStringCatalog parse_quest_string_text(std::string_view text) noexcept {
    QuestStringCatalog catalog;
    QuestStringEntry* current = nullptr;
    bool in_description = false;
    std::size_t cursor = 0;
    while (cursor <= text.size()) {
        const auto end = text.find('\n', cursor);
        const auto line = trim(text.substr(cursor, end == std::string_view::npos
                                                   ? text.size() - cursor : end - cursor));
        if (line.starts_with("$SUBQUESTSTR")) {
            const auto rest = trim(line.substr(12));
            const auto split = rest.find_first_of(" \t");
            std::uint16_t quest = 0, subquest = 0;
            if (split != std::string_view::npos && parse_id(rest.substr(0, split), quest) &&
                parse_id(trim(rest.substr(split)), subquest)) {
                catalog.entries.push_back({quest, subquest, {}, {}});
                current = &catalog.entries.back();
            }
            in_description = false;
        } else if (current && line.starts_with("#TITLE")) {
            current->title = std::string(trim(line.substr(6)));
        } else if (current && line.starts_with("#DESC")) {
            in_description = true;
        } else if (current && in_description && line != "{" && line != "}" && !line.empty()) {
            current->description.emplace_back(line);
        }
        if (end == std::string_view::npos) break;
        cursor = end + 1;
    }
    return catalog;
}

QuestStringCatalog load_quest_string_catalog(const std::filesystem::path& path) {
    const auto file = read_mh_bin(path);
    if (!file.ok()) return {{}, "QuestString.bin could not be decoded"};
    const std::string_view text(reinterpret_cast<const char*>(file.value.data.data()),
                                file.value.data.size());
    return parse_quest_string_text(text);
}

std::wstring big5_to_utf16(std::string_view text) {
    if (text.empty()) return {};
#ifdef _WIN32
    const int required = MultiByteToWideChar(950, MB_ERR_INVALID_CHARS,
        text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) return {};
    std::wstring result(static_cast<std::size_t>(required), L'\0');
    if (MultiByteToWideChar(950, MB_ERR_INVALID_CHARS, text.data(),
                            static_cast<int>(text.size()), result.data(), required) <= 0) return {};
    return result;
#else
    return std::wstring(text.begin(), text.end());
#endif
}

}  // namespace mxh::compat
