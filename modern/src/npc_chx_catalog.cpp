#include "mxh/compat/npc_chx_catalog.hpp"

#include "mxh/compat/mh_file_ex.hpp"

#include <charconv>
#include <cstring>
#include <sstream>

namespace mxh::compat {

std::optional<NpcChxCatalog> NpcChxCatalog::parse_text(
    std::span<const std::uint8_t> payload) {
    if (payload.empty()) return std::nullopt;

    const std::string text(reinterpret_cast<const char*>(payload.data()),
                           payload.size());
    std::istringstream input(text);
    std::string countToken;
    if (!(input >> countToken)) return std::nullopt;

    std::uint32_t count = 0;
    const auto parsed = std::from_chars(
        countToken.data(), countToken.data() + countToken.size(), count);
    if (parsed.ec != std::errc{} ||
        parsed.ptr != countToken.data() + countToken.size() ||
        count == 0 || count > 65536u) {
        return std::nullopt;
    }

    NpcChxCatalog catalog;
    catalog.entries_.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        std::string name;
        if (!(input >> name) || name.empty()) return std::nullopt;
        catalog.entries_.push_back(std::move(name));
    }
    return catalog;
}

std::optional<NpcChxCatalog> NpcChxCatalog::parse_bin(
    std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 5) return std::nullopt;

    std::uint32_t first = 0;
    std::memcpy(&first, bytes.data(), sizeof(first));
    if (first == bytes.size()) {
        return parse_text(decrypt_bin_payload(bytes.subspan(4), 0));
    }

    if (bytes.size() < sizeof(MhFileHeader)) return std::nullopt;
    MhFileHeader header{};
    std::memcpy(&header, bytes.data(), sizeof(header));
    if (header.file_size > 256u * 1024u * 1024u) return std::nullopt;

    std::size_t offset = sizeof(MhFileHeader) + 1;
    if (offset + header.file_size > bytes.size()) {
        offset = sizeof(MhFileHeader);
        if (offset + header.file_size > bytes.size()) return std::nullopt;
    }
    return parse_text(decrypt_bin_payload(
        bytes.subspan(offset, header.file_size), header.type));
}

const std::string* NpcChxCatalog::find(std::uint16_t index) const noexcept {
    if (index == 0 || index >= entries_.size()) return nullptr;
    return &entries_[index];
}

} // namespace mxh::compat
