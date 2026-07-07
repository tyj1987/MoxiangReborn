# src/proto — Protocol layer (Phase 4 & 3.4)
#
# This library provides:
#   - Typed enums for all 80+ MP_CATEGORY protocol categories
#   - UserConn / Server sub-protocol enums
#   - Cipher negotiation wire format helpers
#
# No link dependencies — pure header-only.

add_library(mxh_proto INTERFACE)
target_include_directories(mxh_proto INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>"
    "$<INSTALL_INTERFACE:include>")

# mxh_proto is header-only. It re-exports:
#   - mxh::proto::Category  (80+ game protocol categories)
#   - mxh::proto::UserConnProtocol / ServerProtocol
#   - mxh::proto::negotiate::  handshake wire helpers
