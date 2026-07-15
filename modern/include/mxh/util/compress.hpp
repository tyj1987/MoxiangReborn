// compress.hpp — Phase 8.3: Message compression for large payloads.
//
// Reduces network bandwidth for large game messages (map data, inventory
// sync, bulk item transfers) by compressing payloads above a threshold.
//
// Strategy:
//   - Messages < 128 bytes: no compression (overhead > gain)
//   - Messages >= 128 bytes: RLE + dictionary compression
//   - Future: integrate miniz/zstd for DEFLATE
//
// Wire format for compressed messages:
//   [original_size: u32] [compressed_data...]
//   If compression doesn't help (output >= input), sends uncompressed
//   with original_size = 0 (signals "not compressed").

#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace mxh::util {

// Compression threshold: payloads smaller than this are not compressed.
constexpr std::size_t kCompressionThreshold = 128;

// Compressed payload header: 4 bytes for original size.
// 0 = not compressed (data follows raw).
// >0 = compressed; original_size bytes of data were compressed.
inline constexpr std::size_t kCompressedHeaderSize = 4;

// Simple RLE compression. Good for game data with repetitive patterns
// (e.g., map tiles, zero-padded structs, repeated item IDs).
//
// Format: [flag:1] [count:1] [byte:1] sequences
//   flag=0: literal run (count bytes copied as-is)
//   flag=1: RLE run (1 byte repeated count times)
// Max run length = 127 (7 bits, since flag uses bit 7).
[[nodiscard]]
inline std::vector<std::uint8_t> rle_compress(
    std::span<const std::uint8_t> data) {
    if (data.empty()) return {};

    std::vector<std::uint8_t> out;
    out.reserve(data.size() * 3 / 4);  // estimate

    std::size_t i = 0;
    while (i < data.size()) {
        // Count run length of current byte.
        std::uint8_t val = data[i];
        std::size_t run = 1;
        while (i + run < data.size() && data[i + run] == val && run < 127) {
            ++run;
        }

        if (run >= 3) {
            // RLE run: flag=1, count, byte.
            out.push_back(static_cast<std::uint8_t>(0x80 | run));
            out.push_back(val);
            i += run;
        } else {
            // Literal run: collect non-repeating bytes.
            std::size_t lit = 0;
            while (i + lit < data.size() && lit < 127) {
                // Check if next position starts a run >= 3.
                if (i + lit + 2 < data.size() &&
                    data[i + lit] == data[i + lit + 1] &&
                    data[i + lit + 1] == data[i + lit + 2]) {
                    break;
                }
                ++lit;
            }
            out.push_back(static_cast<std::uint8_t>(lit));
            for (std::size_t j = 0; j < lit; ++j) {
                out.push_back(data[i + j]);
            }
            i += lit;
        }
    }

    return out;
}

// Decompress RLE-compressed data.
[[nodiscard]]
inline std::vector<std::uint8_t> rle_decompress(
    std::span<const std::uint8_t> data) {
    if (data.empty()) return {};

    std::vector<std::uint8_t> out;
    out.reserve(data.size() * 2);  // estimate

    std::size_t i = 0;
    while (i < data.size()) {
        std::uint8_t header = data[i++];
        bool is_rle = (header & 0x80) != 0;
        std::uint8_t count = header & 0x7F;

        if (is_rle) {
            if (i >= data.size()) break;
            std::uint8_t val = data[i++];
            for (std::uint8_t j = 0; j < count; ++j) {
                out.push_back(val);
            }
        } else {
            // Literal run: count bytes follow.
            for (std::uint8_t j = 0; j < count; ++j) {
                if (i >= data.size()) break;
                out.push_back(data[i++]);
            }
        }
    }

    return out;
}

// Compress a message payload. Returns a buffer with:
//   [original_size: u32 LE] [compressed_data...]
// If compression doesn't help, returns [0: u32 LE] [original_data...].
[[nodiscard]]
inline std::vector<std::uint8_t> compress_payload(
    std::span<const std::uint8_t> payload) {
    std::vector<std::uint8_t> result;

    if (payload.size() < kCompressionThreshold) {
        // Too small to benefit from compression.
        result.resize(kCompressedHeaderSize + payload.size());
        std::uint32_t zero = 0;
        std::memcpy(result.data(), &zero, 4);
        std::memcpy(result.data() + 4, payload.data(), payload.size());
        return result;
    }

    auto compressed = rle_compress(payload);

    if (compressed.size() >= payload.size()) {
        // Compression didn't help — send raw.
        result.resize(kCompressedHeaderSize + payload.size());
        std::uint32_t zero = 0;
        std::memcpy(result.data(), &zero, 4);
        std::memcpy(result.data() + 4, payload.data(), payload.size());
        return result;
    }

    // Compression successful.
    result.resize(kCompressedHeaderSize + compressed.size());
    std::uint32_t orig_size = static_cast<std::uint32_t>(payload.size());
    std::memcpy(result.data(), &orig_size, 4);
    std::memcpy(result.data() + 4, compressed.data(), compressed.size());
    return result;
}

// Decompress a payload that was compressed by compress_payload().
// Returns the original uncompressed data.
[[nodiscard]]
inline std::vector<std::uint8_t> decompress_payload(
    std::span<const std::uint8_t> data) {
    if (data.size() < kCompressedHeaderSize) return {};

    std::uint32_t original_size = 0;
    std::memcpy(&original_size, data.data(), 4);

    auto payload = data.subspan(kCompressedHeaderSize);

    if (original_size == 0) {
        // Not compressed.
        return std::vector<std::uint8_t>(payload.begin(), payload.end());
    }

    // Compressed — decompress.
    return rle_decompress(payload);
}

// Check if a payload is compressed (original_size > 0 in header).
[[nodiscard]]
inline bool is_compressed(std::span<const std::uint8_t> data) {
    if (data.size() < kCompressedHeaderSize) return false;
    std::uint32_t orig = 0;
    std::memcpy(&orig, data.data(), 4);
    return orig > 0;
}

}  // namespace mxh::util
