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

# Do not pass MXH_REPO_ROOT or any absolute source path into this target.
# The installed player binary discovers a checkout only by walking from
# the current directory. The unit-test target keeps MXH_REPO_ROOT.

install(TARGETS MoxianAutoPatcher RUNTIME DESTINATION bin)
