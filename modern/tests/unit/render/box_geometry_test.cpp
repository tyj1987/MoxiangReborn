#include "../../../src/render/dx11/box_geometry.hpp"
#include <gtest/gtest.h>
#include <array>
namespace {
using mxh::gx::dx11::make_box_line_vertices;
using mxh::gx::VECTOR3;
TEST(BoxGeometry, NullInputReturnsZeroedVertices) {
    const auto vertices = make_box_line_vertices(nullptr, 0xAABBCCDDu);
    for (const auto& vertex : vertices) { EXPECT_EQ(vertex.position.x, 0.0f); EXPECT_EQ(vertex.position.y, 0.0f); EXPECT_EQ(vertex.position.z, 0.0f); EXPECT_EQ(vertex.color, 0u); }
}
TEST(BoxGeometry, EmitsTwelveEdgesUsingAllThreeCoordinates) {
    const std::array<VECTOR3, 8> corners = {{{0,1,2},{3,1,2},{3,4,2},{0,4,2},{0,1,5},{3,1,5},{3,4,5},{0,4,5}}};
    const auto vertices = make_box_line_vertices(corners.data(), 0x80402010u);
    ASSERT_EQ(vertices.size(), 24u); EXPECT_EQ(vertices[0].position.y, 1.0f); EXPECT_EQ(vertices[4].position.y, 4.0f); EXPECT_EQ(vertices[16].position.z, 2.0f); EXPECT_EQ(vertices[17].position.z, 5.0f);
    for (const auto& vertex : vertices) EXPECT_EQ(vertex.color, 0x80402010u);
}
TEST(BoxGeometry, EdgeOrderingMatchesLegacyGroups) {
    const std::array<VECTOR3, 8> corners = {{{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}}};
    const auto vertices = make_box_line_vertices(corners.data(), 1u);
    EXPECT_EQ(vertices[0].position.x, 0.0f); EXPECT_EQ(vertices[1].position.x, 1.0f); EXPECT_EQ(vertices[8].position.z, 1.0f); EXPECT_EQ(vertices[16].position.y, 0.0f); EXPECT_EQ(vertices[17].position.y, 0.0f);
}
}
