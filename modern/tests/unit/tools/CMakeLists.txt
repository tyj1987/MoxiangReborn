# -----------------------------------------------------------------------------
# mxh_pak_extract_tests — Python unittest covering modern/tools/unpack_pak.py
# (legacy {i:05d}_{real}.bin mode + pak_extract --named mode).
#
# The test script is self-contained: it synthesizes a minimal .pak in a
# temp directory and exercises the parser end-to-end, so it does not depend
# on any real PlayDH resource.
#
# The COMMAND uses `python` resolved from PATH (Windows + POSIX portable).
# CMake 3.20+ supports whitespace-preserving COMMAND lists without an
# intermediate shell, so we just list the args in the right order.
# -----------------------------------------------------------------------------

# Locate python once.  Prefer the explicit vcpkg toolchain; fall back to
# whatever `python` resolves to in PATH at configure time.
find_package(Python3 COMPONENTS Interpreter QUIET)

if(Python3_Interpreter_FOUND)
    set(_MXH_PYTHON_EXE "${Python3_EXECUTABLE}")
else()
    find_program(_MXH_PYTHON_EXE
        NAMES python python3 python3.12 python3.11 python3.10
        DOC "Python interpreter for the pak_extract unit tests"
        REQUIRED)
endif()

add_test(NAME mxh_pak_extract_tests
    COMMAND "${_MXH_PYTHON_EXE}" -m unittest -v
            "${CMAKE_SOURCE_DIR}/tests/unit/tools/test_unpack_pak.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Forward CTest properties so the failure log is readable.
set_tests_properties(mxh_pak_extract_tests PROPERTIES
    LABELS "tools;python"
    TIMEOUT 60)

# -----------------------------------------------------------------------------
# mxh_polish_assets_tests — unit tests for modern/tools/polish_assets.py
# (DDS/TIF writer + 5-class resource generators).  Self-contained, no
# real PlayDH data required.
# -----------------------------------------------------------------------------
add_test(NAME mxh_polish_assets_tests
    COMMAND "${_MXH_PYTHON_EXE}" -m unittest -v
            "${CMAKE_SOURCE_DIR}/tests/unit/tools/polish_assets/test_polish_assets.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_tests_properties(mxh_polish_assets_tests PROPERTIES
    LABELS "tools;python"
    TIMEOUT 180)

# -----------------------------------------------------------------------------
# mxh_render_hfl_tests — unit tests for modern/tools/render_hfl.py
# -----------------------------------------------------------------------------
add_test(NAME mxh_render_hfl_tests
    COMMAND "${_MXH_PYTHON_EXE}" -m unittest -v
            "${CMAKE_SOURCE_DIR}/tests/unit/tools/render_hfl/test_render_hfl.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_tests_properties(mxh_render_hfl_tests PROPERTIES
    LABELS "tools;python"
    TIMEOUT 60)

# -----------------------------------------------------------------------------
# mxh_gen_hfl_placeholders_tests — unit tests for
# modern/tools/gen_hfl_placeholders.py (synthesize missing HFL placeholders
# for maps that have a Map<N>.bmhm but no <N>.hfl on disk).
# -----------------------------------------------------------------------------
add_test(NAME mxh_gen_hfl_placeholders_tests
    COMMAND "${_MXH_PYTHON_EXE}" -m unittest -v
            "${CMAKE_SOURCE_DIR}/tests/unit/tools/gen_hfl_placeholders/test_gen_hfl_placeholders.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_tests_properties(mxh_gen_hfl_placeholders_tests PROPERTIES
    LABELS "tools;python"
    TIMEOUT 60)

# -----------------------------------------------------------------------------
# mxh_audit_minimap_tests — unit tests for
# modern/tools/audit_minimap.py (one-shot health check of PlayDH minimap
# resources: real DDS coverage, encrypted Minimap<N>.bin path index, the
# central image_minimap_path.bin, and the fold-mode dry run that
# polish_assets.py --generate-minimap would perform).
# -----------------------------------------------------------------------------
add_test(NAME mxh_audit_minimap_tests
    COMMAND "${_MXH_PYTHON_EXE}" -m unittest -v
            "${CMAKE_SOURCE_DIR}/tests/unit/tools/audit_minimap/test_audit_minimap.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_tests_properties(mxh_audit_minimap_tests PROPERTIES
    LABELS "tools;python"
    TIMEOUT 60)

# -----------------------------------------------------------------------------
# mxh_integration_minimap_audit_tests — cross-tool consistency ctest.
# Shells out to polish_assets.py --generate-minimap --audit and
# audit_minimap.py --check-fold --json on a synthetic PlayDH tree and
# asserts the two tools report the same existing / known / missing
# counts.  Also asserts that the fold mode (no --audit) actually
# writes only the missing placeholders without clobbering pre-seeded
# real DDS.
# -----------------------------------------------------------------------------
add_test(NAME mxh_integration_minimap_audit_tests
    COMMAND "${_MXH_PYTHON_EXE}" -m unittest -v
            "${CMAKE_SOURCE_DIR}/tests/unit/tools/integration_minimap_audit.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_tests_properties(mxh_integration_minimap_audit_tests PROPERTIES
    LABELS "tools;python;integration"
    TIMEOUT 60)

# -----------------------------------------------------------------------------
# mxh_compat_round_trip_tests — unit tests for
# modern/tools/compat_round_trip.py.  The verifier re-implements the
# legacy CMHFile::CheckCRC signed-i XOR and the modern unsigned-i XOR
# in pure Python so it can byte-diff real PlayDH .bmhm / .ttb / .bsad
# files against the legacy reference.  These unit tests cover the
# classification rules (u16 vs u32 .ttb layout, size-mismatch, BSAD
# acceptance) and lock the documented equivalence between the two
# XOR variants — important because the modern `text_parse.hpp:51` and
# the legacy `(char)i` paths differ in their intermediate arithmetic
# but must produce the same plaintext.
# -----------------------------------------------------------------------------
add_test(NAME mxh_compat_round_trip_tests
    COMMAND "${_MXH_PYTHON_EXE}" -m unittest -v
            "${CMAKE_SOURCE_DIR}/tests/unit/tools/test_compat_round_trip.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_tests_properties(mxh_compat_round_trip_tests PROPERTIES
    LABELS "tools;python;compat"
    TIMEOUT 60)
