#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::test::resources {
// Original test data, not recovered game assets. Encoders implement the
// documented container layouts independently of the production decoders.
inline void put_u32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
}
inline std::vector<std::uint8_t> legacy(std::string_view text) {
    constexpr std::uint32_t type = 42;
    std::vector<std::uint8_t> bytes(text.size() + 14);
    put_u32(bytes, 0, 1); put_u32(bytes, 4, type);
    put_u32(bytes, 8, static_cast<std::uint32_t>(text.size()));
    std::uint8_t crc = type;
    for (std::size_t i = 0; i < text.size(); ++i) {
        bytes[13 + i] = static_cast<std::uint8_t>(static_cast<unsigned char>(text[i]) + i + (i % type == 0 ? type : 0));
        crc = static_cast<std::uint8_t>(crc + bytes[13 + i]);
    }
    bytes[12] = bytes.back() = crc;
    return bytes;
}
inline std::vector<std::uint8_t> current(std::string_view text, const std::array<std::uint8_t, 8>& key) {
    std::vector<std::uint8_t> bytes(text.size() + 24);
    put_u32(bytes, 0, static_cast<std::uint32_t>(bytes.size()));
    for (std::size_t i = 0; i < text.size(); ++i)
        bytes[24 + i] = static_cast<std::uint8_t>(text[i]) ^ key[i % key.size()];
    return bytes;
}
inline constexpr std::array<std::uint8_t,8> login_key{176,34,211,18,242,2,21,114};
inline constexpr std::array<std::uint8_t,8> penalty_key{201,185,170,137,139,153,100,104};
inline constexpr std::string_view login_text =
    "1 Fixture10 10 1 12345 23456 0\n"
    "2 Fixture12 12 2 65535 1 0 65535 0\n"
    "3 Fixture58 58 1 3333 4444 0\n";
inline constexpr std::string_view penalty_text = "1 0 0\n5 0 0\n48 2.4 1.9\n99 2 1.4\n";
inline std::string monster_text() {
    std::string text;
    // Preserve the original regression's 228-instance admission/broadcast
    // load, spread over three groups with distinct IDs, kinds and positions.
    for (unsigned group = 1; group <= 3; ++group) {
        text += "$Group " + std::to_string(group) + "\n{\n#MAXOBJECT 76\n";
        for (unsigned i = 0; i < 76; ++i)
            text += "#ADD 32 " + std::to_string(group * 1000 + i) + " " +
                std::to_string(100 + group) + " " + std::to_string(1000 + i) + " " +
                std::to_string(2000 + group) + " 0\n";
        text += "}\n";
    }
    return text;
}

class TemporaryBin {
public:
    explicit TemporaryBin(const std::vector<std::uint8_t>& bytes) {
        static std::atomic<unsigned> sequence{0};
        for (;;) {
            directory_ = std::filesystem::temp_directory_path() / ("mxh_original_fixture_" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "_" +
                std::to_string(sequence.fetch_add(1)));
            if (std::filesystem::create_directory(directory_)) break;
        }
        path = directory_ / "resource.bin";
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!file) throw std::runtime_error("could not write synthetic resource fixture");
    }
    TemporaryBin(const TemporaryBin&) = delete;
    TemporaryBin& operator=(const TemporaryBin&) = delete;
    ~TemporaryBin() { std::error_code error; std::filesystem::remove_all(directory_, error); }
    std::filesystem::path path;
private:
    std::filesystem::path directory_;
};
}
