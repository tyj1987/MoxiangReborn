# Tests use GoogleTest.
# Three modes (in priority order):
#   1. vcpkg:       cmake -DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake  → find_package(GTest)
#   2. Vendored:    third_party/googletest-src/ exists            → add_subdirectory
#   3. FetchContent: fallback → clone from gitee mirror

if(TARGET GTest::gtest)
    # Mode 1: vcpkg already provided GTest via find_package.
    message(STATUS "GoogleTest: provided by vcpkg/system")
else()
    set(GTEST_SRC "${CMAKE_SOURCE_DIR}/third_party/googletest-src")
    if(EXISTS "${GTEST_SRC}/CMakeLists.txt")
        # Mode 2: Vendored copy.
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        add_subdirectory(${GTEST_SRC} ${CMAKE_BINARY_DIR}/_deps/googletest-build EXCLUDE_FROM_ALL)
        message(STATUS "GoogleTest: vendored from third_party/googletest-src")
    else()
        # Mode 3: FetchContent fallback.
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://gitee.com/mirrors/googletest.git
            GIT_TAG        v1.14.0
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
        message(STATUS "GoogleTest: fetched from gitee mirror")
    endif()
endif()

enable_testing()
add_subdirectory(unit)
add_subdirectory(unit/client)
add_subdirectory(unit/compat)
add_subdirectory(unit/db)
add_subdirectory(unit/game)
add_subdirectory(unit/log)
add_subdirectory(unit/memory)
add_subdirectory(unit/monitor)
add_subdirectory(unit/net)
add_subdirectory(unit/proto)
add_subdirectory(unit/render)
add_subdirectory(unit/server)
add_subdirectory(unit/ui)