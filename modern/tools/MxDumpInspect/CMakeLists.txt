add_executable(mx_dump_inspect mx_dump_inspect.cpp)
target_link_libraries(mx_dump_inspect PRIVATE dbghelp)
if(MSVC)
    target_compile_options(mx_dump_inspect PRIVATE /EHsc /std:c++20 /utf-8)
endif()

# mx_find_throw_site: DIA SDK RVA -> file:line lookup. Used to map
# the GameIn C++ throw site (RVA 0x16A2B4 in mxh_client.exe .text)
# back to modern/src/. Requires msdia140.dll to be present and the
# diaguids.lib import library for the COM type-library stubs.
add_executable(mx_find_throw_site mx_find_throw_site.cpp)
target_include_directories(mx_find_throw_site PRIVATE
    "C:/BuildTools/DIA SDK/include"
)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MX_DIA_LIB_DIR "C:/BuildTools/DIA SDK/lib/amd64")
else()
    set(MX_DIA_LIB_DIR "C:/BuildTools/DIA SDK/lib")
endif()
target_link_directories(mx_find_throw_site PRIVATE "${MX_DIA_LIB_DIR}")
target_link_libraries(mx_find_throw_site PRIVATE
    diaguids
)
if(MSVC)
    target_compile_options(mx_find_throw_site PRIVATE /EHsc /std:c++20 /utf-8)
endif()
