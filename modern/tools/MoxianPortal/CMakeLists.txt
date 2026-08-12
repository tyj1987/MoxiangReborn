# modern/tools/MoxianPortal/CMakeLists.txt
#
# M5.1 portal HTTP gateway — C++17, no game-server dependencies yet.
# M5.3 will add mxh_server to reach account_service for PBKDF2 auth.

# Static library: shared portal sources
add_library(mxh_portal_lib STATIC
    # Portal sources live in src/portal/ and are compiled into this lib.
    # The executable then links the lib instead of recompiling everything.
)

target_sources(mxh_portal_lib PRIVATE
    ${CMAKE_SOURCE_DIR}/src/portal/config.cpp
    ${CMAKE_SOURCE_DIR}/src/portal/http_server.cpp
    ${CMAKE_SOURCE_DIR}/src/portal/jwt_token.cpp
    ${CMAKE_SOURCE_DIR}/src/portal/rate_limiter.cpp
)

target_include_directories(mxh_portal_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/jwt-cpp/include
)

# C++17 — portal does not need C++20 game-server features
target_compile_features(mxh_portal_lib PRIVATE cxx_std_17)

# M5.1: no mxh_server yet (no auth/DB). M5.3 will add it.
target_link_libraries(mxh_portal_lib PRIVATE mxh_log)
if(WIN32)
    target_link_libraries(mxh_portal_lib PRIVATE bcrypt)
endif()

if(MSVC)
    target_compile_definitions(mxh_portal_lib PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(mxh_portal_lib PRIVATE /W3 /FS /MP)
endif()

# Executable
add_executable(mxh_portal
    main.cpp
)

target_link_libraries(mxh_portal PRIVATE mxh_portal_lib)

# Include paths must be explicit for the executable too (PRIVATE linking)
target_include_directories(mxh_portal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

if(MSVC)
    target_compile_definitions(mxh_portal PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(mxh_portal PRIVATE /W4 /FS /MP)
endif()

# Install
install(TARGETS mxh_portal mxh_portal_lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)
