#pragma once

#include <mxh/server/dup_param.hpp>
#include <mxh/compat/detail/text_parse.hpp>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <unordered_map>

namespace mxh::server {

// Source LoadShopItemDupList: Index, #category, Param, comment token.
// Category is retained for audit; lookup is keyed only by Index in the source.
class ShopDupCatalog final : public DupParamLookup {
public:
    struct Entry { std::string category; std::uint32_t param; };
    std::unordered_map<std::uint32_t, Entry> entries;
    bool try_get_dup_param(std::uint32_t index, std::uint32_t& param) const noexcept override {
        const auto it=entries.find(index);
        if (it==entries.end()) return false;
        param=it->second.param; return true;
    }
};

inline std::optional<ShopDupCatalog> parse_shop_dup_bin(
    std::span<const std::uint8_t> raw, std::string& error) {
    error.clear();
    const auto reject=[&](const char* why)->std::optional<ShopDupCatalog> { error=why; return std::nullopt; };
    if (raw.size()<14) return reject("ItemdupOption header truncated");
    const auto word=[&](std::size_t at) {
        return std::uint32_t(raw[at]) | (std::uint32_t(raw[at+1])<<8) |
            (std::uint32_t(raw[at+2])<<16) | (std::uint32_t(raw[at+3])<<24);
    };
    const auto type=word(4), size=word(8);
    if (size>16u*1024u*1024u || raw.size()!=static_cast<std::size_t>(size)+14)
        return reject("ItemdupOption payload size mismatch");
    std::vector<std::uint8_t> payload(raw.begin()+13,raw.end()-1);
    const auto crc=compat::detail::decode_mhfile_text_payload(type,payload);
    if (crc!=raw[12] || crc!=raw.back()) return reject("ItemdupOption checksum mismatch");
    ShopDupCatalog result;
    std::string_view text(reinterpret_cast<const char*>(payload.data()),payload.size());
    std::string normalized(text);
    for (auto& c:normalized) if(c=='\r'||c=='\n') c=' ';
    const auto fields=compat::detail::tokenize(normalized);
    const auto number=[](std::string_view field,std::uint32_t& value) {
        const auto parsed=std::from_chars(field.data(),field.data()+field.size(),value);
        return parsed.ec==std::errc{} && parsed.ptr==field.data()+field.size();
    };
    for(std::size_t i=0;i<fields.size();) {
        std::uint32_t index=0,param=0;
        if(!number(fields[i++],index)) return reject("ItemdupOption invalid index");
        if(index==0) break; // original explicit terminator
        if(fields.size()-i<3) return reject("ItemdupOption row truncated");
        const auto category=fields[i++];
        if(category!="#CHARM" && category!="#HERB" && category!="#INCANTATION" &&
           category!="#SUNDRIES" && category!="#EQUIP") return reject("ItemdupOption unknown category");
        if(!number(fields[i++],param)) return reject("ItemdupOption invalid parameter");
        ++i; // original comment is one GetString token; no text encoding conversion
        if(!result.entries.emplace(index,ShopDupCatalog::Entry{std::string(category),param}).second)
            return reject("ItemdupOption duplicate index");
    }
    if(result.entries.empty()) return reject("ItemdupOption contains no entries");
    return result;
}

inline std::optional<ShopDupCatalog> load_shop_dup_bin(const std::filesystem::path& path,std::string& error) {
    std::ifstream stream(path,std::ios::binary|std::ios::ate);
    if(!stream) { error="cannot open ItemdupOption"; return std::nullopt; }
    const auto size=stream.tellg();
    if(size<0 || size>16*1024*1024+14) { error="invalid ItemdupOption file size"; return std::nullopt; }
    std::vector<std::uint8_t> raw(static_cast<std::size_t>(size));
    stream.seekg(0);
    if(!stream.read(reinterpret_cast<char*>(raw.data()),static_cast<std::streamsize>(raw.size()))) {
        error="cannot read ItemdupOption"; return std::nullopt;
    }
    return parse_shop_dup_bin(raw,error);
}
} // namespace mxh::server
