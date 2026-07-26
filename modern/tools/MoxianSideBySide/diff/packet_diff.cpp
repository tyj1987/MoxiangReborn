// diff/packet_diff.cpp - byte-level packet comparison.
#include "packet_diff.hpp"
#include <algorithm>
#include <vector>

namespace mxh::tools::sidebyside {

PacketDiff diff_one(const Packet& a, const Packet& b,
                    std::size_t index, const DiffOptions& opt) {
    PacketDiff d;
    d.index = index;
    d.direction = a.direction;
    d.category_a = a.category;
    d.protocol_a = a.protocol;
    d.category_b = b.category;
    d.protocol_b = b.protocol;
    d.first_diff_offset = SIZE_MAX;

    auto bytes_a = a.wire_bytes();
    auto bytes_b = b.wire_bytes();

    std::size_t n = std::min(bytes_a.size(), bytes_b.size());
    for (std::size_t i = 0; i < n; ++i) {
        if (opt.ignore_length_prefix && i < 2) continue;
        // [len:2][checksum:1][code:1][category:1][protocol:1]
        // [object_id:4][payload...]
        if (opt.ignore_object_id && i >= 6 && i <= 9) continue;
        if (i >= 10 && !opt.ignore_payload_offsets.empty()) {
            std::size_t off = i - 10;
            if (std::find(opt.ignore_payload_offsets.begin(),
                          opt.ignore_payload_offsets.end(),
                          off) != opt.ignore_payload_offsets.end())
                continue;
        }
        if (bytes_a[i] != bytes_b[i]) {
            d.first_diff_offset = i;
            d.expected_byte = bytes_a[i];
            d.actual_byte   = bytes_b[i];
            return d;
        }
    }
    if (bytes_a.size() != bytes_b.size()) {
        d.first_diff_offset = std::min(bytes_a.size(), bytes_b.size());
        d.expected_byte = (bytes_a.size() > bytes_b.size())
                        ? bytes_a[d.first_diff_offset] : 0;
        d.actual_byte   = (bytes_b.size() > bytes_a.size())
                        ? bytes_b[d.first_diff_offset] : 0;
    }
    return d;
}

std::vector<PacketDiff> diff_traces(const std::vector<Packet>& a,
                                    const std::vector<Packet>& b,
                                    const DiffOptions& opt) {
    std::vector<PacketDiff> out;
    const std::size_t n = std::min(a.size(), b.size());
    out.reserve(std::max(a.size(), b.size()));
    for (std::size_t i = 0; i < n; ++i) {
        auto d = diff_one(a[i], b[i], i, opt);
        if (!d.bytes_equal()) out.push_back(d);
    }
    if (a.size() != b.size()) {
        if (opt.ignore_trace_length_mismatch) return out;
        const bool a_missing = a.size() < b.size();
        const std::size_t first_missing = n;
        const Packet& present = a_missing ? b[first_missing] : a[first_missing];
        PacketDiff d;
        d.index = first_missing;
        d.direction = present.direction;
        d.category_a = a_missing ? 0u : present.category;
        d.protocol_a = a_missing ? 0u : present.protocol;
        d.category_b = a_missing ? present.category : 0u;
        d.protocol_b = a_missing ? present.protocol : 0u;
        d.first_diff_offset = 0u;
        d.expected_byte = a_missing ? 0u : present.wire_bytes().front();
        d.actual_byte = a_missing ? present.wire_bytes().front() : 0u;
        out.push_back(d);
    }
    return out;
}

}  // namespace mxh::tools::sidebyside
