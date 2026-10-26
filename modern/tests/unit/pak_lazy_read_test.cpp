// Index-only pak open. Synthetic archives only; no original game bytes.
//
// File-backed PackFile::open / classify_pak_header seek entry headers.
// Payloads are read when a caller asks for those bytes.

#include "mxh/render/FilesystemFileStorage.hpp"
#ifdef interface
#undef interface
#endif
#ifdef __stdcall
#undef __stdcall
#endif

#include "mxh/compat/pack_file.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#include <io.h>
#else
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

using mxh::compat::PackFile;

struct PeakSample {
    bool ok = false;
    std::uint64_t bytes = 0;
    const char* how = "no peak RSS API on this platform";
};

PeakSample peak_rss() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
        return {false, 0, "GetProcessMemoryInfo failed"};
    }
    return {true, static_cast<std::uint64_t>(counters.PeakWorkingSetSize),
            "GetProcessMemoryInfo PeakWorkingSetSize"};
#elif defined(__linux__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        std::ifstream status("/proc/self/status");
        std::string key;
        unsigned long long kib = 0;
        while (status >> key) {
            if (key == "VmHWM:") {
                status >> kib;
                return {true, kib * 1024ull, "/proc/self/status VmHWM"};
            }
            status.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        return {false, 0, "getrusage failed and VmHWM was not readable"};
    }
    // Linux reports ru_maxrss in kilobytes.
    return {true, static_cast<std::uint64_t>(usage.ru_maxrss) * 1024ull, "getrusage ru_maxrss"};
#else
    return {};
#endif
}

std::uint64_t linux_rchar() {
#if defined(__linux__)
    std::ifstream io("/proc/self/io");
    std::string key;
    unsigned long long value = 0;
    while (io >> key >> value) {
        if (key == "rchar:") return value;
    }
#endif
    return 0;
}

bool linux_has_rchar() {
#if defined(__linux__)
    std::ifstream io("/proc/self/io");
    return static_cast<bool>(io);
#else
    return false;
#endif
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xffu));
}

void poke_u32(std::vector<std::uint8_t>& out, std::size_t offset, std::uint32_t value) {
    out.at(offset) = static_cast<std::uint8_t>(value & 0xffu);
    out.at(offset + 1) = static_cast<std::uint8_t>((value >> 8) & 0xffu);
    out.at(offset + 2) = static_cast<std::uint8_t>((value >> 16) & 0xffu);
    out.at(offset + 3) = static_cast<std::uint8_t>((value >> 24) & 0xffu);
}

std::vector<std::uint8_t> make_worktree_pak(
    const std::vector<std::pair<std::string, std::vector<std::uint8_t>>>& entries,
    std::uint32_t header_count) {
    std::vector<std::uint8_t> out(92, 0);
    poke_u32(out, 0, 1);
    poke_u32(out, 4, header_count);
    for (const auto& [name, data] : entries) {
        const auto name_len = static_cast<std::uint32_t>(name.size());
        const auto real = static_cast<std::uint32_t>(data.size());
        const std::uint32_t total = 32u + name_len + 1u + real;
        append_u32(out, total);
        append_u32(out, real);
        append_u32(out, name_len);
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        out.insert(out.end(), name.begin(), name.end());
        out.push_back(0);
        out.insert(out.end(), data.begin(), data.end());
    }
    return out;
}

std::vector<std::uint8_t> make_original_pak(
    const std::vector<std::pair<std::string, std::vector<std::uint8_t>>>& entries,
    std::uint32_t count) {
    std::vector<std::uint8_t> out(96, 0);
    poke_u32(out, 0, 1);
    poke_u32(out, 4, 0);
    poke_u32(out, 92, count);
    std::size_t cursor = 96;
    for (const auto& [name, data] : entries) {
        const auto name_len = static_cast<std::uint32_t>(name.size());
        const auto real = static_cast<std::uint32_t>(data.size());
        const std::uint32_t total = 32u + name_len + 1u + real;
        append_u32(out, total);
        append_u32(out, real);
        append_u32(out, name_len);
        append_u32(out, static_cast<std::uint32_t>(cursor));
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        append_u32(out, 0);
        out.insert(out.end(), name.begin(), name.end());
        out.push_back(0);
        out.insert(out.end(), data.begin(), data.end());
        cursor += total;
    }
    return out;
}

class TempDir {
public:
    explicit TempDir(const char* tag) {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                (std::string(tag) + std::to_string(static_cast<long long>(stamp)));
        std::filesystem::create_directories(path_);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }
    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

void write_bytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary);
    ASSERT_TRUE(out) << path;
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(out);
}

// Logical size is `file_size`. Only `prefix` is written; the tail is a hole.
void write_sparse(const std::filesystem::path& path, const std::vector<std::uint8_t>& prefix,
                  std::uint64_t file_size) {
    ASSERT_LT(prefix.size(), file_size);
#if defined(_WIN32)
    const int fd = _open(path.string().c_str(), _O_CREAT | _O_TRUNC | _O_RDWR | _O_BINARY, 0644);
    ASSERT_GE(fd, 0) << path;
    ASSERT_EQ(_write(fd, prefix.data(), static_cast<unsigned>(prefix.size())),
              static_cast<int>(prefix.size()));
    ASSERT_EQ(_chsize_s(fd, static_cast<__int64>(file_size)), 0);
    _close(fd);
#else
    const int fd = ::open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR | O_CLOEXEC, 0644);
    ASSERT_GE(fd, 0) << path;
    ASSERT_EQ(::write(fd, prefix.data(), prefix.size()), static_cast<ssize_t>(prefix.size()));
    ASSERT_EQ(::ftruncate(fd, static_cast<off_t>(file_size)), 0);
    struct stat st {};
    ASSERT_EQ(::fstat(fd, &st), 0);
    ::close(fd);
    const auto allocated = static_cast<std::uint64_t>(st.st_blocks) * 512ull;
    ASSERT_LT(allocated, 16ull * 1024ull * 1024ull)
        << "sparse pak consumed " << allocated << " bytes on disk";
#endif
}

// Original layout, two entries. The second payload is a hole of `huge` bytes.
struct SparseOriginal {
    std::vector<std::uint8_t> prefix;
    std::uint64_t file_size = 0;
    std::string small_name;
    std::vector<std::uint8_t> small_payload;
    std::string big_name;
    std::uint32_t big_size = 0;
};

SparseOriginal make_sparse_original(std::string small_name, std::vector<std::uint8_t> small_payload,
                                    std::string big_name, std::uint32_t huge) {
    SparseOriginal out;
    out.small_name = std::move(small_name);
    out.small_payload = std::move(small_payload);
    out.big_name = std::move(big_name);
    out.big_size = huge;
    out.prefix.assign(96, 0);
    poke_u32(out.prefix, 0, 1);
    poke_u32(out.prefix, 4, 0);
    poke_u32(out.prefix, 92, 2);
    const auto small_len = static_cast<std::uint32_t>(out.small_name.size());
    const auto small_real = static_cast<std::uint32_t>(out.small_payload.size());
    const std::uint32_t small_total = 32u + small_len + 1u + small_real;
    append_u32(out.prefix, small_total);
    append_u32(out.prefix, small_real);
    append_u32(out.prefix, small_len);
    append_u32(out.prefix, 96);
    append_u32(out.prefix, 0);
    append_u32(out.prefix, 0);
    append_u32(out.prefix, 0);
    append_u32(out.prefix, 0);
    out.prefix.insert(out.prefix.end(), out.small_name.begin(), out.small_name.end());
    out.prefix.push_back(0);
    out.prefix.insert(out.prefix.end(), out.small_payload.begin(), out.small_payload.end());
    const auto big_at = static_cast<std::uint32_t>(out.prefix.size());
    const auto big_len = static_cast<std::uint32_t>(out.big_name.size());
    const std::uint32_t big_total = 32u + big_len + 1u + huge;
    append_u32(out.prefix, big_total);
    append_u32(out.prefix, huge);
    append_u32(out.prefix, big_len);
    append_u32(out.prefix, big_at);
    append_u32(out.prefix, 0);
    append_u32(out.prefix, 0);
    append_u32(out.prefix, 0);
    append_u32(out.prefix, 0);
    out.prefix.insert(out.prefix.end(), out.big_name.begin(), out.big_name.end());
    out.prefix.push_back(0);
    out.file_size = static_cast<std::uint64_t>(big_at) + big_total;
    return out;
}

std::string mount_order_line(std::string_view logged) {
    const auto at = logged.find("[pak] mount order:");
    if (at == std::string_view::npos) return {};
    const auto end = logged.find('\n', at);
    if (end == std::string_view::npos) return std::string(logged.substr(at));
    return std::string(logged.substr(at, end - at));
}

std::string read_entry(mxh::gx::FilesystemFileStorage* storage, const char* name) {
    char mutable_name[128]{};
    std::snprintf(mutable_name, sizeof(mutable_name), "%s", name);
    void* file = storage->FSOpenFile(mutable_name, mxh::gx::FSFILE_ACCESSMODE_BINARY);
    if (!file) return {};
    const auto size = storage->FSSeek(file, 0, mxh::gx::FSFILE_SEEK_END);
    storage->FSSeek(file, 0, mxh::gx::FSFILE_SEEK_SET);
    std::string bytes(size, '\0');
    if (size != 0 && storage->FSRead(file, bytes.data(), size) != size) bytes.clear();
    storage->FSCloseFile(file);
    return bytes;
}

constexpr std::uint32_t kHugePayload = 384u * 1024u * 1024u;
constexpr std::uint64_t kPeakBudget = 64ull * 1024ull * 1024ull;

}  // namespace

TEST(PackLazy, BufferAndFileReadsMatchBothLayouts) {
    const std::vector<std::uint8_t> alpha{'A', '1'};
    const std::vector<std::uint8_t> beta{'B', '2', '3'};
    const auto playdh = make_worktree_pak({{"Map\\A.bin", alpha}, {"Map\\B.bin", beta}}, 2);
    auto corrupt = playdh;
    poke_u32(corrupt, 92, 0x00FFFFF0u);
    const auto original = make_original_pak({{"Map\\A.bin", alpha}, {"water.tif", beta}}, 2);

    const TempDir dir("mxh-pak-lazy-match-");
    write_bytes(dir.path() / "play.pak", playdh);
    write_bytes(dir.path() / "corrupt.pak", corrupt);
    write_bytes(dir.path() / "orig.pak", original);

    for (const auto& bytes : {playdh, corrupt, original}) {
        const auto from_buffer = PackFile::open_buffer(bytes);
        ASSERT_NE(from_buffer, nullptr);
        const auto* first = from_buffer->find("Map\\A.bin");
        ASSERT_NE(first, nullptr);
        EXPECT_FALSE(first->raw_view.empty());
        EXPECT_EQ(std::vector<std::uint8_t>(first->raw_view.begin(), first->raw_view.end()), alpha);
    }

    std::string error;
    const auto from_disk = PackFile::open(dir.path() / "play.pak", &error);
    ASSERT_NE(from_disk, nullptr) << error;
    EXPECT_EQ(from_disk->read("Map\\A.bin"), alpha);
    EXPECT_EQ(from_disk->read("Map\\B.bin"), beta);
    const auto* disk_entry = from_disk->find("map\\b.bin");
    ASSERT_NE(disk_entry, nullptr);
    EXPECT_TRUE(disk_entry->raw_view.empty());
    EXPECT_TRUE(from_disk->has_payload(*disk_entry));
    EXPECT_EQ(from_disk->read("Map\\B.bin"),
              PackFile::open_buffer(playdh)->read("Map\\B.bin"));

    const auto corrupt_disk = PackFile::open(dir.path() / "corrupt.pak", &error);
    ASSERT_NE(corrupt_disk, nullptr) << error;
    EXPECT_EQ(corrupt_disk->parsed_count(), 2u);
    EXPECT_EQ(corrupt_disk->read("Map\\B.bin"), beta);
    EXPECT_EQ(corrupt_disk->read("Map\\B.bin"), PackFile::open_buffer(corrupt)->read("Map\\B.bin"));

    const auto orig_disk = PackFile::open(dir.path() / "orig.pak", &error);
    ASSERT_NE(orig_disk, nullptr) << error;
    EXPECT_EQ(orig_disk->file_count(), 2u);
    EXPECT_EQ(orig_disk->header().file_item_num, 0u);
    EXPECT_EQ(orig_disk->read("water.tif"), beta);
    EXPECT_EQ(orig_disk->read("water.tif"), PackFile::open_buffer(original)->read("water.tif"));
    const auto* water = orig_disk->find("water.tif");
    ASSERT_NE(water, nullptr);
    EXPECT_TRUE(water->raw_view.empty());
    std::vector<std::uint8_t> via_at(water->size);
    ASSERT_TRUE(orig_disk->read_at(water->data_offset, via_at.data(), via_at.size()));
    EXPECT_EQ(via_at, beta);
}

TEST(PackLazy, AmbiguousAndNeitherLayoutsError) {
    std::vector<std::uint8_t> both(4324, 0);
    const auto write_min = [&](std::size_t at) { poke_u32(both, at, 33); };
    poke_u32(both, 0, 1);
    poke_u32(both, 4, 2);
    write_min(96);
    write_min(129);
    poke_u32(both, 162, 2083);
    poke_u32(both, 166, 2050);
    std::size_t at = 2245;
    for (int index = 0; index < 63; ++index) {
        write_min(at);
        at += 33;
    }
    poke_u32(both, 92, 66);
    poke_u32(both, 96, 33);
    poke_u32(both, 158, 4166);
    poke_u32(both, 162, 2083);
    poke_u32(both, 166, 2050);
    const auto probe = mxh::compat::probe_pak_layouts(both);
    EXPECT_TRUE(probe.playdh92_exact_eof);
    EXPECT_TRUE(probe.original96_exact_eof);

    std::string error = "stale";
    EXPECT_EQ(PackFile::open_buffer(both, &error), nullptr);
    EXPECT_EQ(error, "ambiguous/unrecognized header");

    std::vector<std::uint8_t> neither(200, 0);
    poke_u32(neither, 0, 1);
    poke_u32(neither, 4, 3);
    poke_u32(neither, 92, 5);
    const auto neither_probe = mxh::compat::probe_pak_layouts(neither);
    EXPECT_FALSE(neither_probe.playdh92_exact_eof);
    EXPECT_FALSE(neither_probe.original96_exact_eof);
    error = "stale";
    EXPECT_EQ(PackFile::open_buffer(neither, &error), nullptr);
    EXPECT_EQ(error, "ambiguous/unrecognized header");

    const TempDir dir("mxh-pak-lazy-bad-");
    write_bytes(dir.path() / "both.pak", both);
    write_bytes(dir.path() / "neither.pak", neither);
    const auto file_probe = mxh::compat::probe_pak_file(dir.path() / "both.pak");
    EXPECT_TRUE(file_probe.opened);
    EXPECT_FALSE(file_probe.read_error);
    EXPECT_TRUE(file_probe.layout.playdh92_exact_eof);
    EXPECT_TRUE(file_probe.layout.original96_exact_eof);
    testing::internal::CaptureStderr();
    error = "stale";
    EXPECT_EQ(PackFile::open(dir.path() / "both.pak", &error), nullptr);
    const auto logged = testing::internal::GetCapturedStderr();
    EXPECT_EQ(error, "ambiguous/unrecognized header");
    EXPECT_NE(logged.find("ERROR [pak] ambiguous/unrecognized header: both.pak"), std::string::npos)
        << logged;
    error.clear();
    EXPECT_EQ(PackFile::open(dir.path() / "neither.pak", &error), nullptr);
    EXPECT_EQ(error, "ambiguous/unrecognized header");
}

TEST(PackLazy, ReadingOneEntryDoesNotReadTheOther) {
    const TempDir dir("mxh-pak-lazy-one-");
    const auto spec = make_sparse_original("small.bin", {'P', 'I', 'N', 'G'}, "big.bin", kHugePayload);
    const auto path = dir.path() / "one.pak";
    write_sparse(path, spec.prefix, spec.file_size);

    const auto before = peak_rss();
    if (!before.ok) GTEST_SKIP() << before.how;
    const auto rchar_before = linux_rchar();
    const bool count_reads = linux_has_rchar();

    std::string error;
    const auto pack = PackFile::open(path, &error);
    ASSERT_NE(pack, nullptr) << error;
    const auto* small = pack->find("small.bin");
    const auto* big = pack->find("big.bin");
    ASSERT_NE(small, nullptr);
    ASSERT_NE(big, nullptr);
    EXPECT_TRUE(small->raw_view.empty());
    EXPECT_TRUE(big->raw_view.empty());
    EXPECT_EQ(big->size, kHugePayload);
    EXPECT_EQ(pack->read("small.bin"), spec.small_payload);
    // find() folds case, so "BIG.BIN" is big.bin. A missing name must stay empty
    // without pulling the sparse hole into the process.
    EXPECT_TRUE(pack->read("missing.bin").empty());

    const auto after = peak_rss();
    ASSERT_TRUE(after.ok) << after.how;
    const auto delta = after.bytes >= before.bytes ? after.bytes - before.bytes : 0;
    EXPECT_LT(delta, kPeakBudget) << after.how << " grew by " << delta;
    if (count_reads) {
        const auto rchar_delta = linux_rchar() - rchar_before;
        EXPECT_LT(rchar_delta, 2ull * 1024ull * 1024ull)
            << "open+small read transferred " << rchar_delta
            << " bytes from a " << spec.file_size << " byte pak";
    }
}

TEST(PackLazy, ConcurrentReadsStayConsistent) {
    const auto bytes = make_original_pak(
        {{"left.bin", {1, 2, 3, 4}}, {"right.bin", {9, 8, 7}}}, 2);
    const TempDir dir("mxh-pak-lazy-threads-");
    write_bytes(dir.path() / "thr.pak", bytes);
    const auto pack = PackFile::open(dir.path() / "thr.pak");
    ASSERT_NE(pack, nullptr);
    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    for (int n = 0; n < 4; ++n) {
        threads.emplace_back([&] {
            for (int i = 0; i < 30; ++i) {
                if (pack->read("left.bin") != std::vector<std::uint8_t>({1, 2, 3, 4})) {
                    failures.fetch_add(1);
                }
                if (pack->read("right.bin") != std::vector<std::uint8_t>({9, 8, 7})) {
                    failures.fetch_add(1);
                }
            }
        });
    }
    for (auto& thread : threads) thread.join();
    EXPECT_EQ(failures.load(), 0);
}

TEST(FilesystemFileStorage, SyntheticLayoutsKeepMountOrder) {
    const TempDir dir("mxh-pak-lazy-order-");
    write_bytes(dir.path() / "z_playdh.pak",
                make_worktree_pak({{"shared.bin", {'f', 'r', 'o', 'm', '-', 'z'}}}, 1));
    write_bytes(dir.path() / "a_original.pak",
                make_original_pak({{"shared.bin", {'f', 'r', 'o', 'm', '-', 'a'}},
                                   {"only96.bin", {'9', '6'}}},
                                  2));
    write_bytes(dir.path() / "m_playdh.pak",
                make_worktree_pak({{"shared.bin", {'f', 'r', 'o', 'm', '-', 'm'}}}, 1));

    const auto m = mxh::gx::classify_pak_header(dir.path() / "m_playdh.pak");
    const auto z = mxh::gx::classify_pak_header(dir.path() / "z_playdh.pak");
    const auto a = mxh::gx::classify_pak_header(dir.path() / "a_original.pak");
    EXPECT_EQ(m.kind, mxh::gx::PakHeaderKind::PlayDh92);
    EXPECT_TRUE(m.playdh92_valid);
    EXPECT_FALSE(m.original96_valid);
    EXPECT_EQ(m.entry_count, 1u);
    EXPECT_EQ(z.kind, mxh::gx::PakHeaderKind::PlayDh92);
    EXPECT_EQ(z.entry_count, 1u);
    EXPECT_EQ(a.kind, mxh::gx::PakHeaderKind::Original96);
    EXPECT_FALSE(a.playdh92_valid);
    EXPECT_TRUE(a.original96_valid);
    EXPECT_EQ(a.entry_count, 2u);

    testing::internal::CaptureStderr();
    auto* storage = new mxh::gx::FilesystemFileStorage(dir.path());
    ASSERT_TRUE(storage->Initialize(0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE));
    const auto logged = testing::internal::GetCapturedStderr();
    EXPECT_EQ(mount_order_line(logged),
              "[pak] mount order: 1=m_playdh.pak(92) 2=z_playdh.pak(92) 3=a_original.pak(96)")
        << logged;
    EXPECT_EQ(read_entry(storage, "shared.bin"), "from-m");
    EXPECT_EQ(read_entry(storage, "only96.bin"), "96");
    storage->Release();
}

TEST(FilesystemFileStorage, ThreeLargePaksStayWithinPeakBudget) {
    const auto before = peak_rss();
    if (!before.ok) {
        GTEST_SKIP() << before.how;
    }
    const TempDir dir("mxh-pak-lazy-peak-");
    const char* names[] = {"a.pak", "b.pak", "c.pak"};
    const char payloads[] = {'A', 'B', 'C'};
    for (int i = 0; i < 3; ++i) {
        const auto spec = make_sparse_original(
            "small.bin", {static_cast<std::uint8_t>(payloads[i])}, "big.bin", kHugePayload);
        write_sparse(dir.path() / names[i], spec.prefix, spec.file_size);
        const auto header = mxh::gx::classify_pak_header(dir.path() / names[i]);
        EXPECT_EQ(header.kind, mxh::gx::PakHeaderKind::Original96) << names[i];
        EXPECT_EQ(header.entry_count, 2u) << names[i];
    }

    const auto opened = peak_rss();
    testing::internal::CaptureStderr();
    auto* storage = new mxh::gx::FilesystemFileStorage(dir.path());
    ASSERT_TRUE(storage->Initialize(0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE));
    const auto logged = testing::internal::GetCapturedStderr();
    EXPECT_EQ(mount_order_line(logged),
              "[pak] mount order: 1=a.pak(96) 2=b.pak(96) 3=c.pak(96)")
        << logged;
    EXPECT_EQ(read_entry(storage, "small.bin"), "A");
    storage->Release();

    const auto pack_b = PackFile::open(dir.path() / "b.pak");
    const auto pack_c = PackFile::open(dir.path() / "c.pak");
    ASSERT_NE(pack_b, nullptr);
    ASSERT_NE(pack_c, nullptr);
    EXPECT_EQ(pack_b->read("small.bin"), std::vector<std::uint8_t>({'B'}));
    EXPECT_EQ(pack_c->read("small.bin"), std::vector<std::uint8_t>({'C'}));
    const auto* big = pack_c->find("big.bin");
    ASSERT_NE(big, nullptr);
    EXPECT_EQ(big->size, kHugePayload);
    EXPECT_TRUE(big->raw_view.empty());

    const auto after = peak_rss();
    ASSERT_TRUE(after.ok) << after.how;
    const auto base = opened.ok ? opened.bytes : before.bytes;
    const auto delta = after.bytes >= base ? after.bytes - base : 0;
    EXPECT_LT(delta, kPeakBudget)
        << after.how << " delta " << delta
        << " after three " << kHugePayload << "-byte paks (base " << base << ")";
    (void)before;
}
