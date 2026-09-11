#include "mxh/game/kyunggong_catalog.hpp"
#include <array>
#include <charconv>
#include <cmath>

namespace mxh::game {
namespace {
bool word(std::string_view token, std::uint16_t& result, bool effect=false) {
    if(effect && token=="-1") { result=65535; return true; }
    unsigned value=0;
    auto parsed=std::from_chars(token.data(),token.data()+token.size(),value);
    if(parsed.ec!=std::errc{} || parsed.ptr!=token.data()+token.size() || value>65535) return false;
    result=static_cast<std::uint16_t>(value); return true;
}
}
std::optional<KyungGongCatalog> KyungGongCatalog::parse(std::string_view text, std::string& error) {
    error.clear();
    if(text.size()>256*1024 || text.find('\0')!=std::string_view::npos) {
        error="invalid lightness text size or embedded NUL"; return std::nullopt;
    }
    std::size_t cursor=0;
    auto next=[&]() -> std::string_view {
        while(cursor<text.size() && static_cast<unsigned char>(text[cursor])<=32) ++cursor;
        auto start=cursor;
        while(cursor<text.size() && static_cast<unsigned char>(text[cursor])>32) ++cursor;
        return text.substr(start,cursor-start);
    };
    KyungGongCatalog catalog;
    for(;;) {
        std::array<std::string_view,9> fields{};
        fields[0]=next(); if(fields[0].empty()) break;
        for(std::size_t i=1;i<fields.size();++i) fields[i]=next();
        if(fields.back().empty()) { error="truncated lightness record"; return std::nullopt; }
        KyungGongInfo row;
        auto speed=std::from_chars(fields[4].data(),fields[4].data()+fields[4].size(),row.speed);
        if(!word(fields[0],row.id) || !row.id || fields[1].size()>255 ||
           !word(fields[2],row.need_mp) || !word(fields[3],row.move_type) ||
           speed.ec!=std::errc{} || speed.ptr!=fields[4].data()+fields[4].size() ||
           !std::isfinite(row.speed) || row.speed<0 ||
           !word(fields[5],row.change_time) || !word(fields[6],row.start_effect,true) ||
           !word(fields[7],row.ongoing_effect,true) || !word(fields[8],row.end_effect,true)) {
            error="invalid lightness record field"; return std::nullopt;
        }
        row.source_name=fields[1];
        if(!catalog.m_entries.emplace(row.id,std::move(row)).second) {
            error="duplicate lightness id"; return std::nullopt;
        }
        if(catalog.size()>4096) { error="lightness row limit exceeded"; return std::nullopt; }
    }
    if(!catalog.size()) { error="lightness catalog is empty"; return std::nullopt; }
    return catalog;
}
const KyungGongInfo* KyungGongCatalog::find(std::uint16_t id) const noexcept {
    const auto found=m_entries.find(id);
    return found==m_entries.end()?nullptr:&found->second;
}
}
