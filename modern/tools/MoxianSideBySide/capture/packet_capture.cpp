// capture/packet_capture.cpp - in-memory packet capture.
#include "packet.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>

namespace mxh::tools::sidebyside {

bool save_capture(const std::vector<Packet>& trace,
                  const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "# MoxianSideBySide capture v1\n";
    for (const auto& p : trace) {
        out << p.timestamp_ns << ' '
            << p.direction << ' '
            << static_cast<int>(p.category) << ' '
            << static_cast<int>(p.protocol) << ' '
            << p.length << ' ';
        auto bytes = p.wire_bytes();
        out << std::hex << std::setfill('0');
        for (auto b : bytes) {
            out << std::setw(2) << static_cast<int>(b);
        }
        out << std::dec << "\n";
    }
    return out.good();
}

std::vector<Packet> load_capture(const std::string& path) {
    std::vector<Packet> out;
    std::ifstream in(path, std::ios::binary);
    if (!in) return out;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream is(line);
        Packet p;
        std::uint64_t ts;
        int cat, proto;
        std::size_t len;
        std::string hex;
        if (!(is >> ts >> p.direction >> cat >> proto >> len >> hex))
            continue;
        p.timestamp_ns = ts;
        p.category = static_cast<std::uint8_t>(cat);
        p.protocol = static_cast<std::uint8_t>(proto);
        p.length = static_cast<std::uint32_t>(len);
        std::vector<std::uint8_t> wire;
        for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
            int b;
            std::istringstream(hex.substr(i, 2)) >> std::hex >> b;
            wire.push_back(static_cast<std::uint8_t>(b));
        }
        if (wire.size() >= 10) {
            const auto bodySize = static_cast<std::size_t>(wire[0]) |
                                  (static_cast<std::size_t>(wire[1]) << 8u);
            if (bodySize + 2u != wire.size() || bodySize < 8u) continue;
            p.checksum = wire[2];
            p.code = static_cast<std::int8_t>(wire[3]);
            p.category = wire[4];
            p.protocol = wire[5];
            p.object_id = static_cast<std::uint32_t>(wire[6]) |
                          (static_cast<std::uint32_t>(wire[7]) << 8u) |
                          (static_cast<std::uint32_t>(wire[8]) << 16u) |
                          (static_cast<std::uint32_t>(wire[9]) << 24u);
            p.length = static_cast<std::uint32_t>(bodySize - 8u);
            p.payload.assign(wire.begin() + 10, wire.end());
        }
        out.push_back(std::move(p));
    }
    return out;
}

}  // namespace mxh::tools::sidebyside
