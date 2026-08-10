// PackFile.cpp - Implementation of .pak format reader.
//
// Original source: 墨香【源码】\4DyuchiFileStorage\PackFile.cpp + typedef.h
//
// Layout (verified against real .pak files):
//   - 92-byte PACK_FILE_HEADER
//   - Per entry: 32-byte FSFILE_HEADER + nameLen bytes + 1 NUL + realSize bytes data
//   - Entries are stored sequentially (each entry size = dwTotalSize)

#include "mxh/compat/pack_file.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <utility>

namespace mxh::compat {

PackFile::~PackFile() = default;

PackFile::PackFile(PackFile&& other) noexcept
    : header_(other.header_),
      entries_(std::move(other.entries_)),
      raw_(std::move(other.raw_)) {
    other.header_ = {};
}

PackFile& PackFile::operator=(PackFile&& other) noexcept {
    if (this != &other) {
        header_ = other.header_;
        entries_ = std::move(other.entries_);
        raw_ = std::move(other.raw_);
        other.header_ = {};
    }
    return *this;
}

bool PackFile::is_pak(std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < sizeof(PackFileHeader)) return false;
    PackFileHeader h{};
    std::memcpy(&h, bytes.data(), sizeof(h));
    return h.version == 0x00000001  // CURRENT_VERSION
        && h.file_item_num > 0
        && h.file_item_num <= 1'000'000;
}

std::unique_ptr<PackFile> PackFile::open(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return nullptr;
    const auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(size);
    if (!f.read(reinterpret_cast<char*>(buf.data()), size)) return nullptr;
    return open_buffer(std::move(buf));
}

std::unique_ptr<PackFile> PackFile::open_buffer(std::vector<std::uint8_t> bytes) {
    if (!is_pak(bytes)) return nullptr;

    auto pack = std::unique_ptr<PackFile>(new PackFile());
    pack->raw_ = std::move(bytes);

    std::memcpy(&pack->header_, pack->raw_.data(), sizeof(PackFileHeader));

    // Walk entries sequentially starting at offset sizeof(PACK_FILE_HEADER) = 92.
    constexpr std::size_t kHeaderEnd = sizeof(PackFileHeader);
    std::size_t cursor = kHeaderEnd;
    pack->entries_.reserve(pack->header_.file_item_num);

    for (std::uint32_t i = 0; i < pack->header_.file_item_num; ++i) {
        if (cursor + sizeof(PackFileDesc) > pack->raw_.size()) break;

        PackFileDesc desc{};
        std::memcpy(&desc, pack->raw_.data() + cursor, sizeof(desc));
        cursor += sizeof(PackFileDesc);

        // Sanity: dwTotalSize must account for at least 32+1+0 (header + NUL + 0 data).
        if (desc.total_size < sizeof(PackFileDesc) + 1) break;
        if (desc.file_name_len > 4096) break;  // sanity cap
        if (desc.real_file_size > pack->raw_.size()) break;

        // Read filename.
        if (cursor + desc.file_name_len > pack->raw_.size()) break;
        std::string name(reinterpret_cast<const char*>(pack->raw_.data() + cursor),
                         desc.file_name_len);
        cursor += desc.file_name_len;

        // Skip 1 NUL byte (seen in real files).
        if (cursor + 1 > pack->raw_.size()) break;
        cursor += 1;

        PackEntry entry;
        entry.name = std::move(name);
        entry.entry_offset = static_cast<std::uint32_t>(
            cursor - desc.file_name_len - 1 - sizeof(PackFileDesc));
        entry.data_offset = static_cast<std::uint32_t>(cursor);
        entry.size = desc.real_file_size;
        if (entry.data_offset + entry.size <= pack->raw_.size()) {
            entry.raw_view = {pack->raw_.data() + entry.data_offset, entry.size};
        }
        pack->entries_.push_back(std::move(entry));

        // Advance to next entry: skip past file data.
        // (Use dwTotalSize - actual_consumed = realSize, since we already consumed 32+nameLen+1.)
        std::size_t advance = static_cast<std::size_t>(desc.total_size)
                            - sizeof(PackFileDesc)
                            - desc.file_name_len
                            - 1;
        if (cursor + advance > pack->raw_.size()) break;
        cursor += advance;
    }

    return pack;
}

const PackEntry* PackFile::find(std::string_view name) const noexcept {
    auto eq_ci = [](std::string_view a, std::string_view b) {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            const char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(a[i])));
            const char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(b[i])));
            if (ca != cb) return false;
        }
        return true;
    };

    for (const auto& e : entries_) {
        if (eq_ci(e.name, name)) return &e;
    }
    return nullptr;
}

std::vector<std::uint8_t> PackFile::read(std::string_view name) const {
    std::vector<std::uint8_t> out;
    if (const PackEntry* e = find(name)) {
        out.assign(e->raw_view.begin(), e->raw_view.end());
    }
    return out;
}

std::vector<std::string> PackFile::list_names() const {
    std::vector<std::string> out;
    out.reserve(entries_.size());
    for (const auto& e : entries_) {
        out.push_back(e.name);
    }
    return out;
}

}  // namespace mxh::compat
