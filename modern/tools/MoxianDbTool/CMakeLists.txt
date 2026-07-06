add_executable(mxh_db_tool
    main.cpp
)

target_link_libraries(mxh_db_tool PRIVATE mxh_db)

if(MSVC)
    target_compile_definitions(mxh_db_tool PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()