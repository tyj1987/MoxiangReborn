#pragma once
#include <mxh/server/discard_avatar_item.hpp>
#include <mxh/compat/detail/text_parse.hpp>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <unordered_map>

namespace mxh::server {
// GameResourceManager::LoadAvatarEquipList: WORD id, BYTE gender/position,
// then eAvatar_Max WORD masks, in source order. Original bytes remain read-only.
struct AvatarEquipCatalog {
    struct Entry { std::uint8_t gender{}; AvatarEquipRow equip; };
    std::unordered_map<std::uint16_t, Entry> entries;
    std::vector<std::uint16_t> duplicate_ids;
};

inline std::optional<AvatarEquipCatalog> parse_avatar_equip_bin(
    std::span<const std::uint8_t> raw, std::string& error) {
    error.clear();
    const auto reject=[&](const char* why)->std::optional<AvatarEquipCatalog> { error=why; return std::nullopt; };
    if(raw.size()<14) return reject("AvatarEquip header truncated");
    const auto word=[&](std::size_t p) { return std::uint32_t(raw[p]) | std::uint32_t(raw[p+1])<<8 |
        std::uint32_t(raw[p+2])<<16 | std::uint32_t(raw[p+3])<<24; };
    const auto type=word(4), size=word(8);
    if(size>16u*1024u*1024u || raw.size()!=std::size_t(size)+14)
        return reject("AvatarEquip payload size mismatch");
    std::vector<std::uint8_t> payload(raw.begin()+13,raw.end()-1);
    const auto crc=compat::detail::decode_mhfile_text_payload(type,payload);
    if(crc!=raw[12] || crc!=raw.back()) return reject("AvatarEquip checksum mismatch");
    std::string text(payload.begin(),payload.end());
    for(auto& c:text) if(c=='\r'||c=='\n') c=' ';
    const auto fields=compat::detail::tokenize(text);
    constexpr auto width=3+game::EAvatarCount;
    if(fields.empty() || fields.size()%width) return reject("AvatarEquip row truncated or empty");
    AvatarEquipCatalog result;
    for(std::size_t i=0;i<fields.size();i+=width) {
        std::array<std::uint32_t,width> values{};
        for(std::size_t j=0;j<width;++j) {
            const auto token=fields[i+j];
            const auto parsed=std::from_chars(token.data(),token.data()+token.size(),values[j]);
            if(parsed.ec!=std::errc{} || parsed.ptr!=token.data()+token.size() || values[j]>65535)
                return reject("AvatarEquip invalid WORD");
        }
        if(values[0]==0 || values[1]>255 || values[2]>=game::EAvatarCount)
            return reject("AvatarEquip invalid identity or BYTE/slot");
        AvatarEquipCatalog::Entry entry;
        entry.gender=static_cast<std::uint8_t>(values[1]);
        entry.equip.position=static_cast<std::uint8_t>(values[2]);
        for(std::size_t j=0;j<game::EAvatarCount;++j) entry.equip.item[j]=static_cast<std::uint16_t>(values[j+3]);
        const auto id=static_cast<std::uint16_t>(values[0]);
        const auto [found,inserted]=result.entries.emplace(id,entry);
        if(!inserted) {
            // reference/legacy-source/4dddd9a6/[Lib]YHLibrary/hashtable.h:
            // Add (115-135) prepends; GetData (138-151) returns the first match.
            result.duplicate_ids.push_back(id);
            found->second=entry;
        }
    }
    return result;
}

inline std::optional<AvatarEquipCatalog> load_avatar_equip_bin(const std::filesystem::path& path,std::string& error) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file) { error="cannot open AvatarEquip"; return std::nullopt; }
    const auto size=file.tellg();
    if(size<0 || size>16*1024*1024+14) { error="invalid AvatarEquip file size"; return std::nullopt; }
    std::vector<std::uint8_t> raw(static_cast<std::size_t>(size)); file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(raw.data()),static_cast<std::streamsize>(raw.size()))) {
        error="cannot read AvatarEquip"; return std::nullopt;
    }
    return parse_avatar_equip_bin(raw,error);
}
} // namespace mxh::server
