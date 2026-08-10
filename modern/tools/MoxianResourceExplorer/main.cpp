// MoxianResourceExplorer - Command-line tool to inspect old Moxian resource formats.

#include "mxh/compat/bsad_area.hpp"
#include "mxh/compat/bmhm_map.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/compat/pack_file.hpp"
#include "mxh/compat/sound_list.hpp"
#include "mxh/compat/hfl_height_field.hpp"
#include "mxh/compat/stm_static_model.hpp"

#include <cstdlib>
#include <cfloat>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

void print_usage() {
    std::cout << R"(Moxian Resource Explorer

USAGE:
    mxh_explorer <command> [options]

COMMANDS:
    info       <file.bin>          Show metadata of a .bin file
    extract    <file.bin> [-o DIR] Decrypt .bin and write payload to file
    list       <file.pak>          List all entries in a .pak
    extract-pak <file.pak> <name> [-o DIR]
                                  Extract one entry from a .pak
    bsad       <file.bsad>         Visualize a skill area
    map        <file.bmhm>         Show map header info
    sound-list <SoundList.bin>     List original audio IDs and playback metadata
    hfl        <file.hfl>          Show terrain dimensions and texture palette
    stm        <file.stm>          Show static scene materials and mesh totals

OPTIONS:
    -o, --output DIR    Output directory
    -h, --help          Show this help
)";
}

int cmd_info(const fs::path& path) {
    auto result = mxh::compat::read_mh_bin(path);
    if (!result.ok()) {
        std::cerr << "ERROR: failed to read " << path << " (code="
                  << static_cast<int>(result.error) << ")\n";
        return 1;
    }
    const auto& file = result.value;
    std::cout << "File:        " << path << "\n";
    std::cout << "Header:\n";
    std::cout << "  version:   0x" << std::hex << std::setw(8)
              << std::setfill('0') << file.header.version << std::dec << "\n";
    std::cout << "  type:      " << file.header.type << "\n";
    std::cout << "  file_size: " << file.header.file_size << " bytes\n";
    std::cout << "Payload:\n";
    std::cout << "  bytes:     " << file.data.size() << "\n";
    if (!file.data.empty()) {
        std::cout << "  first 32:  ";
        for (std::size_t i = 0; i < std::min<std::size_t>(32, file.data.size()); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(file.data[i]) << " ";
        }
        std::cout << std::dec << "\n";
    }
    return 0;
}

int cmd_sound_list(const fs::path& path) {
    mxh::compat::SoundList list;
    std::string error;
    if (!mxh::compat::load_sound_list(path, list, &error)) {
        std::cerr << "ERROR: " << error << ": " << path << "\n";
        return 1;
    }
    std::size_t available = 0;
    for (const auto& entry : list.entries) {
        if (entry.available) ++available;
        std::cout << entry.index << '\t' << entry.file_name
                  << "\tloop=" << entry.loop
                  << "\tstream=" << entry.streaming
                  << "\tmin=" << entry.min_distance
                  << "\tmax=" << entry.max_distance
                  << "\tvolume=" << entry.volume
                  << "\tavailable=" << entry.available << '\n';
    }
    std::cerr << "SoundList: entries=" << list.entries.size()
              << " available=" << available
              << " null=" << (list.entries.size() - available) << '\n';
    return 0;
}

int cmd_hfl(const fs::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) { std::cerr << "ERROR: cannot open " << path << "\n"; return 1; }
    const auto size = static_cast<std::size_t>(input.tellg());
    input.seekg(0);
    std::vector<std::uint8_t> bytes(size);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size))) return 1;
    mxh::compat::HflHeightField hfl;
    std::string error;
    if (!mxh::compat::parse_hfl(bytes, hfl, &error)) {
        std::cerr << "ERROR: " << error << "\n"; return 1;
    }
    std::vector<bool> used(hfl.textures.size(), false);
    for (const auto tile : hfl.tiles) if ((tile & 0x3fffu) < used.size()) used[tile & 0x3fffu] = true;
    std::cout << "HFL version=" << hfl.version << " heights=" << hfl.desc.height_count_x << 'x'
              << hfl.desc.height_count_z << " tiles=" << hfl.desc.tile_count_x << 'x'
              << hfl.desc.tile_count_z << " textures=" << hfl.textures.size() << "\n";
    for (std::size_t i = 0; i < hfl.textures.size(); ++i)
        std::cout << i << '\t' << hfl.textures[i].index << '\t' << hfl.textures[i].name
                  << "\tused=" << used[i] << '\n';
    return 0;
}

int cmd_stm(const fs::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) { std::cerr << "ERROR: cannot open " << path << "\n"; return 1; }
    const auto size = static_cast<std::size_t>(input.tellg());
    input.seekg(0);
    std::vector<std::uint8_t> bytes(size);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size))) return 1;
    mxh::compat::StmStaticModel stm;
    std::string error;
    const bool parsed = path.extension() == ".mod"
        ? mxh::compat::parse_mod(bytes, stm, &error)
        : mxh::compat::parse_stm(bytes, stm, &error);
    if (!parsed) {
        std::cerr << "ERROR: " << error << "\n"; return 1;
    }
    std::size_t vertices = 0, faces = 0;
    std::array<float, 3> low{FLT_MAX, FLT_MAX, FLT_MAX};
    std::array<float, 3> high{-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (const auto& mesh : stm.meshes) {
        vertices += mesh.positions.size();
        for (const auto& position : mesh.positions)
            for (int axis = 0; axis < 3; ++axis) {
                low[axis] = std::min(low[axis], position[axis]);
                high[axis] = std::max(high[axis], position[axis]);
            }
        for (const auto& group : mesh.face_groups) faces += group.indices.size() / 3;
    }
    std::cout << "STM version=" << stm.version << " materials=" << stm.materials.size()
              << " meshes=" << stm.meshes.size() << " vertices=" << vertices
              << " faces=" << faces << " collision_offset=" << stm.collision_offset << "\n";
    std::cout << "bounds=(" << low[0] << ',' << low[1] << ',' << low[2] << ")..("
              << high[0] << ',' << high[1] << ',' << high[2] << ")\n";
    if (!stm.meshes.empty()) {
        const auto& t = stm.meshes.front().transform;
        std::cout << "first_transform=(" << t[0] << ',' << t[5] << ',' << t[10]
                  << "; " << t[12] << ',' << t[13] << ',' << t[14] << ")\n";
    }
    for (const auto& material : stm.materials)
        std::cout << material.index << '\t' << material.texture_name << '\t'
                  << "alpha=" << material.transparency << " flags=" << material.flags << '\n';
    return 0;
}

int cmd_extract(const fs::path& path, const fs::path& out_dir) {
    auto result = mxh::compat::read_mh_bin(path);
    if (!result.ok()) {
        std::cerr << "ERROR: read failed (code=" << static_cast<int>(result.error) << ")\n";
        return 1;
    }
    fs::create_directories(out_dir);
    auto out = out_dir / (path.stem().string() + ".dec");
    std::ofstream f(out, std::ios::binary);
    if (!f) {
        std::cerr << "ERROR: cannot write " << out << "\n";
        return 1;
    }
    f.write(reinterpret_cast<const char*>(result.value.data.data()),
            static_cast<std::streamsize>(result.value.data.size()));
    std::cout << "Wrote " << result.value.data.size() << " bytes to " << out << "\n";
    return 0;
}

int cmd_list(const fs::path& path) {
    auto pack = mxh::compat::PackFile::open(path);
    if (!pack) {
        std::cerr << "ERROR: cannot open .pak: " << path << "\n";
        return 1;
    }
    std::cout << "Pack: " << path << "\n";
    std::cout << "Header: total_size=" << pack->header().version
              << " count=" << pack->header().file_item_num
              << " version=0x" << std::hex << pack->header().version
              << " flag=" << pack->header().flag << std::dec << "\n";
    std::cout << "Entries (" << pack->file_count() << "):\n";
    for (const auto& e : pack->entries()) {
        std::cout << "  " << std::setw(8) << e.size << "  " << e.name << "\n";
    }
    return 0;
}

int cmd_extract_pak(const fs::path& pack_path,
                    std::string_view entry_name,
                    const fs::path& out_dir) {
    auto pack = mxh::compat::PackFile::open(pack_path);
    if (!pack) {
        std::cerr << "ERROR: cannot open .pak: " << pack_path << "\n";
        return 1;
    }
    auto bytes = pack->read(entry_name);
    if (bytes.empty()) {
        std::cerr << "ERROR: entry not found: " << entry_name << "\n";
        return 1;
    }
    fs::create_directories(out_dir);
    auto safe_name = std::string(entry_name);
    std::replace(safe_name.begin(), safe_name.end(), '\\', '_');
    std::replace(safe_name.begin(), safe_name.end(), '/', '_');
    auto out = out_dir / safe_name;
    std::ofstream f(out, std::ios::binary);
    if (!f) {
        std::cerr << "ERROR: cannot write " << out << "\n";
        return 1;
    }
    f.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    std::cout << "Wrote " << bytes.size() << " bytes to " << out << "\n";
    return 0;
}

int cmd_bsad(const fs::path& path) {
    auto area = mxh::compat::BsadArea::load(path);
    if (area.cells.empty()) {
        std::cerr << "ERROR: cannot parse .bsad: " << path << "\n";
        return 1;
    }
    std::cout << "File: " << path << "\n";
    std::cout << "Size: " << area.header.width << " x " << area.header.height << "\n\n";
    std::cout << "Legend: '.'=empty  '#'=hit  'X'=block\n\n";
    for (std::uint32_t y = 0; y < area.header.height; ++y) {
        for (std::uint32_t x = 0; x < area.header.width; ++x) {
            const auto idx = y * area.header.width + x;
            const auto cell = area.cells[idx];
            switch (cell) {
                case mxh::compat::BsadCell::Empty: std::cout << ". "; break;
                case mxh::compat::BsadCell::Hit:   std::cout << "# "; break;
                case mxh::compat::BsadCell::Block: std::cout << "X "; break;
                default: std::cout << "? "; break;
            }
        }
        std::cout << "\n";
    }
    return 0;
}

int cmd_map(const fs::path& path) {
    auto opt = mxh::compat::BmhmMap::load(path);
    if (!opt.has_value()) {
        std::cerr << "ERROR: not a parseable .bmhm / .mhm file: " << path << "\n";
        return 1;
    }
    auto& m = *opt;
    std::cout << "File: " << path << "\n";
    if (m.has_bmhm_header()) {
        const auto& h = m.header();
        std::cout << "Header:       version=" << h.version
                  << " type=" << h.type
                  << " file_size=" << h.file_size << "\n";
    } else {
        std::cout << "Header:       (none — plaintext .mhm)\n";
    }
    const auto& d = m.desc();
    std::cout << "Sight:        " << d.default_sight << "\n";
    std::cout << "FOV:          " << d.fov << " degrees\n";
    std::cout << "Fog:          " << (d.fog_enabled ? "on" : "off")
              << " density=" << d.fog_density
              << " start=" << d.fog_start
              << " end=" << d.fog_end << "\n";
    std::cout << "Map file:     " << d.map_file_name << "\n";
    std::cout << "Tile file:    " << d.tile_file_name << "\n";
    std::cout << "Sky mod:      " << d.sky_mod << "\n";
    std::cout << "BGM #:        " << d.bgm_sound_num << "\n";
    std::cout << "Sun:          " << (d.sun_enabled ? "on" : "off")
              << " dist=" << d.sun_distance
              << " object=" << d.sun_object << "\n";
    std::cout << "Cloud:        " << d.cloud_num
              << " (" << d.cloud_h_min << ".." << d.cloud_h_max << ")\n";
    std::cout << "Fix height:   " << (d.fix_height_enabled ? "on" : "off")
              << " y=" << d.fix_height << "\n";
    std::cout << "Plaintext:    " << m.plaintext().size() << " bytes\n";
    if (!m.plaintext().empty()) {
        auto first_n = m.plaintext().substr(0, std::min<std::size_t>(m.plaintext().size(), 80));
        std::cout << "  head:        \"" << first_n << "\"\n";
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    std::string_view cmd = argv[1];
    if (cmd == "-h" || cmd == "--help") {
        print_usage();
        return 0;
    }

    // Parse -o flag anywhere after command.
    fs::path out_dir = fs::current_path();
    std::vector<std::string> positional;
    for (int i = 2; i < argc; ++i) {
        std::string_view a = argv[i];
        if (a == "-o" || a == "--output") {
            if (i + 1 < argc) {
                out_dir = argv[++i];
            }
        } else if (a == "-h" || a == "--help") {
            print_usage();
            return 0;
        } else {
            positional.emplace_back(a);
        }
    }

    try {
        if (cmd == "info" && positional.size() == 1) {
            return cmd_info(positional[0]);
        }
        if (cmd == "extract" && positional.size() == 1) {
            return cmd_extract(positional[0], out_dir);
        }
        if (cmd == "list" && positional.size() == 1) {
            return cmd_list(positional[0]);
        }
        if (cmd == "extract-pak" && positional.size() == 2) {
            return cmd_extract_pak(positional[0], positional[1], out_dir);
        }
        if (cmd == "bsad" && positional.size() == 1) {
            return cmd_bsad(positional[0]);
        }
        if (cmd == "map" && positional.size() == 1) {
            return cmd_map(positional[0]);
        }
        if (cmd == "sound-list" && positional.size() == 1) {
            return cmd_sound_list(positional[0]);
        }
        if (cmd == "hfl" && positional.size() == 1) {
            return cmd_hfl(positional[0]);
        }
        if (cmd == "stm" && positional.size() == 1) {
            return cmd_stm(positional[0]);
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 2;
    }

    std::cerr << "ERROR: invalid arguments (run with --help)\n";
    return 1;
}
