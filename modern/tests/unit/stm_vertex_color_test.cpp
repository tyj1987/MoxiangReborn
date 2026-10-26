// DX-free tests for the legacy <map>.vcl STM vertex-color contract.
#include "mxh/compat/stm_vertex_color.hpp"
#include "mxh/compat/stm_static_model.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>

using namespace mxh::compat;

namespace {
constexpr std::uint32_t kRigid = 0x00000100u;
constexpr std::uint32_t kLightMap = 0x00000003u;
constexpr std::uint32_t kBillboard = 0x00000030u;

std::vector<std::uint8_t> fill(std::size_t vertices, std::uint8_t seed) {
    std::vector<std::uint8_t> out(vertices * 4u);
    for (std::size_t i = 0; i < out.size(); ++i) out[i] = static_cast<std::uint8_t>(seed + i);
    return out;
}
}  // namespace

TEST(StmVertexColor, D3DColorByteOrderIsBGRA) {
    // On-disk / in-memory bytes B,G,R,A -> DWORD 0xAARRGGBB.
    const std::uint8_t bytes[4] = {0x25, 0x24, 0x2d, 0xff};
    const std::uint32_t argb = read_d3dcolor(bytes);
    EXPECT_EQ(argb, 0xff2d2425u);
    EXPECT_EQ((argb >> 16) & 0xffu, 0x2du);  // R
    EXPECT_EQ((argb >> 8) & 0xffu, 0x24u);   // G
    EXPECT_EQ(argb & 0xffu, 0x25u);          // B
    EXPECT_EQ(argb >> 24, 0xffu);            // A
    // R8G8B8A8_UNORM memory order is R,G,B,A.
    EXPECT_EQ(d3dcolor_to_rgba8(argb), 0xff25242du);
    const std::uint8_t pure_red[4] = {0x00, 0x00, 0xff, 0x80};
    EXPECT_EQ(d3dcolor_to_rgba8(read_d3dcolor(pure_red)), 0x800000ffu);
}

TEST(StmVertexColor, SkipsLightMapAndBillboardMeshes) {
    EXPECT_TRUE(stm_mesh_uses_vcl(kRigid));
    EXPECT_TRUE(stm_mesh_uses_vcl(0x1u));
    EXPECT_FALSE(stm_mesh_uses_vcl(kRigid | kLightMap));
    EXPECT_FALSE(stm_mesh_uses_vcl(kRigid | kBillboard));

    const std::vector<StmVclMeshInfo> meshes{
        {kRigid, 2}, {kRigid | kLightMap, 5}, {kRigid | kBillboard, 4}, {kRigid, 1}};
    const auto vcl = fill(3, 0x10);
    const auto r = assign_stm_vertex_colors(meshes, &vcl);
    ASSERT_EQ(r.status, StmVclStatus::Matched);
    EXPECT_EQ(r.expected_vertices, 3u);
    ASSERT_EQ(r.per_mesh.size(), 4u);
    EXPECT_TRUE(r.per_mesh[1].empty());
    EXPECT_TRUE(r.per_mesh[2].empty());
    ASSERT_EQ(r.per_mesh[0].size(), 2u);
    ASSERT_EQ(r.per_mesh[3].size(), 1u);
    EXPECT_EQ(r.per_mesh[0][0], 0x13121110u);
    EXPECT_EQ(r.per_mesh[0][1], 0x17161514u);
    EXPECT_EQ(r.per_mesh[3][0], 0x1b1a1918u);  // continues after skipped meshes
}

TEST(StmVertexColor, Map10SizeMatchSynthetic) {
    // Map10 facts: 79504 vcl vertices -> 318016 bytes. Synthetic split.
    const std::vector<StmVclMeshInfo> meshes{
        {kRigid, 40000}, {kRigid | kLightMap, 1234}, {kRigid, 39504}, {kRigid | kBillboard, 8}};
    EXPECT_EQ(stm_vcl_expected_vertices(meshes), 79504u);
    const std::vector<std::uint8_t> vcl(318016u, 0x7f);
    const auto r = assign_stm_vertex_colors(meshes, &vcl);
    EXPECT_EQ(r.status, StmVclStatus::Matched);
    EXPECT_EQ(r.loaded_bytes, 318016u);
    EXPECT_EQ(r.per_mesh[2].back(), 0x7f7f7f7fu);
}

TEST(StmVertexColor, MismatchFallsBackToWhite) {
    const std::vector<StmVclMeshInfo> meshes{{kRigid, 79504}};
    for (std::size_t size : {318012u, 318020u, 318017u, 0u}) {
        const std::vector<std::uint8_t> vcl(size, 0x11);
        const auto r = assign_stm_vertex_colors(meshes, &vcl);
        EXPECT_EQ(r.status, StmVclStatus::Mismatch) << size;
        ASSERT_EQ(r.per_mesh[0].size(), 79504u);
        for (auto c : r.per_mesh[0]) ASSERT_EQ(c, kD3DColorWhite);
    }
}

TEST(StmVertexColor, MissingFileIsWhite) {
    const std::vector<StmVclMeshInfo> meshes{{kRigid, 3}, {kLightMap, 2}};
    const auto r = assign_stm_vertex_colors(meshes, nullptr);
    EXPECT_EQ(r.status, StmVclStatus::Missing);
    EXPECT_EQ(r.loaded_bytes, 0u);
    EXPECT_EQ(r.per_mesh[0], std::vector<std::uint32_t>(3, kD3DColorWhite));
    EXPECT_TRUE(r.per_mesh[1].empty());
}

// Optional: real original data, read-only, via MXH_ORIGINAL_MAP_DIR
// (directory containing 10.stm and 10.vcl). Never committed.
TEST(StmVertexColor, RealMap10VclMatchesStm) {
    const char* dir = std::getenv("MXH_ORIGINAL_MAP_DIR");
    if (!dir || !*dir) GTEST_SKIP() << "MXH_ORIGINAL_MAP_DIR not set";
    const std::filesystem::path base(dir);
    const auto stm_path = base / "10.stm";
    const auto vcl_path = base / "10.vcl";
    if (!std::filesystem::exists(stm_path) || !std::filesystem::exists(vcl_path))
        GTEST_SKIP() << "10.stm / 10.vcl not found in " << dir;
    auto slurp = [](const std::filesystem::path& p) {
        std::ifstream f(p, std::ios::binary);
        return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(f), {});
    };
    const auto stm_bytes = slurp(stm_path);
    const auto vcl = slurp(vcl_path);
    StmStaticModel model;
    std::string error;
    ASSERT_TRUE(parse_stm(stm_bytes, model, &error)) << error;
    std::vector<StmVclMeshInfo> meshes;
    for (const auto& m : model.meshes) meshes.push_back({m.mesh_flags, m.positions.size()});
    const auto r = assign_stm_vertex_colors(meshes, &vcl);
    EXPECT_EQ(r.expected_vertices, 79504u);
    EXPECT_EQ(r.loaded_bytes, 318016u);
    EXPECT_EQ(r.status, StmVclStatus::Matched);
}

TEST(StmVertexColor, BufferDecisionFlagsCountMismatch) {
    EXPECT_EQ(stm_vcl_buffer_decision(0, 10), StmVclBufferDecision::None);
    EXPECT_EQ(stm_vcl_buffer_decision(10, 10), StmVclBufferDecision::Build);
    EXPECT_EQ(stm_vcl_buffer_decision(9, 10), StmVclBufferDecision::Mismatch);
    EXPECT_EQ(stm_vcl_buffer_decision(11, 10), StmVclBufferDecision::Mismatch);
}

TEST(StmVertexColor, GateFailureNamesAndPriority) {
    StmVclGateInput in;
    in.vertex_color_path = true;
    EXPECT_EQ(stm_vcl_gate_failure(in), nullptr);
    in.shader_failed = true;
    EXPECT_STREQ(stm_vcl_gate_failure(in), "stmVclShaderFailed");
    in.buffer_mismatch = 2;
    EXPECT_STREQ(stm_vcl_gate_failure(in), "stmVclBufferMismatch");
    in.vcl_missing = true;
    EXPECT_STREQ(stm_vcl_gate_failure(in), "stmVclMissing");
    in.vcl_mismatch = true;
    EXPECT_STREQ(stm_vcl_gate_failure(in), "stmVclMismatch");
    // MXH_STM_LIGHTING=fixed (non-original): never gated.
    in.vertex_color_path = false;
    EXPECT_EQ(stm_vcl_gate_failure(in), nullptr);
}
