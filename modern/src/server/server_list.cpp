// server_list.cpp

#include "mxh/server/server_list.hpp"
#include <algorithm>
#include <cstring>

namespace mxh::server {

bool ServerList::add(const ServerListEntry& e) noexcept {
    if (e.port == 0) return false;
    for (const auto& x : entries_) {
        if (x.channel_num == e.channel_num && x.kind == e.kind) return false;
    }
    entries_.push_back(e);
    return true;
}

const ServerListEntry* ServerList::find_for_map(std::uint16_t map_num,
                                                  std::uint8_t kind) const noexcept {
    const ServerListEntry* pick = nullptr;
    for (const auto& e : entries_) {
        if (e.kind != kind) continue;
        if (e.map_num != map_num) continue;
        if (!pick || e.channel_num < pick->channel_num) pick = &e;
    }
    return pick;
}

}  // namespace mxh::server
