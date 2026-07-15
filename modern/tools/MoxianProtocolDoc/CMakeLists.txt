# MoxianProtocolDoc - Protocol documentation generator
#
# Parses Protocol.h and generates protocol documentation.

add_executable(MoxianProtocolDoc main.cpp)

target_compile_features(MoxianProtocolDoc PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(MoxianProtocolDoc PRIVATE /W3 /utf-8)
endif()

install(TARGETS MoxianProtocolDoc RUNTIME DESTINATION bin)
