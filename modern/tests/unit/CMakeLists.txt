add_executable(mxh_compat_tests
    mh_file_ex_test.cpp
    pack_file_test.cpp
    bsad_area_test.cpp
    chx_real_resource_test.cpp
)

target_link_libraries(mxh_compat_tests PRIVATE mxh_compat gtest_main)

include(GoogleTest)
# See tests/unit/render/CMakeLists.txt for why we don't use gtest_discover_tests.
gtest_add_tests(TARGET mxh_compat_tests
    MhFileEx.RoundtripBasicType0
    MhFileEx.RoundtripType1ExtraSubtraction
    MhFileEx.RejectTooShort
    MhFileEx.Crc8Zero
    MhFileEx.Crc8Basic
    MhFileEx.WriteAndReadBack
    PackFile.FindCaseInsensitive
    PackFile.RejectsNonPakBlob
    PackFile.ParsesRealEffectPak
    PackFile.ParsesAllRealPakFiles
    PackFile.ReadRealEntryData
    BsadArea.Parse3x3CrossShape
    BsadArea.EmptyArea
    BsadArea.RejectsTooSmall
    BsadArea.RejectsOutOfBoundsQuery
    ChxModelRealResource.ManChxIsTextMetadata
    ChxModelRealResource.MonsterBinSmoke
    ChxModelRealResource.CharacterPakFileCount
    ChxModelRealResource.ChxModelParseRejectsTextChx
)