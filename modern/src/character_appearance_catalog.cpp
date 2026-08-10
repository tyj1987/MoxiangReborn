#include "mxh/compat/character_appearance_catalog.hpp"
#include "mxh/compat/mh_file_ex.hpp"

#include <charconv>
#include <cstring>
#include <string_view>

namespace mxh::compat {
namespace {
std::optional<std::vector<std::string>> tokens(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < sizeof(MhFileHeader)) return std::nullopt;
    MhFileHeader header{};
    std::memcpy(&header, bytes.data(), sizeof(header));
    if (!header.file_size || header.file_size > 16u * 1024u * 1024u) return std::nullopt;
    std::size_t offset = sizeof(MhFileHeader) + 1;
    if (offset + header.file_size > bytes.size()) {
        offset = sizeof(MhFileHeader);
        if (offset + header.file_size > bytes.size()) return std::nullopt;
    }
    const auto payload = decrypt_bin_payload(bytes.subspan(offset, header.file_size), header.type);
    std::vector<std::string> out;
    std::size_t cursor = 0;
    while (cursor < payload.size()) {
        while (cursor < payload.size() && payload[cursor] <= 0x20) ++cursor;
        const auto begin = cursor;
        while (cursor < payload.size() && payload[cursor] > 0x20) ++cursor;
        if (cursor > begin) out.emplace_back(reinterpret_cast<const char*>(payload.data() + begin), cursor - begin);
    }
    return out.empty() ? std::nullopt : std::optional{std::move(out)};
}

bool countToken(std::string_view value, std::size_t& count) {
    unsigned parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || parsed > 65535u) return false;
    count = parsed;
    return true;
}
}

std::optional<CharacterModList> CharacterAppearanceCatalog::parse_mod_list_bin(
    std::span<const std::uint8_t> bytes) {
    const auto fields = tokens(bytes);
    if (!fields || fields->size() < 2) return std::nullopt;
    std::size_t count = 0;
    if (!countToken((*fields)[1], count) || fields->size() != count + 2) return std::nullopt;
    CharacterModList list;
    list.base_object = (*fields)[0];
    list.mod_files.assign(fields->begin() + 2, fields->end());
    return list;
}

std::optional<std::vector<std::string>> CharacterAppearanceCatalog::parse_part_list_bin(
    std::span<const std::uint8_t> bytes) {
    const auto fields = tokens(bytes);
    if (!fields) return std::nullopt;
    std::size_t count = 0;
    if (!countToken((*fields)[0], count) || fields->size() != count + 1) return std::nullopt;
    return std::vector<std::string>(fields->begin() + 1, fields->end());
}

} // namespace mxh::compat
