# mxh_client_lib: Modern Moxian client state machine + engine glue
# -----------------------------------------------------------------------------
# Phase A.1.6 — bundles CMainGame (state machine) + CGameState (base) +
# CEngine (renderer/network stub) + the 9 eGAMESTATE concrete classes into
# a single static library that the MoxianClient executable links against.
# -----------------------------------------------------------------------------
add_library(mxh_client_lib STATIC
    CMainGame.cpp
    CMainTitle.cpp
    GameStateStubs.cpp
)
target_include_directories(mxh_client_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(mxh_client_lib PUBLIC mxh_render mxh_log)
if(MSVC)
    target_compile_options(mxh_client_lib PRIVATE /W3 /utf-8)
endif()
