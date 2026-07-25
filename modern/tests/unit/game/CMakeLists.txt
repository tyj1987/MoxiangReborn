add_executable(mxh_game_tests
    game_types_test.cpp
)
target_include_directories(mxh_game_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(mxh_game_tests
    PRIVATE
    mxh_game
    gtest_main
)
if(MSVC)
    target_compile_options(mxh_game_tests PRIVATE /W3 /utf-8)
endif()

include(GoogleTest)
gtest_add_tests(TARGET mxh_game_tests)
