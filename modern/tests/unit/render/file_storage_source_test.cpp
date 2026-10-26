// describeSource() and FSOpenFile() must resolve through the same lookup
// (FilesystemFileStorage::locate): loose file, inside a pak, not found.
// Synthetic paks only; no original game data.
#include "mxh/render/FilesystemFileStorage.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace {
using Entries = std::vector<std::pair<std::string, std::string>>;

void put_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xffu));
}
void poke_u32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) out[at + i] = static_cast<std::uint8_t>((v >> (8 * i)) & 0xffu);
}
// original96=true: 96-byte original header (#7); false: 92-byte worktree header.
std::vector<std::uint8_t> make_pak(const Entries& entries, bool original96) {
    std::vector<std::uint8_t> out(original96 ? 96 : 92, 0);
    poke_u32(out, 0, 1);
    if (original96) poke_u32(out, 92, static_cast<std::uint32_t>(entries.size()));
    else poke_u32(out, 4, static_cast<std::uint32_t>(entries.size()));
    for (const auto& [name, data] : entries) {
        put_u32(out, static_cast<std::uint32_t>(32 + name.size() + 1 + data.size()));
        put_u32(out, static_cast<std::uint32_t>(data.size()));
        put_u32(out, static_cast<std::uint32_t>(name.size()));
        for (int i = 0; i < 5; ++i) put_u32(out, 0);
        out.insert(out.end(), name.begin(), name.end());
        out.push_back(0);
        out.insert(out.end(), data.begin(), data.end());
    }
    return out;
}
void write_file(const std::filesystem::path& p, const std::string& s) {
    std::ofstream f(p, std::ios::binary);
    f.write(s.data(), static_cast<std::streamsize>(s.size()));
}
void write_bytes(const std::filesystem::path& p, const std::vector<std::uint8_t>& b) {
    std::ofstream f(p, std::ios::binary);
    f.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
}
std::string open_read(mxh::gx::FilesystemFileStorage* fs, const char* name, bool* opened) {
    void* h = fs->FSOpenFile(const_cast<char*>(name), mxh::gx::FSFILE_ACCESSMODE_BINARY);
    *opened = h != nullptr;
    if (!h) return {};
    const auto size = fs->FSSeek(h, 0, mxh::gx::FSFILE_SEEK_END);
    fs->FSSeek(h, 0, mxh::gx::FSFILE_SEEK_SET);
    std::string out(size, '\0');
    fs->FSRead(h, out.data(), size);
    fs->FSCloseFile(h);
    return out;
}
}  // namespace

TEST(FilesystemFileStorage, DescribeSourceMatchesOpenForLoosePakAndMissing) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() /
        ("mxh-src-" + std::to_string(static_cast<long long>(stamp)));
    std::filesystem::create_directories(root);
    // Both paks carry inner.vcl with pak-specific content, so the bytes FSOpenFile
    // returns identify which pak won; describeSource must name that same pak.
    write_bytes(root / "alpha.pak", make_pak({{"inner.vcl", "from-alpha"}, {"loose.vcl", "pak-copy"}}, true));
    write_bytes(root / "beta.pak", make_pak({{"inner.vcl", "from-beta"}}, false));
    write_file(root / "loose.vcl", "loose-bytes");

    testing::internal::CaptureStderr();
    auto* fs = new mxh::gx::FilesystemFileStorage(root);
    ASSERT_TRUE(fs->Initialize(0, 0, 0, mxh::gx::FILE_ACCESS_METHOD_ONLY_FILE));
    const std::string log = testing::internal::GetCapturedStderr();
    const auto mountPos = log.find("[pak] mount order:");
    ASSERT_NE(mountPos, std::string::npos) << log;
    const std::string mountLine = log.substr(mountPos, log.find('\n', mountPos) - mountPos);

    bool opened = false;
    // 1. Loose file wins over the pak copy, in both paths.
    EXPECT_EQ(open_read(fs, "loose.vcl", &opened), "loose-bytes");
    EXPECT_TRUE(opened);
    const auto loose = fs->describeSource("loose.vcl");
    EXPECT_EQ(loose.rfind("loose:", 0), 0u) << loose;
    EXPECT_NE(loose.find("loose.vcl"), std::string::npos) << loose;

    // 2. Inside a pak: describeSource names the pak whose bytes were opened,
    // using the exact name printed in "[pak] mount order", first in that order.
    const std::string bytes = open_read(fs, "Map/inner.vcl", &opened);
    ASSERT_TRUE(opened);
    const std::string expectedPak = bytes == "from-alpha" ? "alpha.pak" : "beta.pak";
    ASSERT_TRUE(bytes == "from-alpha" || bytes == "from-beta") << bytes;
    EXPECT_EQ(fs->describeSource("Map/inner.vcl"), "pak:" + expectedPak);
    const auto first = mountLine.find("1=");
    ASSERT_NE(first, std::string::npos) << mountLine;
    EXPECT_EQ(mountLine.compare(first + 2, expectedPak.size(), expectedPak), 0) << mountLine;
    char innerName[] = "inner.vcl";
    EXPECT_TRUE(fs->IsExistInFileStorage(innerName));

    // 3. Not found: both report nothing.
    EXPECT_EQ(open_read(fs, "nope.vcl", &opened), "");
    EXPECT_FALSE(opened);
    EXPECT_EQ(fs->describeSource("nope.vcl"), "");
    char nope[] = "nope.vcl";
    EXPECT_FALSE(fs->IsExistInFileStorage(nope));

    fs->Release();
    std::filesystem::remove_all(root);
}
