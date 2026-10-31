# MoxianAutoPatcher - Modern auto-update tool for Moxian game
#
# Replaces legacy AutoPatchToolWin32 with a modern C++ application.

add_executable(MoxianAutoPatcher main.cpp patch_transaction.cpp patch_rehearsal.cpp)

target_compile_features(MoxianAutoPatcher PRIVATE cxx_std_17)

if(WIN32)
    target_link_libraries(MoxianAutoPatcher PRIVATE ws2_32 bcrypt)
endif()

if(MSVC)
    target_compile_options(MoxianAutoPatcher PRIVATE /W3 /utf-8)
endif()

get_filename_component(MXH_REPO_ROOT "${CMAKE_SOURCE_DIR}/.." ABSOLUTE)
file(TO_CMAKE_PATH "${MXH_REPO_ROOT}" MXH_REPO_ROOT_POSIX)
target_compile_definitions(MoxianAutoPatcher PRIVATE "MXH_REPO_ROOT=\"${MXH_REPO_ROOT_POSIX}\"")

install(TARGETS MoxianAutoPatcher RUNTIME DESTINATION bin)
