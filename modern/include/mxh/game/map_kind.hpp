#pragma once
#include <charconv>
#include <cstdint>
#include <map>
#include <optional>
#include <sstream>
#include <set>
#include <string>
#include <string_view>
#include "mxh/game/exp_penalty.hpp"

namespace mxh::game {
struct MapKindTable : std::map<std::uint32_t,std::uint32_t> {
    std::set<std::uint32_t> unresolved_maps;
    std::optional<std::uint32_t> resolved_flags(std::uint32_t id) const {
        const auto found=find(id);
        if(found==end() || unresolved_maps.count(id)) return std::nullopt;
        return found->second;
    }
};
inline constexpr std::uint32_t present_revive_exempt_map_mask=128|256|2048|4096|8192;
// GameResourceManager::LoadMapKindInfo: map id, opaque encoded name, flags, #.
// Preserve unknown map semantics explicitly; runtime callers use resolved_flags.
inline std::optional<MapKindTable> parse_map_kind_text(std::string_view text) {
    std::istringstream input{std::string(text)};
    MapKindTable result;
    std::string token,name;
    while(input>>token) {
        std::uint32_t id=0;
        auto [end,error]=std::from_chars(token.data(),token.data()+token.size(),id);
        if(error!=std::errc{} || end!=token.data()+token.size()) return std::nullopt;
        if(!id) { if(input>>token) return std::nullopt; break; }
        if(!(input>>name)) return std::nullopt;
        std::uint32_t bits=0;
        bool complete=false;
        while(input>>token) {
            if(token=="#") { complete=true; break; }
            if(token=="|") continue;
            if(token=="MAPVIEW") bits|=64;
            else if(token=="EVENT") bits|=128;
            else if(token=="RUNNING") bits|=129; // original enum, not a new power of two
            else if(token=="SIEGEWAR") bits|=256;
            else if(token=="BOSS") bits|=512;
            else if(token=="TITAN") bits|=1024;
            else if(token=="QUEST") bits|=2048;
            else if(token=="TOURNAMENT") bits|=4096;
            else if(token=="SURVIVAL") bits|=8192;
            else result.unresolved_maps.insert(id);
        }
        if(!complete || !result.emplace(id,bits).second) return std::nullopt;
    }
    if(result.empty()) return std::nullopt;
    return result;
}
inline std::optional<MapKindTable> decode_map_kind(std::span<const std::uint8_t> raw) {
    if(raw.size()<14 || raw.size()>65536) return std::nullopt;
    const auto u32=[&](std::size_t at) {
        return std::uint32_t(raw[at])|(std::uint32_t(raw[at+1])<<8)|
            (std::uint32_t(raw[at+2])<<16)|(std::uint32_t(raw[at+3])<<24);
    };
    if(!u32(4) || u32(8)!=raw.size()-14) return std::nullopt;
    std::vector<std::uint8_t> decoded(raw.begin()+13,raw.end()-1);
    const auto crc=mxh::compat::detail::decode_mhfile_text_payload(u32(4),decoded);
    if(crc!=raw[12] || crc!=raw.back()) return std::nullopt;
    return parse_map_kind_text(std::string_view(reinterpret_cast<const char*>(decoded.data()),decoded.size()));
}
} // namespace mxh::game
