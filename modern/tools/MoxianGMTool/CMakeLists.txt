# MoxianGMTool - Modern GM management tool
#
# Replaces legacy DS_RMTool with a modern C++ HTTP server.

add_executable(MoxianGMTool main.cpp)

target_compile_features(MoxianGMTool PRIVATE cxx_std_17)

if(WIN32)
    target_link_libraries(MoxianGMTool PRIVATE ws2_32)
endif()

if(MSVC)
    target_compile_options(MoxianGMTool PRIVATE /W3 /utf-8)
endif()

install(TARGETS MoxianGMTool RUNTIME DESTINATION bin)
