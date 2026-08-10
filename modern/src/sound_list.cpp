#include "mxh/compat/sound_list.hpp"

#include "mxh/compat/mh_file_ex.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>

namespace mxh::compat {
namespace {

void setError(std::string* error, std::string message) {
    if (error) *error = std::move(message);
}

bool endsWithNullWav(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    constexpr std::string_view suffix = "NULL.WAV";
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

} // namespace

bool parse_sound_list_payload(std::span<const std::uint8_t> payload,
                              SoundList& output, std::string* error) {
    output.entries.clear();
    const std::string text(reinterpret_cast<const char*>(payload.data()), payload.size());
    std::istringstream input(text);
    std::uint32_t count = 0;
    if (!(input >> count) || count > 65536u) {
        setError(error, "invalid SoundList entry count");
        return false;
    }
    output.entries.reserve(count);
    for (std::uint32_t expected = 0; expected < count; ++expected) {
        std::uint32_t index = 0;
        int loop = 0, streaming = 0;
        SoundEntry entry;
        if (!(input >> index >> entry.file_name >> loop >> streaming >>
              entry.min_distance >> entry.max_distance >> entry.volume)) {
            setError(error, "truncated SoundList entry " + std::to_string(expected));
            output.entries.clear();
            return false;
        }
        if (index != expected || index > 65535u) {
            setError(error, "non-sequential SoundList index " + std::to_string(index));
            output.entries.clear();
            return false;
        }
        entry.index = static_cast<std::uint16_t>(index);
        entry.loop = loop != 0;
        entry.streaming = streaming != 0;
        entry.available = !endsWithNullWav(entry.file_name);
        output.entries.push_back(std::move(entry));
    }
    return true;
}

bool load_sound_list(const std::filesystem::path& path, SoundList& output,
                     std::string* error) {
    const auto decoded = read_mh_bin(path);
    if (!decoded) {
        setError(error, "unable to decode SoundList.bin");
        return false;
    }
    return parse_sound_list_payload(decoded.value.data, output, error);
}

} // namespace mxh::compat
