# Build the Item protocol canonical list with the host compiler, then hash it.
# Included from the top-level CMakeLists so MXH_CLIENT_COMPILE_ONLY still
# generates mxh/proto/protocol_hash.hpp and the client manifest.

set(MXH_ITEM_CANON_LIST "${CMAKE_BINARY_DIR}/generated/item_protocol_canonical.txt")
set(MXH_ITEM_CANON_COUNT "${CMAKE_BINARY_DIR}/generated/item_protocol_canonical.count")
set(MXH_ITEM_PROTOCOL_HEADER "${CMAKE_BINARY_DIR}/generated/mxh/proto/protocol_hash.hpp")
set(MXH_ITEM_PROTOCOL_MANIFEST "${CMAKE_BINARY_DIR}/generated/protocol.manifest")
set(MXH_ITEM_PROTOCOL_HASH_SCRIPT "${CMAKE_SOURCE_DIR}/src/proto/MxhItemProtocolHash.cmake")

add_executable(mxh_item_protocol_canon src/proto/item_protocol_canon.cpp)
target_include_directories(mxh_item_protocol_canon PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_features(mxh_item_protocol_canon PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(mxh_item_protocol_canon PRIVATE /W4 /utf-8)
endif()

add_custom_command(
  OUTPUT "${MXH_ITEM_CANON_LIST}" "${MXH_ITEM_CANON_COUNT}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/generated"
  COMMAND "$<TARGET_FILE:mxh_item_protocol_canon>"
          --list "${MXH_ITEM_CANON_LIST}"
          --count "${MXH_ITEM_CANON_COUNT}"
  DEPENDS mxh_item_protocol_canon
          "${CMAKE_SOURCE_DIR}/include/mxh/proto/protocol.hpp"
          "${CMAKE_SOURCE_DIR}/src/proto/item_protocol_canon.cpp"
  VERBATIM
  COMMENT "Emit Item protocol canonical list"
)

add_custom_command(
  OUTPUT "${MXH_ITEM_PROTOCOL_HEADER}" "${MXH_ITEM_PROTOCOL_MANIFEST}"
  COMMAND "${CMAKE_COMMAND}"
          -D "MXH_CANON_INPUT=${MXH_ITEM_CANON_LIST}"
          -D "MXH_CANON_COUNT_FILE=${MXH_ITEM_CANON_COUNT}"
          -D "MXH_CANON_HEADER=${MXH_ITEM_PROTOCOL_HEADER}"
          -D "MXH_CANON_MANIFEST=${MXH_ITEM_PROTOCOL_MANIFEST}"
          -D "MXH_CANON_LOG=1"
          -P "${MXH_ITEM_PROTOCOL_HASH_SCRIPT}"
  DEPENDS "${MXH_ITEM_CANON_LIST}" "${MXH_ITEM_CANON_COUNT}"
          "${MXH_ITEM_PROTOCOL_HASH_SCRIPT}"
  VERBATIM
  COMMENT "Hash Item protocol canonical list"
)

add_custom_target(mxh_item_protocol_hash
  DEPENDS "${MXH_ITEM_PROTOCOL_HEADER}" "${MXH_ITEM_PROTOCOL_MANIFEST}")

add_library(mxh_protocol_hash INTERFACE)
target_include_directories(mxh_protocol_hash INTERFACE "${CMAKE_BINARY_DIR}/generated")
add_dependencies(mxh_protocol_hash mxh_item_protocol_hash)

function(mxh_attach_item_protocol_manifest target)
  add_dependencies(${target} mxh_item_protocol_hash)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${MXH_ITEM_PROTOCOL_MANIFEST}"
            "$<TARGET_FILE_DIR:${target}>/protocol.manifest"
    VERBATIM)
endfunction()
