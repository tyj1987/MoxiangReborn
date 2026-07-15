# MoxianMapEditor - Modern map editor for Moxian game
#
# Replaces legacy 4DyuchiGXMapEditor with a modern C++ application.

add_executable(MoxianMapEditor main.cpp)

target_compile_features(MoxianMapEditor PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(MoxianMapEditor PRIVATE /W3 /utf-8)
endif()

install(TARGETS MoxianMapEditor RUNTIME DESTINATION bin)
