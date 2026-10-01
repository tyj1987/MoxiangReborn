#pragma once

#include <mxh/compat/detail/text_parse.hpp>
#include <mxh/server/skin_discard_transition.hpp>

#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

namespace mxh::server {

class SkinCatalog final : public SkinDiscardEnv {
public:
    std::vector<SkinSelectItemInfo> normal;
    std::vector<SkinSelectItemInfo> costume;

    std::size_t skin_count(std::uint16_t kind) const noexcept override {
        if (kind == LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN) return normal.size();
        if (kind == LEGACY_SHOP_ITEM_COSTUME_SKIN) return costume.size();
        return 0;
    }

    const SkinSelectItemInfo* skin_at(std::uint16_t kind,
                                      std::size_t index) const noexcept override {
        const auto* rows = kind == LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN ? &normal :
            (kind == LEGACY_SHOP_ITEM_COSTUME_SKIN ? &costume : nullptr);
        return rows && index < rows->size() ? &(*rows)[index] : nullptr;
    }
};

inline std::optional<std::vector<SkinSelectItemInfo>> parse_skin_catalog_file(
    std::span<const std::uint8_t> raw, bool costume, std::string& error) {
    error.clear();
    const auto reject = [&](const char* why)
        -> std::optional<std::vector<SkinSelectItemInfo>> {
        error = why;
        return std::nullopt;
    };
    if (raw.size() < 14u) return reject("skin catalog header truncated");
    const auto dword = [&](std::size_t at) {
        return std::uint32_t(raw[at]) | std::uint32_t(raw[at + 1]) << 8 |
            std::uint32_t(raw[at + 2]) << 16 | std::uint32_t(raw[at + 3]) << 24;
    };
    const auto type = dword(4u);
    const auto size = dword(8u);
    if (size > 16u * 1024u * 1024u || raw.size() != std::size_t(size) + 14u)
        return reject("skin catalog payload size mismatch");
    std::vector<std::uint8_t> payload(raw.begin() + 13, raw.end() - 1);
    const auto checksum = compat::detail::decode_mhfile_text_payload(type, payload);
    if (checksum != raw[12] || checksum != raw.back())
        return reject("skin catalog checksum mismatch");
    std::string text(payload.begin(), payload.end());
    for (auto& value : text) if (value == '\r' || value == '\n') value = ' ';
    const auto fields = compat::detail::tokenize(text);
    const std::size_t width = costume ? 4u : 6u;
    if (fields.empty() || fields.size() % width != 0u)
        return reject("skin catalog row truncated or empty");

    const auto number = [](std::string_view token, std::uint32_t& value) {
        const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
        return parsed.ec == std::errc{} && parsed.ptr == token.data() + token.size();
    };
    std::vector<SkinSelectItemInfo> rows;
    std::unordered_set<std::uint32_t> ids;
    rows.reserve(fields.size() / width);
    for (std::size_t at = 0; at < fields.size(); at += width) {
        std::uint32_t id = 0, level = 0;
        if (!number(fields[at], id) || id == 0u || !number(fields[at + 2], level))
            return reject("skin catalog invalid identity or level");
        if (!ids.insert(id).second) return reject("skin catalog duplicate identity");
        SkinSelectItemInfo row{};
        row.dw_limit_level = level;
        const auto count = costume ? 1u : kSkinItemListMax;
        for (std::size_t part = 0; part < count; ++part) {
            std::uint32_t item = 0;
            if (!number(fields[at + 3 + part], item) || item > 65535u)
                return reject("skin catalog invalid equipment item");
            row.equip_item[part] = static_cast<std::uint16_t>(item);
        }
        rows.push_back(row);
    }
    return rows;
}

inline std::optional<std::vector<SkinSelectItemInfo>> load_skin_catalog_file(
    const std::filesystem::path& path, bool costume, std::string& error) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) { error = "cannot open skin catalog"; return std::nullopt; }
    const auto size = stream.tellg();
    if (size < 0 || size > 16 * 1024 * 1024 + 14) {
        error = "invalid skin catalog file size";
        return std::nullopt;
    }
    std::vector<std::uint8_t> raw(static_cast<std::size_t>(size));
    stream.seekg(0);
    if (!stream.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(raw.size()))) {
        error = "cannot read skin catalog";
        return std::nullopt;
    }
    return parse_skin_catalog_file(raw, costume, error);
}

inline std::optional<SkinCatalog> load_skin_catalog(
    const std::filesystem::path& normal_path,
    const std::filesystem::path& costume_path,
    std::string& error) {
    auto normal = load_skin_catalog_file(normal_path, false, error);
    if (!normal) return std::nullopt;
    auto costume = load_skin_catalog_file(costume_path, true, error);
    if (!costume) return std::nullopt;
    SkinCatalog result;
    result.normal = std::move(*normal);
    result.costume = std::move(*costume);
    return result;
}

} // namespace mxh::server
