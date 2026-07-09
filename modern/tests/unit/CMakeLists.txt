add_executable(mxh_compat_tests
    mh_file_ex_test.cpp
    mh_file_ex_utf8_test.cpp
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
    MhFileExUtf8.C34SentinelRoundtrip
    MhFileExUtf8.C34SentinelWriteAndRead
    MhFileExUtf8.LegacyEucKrTitanServerBinDecodesToEucKr
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

# ---------------------------------------------------------------------------------------------------------------------------------------------
# mxh_crypto_benchmark — HSEL vs AES-256-GCM throughput & latency (Phase 3.5)
# ---------------------------------------------------------------------------------------------------------------------------------------------
add_executable(mxh_crypto_benchmark
    crypto_benchmark.cpp
)

target_link_libraries(mxh_crypto_benchmark PRIVATE mxh_crypto)

# ---------------------------------------------------------------------------------------------------------------------------------------------
# mxh_proto negotiation tests — Phase 3.4: cipher negotiation wire protocol
# Header-only library (no compiled sources); test links against the header directly.
# ---------------------------------------------------------------------------------------------------------------------------------------------
add_executable(mxh_proto_tests
    negotiate_test.cpp
)

target_link_libraries(mxh_proto_tests PRIVATE gtest_main)
target_include_directories(mxh_proto_tests PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
)

add_test(NAME mxh_proto_negotiate_tests
    COMMAND $<TARGET_FILE:mxh_proto_tests>)