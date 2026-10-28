// PackFile.cpp - Implementation of .pak format reader.
//
// Original source: 墨香【源码】\4DyuchiFileStorage\PackFile.cpp + typedef.h
//
// Layout (verified against real .pak files and docs/formats/pak.md):
//   Both walks run on every open. Exactly one that ends at EOF is used.
//   Neither or both: the pak is not loaded.
//   - PlayDH 92: count at offset 4, entries at 92, advance by layout.
//     A count of 0 does not validate.
//   - Original 96: count at offset 92, entries at 96, next entry at
//     cur + total. total must equal 32+name_len+1+real and a NUL must
//     follow the name. A count of 0 does not validate.
//   - When offset 4 is 0 and neither walk validates, the original-96
//     parser still runs so its detailed errors stay.
//   - Per entry: 32-byte FSFILE_HEADER + nameLen bytes + 1 NUL + realSize bytes data
//
// open(path) and probe_pak_file() seek to those headers and filenames.
// Entry payloads are read later, by read() / read_at(), and are not kept
// for the lifetime of the PackFile. open_buffer() still owns the caller's
// bytes because that is the only copy.

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include "mxh/compat/pack_file.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

constexpr const char* kAmbiguousHeader = "ambiguous/unrecognized header";
constexpr std::uint32_t kCurrentVersion = 0x00000001;
constexpr std::uint32_t kMaxEntryCount = 1'000'000u;
constexpr std::size_t kMinEntry = 33;

class ByteSource {
public:
    virtual ~ByteSource() = default;
    [[nodiscard]] virtual std::uint64_t size() const noexcept = 0;
    [[nodiscard]] virtual bool read(std::uint64_t offset, void* dst, std::size_t n) const = 0;
};

class MemorySource final : public ByteSource {
public:
    explicit MemorySource(std::span<const std::uint8_t> bytes) noexcept : bytes_(bytes) {}

    [[nodiscard]] std::uint64_t size() const noexcept override { return bytes_.size(); }

    [[nodiscard]] bool read(std::uint64_t offset, void* dst, std::size_t n) const override {
        if (n == 0) return true;
        if (offset > bytes_.size() || n > bytes_.size() - static_cast<std::size_t>(offset)) {
            return false;
        }
        std::memcpy(dst, bytes_.data() + static_cast<std::size_t>(offset), n);
        return true;
    }

private:
    std::span<const std::uint8_t> bytes_;
};

struct PlayWalk {
    std::uint64_t cursor = sizeof(mxh::compat::PackFileHeader);
    bool io = false;
    std::vector<mxh::compat::PackEntry> entries;
};

// One implementation of the PlayDH break / advance rules.
// `collect` reads filenames and records entries. The probe passes false
// and only moves `cursor`, so payload bytes are never read.
PlayWalk walk_playdh(const ByteSource& src, std::uint32_t count, bool collect) {
    PlayWalk walk;
    walk.cursor = sizeof(mxh::compat::PackFileHeader);
    int debug_printed = 0;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (walk.cursor + sizeof(mxh::compat::PackFileDesc) > src.size()) break;

        mxh::compat::PackFileDesc desc{};
        if (!src.read(walk.cursor, &desc, sizeof(desc))) {
            walk.io = true;
            break;
        }
        const auto entry_off = walk.cursor;
        walk.cursor += sizeof(mxh::compat::PackFileDesc);

        if (desc.total_size < sizeof(mxh::compat::PackFileDesc) + 1) break;
        if (desc.file_name_len > 4096) break;
        if (static_cast<std::uint64_t>(desc.real_file_size) > src.size()) break;

        if (walk.cursor + desc.file_name_len > src.size()) break;
        std::string name;
        if (collect && desc.file_name_len > 0) {
            name.resize(desc.file_name_len);
            if (!src.read(walk.cursor, name.data(), name.size())) {
                walk.io = true;
                break;
            }
        }
        walk.cursor += desc.file_name_len;

        if (walk.cursor + 1 > src.size()) break;
        walk.cursor += 1;

        if (collect) {
            if (debug_printed < 5) {
                std::fprintf(stderr, "  entry %u: name=%s size=%u\n",
                             i, name.c_str(), desc.real_file_size);
                ++debug_printed;
            }
            if (entry_off > 0xffffffffu || walk.cursor > 0xffffffffu) break;
            mxh::compat::PackEntry entry;
            entry.name = std::move(name);
            entry.entry_offset = static_cast<std::uint32_t>(entry_off);
            entry.data_offset = static_cast<std::uint32_t>(walk.cursor);
            entry.size = desc.real_file_size;
            walk.entries.push_back(std::move(entry));
        }

        const std::size_t layout_advance = static_cast<std::size_t>(desc.real_file_size);
        const std::size_t total_advance = static_cast<std::size_t>(desc.total_size)
                                         - sizeof(mxh::compat::PackFileDesc)
                                         - desc.file_name_len
                                         - 1;
        const std::size_t advance = total_advance < layout_advance * 8
                                       ? total_advance
                                       : layout_advance;
        if (static_cast<std::uint64_t>(advance) > src.size() ||
            walk.cursor > src.size() - static_cast<std::uint64_t>(advance)) {
            break;
        }
        walk.cursor += advance;
    }
    return walk;
}

bool playdh_ends_at_eof(const PlayWalk& walk, std::uint32_t count, std::uint64_t file_size) {
    if (walk.io) return false;
    if (count == 0 || count > kMaxEntryCount) return false;
    return walk.cursor == file_size;
}

// `error` and `entries` may be null. A null error keeps the probe path
// allocation-free. Payloads are not read; the original layout checks the
// one NUL byte after the name.
bool walk_original(const ByteSource& src, std::uint32_t count, bool collect,
                   std::string* error, bool* io, std::vector<mxh::compat::PackEntry>* entries) {
    // Build the text only when the caller asked for it. probe_pak_layouts()
    // is noexcept and passes a null error.
    auto fail = [&](auto&& make) -> bool {
        if (error != nullptr) *error = make();
        return false;
    };
    if (io) *io = false;

    if (src.size() < 96) {
        return fail([&] {
            return "truncated pak header: original-style pak (dwFileItemNum at "
                   "offset 4 is 0) needs 96 bytes with the entry count at offset "
                   "92, file size is " +
                   std::to_string(src.size());
        });
    }
    if (count == 0) {
        return fail([] { return std::string("corrupt pak header: entry count at offset 92 is 0"); });
    }
    if (count > kMaxEntryCount) {
        return fail([&] {
            return "entry count " + std::to_string(count) +
                   " is larger than the file allows";
        });
    }

    const auto body = src.size() - 96;
    if (body / kMinEntry < count) {
        return fail([&] {
            return "entry count " + std::to_string(count) +
                   " is larger than the file allows (" +
                   std::to_string(body) +
                   " bytes remain after the 96-byte header)";
        });
    }

    if (entries) {
        entries->clear();
        entries->reserve(count);
    }
    std::uint64_t cur = 96;
    for (std::uint32_t index = 0; index < count; ++index) {
        if (cur > src.size() || src.size() - cur < sizeof(mxh::compat::PackFileDesc)) {
            return fail([&] {
                return "EOF arrived early at pak entry " + std::to_string(index) +
                       " offset " + std::to_string(cur) +
                       ": entry header is 32 bytes but the file ends first "
                       "(declared count " + std::to_string(count) + ")";
            });
        }

        mxh::compat::PackFileDesc desc{};
        if (!src.read(cur, &desc, sizeof(desc))) {
            if (io) *io = true;
            if (error) *error = "failed to read pak";
            return false;
        }

        const std::uint64_t expected =
            32ull + static_cast<std::uint64_t>(desc.file_name_len) + 1ull +
            static_cast<std::uint64_t>(desc.real_file_size);
        if (expected != desc.total_size || desc.total_size < kMinEntry) {
            return fail([&] {
                return "corrupt pak entry " + std::to_string(index) +
                       " at offset " + std::to_string(cur) +
                       ": total does not equal 32+name_len+1+real";
            });
        }
        if (expected > src.size() - cur) {
            return fail([&] {
                return "entry overruns EOF at pak entry " + std::to_string(index) +
                       " offset " + std::to_string(cur) + ": total=" +
                       std::to_string(desc.total_size) + " name_len=" +
                       std::to_string(desc.file_name_len) + " real_size=" +
                       std::to_string(desc.real_file_size) + " but only " +
                       std::to_string(src.size() - cur) +
                       " bytes remain before EOF (declared count " +
                       std::to_string(count) + ")";
            });
        }

        const auto name_off = cur + sizeof(mxh::compat::PackFileDesc);
        const auto name_len = static_cast<std::size_t>(desc.file_name_len);
        if (name_len > src.size() - name_off) {
            return fail([&] {
                return "EOF arrived early at pak entry " + std::to_string(index) +
                       " offset " + std::to_string(cur) + ": filename needs " +
                       std::to_string(name_len) + " bytes";
            });
        }
        std::string name;
        if (collect && name_len > 0) {
            name.resize(name_len);
            if (!src.read(name_off, name.data(), name.size())) {
                if (io) *io = true;
                if (error) *error = "failed to read pak";
                return false;
            }
        }
        if (name_off > src.size() || name_len + 1 > src.size() - name_off) {
            return fail([&] {
                return "corrupt pak entry " + std::to_string(index) +
                       " at offset " + std::to_string(cur) +
                       ": expected NUL after the filename";
            });
        }
        std::uint8_t nul = 0xff;
        if (!src.read(name_off + name_len, &nul, 1)) {
            if (io) *io = true;
            if (error) *error = "failed to read pak";
            return false;
        }
        if (nul != 0) {
            return fail([&] {
                return "corrupt pak entry " + std::to_string(index) +
                       " at offset " + std::to_string(cur) +
                       ": expected NUL after the filename";
            });
        }

        const auto data_off = name_off + name_len + 1;
        if (static_cast<std::uint64_t>(desc.real_file_size) > src.size() - data_off) {
            return fail([&] {
                return "entry overruns EOF at pak entry " + std::to_string(index) +
                       " offset " + std::to_string(cur) + ": payload needs " +
                       std::to_string(desc.real_file_size) + " bytes";
            });
        }
        if (cur > 0xffffffffu || data_off > 0xffffffffu) {
            return fail([] { return std::string("corrupt pak entry: offset does not fit in uint32"); });
        }

        if (collect && entries) {
            mxh::compat::PackEntry entry;
            entry.name = std::move(name);
            entry.entry_offset = static_cast<std::uint32_t>(cur);
            entry.data_offset = static_cast<std::uint32_t>(data_off);
            entry.size = desc.real_file_size;
            entries->push_back(std::move(entry));
        }
        cur += desc.total_size;
    }

    if (cur != src.size()) {
        return fail([&] {
            return std::to_string(src.size() - cur) +
                   " byte(s) remain after the declared count " +
                   std::to_string(count) + " at offset " +
                   std::to_string(cur) + " (file size " +
                   std::to_string(src.size()) + ")";
        });
    }
    return true;
}

struct Decision {
    bool io = false;
    bool playdh92 = false;
    bool original96 = false;
    bool original_ran = false;
    std::uint32_t version = 0;
    std::uint32_t count4 = 0;
    std::uint32_t count92 = 0;
    std::string original_error;
    std::uint8_t prefix[96]{};
    std::size_t prefix_n = 0;
};

Decision decide(const ByteSource& src, bool describe_errors) {
    Decision decision;
    decision.prefix_n = static_cast<std::size_t>(
        std::min<std::uint64_t>(src.size(), sizeof(decision.prefix)));
    if (decision.prefix_n > 0 &&
        !src.read(0, decision.prefix, decision.prefix_n)) {
        decision.io = true;
        return decision;
    }
    if (decision.prefix_n >= 4) {
        std::memcpy(&decision.version, decision.prefix, sizeof(decision.version));
    }
    if (decision.prefix_n >= 8) {
        std::memcpy(&decision.count4, decision.prefix + 4, sizeof(decision.count4));
    }
    if (decision.prefix_n >= 96) {
        std::memcpy(&decision.count92, decision.prefix + 92, sizeof(decision.count92));
    }
    if (decision.version != kCurrentVersion) return decision;

    if (decision.count4 != 0 && decision.count4 <= kMaxEntryCount) {
        const auto play = walk_playdh(src, decision.count4, false);
        if (play.io) {
            decision.io = true;
            return decision;
        }
        decision.playdh92 = playdh_ends_at_eof(play, decision.count4, src.size());
    }
    if (src.size() >= 96) {
        bool io = false;
        std::string* error = describe_errors ? &decision.original_error : nullptr;
        decision.original96 = walk_original(src, decision.count92, false, error, &io, nullptr);
        decision.original_ran = true;
        if (io) {
            decision.io = true;
            decision.original96 = false;
        }
    }
    return decision;
}

struct Built {
    bool ok = false;
    std::string error;
    mxh::compat::PackFileHeader header{};
    std::vector<mxh::compat::PackEntry> entries;
    bool report_parsed_count = false;
    std::uint64_t size = 0;
};

Built build_index(const ByteSource& src) {
    Built built;
    built.size = src.size();
    const auto decision = decide(src, true);
    if (decision.io) {
        built.error = "failed to read pak";
        return built;
    }
    if (decision.playdh92 && decision.original96) {
        built.error = kAmbiguousHeader;
        return built;
    }

    auto take_header = [&]() -> bool {
        if (decision.prefix_n < sizeof(built.header)) return false;
        std::memcpy(&built.header, decision.prefix, sizeof(built.header));
        return true;
    };

    if (decision.original96) {
        bool io = false;
        std::string error;
        if (!walk_original(src, decision.count92, true, &error, &io, &built.entries) || io) {
            built.entries.clear();
            built.error = io ? "failed to read pak" : std::move(error);
            return built;
        }
        if (!take_header()) {
            built.error = "failed to read pak";
            return built;
        }
        built.report_parsed_count = true;
        built.ok = true;
        return built;
    }
    if (decision.playdh92) {
        if (!take_header()) {
            built.error = "failed to read pak";
            return built;
        }
        std::fprintf(stderr, "[PakFile] version=%u file_item_num=%u\n",
                     built.header.version, built.header.file_item_num);
        auto collected = walk_playdh(src, decision.count4, true);
        if (collected.io) {
            built.error = "failed to read pak";
            return built;
        }
        std::fprintf(stderr, "[PakFile] parsed %zu entries, raw_size=%zu\n",
                     collected.entries.size(), static_cast<std::size_t>(built.size));
        built.entries = std::move(collected.entries);
        built.ok = true;
        return built;
    }

    if (decision.version == kCurrentVersion && decision.count4 == 0) {
        if (decision.original_ran) {
            built.error = decision.original_error.empty()
                              ? std::string(kAmbiguousHeader)
                              : decision.original_error;
        } else {
            std::string error;
            bool io = false;
            walk_original(src, decision.count92, false, &error, &io, nullptr);
            if (io) {
                built.error = "failed to read pak";
            } else {
                built.error = error.empty() ? std::string(kAmbiguousHeader) : std::move(error);
            }
        }
        return built;
    }

    if (!mxh::compat::PackFile::is_pak(
            std::span<const std::uint8_t>(decision.prefix, decision.prefix_n))) {
        built.error = "not a pak";
        return built;
    }
    built.error = kAmbiguousHeader;
    return built;
}

}  // namespace

namespace mxh::compat {

struct PakFileBytes {
#ifdef _WIN32
    HANDLE handle = INVALID_HANDLE_VALUE;
    mutable std::mutex mu;
#else
    int fd = -1;
#endif
    std::uint64_t file_size = 0;

    PakFileBytes() = default;
    PakFileBytes(const PakFileBytes&) = delete;
    PakFileBytes& operator=(const PakFileBytes&) = delete;

    ~PakFileBytes() {
#ifdef _WIN32
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
#else
        if (fd >= 0) ::close(fd);
#endif
    }

    // Windows: one shared handle, seek+read under mu so callers do not
    // observe a shared file pointer. POSIX: pread, which is positional.
    [[nodiscard]] bool read(std::uint64_t offset, void* dst, std::size_t length) const {
        if (length == 0) return true;
        if (dst == nullptr) return false;
        if (offset > file_size || length > file_size - offset) return false;
        auto* out = static_cast<std::uint8_t*>(dst);
        std::size_t got = 0;
#ifdef _WIN32
        std::lock_guard<std::mutex> lock(mu);
        while (got < length) {
            LARGE_INTEGER li;
            li.QuadPart = static_cast<LONGLONG>(offset + got);
            if (!SetFilePointerEx(handle, li, nullptr, FILE_BEGIN)) return false;
            const auto chunk = static_cast<DWORD>(
                std::min<std::size_t>(length - got, 1u << 20));
            DWORD n = 0;
            if (!ReadFile(handle, out + got, chunk, &n, nullptr) || n == 0) return false;
            got += n;
        }
#else
        while (got < length) {
            const auto r = ::pread(fd, out + got, length - got,
                                   static_cast<off_t>(offset + got));
            if (r < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            if (r == 0) return false;
            got += static_cast<std::size_t>(r);
        }
#endif
        return true;
    }
};

class FileByteSource final : public ByteSource {
public:
    explicit FileByteSource(const PakFileBytes& file) : file_(file) {}
    [[nodiscard]] std::uint64_t size() const noexcept override { return file_.file_size; }
    [[nodiscard]] bool read(std::uint64_t offset, void* dst, std::size_t n) const override {
        return file_.read(offset, dst, n);
    }

private:
    const PakFileBytes& file_;
};

namespace {

enum class OpenKind { FailedToOpen, FailedToRead, Ok };

struct OpenedFile {
    OpenKind kind = OpenKind::FailedToOpen;
    std::unique_ptr<PakFileBytes> file;
    std::string os_error;
};

OpenedFile open_file(const std::filesystem::path& path) {
    OpenedFile opened;
    auto file = std::unique_ptr<PakFileBytes>(new PakFileBytes());
#ifdef _WIN32
    file->handle = CreateFileW(path.c_str(), GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file->handle == INVALID_HANDLE_VALUE) {
        opened.os_error = std::system_category().message(static_cast<int>(GetLastError()));
        return opened;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file->handle, &size) || size.QuadPart < 0) {
        opened.kind = OpenKind::FailedToRead;
        opened.os_error = std::system_category().message(static_cast<int>(GetLastError()));
        return opened;
    }
    file->file_size = static_cast<std::uint64_t>(size.QuadPart);
#else
    file->fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (file->fd < 0) {
        opened.os_error = std::error_code(errno, std::generic_category()).message();
        return opened;
    }
    struct stat st {};
    if (::fstat(file->fd, &st) != 0 || st.st_size < 0) {
        opened.kind = OpenKind::FailedToRead;
        opened.os_error = std::error_code(errno, std::generic_category()).message();
        return opened;
    }
    file->file_size = static_cast<std::uint64_t>(st.st_size);
#endif
    opened.kind = OpenKind::Ok;
    opened.file = std::move(file);
    return opened;
}

}  // namespace

PackFile::~PackFile() = default;

PackFile::PackFile(PackFile&& other) noexcept
    : header_(other.header_),
      entries_(std::move(other.entries_)),
      buffer_(std::move(other.buffer_)),
      file_(std::move(other.file_)),
      source_size_(other.source_size_),
      from_buffer_(other.from_buffer_),
      report_parsed_count_(other.report_parsed_count_) {
    other.header_ = {};
    other.source_size_ = 0;
    other.from_buffer_ = false;
    other.report_parsed_count_ = false;
}

PackFile& PackFile::operator=(PackFile&& other) noexcept {
    if (this != &other) {
        header_ = other.header_;
        entries_ = std::move(other.entries_);
        buffer_ = std::move(other.buffer_);
        file_ = std::move(other.file_);
        source_size_ = other.source_size_;
        from_buffer_ = other.from_buffer_;
        report_parsed_count_ = other.report_parsed_count_;
        other.header_ = {};
        other.source_size_ = 0;
        other.from_buffer_ = false;
        other.report_parsed_count_ = false;
    }
    return *this;
}

PakFileProbe probe_pak_file(const std::filesystem::path& path) {
    PakFileProbe probe;
    const auto opened = open_file(path);
    if (opened.kind != OpenKind::Ok || !opened.file) {
        probe.opened = false;
        probe.read_error = opened.kind == OpenKind::FailedToRead;
        probe.error = opened.os_error;
        return probe;
    }
    probe.opened = true;
    probe.file_size = opened.file->file_size;
    FileByteSource src(*opened.file);
    const auto decision = decide(src, false);
    if (decision.io) {
        probe.read_error = true;
        probe.error = "failed to read pak";
        return probe;
    }
    probe.count_at_offset_4 = decision.count4;
    probe.count_at_offset_92 = decision.count92;
    probe.has_count_at_offset_92 = decision.prefix_n >= 96;
    probe.layout.playdh92_exact_eof = decision.playdh92;
    probe.layout.original96_exact_eof = decision.original96;
    return probe;
}

PakLayoutProbe probe_pak_layouts(std::span<const std::uint8_t> bytes) noexcept {
    PakLayoutProbe probe;
    if (bytes.size() < 8) return probe;
    MemorySource src(bytes);
    const auto decision = decide(src, false);
    if (decision.io) return probe;
    probe.playdh92_exact_eof = decision.playdh92;
    probe.original96_exact_eof = decision.original96;
    return probe;
}

bool PackFile::is_pak(std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < 8) return false;
    std::uint32_t version = 0;
    std::uint32_t file_item_num = 0;
    std::memcpy(&version, bytes.data(), sizeof(version));
    std::memcpy(&file_item_num, bytes.data() + 4, sizeof(file_item_num));
    if (version != kCurrentVersion) return false;
    if (file_item_num == 0) {
        if (bytes.size() < 96) return false;
        std::uint32_t count = 0;
        std::memcpy(&count, bytes.data() + 92, sizeof(count));
        return count > 0 && count <= kMaxEntryCount;
    }
    if (bytes.size() < sizeof(PackFileHeader)) return false;
    return file_item_num <= kMaxEntryCount;
}

std::unique_ptr<PackFile> PackFile::open(const std::filesystem::path& path,
                                         std::string* error) {
    std::string local_error;
    std::string* const reported = error != nullptr ? error : &local_error;
    reported->clear();
    auto opened = open_file(path);
    if (opened.kind == OpenKind::FailedToOpen || !opened.file) {
        *reported = "failed to open pak";
        return nullptr;
    }
    if (opened.kind == OpenKind::FailedToRead) {
        *reported = "failed to read pak";
        return nullptr;
    }
    FileByteSource src(*opened.file);
    auto built = build_index(src);
    if (!built.ok) {
        *reported = built.error.empty() ? std::string("failed to read pak") : std::move(built.error);
        if (*reported == kAmbiguousHeader) {
            const auto u8 = path.filename().u8string();
            const std::string name(reinterpret_cast<const char*>(u8.data()), u8.size());
            std::fprintf(stderr, "ERROR [pak] ambiguous/unrecognized header: %s\n",
                         name.c_str());
        }
        return nullptr;
    }
    auto pack = std::unique_ptr<PackFile>(new PackFile());
    pack->header_ = built.header;
    pack->entries_ = std::move(built.entries);
    pack->report_parsed_count_ = built.report_parsed_count;
    pack->source_size_ = built.size;
    pack->file_ = std::move(opened.file);
    pack->from_buffer_ = false;
    return pack;
}

std::unique_ptr<PackFile> PackFile::open_buffer(std::vector<std::uint8_t> bytes,
                                                std::string* error) {
    if (error) error->clear();
    MemorySource src(bytes);
    auto built = build_index(src);
    if (!built.ok) {
        if (error) *error = built.error.empty() ? std::string("failed to read pak") : std::move(built.error);
        return nullptr;
    }
    auto pack = std::unique_ptr<PackFile>(new PackFile());
    pack->header_ = built.header;
    pack->entries_ = std::move(built.entries);
    pack->report_parsed_count_ = built.report_parsed_count;
    pack->buffer_ = std::move(bytes);
    pack->from_buffer_ = true;
    pack->source_size_ = pack->buffer_.size();
    for (auto& entry : pack->entries_) {
        const auto end = static_cast<std::uint64_t>(entry.data_offset) + entry.size;
        if (entry.size == 0 || end < entry.data_offset || end > pack->buffer_.size()) continue;
        entry.raw_view = {pack->buffer_.data() + entry.data_offset, entry.size};
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

bool PackFile::has_payload(const PackEntry& entry) const noexcept {
    if (entry.size == 0) return false;
    const auto end = static_cast<std::uint64_t>(entry.data_offset) + entry.size;
    return end >= entry.data_offset && end <= source_size_;
}

bool PackFile::read_at(std::uint64_t offset, void* dst, std::size_t length) const {
    if (length == 0) return true;
    if (dst == nullptr) return false;
    if (offset > source_size_ || length > source_size_ - offset) return false;
    if (from_buffer_) {
        if (offset > buffer_.size() || length > buffer_.size() - static_cast<std::size_t>(offset)) {
            return false;
        }
        std::memcpy(dst, buffer_.data() + static_cast<std::size_t>(offset), length);
        return true;
    }
    if (!file_) return false;
    return file_->read(offset, dst, length);
}

std::vector<std::uint8_t> PackFile::read(std::string_view name) const {
    std::vector<std::uint8_t> out;
    const PackEntry* entry = find(name);
    if (entry == nullptr || !has_payload(*entry)) return out;
    out.resize(entry->size);
    if (!read_at(entry->data_offset, out.data(), out.size())) out.clear();
    return out;
}

std::vector<std::string> PackFile::list_names() const {
    std::vector<std::string> out;
    out.reserve(entries_.size());
    for (const auto& e : entries_) out.push_back(e.name);
    return out;
}

}  // namespace mxh::compat
