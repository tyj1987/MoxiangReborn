// PackFile.cpp - Implementation of .pak format reader.
//
// Original source: 墨香【源码】\4DyuchiFileStorage\PackFile.cpp + typedef.h
//
// Layout (verified against real .pak files and docs/formats/pak.md):
//   - Offset 4 != 0: 92-byte header, entries at 92, advance by layout.
//   - Offset 4 == 0: 96-byte header, count at offset 92, entries at 96,
//     next entry at cur + total. Truncation and corrupt headers fail closed.
//   - Per entry: 32-byte FSFILE_HEADER + nameLen bytes + 1 NUL + realSize bytes data

#include "mxh/compat/pack_file.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <utility>

namespace {

// True when [offset, offset + length) sits inside bytes. length 0 at
// offset == size is in range; a length that would wrap is not.
bool range_in_file(const std::vector<std::uint8_t>& bytes, std::size_t offset,
                   std::size_t length) {
    return length <= bytes.size() && offset <= bytes.size() - length;
}

bool read_u32(const std::vector<std::uint8_t>& bytes, std::size_t offset,
              std::uint32_t& out) {
    if (!range_in_file(bytes, offset, sizeof(out))) return false;
    std::memcpy(&out, bytes.data() + offset, sizeof(out));
    return true;
}

}  // namespace

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
    if (bytes.size() < 8) return false;
    std::uint32_t version = 0;
    std::uint32_t file_item_num = 0;
    std::memcpy(&version, bytes.data(), sizeof(version));
    std::memcpy(&file_item_num, bytes.data() + 4, sizeof(file_item_num));
    if (version != 0x00000001) return false;  // CURRENT_VERSION
    if (file_item_num == 0) {
        // Original 96-byte header. The count is not the field at offset 4.
        if (bytes.size() < 96) return false;
        std::uint32_t count = 0;
        std::memcpy(&count, bytes.data() + 92, sizeof(count));
        return count > 0 && count <= 1'000'000;
    }
    if (bytes.size() < sizeof(PackFileHeader)) return false;
    return file_item_num <= 1'000'000;
}

std::unique_ptr<PackFile> PackFile::open(const std::filesystem::path& path,
                                         std::string* error) {
    if (error) error->clear();
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        if (error) *error = "failed to open pak";
        return nullptr;
    }
    const auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(size);
    if (!f.read(reinterpret_cast<char*>(buf.data()), size)) {
        if (error) *error = "failed to read pak";
        return nullptr;
    }
    return open_buffer(std::move(buf), error);
}

std::unique_ptr<PackFile> PackFile::open_original_96(std::vector<std::uint8_t> bytes,
                                                     std::string* error) {
    auto fail = [&](std::string message) -> std::unique_ptr<PackFile> {
        if (error) *error = std::move(message);
        return nullptr;
    };

    // Offset 4 is 0. Anything short of the 96-byte header is truncated,
    // not a 92-byte pak with a zero count.
    if (bytes.size() < 96) {
        return fail(
            "truncated pak header: original-style pak (dwFileItemNum at "
            "offset 4 is 0) needs 96 bytes with the entry count at offset "
            "92, file size is " + std::to_string(bytes.size()));
    }

    std::uint32_t count = 0;
    if (!read_u32(bytes, 92, count)) {
        return fail(
            "truncated pak header: entry count at offset 92 could not be read");
    }
    // A zero count is not the original Map.pak shape. Keep rejecting it so
    // a 96-byte version-1 header of zeros still fails open.
    if (count == 0) {
        return fail("corrupt pak header: entry count at offset 92 is 0");
    }
    if (count > 1'000'000u) {
        return fail("entry count " + std::to_string(count) +
                    " is larger than the file allows");
    }

    // Smallest entry is 32 + 0-byte name + NUL + 0-byte payload.
    constexpr std::size_t kMinEntry = 33;
    const std::size_t body = bytes.size() - 96;
    if (body / kMinEntry < count) {
        return fail("entry count " + std::to_string(count) +
                    " is larger than the file allows (" +
                    std::to_string(body) +
                    " bytes remain after the 96-byte header)");
    }

    std::vector<PackEntry> parsed;
    parsed.reserve(count);
    std::size_t cur = 96;
    for (std::uint32_t index = 0; index < count; ++index) {
        if (cur > bytes.size() || bytes.size() - cur < sizeof(PackFileDesc)) {
            return fail(
                "EOF arrived early at pak entry " + std::to_string(index) +
                " offset " + std::to_string(cur) +
                ": entry header is 32 bytes but the file ends first "
                "(declared count " + std::to_string(count) + ")");
        }

        PackFileDesc desc{};
        std::memcpy(&desc, bytes.data() + cur, sizeof(desc));

        const std::uint64_t expected =
            32ull + static_cast<std::uint64_t>(desc.file_name_len) + 1ull +
            static_cast<std::uint64_t>(desc.real_file_size);
        if (expected != desc.total_size || desc.total_size < kMinEntry) {
            return fail(
                "corrupt pak entry " + std::to_string(index) +
                " at offset " + std::to_string(cur) +
                ": total does not equal 32+name_len+1+real");
        }
        // expected == total_size, so it fits in uint32 and in size_t.
        if (expected > bytes.size() - cur) {
            return fail(
                "entry overruns EOF at pak entry " + std::to_string(index) +
                " offset " + std::to_string(cur) + ": total=" +
                std::to_string(desc.total_size) + " name_len=" +
                std::to_string(desc.file_name_len) + " real_size=" +
                std::to_string(desc.real_file_size) + " but only " +
                std::to_string(bytes.size() - cur) +
                " bytes remain before EOF (declared count " +
                std::to_string(count) + ")");
        }

        const std::size_t name_off = cur + sizeof(PackFileDesc);
        const std::size_t name_len = desc.file_name_len;
        const std::uint8_t* name_bytes = nullptr;
        if (!range_in_file(bytes, name_off, name_len)) {
            return fail(
                "EOF arrived early at pak entry " + std::to_string(index) +
                " offset " + std::to_string(cur) + ": filename needs " +
                std::to_string(name_len) + " bytes");
        }
        name_bytes = bytes.data() + name_off;
        if (!range_in_file(bytes, name_off + name_len, 1) ||
            bytes[name_off + name_len] != 0) {
            return fail(
                "corrupt pak entry " + std::to_string(index) +
                " at offset " + std::to_string(cur) +
                ": expected NUL after the filename");
        }

        const std::size_t data_off = name_off + name_len + 1;
        if (!range_in_file(bytes, data_off, desc.real_file_size)) {
            return fail(
                "entry overruns EOF at pak entry " + std::to_string(index) +
                " offset " + std::to_string(cur) + ": payload needs " +
                std::to_string(desc.real_file_size) + " bytes");
        }
        if (cur > 0xffffffffu || data_off > 0xffffffffu) {
            return fail("corrupt pak entry: offset does not fit in uint32");
        }

        PackEntry entry;
        entry.name.assign(reinterpret_cast<const char*>(name_bytes), name_len);
        entry.entry_offset = static_cast<std::uint32_t>(cur);
        entry.data_offset = static_cast<std::uint32_t>(data_off);
        entry.size = desc.real_file_size;
        parsed.push_back(std::move(entry));
        cur += desc.total_size;
    }

    if (cur != bytes.size()) {
        return fail(std::to_string(bytes.size() - cur) +
                    " byte(s) remain after the declared count " +
                    std::to_string(count) + " at offset " +
                    std::to_string(cur) + " (file size " +
                    std::to_string(bytes.size()) + ")");
    }

    auto pack = std::unique_ptr<PackFile>(new PackFile());
    std::memcpy(&pack->header_, bytes.data(), sizeof(PackFileHeader));
    pack->raw_ = std::move(bytes);
    pack->entries_ = std::move(parsed);
    for (auto& entry : pack->entries_) {
        if (!range_in_file(pack->raw_, entry.data_offset, entry.size)) {
            return fail("entry overruns EOF while publishing the payload view");
        }
        entry.raw_view = {pack->raw_.data() + entry.data_offset, entry.size};
    }
    return pack;
}

std::unique_ptr<PackFile> PackFile::open_buffer(std::vector<std::uint8_t> bytes,
                                                std::string* error) {
    if (error) error->clear();

    // Same rule as unpack_pak.py: a zero at offset 4 selects the 96-byte
    // header. Non-zero falls through to the 92-byte loop below, unchanged.
    std::uint32_t version = 0;
    std::uint32_t file_item_num = 0;
    if (read_u32(bytes, 0, version) && read_u32(bytes, 4, file_item_num) &&
        version == 0x00000001 && file_item_num == 0) {
        return open_original_96(std::move(bytes), error);
    }

    if (!is_pak(bytes)) {
        if (error) *error = "not a pak";
        return nullptr;
    }

    auto pack = std::unique_ptr<PackFile>(new PackFile());
    pack->raw_ = std::move(bytes);

    std::memcpy(&pack->header_, pack->raw_.data(), sizeof(PackFileHeader));

    // Walk entries sequentially starting at offset sizeof(PACK_FILE_HEADER) = 92.
    constexpr std::size_t kHeaderEnd = sizeof(PackFileHeader);
    std::size_t cursor = kHeaderEnd;
    pack->entries_.reserve(pack->header_.file_item_num);

    std::fprintf(stderr, "[PakFile] version=%u file_item_num=%u\n",
                 pack->header_.version, pack->header_.file_item_num);
    int debug_printed = 0;
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

        if (debug_printed < 5) {
            std::fprintf(stderr, "  entry %u: name=%s size=%u\n",
                         i, name.c_str(), desc.real_file_size);
            debug_printed++;
        }

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

        // Advance to next entry: prefer the parsed layout (header + name +
        // 1 NUL + data) over total_size, because some real files have
        // corrupt total_size that walks the cursor off the end of the
        // file. Both formulas agree on the entry size for well-formed
        // builds; the layout-based one is what the legacy reader uses.
        std::size_t layout_advance = static_cast<std::size_t>(desc.real_file_size);
        std::size_t total_advance = static_cast<std::size_t>(desc.total_size)
                                  - sizeof(PackFileDesc)
                                  - desc.file_name_len
                                  - 1;
        std::size_t advance = total_advance < layout_advance * 8
                                ? total_advance
                                : layout_advance;
        if (cursor + advance > pack->raw_.size()) break;
        cursor += advance;
    }

    std::fprintf(stderr, "[PakFile] parsed %zu entries, raw_size=%zu\n",
                 pack->entries_.size(), pack->raw_.size());
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
