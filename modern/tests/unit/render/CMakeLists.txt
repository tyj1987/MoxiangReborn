add_executable(mxh_render_tests
    tga_loader_test.cpp
    mesh_geometry_test.cpp
    font_object_test.cpp
    height_field_test.cpp
    hfield_object_test.cpp
    height_field_pool_test.cpp
    effect_shader_test.cpp
    material_test.cpp
    dynamic_light_test.cpp
    texture_loader_test.cpp
)

target_link_libraries(mxh_render_tests PRIVATE mxh_render gtest_main)

# mxh_render already exposes ${CMAKE_SOURCE_DIR}/src/render/dx11 PUBLIC,
# so the test gets texture_loader.hpp transitively via the link line.
# No extra target_include_directories needed here.

include(GoogleTest)
# Hand-register tests via gtest_add_tests. We can't use gtest_discover_tests
# because the POST_BUILD cmake_test_discovery_*.json step fails when the
# build directory contains full-width CJK characters in the path (the cmake
# script that writes the JSON file uses ANSI APIs that mangle CJK chars).
gtest_add_tests(TARGET mxh_render_tests
    TgaLoader.RejectsTruncatedHeader
    TgaLoader.Decodes24BitTopDown
    TgaLoader.Decodes24BitBottomUpFlipped
    TgaLoader.Decodes32BitPreservesAlpha
    TgaLoader.AutoDetectDispatchesToTga
    TgaLoader.UnknownImageTypeProducesEmpty
    TgaLoader.RLEPackAndUnpackRoundtrip
    MeshGeometryTest.MeshDescAndFaceDescContract
    MeshGeometryTest.IVertexLayout
    MeshGeometryTest.TVertexLayout
    MeshGeometryTest.CubeTriangleCount
    MatrixMathTest.OrthographicLHHasCorrectDiagonal
    MatrixMathTest.LookAtLHProducesValidMatrix
    MatrixMathTest.IdentityHasCorrectDiagonal
    HeightFieldTest.BuildTileIndicesProducesCorrectCount
    HeightFieldTest.BuildTileIndicesProducesCCWTriangles
    HeightFieldTest.HeightFieldConstructsWithNullDevice
    HeightFieldTest.HeightAtBilinearInterpolation
    HeightFieldTest.HeightAtCenterReturnsCenterValue
    HeightFieldTest.HeightAtClampedToEdges
    HeightFieldTest.HeightAtEmptyMapReturnsZero
    HeightFieldTest.TileWorldSizeIsCorrect
    HeightFieldTest.MaxLodLevelsConstantExists
    EffectShaderTest.EffectEntryDefaultValues
    EffectShaderTest.PaletteConstructsWithNullDevice
    EffectShaderTest.PaletteBuildFromNullDescReturnsFalse
    EffectShaderTest.PaletteBuildFromDescWithEmptyTexName
    EffectShaderTest.PaletteBuildFromDescMultipleEntries
    EffectShaderTest.GetEffectOutOfRangeReturnsNull
    EffectShaderTest.ClearEmptiesPalette
    EffectShaderTest.SetSphereMapMatrixOutputProperties
    EffectShaderTest.SetSphereMapMatrixNullPtrNoCrash
    EffectShaderTest.SetWaveTexMatrixOutputProperties
    EffectShaderTest.SetWaveTexMatrixNullPtrNoCrash
    EffectShaderTest.SetWaveTexMatrixAnimatedValuesInRange
    MaterialDataTest.DefaultFieldsAreZero
    MaterialDataTest.FieldsCanBeSet
    MaterialDataTest.TextureFieldsDefaultToEmpty
    MaterialSetTest.DefaultConstructsWithEmptyEntries
    MaterialSetTest.CanOwnMultipleMaterialEntries
    MaterialContractTest.GetDiffuseTexmapNameReturnsEmptyWhenNotSet
    MaterialContractTest.GetDiffuseTexmapNameReturnsSetValue
    MaterialContractTest.GetBumpTexmapNameReturnsSetValue
    MaterialContractTest.GetReflectTexmapNameReturnsSetValue
    MaterialContractTest.GetFlagReturnsDwFlag
    MaterialContractTest.GetTransparencyReturnsFloatValue
    MaterialContractTest.MaterialTableEntryPointsToMaterial
    MaterialContractTest.FillHelperProducesExpectedValues
    MaterialSetContractTest.MultipleEntriesFormValidMaterialTable
    DynamicLightDefaults.fieldsAreDefault
    DynamicLightDefaults.pointLightFields
    DynamicLightDefaults.flagsValues
    LightCBInit.baseDirectionalLightDefaults
    LightCBInit.extendedSlotsZeroed
    ColorConversion.opaqueWhite
    ColorConversion.opaqueRed
    ColorConversion.semiTransparentBlue
    ColorConversion.zeroAlpha
    LightIndexDesc.structLayout
    DynamicLightConstants.maxLightsIs8
    TriBufferMagic.magicValueIsTRIB
    TextureLoaderTGA.SaveTGARoundTripPreservesPixels
    TextureLoaderTGA.SaveTGARejectsEmptyTexture
    TextureLoaderDDS.SaveDDSMagicAndHeaderLayout
    TextureLoaderDDS.SaveDDSPixelDataIsBGRA
    TextureLoaderDDS.SaveDDSRejectsEmptyTexture
    TextureLoaderAutoDetect.UnknownHeaderReturnsEmpty
    HFieldObjectDefaults.ConstructWithoutDeviceDoesNotCrash
    HFieldObjectDefaults.QueryInterfaceIUnknownReturnsSelf
    HFieldObjectDefaults.QueryInterfaceUnknownIIDReturnsENoInterface
    HFieldObjectRefcount.RefcountIncrementsAndDecrements
    HFieldObjectCreate.RefusedWithNullHFieldDesc
    HFieldObjectCreate.RefusedWithZeroFaceCount
    HFieldObjectColor.SetVertexColorAllFillsUniformly
    HFieldObjectColor.SetVertexColorCopiesArrayAndZeroFillsTail
    HFieldObjectStateSetters.DetailLevelDistanceMaskMustUpdateAreNoop
    HFieldObjectAlpha.UpdateAlphaMapTogglesFlagThenCleanupResets
    HFieldObjectAlpha.UpdateAlphaMapRejectsNullDesc
    HFieldObjectAlpha.UpdateAlphaMapRecordsIntegratedTileInAlphaMap
    HFieldObjectAlpha.CleanupAlphaMapResetsWeightsToTileZeroDefault
    HFieldObjectColor.NoImplicitColorAllocationBeforeCreate
    HFieldObjectMath.IndexCountIsSixTimesFacesWhenCreated
    HeightFieldPoolCaps.ConstantValuesMatchLegacyArraySize
    HeightFieldPoolGuards.InitiallizeRejectsZeroIndices
    HeightFieldPoolGuards.CreateRejectsOutOfRange
    HeightFieldPoolGuards.LockRequiresExistingEntry
    HeightFieldPoolGuards.UnlockWithoutLockIsNoop
    HeightFieldManagerUtilities.MakePoolKeyEncodesLodAndPosMask
    HeightFieldManagerUtilities.MakePoolKeyHandlesMaxLod
    HeightFieldTilePalette.LoadRejectsNullInputs
)