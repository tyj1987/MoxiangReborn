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

These commands are not registered as required CI tests. When the listed
files are absent from the checkout, print a skip reason. Do not treat the
skip as a pass.

1. Open every archive named by the external-only list from `D:\MX`,
   read-only. The checkout does not contain those blobs.
2. `scripts/verify_real_resources.py` — archive counts for the same paths,
   read-only from `D:\MX` via `MXH_PLAYDH_ROOT`.
3. `scripts/verify_algorithms.ps1` — structure check for the same paths.
   Absent checkout files are skipped.
4. `modern/tools/compat_round_trip.py` against `.bmhm`, `.ttb`, and `.bsad`
   under `D:\MX`. The unittest
   `modern/tests/unit/tools/test_compat_round_trip.py` is synthetic and
   stays required. It does not open the external-only archives.
5. `scripts/clean-deploy.ps1` reads the external PlayDH directory only from
   `MXH_PLAYDH_ROOT` and exits 1 when that variable is unset. It does not
   fall back to `modern/data/PlayDH`. The self-test covers the unset, the
   outside-repo, and the inside-repo cases:

   `pwsh -NoProfile -File scripts/clean-deploy.ps1 -SelfTest`
