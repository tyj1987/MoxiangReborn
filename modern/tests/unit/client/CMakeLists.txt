add_executable(mxh_client_tests
    cmain_game_test.cpp
    game_state_stubs_test.cpp
    cmain_title_test.cpp
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
