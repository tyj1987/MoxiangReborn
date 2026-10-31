# Single hash of the Item protocol canonical list.
# Input is Name=Value lines (LF or CRLF). Blank lines and full-line comments
# (# or //) are ignored. Each kept line is trimmed, then the lines are joined
# with LF and a trailing LF. string(SHA256) hashes that text.
# The build passes MXH_CANON_COUNT_FILE. A count of 0, or a count that
# disagrees with that file, fails the build.

if(NOT DEFINED MXH_CANON_INPUT OR MXH_CANON_INPUT STREQUAL "")
  message(FATAL_ERROR "MXH_CANON_INPUT is required")
endif()
if(NOT EXISTS "${MXH_CANON_INPUT}")
  message(FATAL_ERROR "item protocol canonical list is missing: ${MXH_CANON_INPUT}")
endif()

file(READ "${MXH_CANON_INPUT}" _raw)
string(REPLACE "\r\n" "\n" _raw "${_raw}")
string(REPLACE "\r" "\n" _raw "${_raw}")
string(REPLACE "\n" ";" _lines "${_raw}")

set(_canon "")
set(_count 0)
set(_line1 "")
set(_line2 "")
set(_line3 "")
foreach(_line IN LISTS _lines)
  string(STRIP "${_line}" _line)
  if(_line STREQUAL "")
    continue()
  endif()
  if(_line MATCHES "^#" OR _line MATCHES "^//")
    continue()
  endif()
  if(NOT _line MATCHES "^[A-Za-z_][A-Za-z0-9_]*=[0-9]+$")
    message(FATAL_ERROR "bad item protocol canonical line: ${_line}")
  endif()
  math(EXPR _count "${_count} + 1")
  if(_count EQUAL 1)
    set(_line1 "${_line}")
  elseif(_count EQUAL 2)
    set(_line2 "${_line}")
  elseif(_count EQUAL 3)
    set(_line3 "${_line}")
  endif()
  string(APPEND _canon "${_line}\n")
endforeach()

if(_count EQUAL 0)
  message(FATAL_ERROR "item protocol canonical list is empty")
endif()

if(DEFINED MXH_CANON_COUNT_FILE AND NOT MXH_CANON_COUNT_FILE STREQUAL "")
  if(NOT EXISTS "${MXH_CANON_COUNT_FILE}")
    message(FATAL_ERROR "item protocol count file is missing: ${MXH_CANON_COUNT_FILE}")
  endif()
  file(READ "${MXH_CANON_COUNT_FILE}" _expected_raw)
  string(STRIP "${_expected_raw}" _expected)
  if(NOT _expected MATCHES "^[0-9]+$")
    message(FATAL_ERROR "item protocol count file is invalid")
  endif()
  if(_expected EQUAL 0)
    message(FATAL_ERROR "item protocol canonical list is empty")
  endif()
  if(NOT _count EQUAL _expected)
    message(FATAL_ERROR
      "canonical entry count ${_count} != kItemProtocolCount ${_expected}")
  endif()
endif()

string(SHA256 _hash "${_canon}")
string(TOLOWER "${_hash}" _hash)
string(LENGTH "${_hash}" _hash_len)
# CMake's regex engine has no {64} quantifier.
if(NOT _hash_len EQUAL 64 OR NOT _hash MATCHES "^[0-9a-f]+$")
  message(FATAL_ERROR "item protocol sha256 was not 64 lowercase hex")
endif()

if(MXH_CANON_LOG)
  message(STATUS "ITEM_PROTOCOL_CANONICAL_1 ${_line1}")
  message(STATUS "ITEM_PROTOCOL_CANONICAL_2 ${_line2}")
  message(STATUS "ITEM_PROTOCOL_CANONICAL_3 ${_line3}")
  message(STATUS "ITEM_PROTOCOL_SHA256 ${_hash}")
endif()

if(DEFINED MXH_CANON_HASH_OUT AND NOT MXH_CANON_HASH_OUT STREQUAL "")
  get_filename_component(_hash_dir "${MXH_CANON_HASH_OUT}" DIRECTORY)
  if(NOT _hash_dir STREQUAL "")
    file(MAKE_DIRECTORY "${_hash_dir}")
  endif()
  file(WRITE "${MXH_CANON_HASH_OUT}" "${_hash}\n")
endif()

if(DEFINED MXH_CANON_HEADER AND NOT MXH_CANON_HEADER STREQUAL "")
  get_filename_component(_header_dir "${MXH_CANON_HEADER}" DIRECTORY)
  if(NOT _header_dir STREQUAL "")
    file(MAKE_DIRECTORY "${_header_dir}")
  endif()
  file(WRITE "${MXH_CANON_HEADER}"
"#pragma once
// Generated from the compiled Item protocol canonical list. Do not edit.
namespace mxh::proto {
inline constexpr char kItemProtocolSha256[] = \"${_hash}\";
}
")
endif()

if(DEFINED MXH_CANON_MANIFEST AND NOT MXH_CANON_MANIFEST STREQUAL "")
  get_filename_component(_manifest_dir "${MXH_CANON_MANIFEST}" DIRECTORY)
  if(NOT _manifest_dir STREQUAL "")
    file(MAKE_DIRECTORY "${_manifest_dir}")
  endif()
  file(WRITE "${MXH_CANON_MANIFEST}" "item_protocol_sha256=${_hash}\n")
endif()
