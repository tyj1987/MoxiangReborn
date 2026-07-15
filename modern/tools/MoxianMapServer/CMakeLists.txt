# MoxianMapServer - Phase 8 P0 per-map game server.
#
# Mirrors the locale build matrix from MoxianAgentServer.
# Each locale target defines the matching preprocessor macros so any
# pulled-in legacy code compiles correctly.

set(_MAP_COMMON_DEFS
    _MAPSERVER_
    _FILE_BIN_
    _USINGTOOL_
    WIN32
    _WINDOWS
    _MBCS
)

foreach(_locale_def IN ITEMS KOR CHINA JAPAN HK TL)
    if(_locale_def STREQUAL "KOR")
        set(_extras "_KOR_LOCAL_")
    elseif(_locale_def STREQUAL "CHINA")
        set(_extras "_CHINA_LOCAL_;TAIWAN_LOCAL")
    elseif(_locale_def STREQUAL "JAPAN")
        set(_extras "_JAPAN_LOCAL_")
    elseif(_locale_def STREQUAL "HK")
        set(_extras "_HK_LOCAL_;_IGNORE_ASSERT_;_TW_LOCAL_;_NPROTECT_")
    elseif(_locale_def STREQUAL "TL")
        set(_extras "_TL_LOCAL_")
    endif()

    set(_all_defs "${_MAP_COMMON_DEFS}")
    foreach(_d IN LISTS _extras)
        list(APPEND _all_defs "${_d}")
    endforeach()

    add_executable(mxh_map_server_${_locale_def} main.cpp)
    target_compile_definitions(mxh_map_server_${_locale_def} PRIVATE ${_all_defs})
    target_link_libraries(mxh_map_server_${_locale_def} PRIVATE
        mxh_server mxh_net mxh_crypto mxh_db mxh_log
    )

    if(MSVC)
        target_compile_options(mxh_map_server_${_locale_def} PRIVATE /W3 /utf-8)
    endif()
endforeach()
