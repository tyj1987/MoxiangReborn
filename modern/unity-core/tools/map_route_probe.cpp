#include "mxh/compat/map_change_catalog.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include <iostream>
#include <iomanip>

int main(int argc, char** argv) {
    if (argc == 3 && std::string_view(argv[2]) == "--decoded") {
        const auto data = mxh::compat::read_mh_bin(std::filesystem::u8path(argv[1]));
        if (!data.ok()) return 1;
        for (const unsigned char value : data.value.data) {
            if ((value >= 32 && value < 127) || value == '\n' || value == '\r' || value == '\t')
                std::cout << static_cast<char>(value);
            else std::cout << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(value) << std::dec;
        }
        return 0;
    }
    if (argc != 2) return 2;
    const auto catalog = mxh::compat::load_map_change_bin(std::filesystem::u8path(argv[1]));
    if (!catalog || catalog->entries.empty()) return 1;
    std::cout << "entries=" << catalog->entries.size() << '\n';
    for (const auto& entry : catalog->entries) {
        if (entry.current_map_num != 10) continue;
        std::cout << "map=" << entry.current_map_num << " destination=" << entry.move_map_num
                  << " origin=" << entry.current_x << ',' << entry.current_z
                  << " arrival=" << entry.move_x << ',' << entry.move_z << " name_hex=";
        for (unsigned char value : entry.object_name)
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(value);
        std::cout << std::dec << '\n';
    }
}
