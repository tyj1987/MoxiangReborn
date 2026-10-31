# mxh_client_e2e: Modern Moxian client end-to-end smoke test.
# -----------------------------------------------------------------------------
# Phase B.2.5 - headless tool that drives CLoginState + CCharSelectState +
# CInGameState against a real 3-server chain.  Uses CreateProcessW to spawn
# LoginServer/AgentServer/MapServer as children, then drives the protocol via
# the modern state classes (not the Python simulator).
#
# B.2.5.3 - also registered as a ctest entry so it runs as part of the
# standard test suite.  Default spawns the three servers and tears them
# down on exit.  Ports are not written into this file: mxh_client_e2e_ctest
# reads MXH_E2E_PORT_BASE when ctest starts the test and execs mxh_client_e2e
# with --login-port, --agent-port, --map-port, and --map12-port.
# Unset or empty -> 26001 / 27001 / 28001 / 28002.
# Set to N -> N / N+1000 / N+2000 / N+2001 (N=26001 matches the defaults).
# The harness applies the same rule when those flags are omitted.
# Resolved ports must be in 1024..65535, including N+2001.
# 16001, 17001, 18001, 36001, 37001, and 38001 are refused before
# any server starts.
# -----------------------------------------------------------------------------
add_executable(mxh_client_e2e main.cpp)
target_include_directories(mxh_client_e2e PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
)
target_link_libraries(mxh_client_e2e PRIVATE mxh_client_lib mxh_db mxh_server mxh_log)
add_dependencies(mxh_client_e2e
    mxh_login_server
    mxh_agent_server_CHINA
    mxh_map_server_CHINA
)
if(MSVC)
    target_compile_definitions(mxh_client_e2e PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(mxh_client_e2e PRIVATE /W3 /utf-8)
    target_link_libraries(mxh_client_e2e PRIVATE ws2_32)
endif()

# Reads MXH_E2E_PORT_BASE at test execution and passes the four port flags.
# No client libraries, so the port formula can be built without the servers.
add_executable(mxh_client_e2e_ctest ctest_launch.cpp)
target_include_directories(mxh_client_e2e_ctest PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/support
)
target_compile_features(mxh_client_e2e_ctest PRIVATE cxx_std_20)
if(MSVC)
    target_compile_definitions(mxh_client_e2e_ctest PRIVATE
        _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_compile_options(mxh_client_e2e_ctest PRIVATE /W3 /utf-8)
endif()

# Register as a ctest test.  The tool exits 0 only when all 3 protocol
# steps pass, so ctest's pass/fail reflects the E2E result.  We use
# a 60s timeout because the tool spawns 3 servers (~2s to listen) and
# then runs the protocol (~1s per step) with up to 10s per-step slack.
add_test(NAME MoxianClientE2E COMMAND mxh_client_e2e_ctest
    $<TARGET_FILE:mxh_client_e2e>
    --login-exe $<TARGET_FILE:mxh_login_server>
    --agent-exe $<TARGET_FILE:mxh_agent_server_CHINA>
    --map-exe $<TARGET_FILE:mxh_map_server_CHINA>
    --login-host 127.0.0.1
    # Not selected by ci/windows_behavior_tests.txt or the Linux ctest
    # regexes, so CI does not need MXH_SMOKE_PASSWORD.
    --password-env MXH_SMOKE_PASSWORD
)
set_tests_properties(MoxianClientE2E PROPERTIES
    TIMEOUT 60
    LABELS "e2e;client;server"
    PASS_REGULAR_EXPRESSION "all 5 protocol steps passed"
    FAIL_REGULAR_EXPRESSION "FAIL|FAIL: timed out"
)

add_test(NAME MoxianClientE2EDumpCli COMMAND mxh_client_e2e
    --dump-cli --login-host 192.168.2.107 --no-spawn
)
set_tests_properties(MoxianClientE2EDumpCli PROPERTIES
    TIMEOUT 15
    LABELS "e2e;client"
    PASS_REGULAR_EXPRESSION "login_host=192.168.2.107"
)

# No password argument. The cmake script is the only check: the phrase
# must be present and the child exit code must be 3. Child output is not
# printed. PASS_REGULAR_EXPRESSION is omitted because CTest ignores the
# process exit code when that property is set.
add_test(NAME MoxianClientE2EPasswordRequired
    COMMAND ${CMAKE_COMMAND}
        -DEXE=$<TARGET_FILE:mxh_client_e2e>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/password_required_ctest.cmake
)
set_tests_properties(MoxianClientE2EPasswordRequired PROPERTIES
    TIMEOUT 15
    LABELS "e2e;client"
)
