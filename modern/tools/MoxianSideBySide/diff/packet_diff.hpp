// diff/packet_diff.hpp - byte-level packet comparison.
#pragma once
#include "../packet.hpp"
#include <cstddef>
#include <vector>

namespace mxh::tools::sidebyside {

struct DiffOptions {
    std::vector<std::size_t> ignore_payload_offsets;
    bool ignore_object_id = false;
    bool ignore_timestamp_bytes = false;
    // When true, mismatched trace lengths do not count as diffs.
    // Useful when only one side has a working server.
    bool ignore_trace_length_mismatch = false;
};

PacketDiff diff_one(const Packet& a, const Packet& b,
                    std::size_t index, const DiffOptions& opt);

std::vector<PacketDiff> diff_traces(const std::vector<Packet>& a,
                                    const std::vector<Packet>& b,
                                    const DiffOptions& opt);

}
