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

# -----------------------------------------------------------------------------
# mxh_crypto tests — HSEL stream cipher (Phase 3.2)
# -----------------------------------------------------------------------------
add_executable(mxh_crypto_tests
    hsel_stream_test.cpp
    aes_gcm_test.cpp
)

target_link_libraries(mxh_crypto_tests PRIVATE mxh_crypto gtest_main)

# Use add_test (not gtest_add_tests) to run ALL tests in the binary,
# including parameterized tests (AllTypesSizes/HselRoundTripTest.*).
# gtest_add_tests can't enumerate parameterized test names.
add_test(NAME mxh_crypto_all_tests
    COMMAND $<TARGET_FILE:mxh_crypto_tests>)