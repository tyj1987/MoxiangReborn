#include "mxh/server/npc_shop.hpp"
#include "mxh/server/dealitem_parser.hpp"
#include <charconv>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "usage: shop_catalog_probe Dealitem.bin map_number|all\n"; return 2; }
    unsigned map = 0;
    const std::string_view value(argv[2]);
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), map);
    if (value != "all" && (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || map == 0 || map > 65535)) return 2;
    const auto data = mxh::server::load_dealitem(argv[1]);
    if (!data.error_message.empty() || data.parse_errors != 0 || data.stored_crc != data.decoded_crc) {
        std::cerr << "Catalog parse/CRC validation failed\n"; return 1;
    }
    std::cout << "{\"formatVersion\":1,\"map\":" << map << ",\"npcs\":[";
    bool first = true;
    for (const auto& npc : data.npcs) {
        if (value != "all" && npc.map_num != map) continue;
        if (!first) std::cout << ',';
        first = false;
        std::cout << "{\"id\":" << npc.npc_index << ",\"map\":" << npc.map_num << ",\"kind\":" << npc.npc_kind
                  << ",\"x\":" << npc.point_x << ",\"z\":" << npc.point_z << ",\"items\":[";
        bool first_item = true;
        if (const auto catalog = mxh::server::catalog_for_npc(data, npc.npc_index)) {
            for (const auto& item : catalog->entries) {
                if (!first_item) std::cout << ',';
                first_item = false;
                std::cout << item.item_id;
            }
        }
        std::cout << "]}";
    }
    std::cout << "]}\n";
    return 0;
}
