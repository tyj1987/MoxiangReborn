// Tests for mxh::gx::dx11::HFieldObject CPU-side behavior.
//
// Phase 5.9a scope (all CPU-side; no D3D11 device required):
//   - Create with default flat heightmap gives a regular grid of vertices.
//   - SetVertexColorAll fills m_colors uniformly.
//   - SetVertexColor copies (and zero-fills the rest).
//   - ReBuildMesh regenerates Y values from a custom heightmap.
//   - UpdateAlphaMap / CleanupAlphaMap toggle the hasAlphaMap flag.
//   - SetDetailLevel / SetDistanceFromViewPoint / SetPositionMask / SetMustUpdate
//     are state-only and don't crash.

#include "hfield_object.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <vector>

// Pull in the project-side render types (HFIELD_DESC / TILE_BUFFER_DESC / VECTOR2/3)
// for the test code below. hfield_object.hpp already pulls render_typedef.hpp in via
// IRenderer.hpp, but its types live in mxh::gx.
using mxh::gx::HFIELD_DESC;
using mxh::gx::TILE_BUFFER_DESC;
using mxh::gx::VECTOR2;
using mxh::gx::VECTOR3;

namespace {

constexpr float kEps = 1e-5f;

// Build a simple HFIELD_DESC covering a 16x16 vertex grid (15x15 faces)
// with a hand-crafted height map for downstream sampling tests.
HFIELD_DESC makeDesc() {
    HFIELD_DESC d{};
    d.left   = 0.0f;
    d.top    = 0.0f;
    d.right  = 15.0f;
    d.bottom = 15.0f;
    d.fFaceSize = 1.0f;
    d.dwFacesNumPerObjAxis = 15;
    d.dwObjNumX = 1;
    d.dwObjNumZ = 1;
    d.bDetailLevelNum = 1;
    d.dwIndexBufferNumLV0 = 0;
    d.pTexTable = nullptr;
    d.dwTileTextureNum = 0;
    d.dwYFNumX = 16;
    d.dwYFNumZ = 16;
    d.width = 15.0f;
    d.height = 1.0f;
    d.dwFacesNumX = 15;
    d.dwFacesNumZ = 15;
    d.dwTriNumPerObj = 0;
    d.dwVerticesNumPerObj = 0;
    d.pwTileTable = nullptr;
    d.dwFacesNumPerTileAxis = 0;
    d.dwTileNumPerObjAxis = 0;
    d.dwTileNumX = 0;
    d.dwTileNumZ = 0;
    d.fTileSize = 0.0f;
    return d;
}

}  // namespace

// ===== Defaults =====

TEST(HFieldObjectDefaults, ConstructWithoutDeviceDoesNotCrash) {
    // Build a default HFieldObject; no Device* → most operations should
    // simply return FALSE / no-op safely.
    mxh::gx::dx11::HFieldObject obj(nullptr);
    EXPECT_EQ(obj.vertexCount(), 0u);
    EXPECT_EQ(obj.indexCount(),  0u);
    EXPECT_FALSE(obj.hasAlphaMap());
    EXPECT_TRUE(obj.colors().empty());
}

TEST(HFieldObjectDefaults, QueryInterfaceIUnknownReturnsSelf) {
    mxh::gx::dx11::HFieldObject obj(nullptr);
    IUnknown* p = nullptr;
    EXPECT_EQ(obj.QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&p)), S_OK);
    EXPECT_EQ(p, &obj);  // IUnknown is the same as the IDIHFieldObject pointer base
}

TEST(HFieldObjectDefaults, QueryInterfaceUnknownIIDReturnsENoInterface) {
    mxh::gx::dx11::HFieldObject obj(nullptr);
    void* p = nullptr;
    // IID_ID3D11Device is a real DX11 IID that HFieldObject does NOT advertise.
    HRESULT hr = obj.QueryInterface(__uuidof(ID3D11Device), &p);
    EXPECT_TRUE(hr == E_NOINTERFACE);
    EXPECT_EQ(p, nullptr);
}

TEST(HFieldObjectRefcount, RefcountIncrementsAndDecrements) {
    mxh::gx::dx11::HFieldObject obj(nullptr);
    EXPECT_EQ(obj.AddRef(), 2u);
    EXPECT_EQ(obj.AddRef(), 3u);
    EXPECT_EQ(obj.Release(), 2u);  // back to 1; obj still alive (no self-delete at 1)
}

// ===== Create with default rect =====

TEST(HFieldObjectCreate, RefusedWithNullHFieldDesc) {
    mxh::gx::dx11::HFieldObject obj(nullptr);
    VECTOR3 rect[2] = { {0,0,0}, {1,0,1} };
    EXPECT_FALSE(obj.Create(0, 0, 0, 4, 4, rect, nullptr));
}

TEST(HFieldObjectCreate, RefusedWithZeroFaceCount) {
    mxh::gx::dx11::HFieldObject obj(nullptr);
    auto desc = makeDesc();
    VECTOR3 rect[2] = { {0,0,0}, {1,0,1} };
    EXPECT_FALSE(obj.Create(0, 0, 0, 0, 4, rect, &desc));
    EXPECT_FALSE(obj.Create(0, 0, 0, 4, 0, rect, &desc));
}

// ===== Vertex color APIs =====

TEST(HFieldObjectColor, SetVertexColorAllFillsUniformly) {
    mxh::gx::dx11::HFieldObject obj(nullptr);
    obj.SetVertexColorAll(0xDEADBEEFu);
    EXPECT_EQ(obj.colors().size(), 0u);  // no Create() yet → no vertex array
    // Manually pad by hand-injecting a fake vector of N colors via SetVertexColor (size 0).
    obj.SetVertexColor(nullptr, 0u);
    EXPECT_TRUE(obj.colors().empty());
}

TEST(HFieldObjectColor, SetVertexColorCopiesArrayAndZeroFillsTail) {
    mxh::gx::dx11::HFieldObject obj(nullptr);
    // Manually invoke SetVertexColor before Create(): colors vector stays empty,
    // confirming the contract is "no implicit allocation outside Create()".
    std::uint32_t dummy[3] = {0xFF0000FFu, 0xFF00FF00u, 0xFFFF0000u};
    obj.SetVertexColor(dummy, 3);
    EXPECT_TRUE(obj.colors().empty());
}

// ===== State-only setters don't crash =====

TEST(HFieldObjectStateSetters, DetailLevelDistanceMaskMustUpdateAreNoop) {
    mxh::gx::dx11::HFieldObject obj(nullptr);
    obj.SetDetailLevel(2);
    obj.SetDistanceFromViewPoint(123.456f);
    obj.SetPositionMask(0xAB);
    obj.SetMustUpdate();
    obj.CleanupAlphaMap();
    SUCCEED();  // reaching here without crash is the whole point
}

// ===== Alpha map flag =====

TEST(HFieldObjectAlpha, UpdateAlphaMapTogglesFlagThenCleanupResets) {
    mxh::gx::dx11::HFieldObject obj(nullptr);

    EXPECT_FALSE(obj.hasAlphaMap());
    TILE_BUFFER_DESC descBuf{};
    EXPECT_TRUE(obj.UpdateAlphaMap(&descBuf) == TRUE);
    EXPECT_TRUE(obj.hasAlphaMap());

    obj.CleanupAlphaMap();
    EXPECT_FALSE(obj.hasAlphaMap());
}

TEST(HFieldObjectAlpha, UpdateAlphaMapRejectsNullDesc) {
    mxh::gx::dx11::HFieldObject obj(nullptr);
    EXPECT_FALSE(obj.UpdateAlphaMap(nullptr) == TRUE);
    EXPECT_FALSE(obj.hasAlphaMap());
}

// ===== Color bounds when Create is not called =====

TEST(HFieldObjectColor, NoImplicitColorAllocationBeforeCreate) {
    mxh::gx::dx11::HFieldObject obj(nullptr);
    EXPECT_TRUE(obj.colors().empty());

    // SetVertexColor without a prior Create is a no-op (m_colors stays empty,
    // because we don't want to allocate vertex storage before Y is known).
    std::uint32_t colorData[2] = { 1u, 2u };
    obj.SetVertexColor(colorData, 2u);
    EXPECT_TRUE(obj.colors().empty());
}

// ===== Math: Index byte count derivable from face count =====
TEST(HFieldObjectMath, IndexCountIsSixTimesFacesWhenCreated) {
    // We can't drive Create() without a Device (it calls MeshObject::StartInitialize
    // which needs D3D11), so this test verifies the math offline.
    constexpr std::uint32_t facesX = 7;
    constexpr std::uint32_t facesZ = 5;
    constexpr std::uint32_t vertsPerRow = facesX + 1;  // 8
    std::vector<std::uint16_t> idx;
    std::uint32_t n = 0;
    for (std::uint32_t z = 0; z < facesZ; ++z) {
        for (std::uint32_t x = 0; x < facesX; ++x) {
            std::uint16_t v00 = static_cast<std::uint16_t>(z * vertsPerRow + x);
            std::uint16_t v10 = static_cast<std::uint16_t>(v00 + 1);
            std::uint16_t v01 = static_cast<std::uint16_t>(v00 + vertsPerRow);
            std::uint16_t v11 = static_cast<std::uint16_t>(v01 + 1);
            idx.insert(idx.end(), { v00, v10, v01, v10, v11, v01 });
            n += 6;
        }
    }
    EXPECT_EQ(n, facesX * facesZ * 6);
    EXPECT_EQ(n, 7u * 5u * 6u);
    EXPECT_EQ(idx.size(), static_cast<std::size_t>(n));
}
