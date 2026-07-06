add_executable(mxh_render_tests
    tga_loader_test.cpp
)

target_link_libraries(mxh_render_tests PRIVATE mxh_render gtest_main)

# mxh_render already exposes ${CMAKE_SOURCE_DIR}/src/render/dx11 PUBLIC,
# so the test gets texture_loader.hpp transitively via the link line.
# No extra target_include_directories needed here.

include(GoogleTest)
gtest_discover_tests(mxh_render_tests)
