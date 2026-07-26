# mxh_side_by_side: MoxianSideBySide side-by-side verification tool.
# Phase 6.1 of the 12-week plan.
add_executable(mxh_side_by_side
    main.cpp
    packet.cpp
    replay/replay.cpp
    capture/packet_capture.cpp
    diff/packet_diff.cpp
)
target_include_directories(mxh_side_by_side PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
if(MSVC)
    target_compile_definitions(mxh_side_by_side PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(mxh_side_by_side PRIVATE /W3 /utf-8)
    target_link_libraries(mxh_side_by_side PRIVATE ws2_32)
endif()
