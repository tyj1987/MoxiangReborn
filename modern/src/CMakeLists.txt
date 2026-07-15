add_library(mxh_compat STATIC
    mh_file_ex.cpp
    pack_file.cpp
    bmhm_map.cpp
    ttb_tile_table.cpp
    chx_model.cpp
    chr_motion.cpp
    bsad_area.cpp
)

target_include_directories(mxh_compat PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(mxh_compat PUBLIC cxx_std_20)

if(MSVC)
    target_compile_definitions(mxh_compat PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(mxh_compat PRIVATE /FS /MP)
endif()

# 安装（可选）
install(TARGETS mxh_compat
    ARCHIVE DESTINATION lib
)

# -----------------------------------------------------------------------------
# mxh_db: Database abstraction layer (Phase 2)
# SQLite source: vcpkg (find_package) > vendored amalgamation
# -----------------------------------------------------------------------------

# Check if vcpkg/system provides SQLite3.
find_package(SQLite3 QUIET)

if(SQLite3_FOUND)
    message(STATUS "SQLite3: provided by vcpkg/system (${SQLite3_VERSION})")
    add_library(mxh_db STATIC
        db_adapter.cpp
        sqlite_adapter.cpp
        db_factory.cpp
        mssql_odbc_adapter.cpp
    )
    target_link_libraries(mxh_db PRIVATE SQLite::SQLite3)
    target_include_directories(mxh_db PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    )
else()
    message(STATUS "SQLite3: vendored amalgamation (third_party/sqlite)")
    add_library(mxh_db STATIC
        db_adapter.cpp
        sqlite_adapter.cpp
        db_factory.cpp
        mssql_odbc_adapter.cpp
        ../third_party/sqlite/sqlite3.c
    )
    target_include_directories(mxh_db PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party/sqlite
    )
endif()

# Phase 10.2: Microsoft SQL Server via native Windows ODBC (odbc32.lib).
# Mirrors the legacy [Lib]DBThread ODBC layer. Windows-only — Linux would
# use unixODBC + libiodbc; out of scope for now (the game is Win32-only).
if(WIN32)
    target_link_libraries(mxh_db PRIVATE odbc32)
endif()

target_compile_features(mxh_db PUBLIC cxx_std_20)

if(MSVC)
    target_compile_definitions(mxh_db PUBLIC
        _CRT_SECURE_NO_WARNINGS
        SQLITE_ENABLE_JSON1
        SQLITE_THREADSAFE=1
    )
    if(NOT SQLite3_FOUND)
        # SQLite amalgamation produces a wall of warnings under /W4.
        target_compile_options(mxh_db PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:/W3>
        )
    endif()
endif()

install(TARGETS mxh_db
    ARCHIVE DESTINATION lib
)

# -----------------------------------------------------------------------------
# mxh_net: Networking layer (Phase 4 + Phase 9 cross-platform)
# -----------------------------------------------------------------------------
add_library(mxh_net STATIC
    net/net.cpp
    net/socket.cpp
    # net/iocp/iocp.cpp  # Temporarily disabled due to pre-existing errors
)

target_include_directories(mxh_net PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(mxh_net PUBLIC cxx_std_20)
target_link_libraries(mxh_net PUBLIC mxh_compat)

if(WIN32)
    target_link_libraries(mxh_net PRIVATE Ws2_32)
endif()

install(TARGETS mxh_net ARCHIVE DESTINATION lib)

# -----------------------------------------------------------------------------
# mxh_crypto: HSEL replacement (Phase 3) — AES-256-GCM via Windows CNG
# -----------------------------------------------------------------------------
add_library(mxh_crypto STATIC
    crypto/crypto.cpp
    crypto/hsel_stream.cpp
)

target_include_directories(mxh_crypto PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(mxh_crypto PUBLIC cxx_std_20)
target_link_libraries(mxh_crypto PUBLIC mxh_net)

# bcrypt.dll is loaded dynamically — no static .lib or SDK header needed.

install(TARGETS mxh_crypto ARCHIVE DESTINATION lib)

# -----------------------------------------------------------------------------
# mxh_server: Minimal game server (Distribute + Agent + Map stubs)
# -----------------------------------------------------------------------------
add_library(mxh_server STATIC
    server/login_handler.cpp
    server/agent_handler.cpp
    server/map_handler.cpp
)

target_include_directories(mxh_server PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(mxh_server PUBLIC cxx_std_20)
target_link_libraries(mxh_server PUBLIC mxh_net mxh_crypto mxh_db mxh_monitor)

install(TARGETS mxh_server ARCHIVE DESTINATION lib)

# -----------------------------------------------------------------------------
# mxh_log: minimal logging facility (used by all modules)
# -----------------------------------------------------------------------------
add_library(mxh_log STATIC
    log/mlog.cpp
)
target_include_directories(mxh_log PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}              # so log/mlog.cpp finds mlog.hpp
)
target_compile_features(mxh_log PUBLIC cxx_std_20)

# -----------------------------------------------------------------------------
# mxh_memory: High-performance memory pool (Phase 12.4)
# -----------------------------------------------------------------------------
add_library(mxh_memory STATIC
    memory/memory_pool.cpp
)
target_include_directories(mxh_memory PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(mxh_memory PUBLIC cxx_std_20)
target_link_libraries(mxh_memory PUBLIC mxh_log)

install(TARGETS mxh_memory ARCHIVE DESTINATION lib)

# -----------------------------------------------------------------------------
# mxh_monitor: Performance monitoring (Phase 12.4)
# -----------------------------------------------------------------------------
add_library(mxh_monitor STATIC
    monitor/performance_monitor.cpp
)
target_include_directories(mxh_monitor PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(mxh_monitor PUBLIC cxx_std_20)
target_link_libraries(mxh_monitor PUBLIC mxh_log)

install(TARGETS mxh_monitor ARCHIVE DESTINATION lib)

# -----------------------------------------------------------------------------
# mxh_render: DirectX 11 port of 4Dyuchi GX renderer (Phase 5)
# -----------------------------------------------------------------------------
add_subdirectory(render)

# -----------------------------------------------------------------------------
# mxh_ui: Modern C++ UI framework (Phase 6)
# -----------------------------------------------------------------------------
add_subdirectory(ui)