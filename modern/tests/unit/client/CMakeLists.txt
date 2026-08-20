add_executable(mxh_client_tests
    cmain_game_test.cpp
    game_state_stubs_test.cpp
    cmain_title_test.cpp
    clogin_state_test.cpp
    ccharselect_state_test.cpp
    cchar_make_state_test.cpp
    cingame_state_test.cpp
    cingame_input_test.cpp
    client_e2e_flow_test.cpp
    network_event_queue_test.cpp
    state_transfer_test.cpp
    sprite_render_geometry_test.cpp
    logical_viewport_test.cpp
    client_ui_runtime_test.cpp
)

target_include_directories(mxh_client_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
)
target_link_libraries(mxh_client_tests
    PRIVATE
    mxh_client_lib
    mxh_log
    mxh_render
    gtest_main
)

if(MSVC)
    target_compile_options(mxh_client_tests PRIVATE /W3 /utf-8)
endif()

include(GoogleTest)
gtest_add_tests(TARGET mxh_client_tests)
