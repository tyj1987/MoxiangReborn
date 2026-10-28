# mxh_client: Modern Moxian client main executable
# -----------------------------------------------------------------------------
# Windows client composition target. The executable owns the Win32 message
# pump and composes the modern state, UI, renderer, audio, and resource
# profile seams; staged world loading and state transitions are implemented in
# main.cpp and the linked client libraries.
# -----------------------------------------------------------------------------
add_executable(mxh_client WIN32 main.cpp)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(mxh_client PRIVATE MXH_DEV_AUTOMATION=1)
endif()
target_include_directories(mxh_client PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
)
target_link_libraries(mxh_client PRIVATE mxh_ui mxh_client_lib mxh_render mxh_audio mxh_log)
if(MSVC)
    target_compile_options(mxh_client PRIVATE /W3 /utf-8)
    target_link_libraries(mxh_client PRIVATE d3d11 dxgi d3dcompiler shell32)
    # Phase 0 §6.5: GameLoading state allocates ~125 MB for
    # terrain / static / entity scenes.  Combined with CInGameState's
    # UI runtime and lambda captures, peak working set reaches
    # ~1.6 GB on a 32-bit process, which trips std::bad_alloc
    # before the GameIn render finishes.  Marking the binary
    # large-address-aware lets the 32-bit process use up to 4 GB
    # of user-mode address space on 64-bit Windows (the boot
    # option increaseuserva <n> may also be needed on the host for
    # 3 GB+ on a 32-bit Windows kernel; not required on the
    # supported x64 capture host).  See /LARGEADDRESSAWARE in MSDN.
    target_link_options(mxh_client PRIVATE /LARGEADDRESSAWARE)
endif()
