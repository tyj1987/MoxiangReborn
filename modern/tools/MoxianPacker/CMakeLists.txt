# MoxianPacker - Modern CLI tool for Moxian resource packing
#
# Replaces legacy PackingTool.exe with a modern C++ CLI application.

add_executable(MoxianPacker main.cpp)

target_compile_features(MoxianPacker PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(MoxianPacker PRIVATE /W3 /utf-8)
endif()

install(TARGETS MoxianPacker RUNTIME DESTINATION bin)
