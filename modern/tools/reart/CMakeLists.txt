cmake_minimum_required(VERSION 3.20)
project(moxiang_reart_probe LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
get_filename_component(MXH_MODERN "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
add_executable(reart_probe native_probe.cpp
  ${MXH_MODERN}/src/stm_static_model.cpp
  ${MXH_MODERN}/src/anm_motion.cpp
  ${MXH_MODERN}/src/chx_model.cpp
  ${MXH_MODERN}/src/hfl_height_field.cpp
  ${MXH_MODERN}/src/mh_file_ex.cpp)
target_include_directories(reart_probe PRIVATE ${MXH_MODERN}/include)
if(MSVC)
  target_compile_options(reart_probe PRIVATE /W4 /EHsc /utf-8)
else()
  target_compile_options(reart_probe PRIVATE -Wall -Wextra -Wpedantic)
endif()
