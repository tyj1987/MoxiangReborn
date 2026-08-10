#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat {

struct SoundEntry {
    std::uint16_t index = 0;
    std::string file_name;
    bool loop = false;
    bool streaming = false;
    float min_distance = 0.0f;
    float max_distance = 0.0f;
    float volume = 0.0f;
    bool available = true;
};

struct SoundList {
    std::vector<SoundEntry> entries;
};

// Parses the decrypted whitespace-delimited payload consumed by the legacy
// CSoundFileManager. The first token is the entry count, followed by seven
// fields per entry.
[[nodiscard]] bool parse_sound_list_payload(std::span<const std::uint8_t> payload,
                                            SoundList& output,
                                            std::string* error = nullptr);

// Reads and decrypts the original MHFile SoundList.bin, then parses it.
[[nodiscard]] bool load_sound_list(const std::filesystem::path& path,
                                   SoundList& output,
                                   std::string* error = nullptr);

} // namespace mxh::compat
