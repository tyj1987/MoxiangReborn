// material_test.cpp — Material system unit tests.
//
// Phase 5.4: CreateMaterialSet / CreateMaterial / DeleteMaterial
//
// Tests the CPU-side material structures (MaterialData, MaterialSet) and the
// raw material API contract.  We avoid including CoD3DDeviceDX11 here because
// that header transitively pulls in <objbase.h> which must come BEFORE
// <windows.h>, while the Winsock2 headers needed by the network layer must
// come AFTER <windows.h>.  The full integration with CoD3DDeviceDX11 is
// tested via the RenderDemo integration binary.
//
// What we CAN test here without DX/Winsock conflicts:
//   - MaterialData struct field layout
//   - MaterialSet ownership and entry management
//   - MATERIAL / MATERIAL_TABLE typedef contracts

// Include material.hpp FIRST — it includes winsock2.h before <windows.h> (via <d3d11.h>),
// which satisfies Windows SDK header ordering rules. After that, render_typedef.hpp's
// <windows.h> include is harmless because WIN32_LEAN_AND_MEAN is defined globally.
#include "material.hpp"
#include "mxh/render/render_typedef.hpp"

#include <gtest/gtest.h>
#include <cstring>
#include <vector>

namespace {

using mxh::gx::MATERIAL;

// Fill a MATERIAL struct with known test values.
void fill_material(MATERIAL& m, const char* diffuse,
                   std::uint32_t diffuseColor,
                   std::uint32_t ambientColor,
                   float transparency) {
    std::memset(&m, 0, sizeof(m));
    m.dwDiffuse     = diffuseColor;
    m.dwAmbient     = ambientColor;
    m.dwSpecular    = 0xFFFFFFFF;
    m.fTransparency = transparency;
    m.fShine        = 64.0f;
    m.fShineStrength = 1.0f;
    m.dwFlag        = 0x12345678;
    if (diffuse) {
        std::strncpy(m.szDiffuseTexmapFileName, diffuse,
                     sizeof(m.szDiffuseTexmapFileName) - 1);
    }
}

}  // namespace

namespace mxh::gx::dx11 {

// ---------------------------------------------------------------------------
// MaterialData struct tests (CPU-only — no DX11 dependencies).
// ---------------------------------------------------------------------------

TEST(MaterialDataTest, DefaultFieldsAreZero) {
    MaterialData m;
    EXPECT_EQ(m.dwDiffuse,     0u);
    EXPECT_EQ(m.dwAmbient,       0u);
    EXPECT_EQ(m.dwSpecular,      0u);
    EXPECT_FLOAT_EQ(m.fTransparency,  0.0f);
    EXPECT_FLOAT_EQ(m.fShine,          0.0f);
    EXPECT_FLOAT_EQ(m.fShineStrength,  0.0f);
    EXPECT_EQ(m.dwFlag,         0u);
    EXPECT_EQ(m.borderColor,    0u);
    EXPECT_FALSE(m.diffuse.loaded);
    EXPECT_FALSE(m.reflect.loaded);
    EXPECT_FALSE(m.bump.loaded);
}

TEST(MaterialDataTest, FieldsCanBeSet) {
    MaterialData m;
    m.dwDiffuse      = 0xAABBCCDD;
    m.dwAmbient      = 0x11223344;
    m.dwSpecular     = 0xFFEEDDCC;
    m.fTransparency  = 0.75f;
    m.fShine         = 128.0f;
    m.fShineStrength = 0.5f;
    m.dwFlag         = 0xDEADBEEF;
    m.borderColor    = 0xFFFF0000;

    EXPECT_EQ(m.dwDiffuse,     0xAABBCCDDu);
    EXPECT_EQ(m.dwAmbient,     0x11223344u);
    EXPECT_EQ(m.dwSpecular,    0xFFEEDDCCu);
    EXPECT_FLOAT_EQ(m.fTransparency,  0.75f);
    EXPECT_FLOAT_EQ(m.fShine,          128.0f);
    EXPECT_FLOAT_EQ(m.fShineStrength,  0.5f);
    EXPECT_EQ(m.dwFlag,       0xDEADBEEFu);
    EXPECT_EQ(m.borderColor,  0xFFFF0000u);
}

TEST(MaterialDataTest, TextureFieldsDefaultToEmpty) {
    MaterialData m;
    EXPECT_FALSE(m.diffuse.loaded);
    EXPECT_FALSE(m.reflect.loaded);
    EXPECT_FALSE(m.bump.loaded);
    EXPECT_EQ(m.diffuse.width,  0u);
    EXPECT_EQ(m.diffuse.height, 0u);
}

// ---------------------------------------------------------------------------
// MaterialSet tests.
// ---------------------------------------------------------------------------

TEST(MaterialSetTest, DefaultConstructsWithEmptyEntries) {
    MaterialSet set;
    EXPECT_TRUE(set.entries.empty());
}

TEST(MaterialSetTest, CanOwnMultipleMaterialEntries) {
    MaterialSet set;
    set.entries.reserve(3);
    set.entries.push_back(nullptr);  // slot 0: null (valid "no material")
    set.entries.push_back(nullptr);  // slot 1: null

    EXPECT_EQ(set.entries.size(), 2u);
    EXPECT_EQ(set.entries[0], nullptr);
    EXPECT_EQ(set.entries[1], nullptr);
}

// ---------------------------------------------------------------------------
// MATERIAL struct contract tests (compatibility with original engine).
// ---------------------------------------------------------------------------

TEST(MaterialContractTest, GetDiffuseTexmapNameReturnsEmptyWhenNotSet) {
    MATERIAL m{};
    EXPECT_STREQ(m.GetDiffuseTexmapName(), "");
}

TEST(MaterialContractTest, GetDiffuseTexmapNameReturnsSetValue) {
    MATERIAL m{};
    std::strncpy(m.szDiffuseTexmapFileName, "test_d.ktx",
                 sizeof(m.szDiffuseTexmapFileName) - 1);
    EXPECT_STREQ(m.GetDiffuseTexmapName(), "test_d.ktx");
}

TEST(MaterialContractTest, GetBumpTexmapNameReturnsSetValue) {
    MATERIAL m{};
    std::strncpy(m.szBumpTexmapFileName, "normal.ktx",
                 sizeof(m.szBumpTexmapFileName) - 1);
    EXPECT_STREQ(m.GetBumpTexmapName(), "normal.ktx");
}

TEST(MaterialContractTest, GetReflectTexmapNameReturnsSetValue) {
    MATERIAL m{};
    std::strncpy(m.szReflectTexmapFileName, "env.ktx",
                 sizeof(m.szReflectTexmapFileName) - 1);
    EXPECT_STREQ(m.GetReflectTexmapName(), "env.ktx");
}

TEST(MaterialContractTest, GetFlagReturnsDwFlag) {
    MATERIAL m{};
    m.dwFlag = 0x87654321;
    EXPECT_EQ(m.GetFlag(), 0x87654321u);
}

TEST(MaterialContractTest, GetTransparencyReturnsFloatValue) {
    MATERIAL m{};
    m.fTransparency = 0.25f;
    EXPECT_FLOAT_EQ(m.GetTransparency(), 0.25f);
}

TEST(MaterialContractTest, MaterialTableEntryPointsToMaterial) {
    MATERIAL m{};
    fill_material(m, "diffuse.tga", 0xFFFFFFFF, 0xFF808080, 0.5f);

    MATERIAL_TABLE table{};
    table.pMtl       = &m;
    table.dwMtlIndex = 42;

    ASSERT_NE(table.pMtl, nullptr);
    EXPECT_EQ(table.dwMtlIndex, 42u);
    EXPECT_STREQ(table.pMtl->GetDiffuseTexmapName(), "diffuse.tga");
}

TEST(MaterialContractTest, FillHelperProducesExpectedValues) {
    MATERIAL m{};
    fill_material(m, "skin_d.ktx", 0xAABBCCDD, 0x11223344, 0.33f);
    EXPECT_STREQ(m.GetDiffuseTexmapName(), "skin_d.ktx");
    EXPECT_EQ(m.dwDiffuse,     0xAABBCCDDu);
    EXPECT_EQ(m.dwAmbient,     0x11223344u);
    EXPECT_FLOAT_EQ(m.fTransparency, 0.33f);
}

// ---------------------------------------------------------------------------
// MATERIAL_TABLE array (multiple materials in one set).
// ---------------------------------------------------------------------------

TEST(MaterialSetContractTest, MultipleEntriesFormValidMaterialTable) {
    MATERIAL mats[3];
    fill_material(mats[0], "mtl0.ktx", 0x11111111, 0x22222222, 0.1f);
    fill_material(mats[1], "mtl1.ktx", 0x33333333, 0x44444444, 0.2f);
    // mats[2]: null entry

    MATERIAL_TABLE table[3];
    table[0].pMtl = &mats[0];
    table[0].dwMtlIndex = 0;
    table[1].pMtl = &mats[1];
    table[1].dwMtlIndex = 1;
    table[2].pMtl = nullptr;  // null slot
    table[2].dwMtlIndex = 2;

    EXPECT_STREQ(table[0].pMtl->GetDiffuseTexmapName(), "mtl0.ktx");
    EXPECT_STREQ(table[1].pMtl->GetDiffuseTexmapName(), "mtl1.ktx");
    EXPECT_EQ(table[2].pMtl, nullptr);
}

}  // namespace mxh::gx::dx11
