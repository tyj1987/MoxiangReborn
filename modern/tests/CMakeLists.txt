# Tests use GoogleTest. Use vendored copy (gitee mirror) since GitHub is blocked.

set(GTEST_SRC "${CMAKE_SOURCE_DIR}/third_party/googletest-src")
if(EXISTS "${GTEST_SRC}/CMakeLists.txt")
    # Use vendored googletest (cloned from gitee.com/mirrors/googletest).
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(${GTEST_SRC} ${CMAKE_BINARY_DIR}/_deps/googletest-build EXCLUDE_FROM_ALL)
else()
    # Fallback: FetchContent from GitHub.
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://gitee.com/mirrors/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()
add_subdirectory(unit)
add_subdirectory(unit/db)
add_subdirectory(unit/net)
add_subdirectory(unit/render)
add_subdirectory(unit/ui)