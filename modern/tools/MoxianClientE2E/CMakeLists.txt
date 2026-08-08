# mxh_client_e2e: Modern Moxian client end-to-end smoke test.
# -----------------------------------------------------------------------------
# Phase B.2.5 - headless tool that drives CLoginState + CCharSelectState +
# CInGameState against a real 3-server chain.  Uses CreateProcessW to spawn
# LoginServer/AgentServer/MapServer as children, then drives the protocol via
# the modern state classes (not the Python simulator).
#
# B.2.5.3 - also registered as a ctest entry so it runs as part of the
# standard test suite.  Pass --no-spawn if 3 servers are already running
# on :16001/:17001/:18001; default spawns them and tears them down on
# exit.  Uses 16001/17001/18001 (not the dev-mode 6001/7001/8001) so it
# can run alongside a developer's working servers.
# -----------------------------------------------------------------------------
add_executable(mxh_client_e2e main.cpp)
target_include_directories(mxh_client_e2e PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
)
target_link_libraries(mxh_client_e2e PRIVATE mxh_client_lib mxh_db mxh_log)
if(MSVC)
    target_compile_definitions(mxh_client_e2e PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(mxh_client_e2e PRIVATE /W3 /utf-8)
    target_link_libraries(mxh_client_e2e PRIVATE ws2_32)
endif()

# Register as a ctest test.  The tool exits 0 only when all 3 protocol
# steps pass, so ctest's pass/fail reflects the E2E result.  We use
# a 60s timeout because the tool spawns 3 servers (~2s to listen) and
# then runs the protocol (~1s per step) with up to 10s per-step slack.
add_test(NAME MoxianClientE2E COMMAND mxh_client_e2e)
set_tests_properties(MoxianClientE2E PROPERTIES
    TIMEOUT 60
    LABELS "e2e;client;server"
    PASS_REGULAR_EXPRESSION "all 5 protocol steps passed"
    FAIL_REGULAR_EXPRESSION "FAIL|FAIL: timed out"
)
