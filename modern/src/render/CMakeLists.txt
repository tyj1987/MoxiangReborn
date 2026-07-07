# mxh_render: 4Dyuchi GX renderer port to DirectX 11
# 1:1 method signatures with original DX8 IRenderer; DX11 backend.

add_library(mxh_render STATIC
    dx11/device.cpp
    dx11/primitives.cpp
    dx11/sprite.cpp
    dx11/texture_loader.cpp
    dx11/mesh_object.cpp
    dx11/mesh_shaders.cpp
    dx11/font_object.cpp
    dx11/height_field.cpp
    dx11/hfield_object.cpp
    dx11/effect_shader.cpp
    dx11/renderer.cpp
)

target_include_directories(mxh_render PUBLIC
    ${CMAKE_SOURCE_DIR}/include              # mxh/log, mxh/render public headers (mxh/... style)
    ${CMAKE_SOURCE_DIR}/include/mxh/render   # so #include "render_typedef.hpp" also works
    ${CMAKE_CURRENT_SOURCE_DIR}/dx11         # dx11/* internal headers
)

target_link_libraries(mxh_render
    PUBLIC
        mxh_compat     # for I4DyuchiFileStorage implementation (Phase 5: pack files)
        mxh_log
        d3d11
        dxgi
        d3dcompiler
)

if(MSVC)
    target_compile_options(mxh_render PRIVATE /W3 /utf-8 /FS /MP)
endif()