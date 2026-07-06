add_executable(mxh_render_tests
    tga_loader_test.cpp
    mesh_geometry_test.cpp
)

target_link_libraries(mxh_render_tests PRIVATE mxh_render gtest_main)

# Skip gtest_discover_tests — Chinese path prevents JSON output in this build env.
# Run tests manually: build/tests/unit/render/Debug/mxh_render_tests.exe
