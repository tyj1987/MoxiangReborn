add_executable(mxh_render_tests
    tga_loader_test.cpp
    mesh_geometry_test.cpp
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
)