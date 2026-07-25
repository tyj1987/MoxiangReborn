# mxh_client_e2e: Modern Moxian client end-to-end smoke test.
# -----------------------------------------------------------------------------
# Phase B.2.5 - headless tool that drives CLoginState + CCharSelectState +
# CInGameState against a real 3-server chain.  Uses CreateProcessW to spawn
# LoginServer/AgentServer/MapServer as children, then drives the protocol via
# the modern state classes (not the Python simulator).
# -----------------------------------------------------------------------------
add_executable(mxh_client_e2e main.cpp)
target_include_directories(mxh_client_e2e PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
)
target_link_libraries(mxh_client_e2e PRIVATE mxh_client_lib mxh_log)
if(MSVC)
    target_compile_definitions(mxh_client_e2e PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(mxh_client_e2e PRIVATE /W3 /utf-8)
    target_link_libraries(mxh_client_e2e PRIVATE ws2_32)
endif()
