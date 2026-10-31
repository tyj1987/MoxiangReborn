# ROG queue

Checks in this list are not CI passes. A skip is not a pass. Each item
reads the original client tree read-only from `D:\MX` on ROG. Do not copy
those bytes into git, logs, or this document.

Paths that stay in `reference/manifests/playdh-current.sha256.json` but are
not in the git tree are listed only in
`reference/manifests/playdh-external-only.txt`. That file is temporary.
It is deleted together with the repo-tree-versus-manifest comparison, in
whichever phase removes that comparison, at the latest P3 (that phase's
acceptance item). P2 stops using the list.

## Real-file checks

These commands are not registered as required CI tests.
`scripts/verify_real_resources.py` and `scripts/verify_algorithms.ps1` exit 1
when `MXH_PLAYDH_ROOT` is unset or is not a directory. A skip is not a pass.

1. Open every archive named by the external-only list from `D:\MX`,
   read-only. The checkout does not contain those blobs.
2. `scripts/verify_real_resources.py` is rog. It is not a required CI check.
   `MXH_PLAYDH_ROOT` must point at a directory whose layout matches the
   manifest paths level for level. That directory is the PlayDH level, not
   the top of `D:\MX`. The exact ROG path is pending confirmation by Grok
   Bot, the only operator on ROG, and the check is read-only. Until a
   matching directory is confirmed, the 5280 coverage check stays queued
   and does not count as passed. That check is
   `scripts/playdh_baseline_check.py --check --playdh "$MXH_PLAYDH_ROOT"`.
   The run is read-only and reports MATCH, MISSING, MISMATCH, and
   CASE_MISMATCH counts. The operator compares the `D:\MX` file count and
   bytes before and after. If `MXH_PLAYDH_ROOT` is unset or is not a directory,
   the script exits 1. A skip is not a pass.
3. `scripts/verify_algorithms.ps1` is rog and is not a required CI check.
   It uses the same `MXH_PLAYDH_ROOT` PlayDH-level directory. It exits 1
   when that variable is unset or the directory is missing.
4. `modern/tools/compat_round_trip.py` against `.bmhm`, `.ttb`, and `.bsad`
   under `D:\MX`. The unittest
   `modern/tests/unit/tools/test_compat_round_trip.py` is synthetic and
   stays required. It does not open the external-only archives.
5. `scripts/clean-deploy.ps1` already requires `-PlayDhRoot` and does not
   fall back to the repo tree. Its mount-order self-test uses synthetic
   archive names. It does not read the external-only blobs.
