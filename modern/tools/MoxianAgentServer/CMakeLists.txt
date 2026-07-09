# MoxianAgentServer - Phase 8 AgentServer build matrix.
#
# Mirrors the legacy [Server]Agent/AgentServer_Debug_<LOCALE>.vcxproj
# 5-target split by adding one executable per locale, each defining the
# matching legacy preprocessor macros. The legacy tree has:
#   Debug_KOR   -> _KOR_LOCAL_
#   Debug_CHINA -> _CHINA_LOCAL_ + TAIWAN_LOCAL
#   Debug_JAPAN -> _JAPAN_LOCAL_
#   Debug_HK    -> _HK_LOCAL_ + _IGNORE_ASSERT_ + _TW_LOCAL_ + _NPROTECT_
#   Debug_TL    -> _TL_LOCAL_
#
# All five targets share the same source (main.cpp). The locale-specific
# differences surface only inside #ifdef branches in the legacy code; modern
# code (mxh_server / mxh_net / mxh_db / mxh_crypto) uses modern typed enums
# (mxh::proto::Category, etc.) and does NOT include any legacy [CC]Header/
# files, so the "anonymous enum redefinition" wall that blocks 4/5 Distribute
# Debug targets (C-35) does NOT apply here.

# Common compile definitions present in every AgentServer build (matching
# legacy DistributeServer_*.vcxproj + AgentServer_*.vcxproj).
set(_AGENT_COMMON_DEFS
    _AGENTSERVER_
    _FILE_BIN_
    _USINGTOOL_
    WIN32
    _WINDOWS
    _MBCS
)

# Per-locale extra preprocessor definitions. _AGENT_COMMON_DEFS is always
# applied; the values here are locale-specific extras only. Pattern mirrors
# the legacy [Server]Distribute/CMakeLists.txt locale matrix (Phase 7.5h)
# which uses an if/elseif chain per locale so the locale->defs mapping is
# obvious in one place.
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

    # Build a clean semicolon-separated list of all defines.
    set(_all_defs "${_AGENT_COMMON_DEFS}")
    foreach(_d IN LISTS _extras)
        list(APPEND _all_defs "${_d}")
    endforeach()

    add_executable(mxh_agent_server_${_locale_def} main.cpp)
    target_compile_definitions(mxh_agent_server_${_locale_def} PRIVATE ${_all_defs})
    target_link_libraries(mxh_agent_server_${_locale_def} PRIVATE
        mxh_server mxh_net mxh_crypto mxh_db mxh_log
    )

    if(MSVC)
        target_compile_options(mxh_agent_server_${_locale_def} PRIVATE /W3 /utf-8)
    endif()
endforeach()