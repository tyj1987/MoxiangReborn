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

## 端口批准 / Port approvals

经战略专家批准（2026-11-01）：ROG 上的 smoke、E2E 和 soak 的 Map12Port 统一用 28002，长期有效，不必每次单独报批。

条件：每次启动前只读确认 28002 没有被其他进程监听，确认结果写入当次报告；若被占用，停下报战略专家，不得换端口。

除 28002 外，任何其他未列入白名单的端口仍须先批。

适用范围包括已排队的 1h soak 与 TYJ-184（均 pin 在 `2bd89f33`，#115 之前的脚本）。

自 #115（`2dc45123`）起，soak 脚本无 Map12Port 默认值，必须显式传 `-Map12Port 28002`。

端口表（白名单与例外只列在这里）：

- 允许：26001 / 27001 / 28001。28002 是上面这条 Map12Port 批准，同样在白名单内，不再逐次报批。
- 禁止：16001 / 17001 / 18001，以及 36001 / 37001 / 38001。
- 28003 仅 #59 Map17。
- 26011 / 27011 / 28011 仅 CI。

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
