add_executable(mxh_explorer
    main.cpp
)

target_link_libraries(mxh_explorer PRIVATE mxh_compat)

if(MSVC)
    target_compile_definitions(mxh_explorer PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()