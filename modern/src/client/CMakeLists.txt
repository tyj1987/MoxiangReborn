# mxh_client_lib: Modern Moxian client state machine + engine glue
# -----------------------------------------------------------------------------
# Bundles CMainGame, CGameState, CEngine and all client state implementations
# into the static library used by the modern client and its integration tests.
# Login, character select/create, GameIn, loading and map-change states use
# their live protocol/UI implementations; only compatibility states remain
# intentionally lightweight.
# -----------------------------------------------------------------------------
add_library(mxh_client_lib STATIC
    AgentSession.cpp
    ClientSettings.cpp
    crash_dump.cpp
    GameLoadingCoordinator.cpp
    EffectRuntime.cpp
    ClientUiRuntime.cpp
    CharMakeOptions.cpp
    LogicalViewport.cpp
    NetworkEventQueue.cpp
    SpriteRenderGeometry.cpp
    CMainGame.cpp
    CMainTitle.cpp
    CLoginState.cpp
    CCharSelectState.cpp
    CharacterPreviewController.cpp
    CCharMake.cpp
    CInGameState.cpp
    GameStateStubs.cpp
)
target_include_directories(mxh_client_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(mxh_client_lib PUBLIC mxh_ui mxh_render mxh_log mxh_net mxh_crypto)
# The library is also linked by the fixture-driven client test executable.
# Its fixture-only resource fallback is inert in production because the host
# always supplies CEngine::playdh_root().
target_compile_definitions(mxh_client_lib PRIVATE MXH_CLIENT_TEST_BUILD=1)
if(MSVC)
    target_compile_options(mxh_client_lib PRIVATE /W3 /utf-8)
endif()
