cmake_minimum_required(VERSION 3.20)
project(MoxianReborn
    VERSION 0.1.0
    DESCRIPTION "Moxian (DarkStory) - Modernized source tree"
    LANGUAGES CXX)

# Enable ctest at the project root so all test subdirectories register
# their gtest_discover_tests into the same root test list. Without this,
# ctest 4.x refuses to find tests in subdirs that call enable_testing()
# without a parent enable_testing().
enable_testing()

# -----------------------------------------------------------------------------
# Global configuration
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# -----------------------------------------------------------------------------
# Sub-projects
# -----------------------------------------------------------------------------
add_subdirectory(src)
add_subdirectory(src/game)
add_subdirectory(src/client)
add_subdirectory(tests)
add_subdirectory(tools/MoxianResourceExplorer)
add_subdirectory(tools/MoxianDbTool)
add_subdirectory(tools/MoxianLoginServer)
add_subdirectory(tools/MoxianAgentServer)
add_subdirectory(tools/MoxianMapServer)
add_subdirectory(tools/MoxianClient)
add_subdirectory(tools/MoxianClientE2E)
add_subdirectory(tools/MoxianRenderDemo)
add_subdirectory(tools/MoxianPacker)
add_subdirectory(tools/MoxianGMTool)
add_subdirectory(tools/MoxianMapEditor)
add_subdirectory(tools/MoxianAutoPatcher)
add_subdirectory(tools/MoxianProtocolDoc)
add_subdirectory(tools/MoxianSideBySide)
add_subdirectory(tools/MoxianPortal)