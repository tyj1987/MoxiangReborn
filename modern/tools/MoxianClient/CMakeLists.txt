# mxh_client: Modern Moxian client main executable
# -----------------------------------------------------------------------------
# Phase A.1 skeleton — boots a Windows window, initialises the DX11 renderer
# via the 4Dyuchi IRenderer interface, installs the cImage render adapter
# (so the Phase 6.4 UI framework can draw), and runs a frame loop driven by
# the Win32 message pump. Game state machine wiring (CMainGame + 8 eGAMESTATE
# classes) lands in A.1.6+; for now the host renders a single placeholder
# cDialog so the render seam is exercised end-to-end.
# -----------------------------------------------------------------------------
add_executable(mxh_client WIN32 main.cpp)
target_include_directories(mxh_client PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
)
target_link_libraries(mxh_client PRIVATE mxh_ui mxh_client_lib mxh_render mxh_audio mxh_log)
if(MSVC)
    target_compile_options(mxh_client PRIVATE /W3 /utf-8)
    target_link_libraries(mxh_client PRIVATE d3d11 dxgi d3dcompiler shell32)
endif()
