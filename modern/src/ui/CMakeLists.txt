# mxh_ui: Modern C++ UI framework (Phase 6)
# -----------------------------------------------------------------------------
# Skeleton build. Phase 6.0 ships the framework + unit tests only; the GPU
# render path, control widgets, and integration with mxh_render land in 6.1+.
# -----------------------------------------------------------------------------
add_library(mxh_ui STATIC
    cWindow.cpp
)

target_include_directories(mxh_ui PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(mxh_ui PUBLIC cxx_std_20)

if(MSVC)
    target_compile_definitions(mxh_ui PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(mxh_ui PRIVATE /FS /MP)
endif()

install(TARGETS mxh_ui ARCHIVE DESTINATION lib)
