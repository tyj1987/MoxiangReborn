add_executable(mxh_render_demo main.cpp)
target_include_directories(mxh_render_demo PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/render/dx11"  # for mesh_object.hpp
)
target_link_libraries(mxh_render_demo PRIVATE mxh_render)
if(MSVC)
    target_compile_options(mxh_render_demo PRIVATE /W3 /utf-8)
endif()