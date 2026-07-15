# MoxianAutoPatcher - Modern auto-update tool for Moxian game
#
# Replaces legacy AutoPatchToolWin32 with a modern C++ application.

add_executable(MoxianAutoPatcher main.cpp)

target_compile_features(MoxianAutoPatcher PRIVATE cxx_std_17)

if(WIN32)
    target_link_libraries(MoxianAutoPatcher PRIVATE ws2_32)
endif()

if(MSVC)
    target_compile_options(MoxianAutoPatcher PRIVATE /W3 /utf-8)
endif()

install(TARGETS MoxianAutoPatcher RUNTIME DESTINATION bin)
