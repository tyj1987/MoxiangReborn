// capture/packet_capture.hpp - capture save/load.
#pragma once
#include "../packet.hpp"
#include <string>
#include <vector>

namespace mxh::tools::sidebyside {
bool save_capture(const std::vector<Packet>& trace, const std::string& path);
std::vector<Packet> load_capture(const std::string& path);
}
