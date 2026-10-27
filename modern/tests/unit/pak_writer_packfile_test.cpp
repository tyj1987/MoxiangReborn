// PackFile reads modern/tools/pak_writer.py output.
//
// The fixture is built at configure/build time by
// tests/unit/tools/make_pak_writer_fixture.py from LCG bytes; no original
// data is involved. The test regenerates the same bytes and checks that
// only the 96-byte layout validates and every entry matches.

#include "mxh/compat/pack_file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#ifndef MXH_PAK_WRITER_FIXTURE
#error "MXH_PAK_WRITER_FIXTURE must name the generated fixture pak"
#endif

namespace {

using mxh::compat::PackFile;

struct FixtureEntry {
    const char* name;
    std::uint32_t size;
};

// Same table as FIXTURE in make_pak_writer_fixture.py, in writer order
// (casefolded name, then raw bytes).
constexpr FixtureEntry kSorted[] = {
    {"A.stm", 1024},
    {"b_tree.MOD", 257},
    {"map/c.hfl", 33},
    {"z.tif", 1},
};

std::vector<std::uint8_t> lcg_bytes(std::uint32_t size) {
    std::uint32_t x = size;
    std::vector<std::uint8_t> out;
    out.reserve(size);
    for (std::uint32_t i = 0; i < size; ++i) {
        x = x * 1103515245u + 12345u;
        out.push_back(static_cast<std::uint8_t>((x >> 16) & 0xffu));
    }
    return out;
}

std::vector<std::uint8_t> read_fixture() {
    std::ifstream in(std::filesystem::path(MXH_PAK_WRITER_FIXTURE), std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

TEST(PakWriterPackFile, OnlyOriginal96LayoutValidates) {
    const auto bytes = read_fixture();
    ASSERT_GT(bytes.size(), 96u);
    const auto probe = mxh::compat::probe_pak_layouts(bytes);
    EXPECT_TRUE(probe.original96_exact_eof);
    EXPECT_FALSE(probe.playdh92_exact_eof);
}

TEST(PakWriterPackFile, EntriesMatchByteForByte) {
    std::string error;
    auto pak = PackFile::open(std::filesystem::path(MXH_PAK_WRITER_FIXTURE), &error);
    ASSERT_NE(pak, nullptr) << error;
    const auto& entries = pak->entries();
    ASSERT_EQ(entries.size(), std::size(kSorted));
    for (std::size_t i = 0; i < entries.size(); ++i) {
        EXPECT_EQ(entries[i].name, kSorted[i].name);
        const auto want = lcg_bytes(kSorted[i].size);
        ASSERT_EQ(entries[i].raw_view.size(), want.size()) << kSorted[i].name;
        EXPECT_TRUE(std::equal(want.begin(), want.end(), entries[i].raw_view.begin()))
            << kSorted[i].name;
        const auto* found = pak->find(kSorted[i].name);
        ASSERT_NE(found, nullptr) << kSorted[i].name;
        EXPECT_EQ(found->data_offset, entries[i].data_offset);
    }
}

}  // namespace
