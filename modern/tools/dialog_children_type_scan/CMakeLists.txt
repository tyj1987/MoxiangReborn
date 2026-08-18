add_executable(mxh_dialog_children_type_scan
    main.cpp
)
target_include_directories(mxh_dialog_children_type_scan PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(mxh_dialog_children_type_scan PRIVATE
    mxh_compat mxh_ui
)
