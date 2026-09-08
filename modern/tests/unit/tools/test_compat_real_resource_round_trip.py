"""Standalone verification helper — NOT registered as a ctest target.

The companion C++ test (`modern/tests/unit/chr_motion_test.cpp`,
`chx_real_resource_test.cpp`, `bmhm_map_test.cpp`) already covers
the real .chr / .chx / .bmhm parsing path end-to-end, so this Python
script is a deferred cross-check rather than a CI gate.  It is
retained in-tree as a starter stub for the next iteration of
real-resource 1:1 verification (the C++ integration test that
covers `ChrModel::load(...)` against mulum.chr / nukim.chr /
younmu00.chr lives at `chx_real_resource_test.cpp` and is the
authoritative lock-in).

Why this file is empty: an earlier draft tried to import
`mxh.compat.chr_motion` from Python, which is a C++ header and
cannot be imported.  Until the C++ side exposes a binding, this
file is intentionally a no-op so it can be tracked without
breaking the build.

Run as a sanity check:
    python modern/tests/unit/tools/test_compat_real_resource_round_trip.py
"""

import sys


def main() -> int:
    print(
        "test_compat_real_resource_round_trip: deferred — see "
        "modern/tests/unit/chr_motion_test.cpp and "
        "chx_real_resource_test.cpp for the real-resource 1:1 "
        "C++ test surface.",
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
