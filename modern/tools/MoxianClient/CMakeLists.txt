# mxh_client: Modern Moxian client main executable
# -----------------------------------------------------------------------------
# Windows client composition target. The executable owns the Win32 message
# pump and composes the modern state, UI, renderer, audio, and resource
# profile seams; staged world loading and state transitions are implemented in
# main.cpp and the linked client libraries.
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
