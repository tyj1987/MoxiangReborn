#include "mxh/server/dealitem_parser.hpp"

#include "mxh/compat/detail/text_parse.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/server/npc_shop.hpp"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <fstream>
#include <limits>

namespace mxh::server {
namespace {

std::uint32_t parse_number(std::string_view token, bool& ok) noexcept {
    std::uint32_t value = 0;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    ok = result.ec == std::errc{} && result.ptr == token.data() + token.size();
    return value;
}

std::uint32_t parse_count(std::string_view token, bool& ok) noexcept {
    if (token == "-1") {
        ok = true;
        return std::numeric_limits<std::uint32_t>::max();
    }
    return parse_number(token, ok);
}

}  // namespace

const DealItemEntry* DealItemNPC::find_item(std::uint32_t item_idx) const noexcept {
    for (const auto& tab : tabs) {
        for (const auto& entry : tab) {
            if (entry.item_idx == item_idx) return &entry;
        }
    }
    return nullptr;
}

const DealItemNPC* DealItemParseResult::find_npc(std::uint32_t npc_index) const noexcept {
    const auto it = npc_index_to_offset.find(npc_index);
    return it == npc_index_to_offset.end() ? nullptr : &npcs[it->second];
}

DealItemParseResult parse_dealitem_bytes(std::span<const std::uint8_t> raw) {
    DealItemParseResult result;
    if (raw.size() < sizeof(mxh::compat::MhFileHeader) + 2) {
        result.error_message = "DealItem.bin is shorter than its header and CRC bytes";
        return result;
    }

    mxh::compat::MhFileHeader header{};
    std::memcpy(&header, raw.data(), sizeof(header));
    result.file_type = header.type;
    const std::size_t payload_offset = sizeof(header) + 1;
    if (header.file_size > raw.size() - payload_offset) {
        result.error_message = "DealItem.bin payload exceeds file size";
        return result;
    }

    result.stored_crc = raw[payload_offset - 1];
    std::vector<std::uint8_t> payload(raw.begin() + static_cast<std::ptrdiff_t>(payload_offset),
                                       raw.begin() + static_cast<std::ptrdiff_t>(payload_offset + header.file_size));
    result.decoded_crc = mxh::compat::detail::decode_mhfile_text_payload(header.type, payload);

    const auto text = std::string_view(reinterpret_cast<const char*>(payload.data()), payload.size());
    std::size_t line_start = 0;
    while (line_start <= text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto line = mxh::compat::detail::trim(text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start : line_end - line_start));
        if (!line.empty()) {
            ++result.rows_seen;
            const auto tokens = mxh::compat::detail::tokenize(line);
            if (tokens.size() < 9) {
                ++result.parse_errors;
            } else {
                bool ok = true;
                const auto map_num = parse_number(tokens[0], ok);
                const auto npc_kind = parse_number(tokens[2], ok);
                const auto npc_index = parse_number(tokens[4], ok);
                const auto point_x = parse_number(tokens[5], ok);
                const auto point_z = parse_number(tokens[6], ok);
                const auto angle = parse_number(tokens[7], ok);
                const auto tab_num = parse_number(tokens[8], ok);
                if (!ok || tab_num == 0 || tab_num > 255) {
                    ++result.parse_errors;
                } else {
                    auto [it, inserted] = result.npc_index_to_offset.emplace(npc_index, result.npcs.size());
                    if (inserted) {
                        result.npcs.push_back(DealItemNPC{});
                        auto& npc = result.npcs.back();
                        npc.npc_index = static_cast<std::uint16_t>(npc_index);
                        npc.map_num = static_cast<std::uint16_t>(map_num);
                        npc.npc_kind = static_cast<std::uint16_t>(npc_kind);
                        npc.point_x = static_cast<std::uint16_t>(point_x);
                        npc.point_z = static_cast<std::uint16_t>(point_z);
                        npc.angle = static_cast<std::uint16_t>(angle);
                        npc.mapname = std::string(tokens[1]);
                        npc.npcname = std::string(tokens[3]);
                    }
                    auto& npc = result.npcs[it->second];
                    npc.max_tabs = std::max(npc.max_tabs, static_cast<std::uint8_t>(tab_num));
                    npc.tabs.resize(npc.max_tabs);
                    const auto tab = static_cast<std::size_t>(tab_num - 1);
                    for (std::size_t offset = 9; offset + 2 < tokens.size(); offset += 3) {
                        bool item_ok = true;
                        const auto item_idx = parse_number(tokens[offset + 1], item_ok);
                        const auto item_count = parse_count(tokens[offset + 2], item_ok);
                        if (!item_ok) { ++result.parse_errors; continue; }
                        if (item_idx != 0) npc.tabs[tab].push_back({item_idx, item_count, static_cast<std::uint16_t>(tab), static_cast<std::uint16_t>(npc.tabs[tab].size())});
                    }
                    ++result.rows_parsed;
                }
            }
        }
        if (line_end == std::string_view::npos) break;
        line_start = line_end + 1;
    }
    return result;
}

DealItemParseResult load_dealitem(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) { DealItemParseResult result; result.error_message = "file open failed"; return result; }
    const auto size = file.tellg();
    if (size < 0) { DealItemParseResult result; result.error_message = "file size failed"; return result; }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return parse_dealitem_bytes(bytes);
}

std::optional<NpcShopCatalog> catalog_for_npc(const DealItemParseResult& result, std::uint32_t npc_index) noexcept {
    const auto* npc = result.find_npc(npc_index);
    if (!npc) return std::nullopt;
    NpcShopCatalog catalog;
    catalog.npc_id = npc_index;
    for (const auto& tab : npc->tabs) for (const auto& entry : tab) {
        if (entry.item_count == 0) continue;
        catalog.entries.push_back({entry.item_idx, 0, entry.item_count == std::numeric_limits<std::uint32_t>::max() ? 0u : static_cast<std::uint16_t>(entry.item_count)});
    }
    return catalog;
}

}  // namespace mxh::server
