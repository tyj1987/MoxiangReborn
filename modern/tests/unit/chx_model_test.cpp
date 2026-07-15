// chx_model_test.cpp - Phase 10.22 chx model parser tests
//
// Covers modern/include/mxh/compat/chx_model.hpp — the .chx
// character model parser skeleton. The header struct + the
// is_chx() / parse() / load() functions are 1:1 with the
// original 3ds Max Biped/Physique export via MAXEXP/MtlExp/
// anmexp plugins.
//
// What's tested:
//   - ChxHeader wire-format size under #pragma pack(1) (32
//     bytes).
//   - ChxHeader field offsets are pinned.
//   - is_chx() accepts plausible headers and rejects
//     everything else (truncated, version out of range, zero
//     counts, absurdly large counts).
//   - parse() populates the header + raw bytes correctly.
//   - load() returns empty on missing path.

#include "mxh/compat/chx_model.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace mxh::compat::test {

namespace {

// Build a fake 32-byte ChxHeader with the given magic + counts.
std::array<std::uint8_t, 32> make_header(std::uint32_t magic,
                                          std::uint32_t version,
                                          std::uint32_t mesh_count,
                                          std::uint32_t bone_count,
                                          std::uint32_t material_count,
                                          std::uint32_t vertex_count,
                                          std::uint32_t index_count,
                                          std::uint32_t reserved) {
    std::array<std::uint8_t, 32> buf{};
    std::uint8_t* p = buf.data();
    auto write_u32 = [&](std::uint32_t v) {
        std::memcpy(p, &v, 4);
        p += 4;
    };
    write_u32(magic);
    write_u32(version);
    write_u32(mesh_count);
    write_u32(bone_count);
    write_u32(material_count);
    write_u32(vertex_count);
    write_u32(index_count);
    write_u32(reserved);
    return buf;
}

}  // namespace

// ===========================================================================
// Wire format
// ===========================================================================

TEST(ChxHeaderTest, SizeIs32Bytes) {
    // Field layout under pack(1):
    //   magic (4) + version (4) + mesh_count (4) + bone_count (4)
    //   + material_count (4) + vertex_count (4) + index_count (4)
    //   + reserved (4) = 32 bytes
    static_assert(sizeof(ChxHeader) == 32,
                  "ChxHeader must be 32 bytes (eight uint32_t, pack(1))");
    EXPECT_EQ(sizeof(ChxHeader), 32u);
}

TEST(ChxHeaderTest, FieldOffsets) {
    ChxHeader h{};
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&h.magic) -
              reinterpret_cast<std::uintptr_t>(&h), 0u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&h.version) -
              reinterpret_cast<std::uintptr_t>(&h), 4u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&h.mesh_count) -
              reinterpret_cast<std::uintptr_t>(&h), 8u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&h.bone_count) -
              reinterpret_cast<std::uintptr_t>(&h), 12u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&h.material_count) -
              reinterpret_cast<std::uintptr_t>(&h), 16u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&h.vertex_count) -
              reinterpret_cast<std::uintptr_t>(&h), 20u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&h.index_count) -
              reinterpret_cast<std::uintptr_t>(&h), 24u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&h.reserved) -
              reinterpret_cast<std::uintptr_t>(&h), 28u);
}

// ===========================================================================
// is_chx()
// ===========================================================================

TEST(ChxIsChxTest, RejectsEmptyInput) {
    std::vector<std::uint8_t> empty;
    EXPECT_FALSE(ChxModel::is_chx(empty));
}

TEST(ChxIsChxTest, RejectsShorterThanHeader) {
    auto buf = make_header('CHLX', 1, 1, 1, 1, 100, 200, 0);
    // Strip the last 4 bytes.
    std::span<const std::uint8_t> truncated(buf.data(), buf.size() - 4);
    EXPECT_FALSE(ChxModel::is_chx(truncated));
}

TEST(ChxIsChxTest, AcceptsPlausibleHeader) {
    auto buf = make_header('CHLX', 1, 1, 1, 1, 100, 200, 0);
    EXPECT_TRUE(ChxModel::is_chx(buf));
}

TEST(ChxIsChxTest, RejectsVersionZero) {
    auto buf = make_header('CHLX', 0, 1, 1, 1, 100, 200, 0);
    EXPECT_FALSE(ChxModel::is_chx(buf));
}

TEST(ChxIsChxTest, RejectsVersionTooLarge) {
    // version > 10 is rejected — the original parser is loose
    // about the magic (region-specific variants) but strict
    // about the version range.
    auto buf = make_header('CHLX', 11, 1, 1, 1, 100, 200, 0);
    EXPECT_FALSE(ChxModel::is_chx(buf));
    auto buf2 = make_header('CHLX', 100, 1, 1, 1, 100, 200, 0);
    EXPECT_FALSE(ChxModel::is_chx(buf2));
}

TEST(ChxIsChxTest, RejectsZeroVertexCount) {
    auto buf = make_header('CHLX', 1, 1, 1, 1, 0, 200, 0);
    EXPECT_FALSE(ChxModel::is_chx(buf));
}

TEST(ChxIsChxTest, RejectsZeroIndexCount) {
    auto buf = make_header('CHLX', 1, 1, 1, 1, 100, 0, 0);
    EXPECT_FALSE(ChxModel::is_chx(buf));
}

TEST(ChxIsChxTest, RejectsAbsurdlyLargeVertexCount) {
    auto buf = make_header('CHLX', 1, 1, 1, 1, 10'000'001, 200, 0);
    EXPECT_FALSE(ChxModel::is_chx(buf));
}

TEST(ChxIsChxTest, RejectsAbsurdlyLargeIndexCount) {
    auto buf = make_header('CHLX', 1, 1, 1, 1, 100, 50'000'001, 0);
    EXPECT_FALSE(ChxModel::is_chx(buf));
}

TEST(ChxIsChxTest, AcceptsBoundaryValues) {
    // version = 1, version = 10, vertex_count = 1 + 10M-1,
    // index_count = 1 + 50M-1 — these are the boundary
    // values and should all be accepted.
    auto buf_min_v = make_header('CHLX', 1,  1, 1, 1, 1, 1, 0);
    EXPECT_TRUE(ChxModel::is_chx(buf_min_v));
    auto buf_max_v = make_header('CHLX', 10, 1, 1, 1, 9'999'999, 49'999'999, 0);
    EXPECT_TRUE(ChxModel::is_chx(buf_max_v));
}

// ===========================================================================
// parse()
// ===========================================================================

TEST(ChxParseTest, RejectsNonChxInput) {
    std::vector<std::uint8_t> empty;
    auto m = ChxModel::parse(empty);
    EXPECT_EQ(m.header.version, 0u);  // default
    EXPECT_TRUE(m.raw.empty());
    EXPECT_TRUE(m.vertices.empty());
    EXPECT_TRUE(m.indices.empty());
}

TEST(ChxParseTest, PopulatesHeaderFromBytes) {
    auto buf = make_header('CHLX', 3, 5, 10, 2, 1000, 2000, 0);
    auto m = ChxModel::parse(buf);
    EXPECT_EQ(m.header.magic,          'CHLX');
    EXPECT_EQ(m.header.version,        3u);
    EXPECT_EQ(m.header.mesh_count,     5u);
    EXPECT_EQ(m.header.bone_count,     10u);
    EXPECT_EQ(m.header.material_count, 2u);
    EXPECT_EQ(m.header.vertex_count,   1000u);
    EXPECT_EQ(m.header.index_count,    2000u);
    EXPECT_EQ(m.header.reserved,       0u);
}

TEST(ChxParseTest, PopulatesRawBuffer) {
    // parse() captures the entire input into `raw` for
    // passthrough, including any trailing data after the
    // header.
    std::vector<std::uint8_t> input;
    auto header = make_header('CHLX', 1, 1, 1, 1, 100, 200, 0);
    input.insert(input.end(), header.begin(), header.end());
    // Append 16 trailing bytes (e.g. a partial mesh table).
    for (int i = 0; i < 16; ++i) input.push_back(static_cast<std::uint8_t>(i));
    auto m = ChxModel::parse(input);
    EXPECT_EQ(m.raw.size(), input.size());
    EXPECT_EQ(std::memcmp(m.raw.data(), input.data(), input.size()), 0);
}

TEST(ChxParseTest, VerticesAndIndicesEmptyInSkeleton) {
    // The current skeleton parser does NOT decode vertex /
    // index / mesh / bone tables — the TODO(Phase 1.3) marker
    // says so. Pin that as the current contract: a successful
    // parse populates header + raw but leaves the per-table
    // vectors empty. A future "real" decoder will need to
    // update this test.
    auto buf = make_header('CHLX', 1, 1, 1, 1, 100, 200, 0);
    auto m = ChxModel::parse(buf);
    EXPECT_TRUE(m.vertices.empty());
    EXPECT_TRUE(m.indices.empty());
}

TEST(ChxParseTest, DefaultConstructedHasZeroedHeader) {
    ChxModel m;
    EXPECT_EQ(m.header.magic,          0u);
    EXPECT_EQ(m.header.version,        0u);
    EXPECT_EQ(m.header.mesh_count,     0u);
    EXPECT_EQ(m.header.bone_count,     0u);
    EXPECT_EQ(m.header.material_count, 0u);
    EXPECT_EQ(m.header.vertex_count,   0u);
    EXPECT_EQ(m.header.index_count,    0u);
    EXPECT_EQ(m.header.reserved,       0u);
    EXPECT_TRUE(m.raw.empty());
    EXPECT_TRUE(m.vertices.empty());
    EXPECT_TRUE(m.indices.empty());
}

// ===========================================================================
// load()
// ===========================================================================

TEST(ChxLoadTest, MissingPathReturnsEmpty) {
    auto m = ChxModel::load("D:/_does_not_exist_/nope.chx");
    EXPECT_EQ(m.header.version, 0u);
    EXPECT_TRUE(m.raw.empty());
}

}  // namespace mxh::compat::test
