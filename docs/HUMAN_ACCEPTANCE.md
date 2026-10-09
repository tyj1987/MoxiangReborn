# 原版 DX11 人工验收入口

当前可玩性与未完成项以 [PLAYABLE_STATUS.md](PLAYABLE_STATUS.md) 和 [KNOWN_BUGS.md](KNOWN_BUGS.md) 为准。历史 G0–G3 标签、自动登录、截图数量、构建通过和本手册都不能代替真人完整验收。

## 准备

- 在本工作树内准备**同一 source-bound 构建目录**，其中同时存在 `tools/MoxianLauncher/MoxianLauncher.exe` 和 `tools/MoxianClient/mxh_client.exe`。明确传给 `-BuildDirectory`，脚本不会搜索或回落到 `modern/build`。新的 r14 HUD 客户端单独构建时，还须在该目录构建 Launcher 后才能用于此入口。
- 本机只运行 Launcher 与 DX11 Client。原版 Login/Agent/Map 和测试数据库在 PVE VM112/CT930；原版端点是 `192.168.2.244:16001/17001/18001`。PVE Login 通告的 `AdvertisedAgentAddress` 必须是客户端可达的 `192.168.2.244`。脚本拒绝 loopback，且不会启动本机三服、SQL 或注入账号凭据。
- 资源是本工作树的 canonical `modern/data/PlayDH`、profile `playdh-current`；精确清单为 5,131 文件。运行前应单独核验全 SHA 与服务器 writer contract；脚本仅向 Launcher 传资源路径，不把存在目录视为资源验收。
- Windows Debug 构建由脚本补入本机 BuildTools x86 Debug CRT 和 Windows SDK x86 UCRT 的实际目录。没有这些目录时会拒绝启动；也可从正确的 `vcvarsall.bat x86` 环境运行。不会依赖历史 `audit/numeric-r11/runtime` 或 scratch 工具链。

先检查参数，不开 GUI：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\run-human-acceptance.ps1 `
  -PlanOnly -BuildDirectory modern\build-gui-r13
```

实际人工采集须把下面的目录替换为经审查、同时含 Launcher 与 Client 的同源构建：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\run-human-acceptance.ps1 `
  -BuildDirectory modern\<source-bound-build-directory>
```

`-LoginHost`、`-LoginPort`、`-AgentPort`、`-MapPort` 可显式覆盖；默认保持上述 PVE 原版端点。`-MinimumEvidenceFrames` 默认为 0，只在操作者为某次采集明确指定数量时检查采集完整性，不是 GUI 或玩法通过门槛。

## 真人场景与证据

操作者通过 Launcher 做资源检查/设置，手动输入测试账号，正常登录、选/建角、入 Map10。依次实际查看移动与其他玩家、NPC/UI、权威伤害与死亡、自然掉落/获得、换图和重登。按 F12 可在 `modern/out/runs/human/<runId>/evidence` 留下场景截图；截图是辅助材料，不能证明服务端接受了移动、物品落库或重登位置持久化。

完整结论必须把同一次运行的 GUI 可见行为、原版三服的业务事件、隔离测试数据库的角色/库存/状态前后值合在一起核对，并列明失败或未覆盖步骤。自动 smoke、客户端预测坐标、单张截图、物品 fixture 和 `run.json` 的 exit 0 均不得写为真人验收 PASS。尤其要把远端玩家外观、HUD 文字、自然掉落/拾取及位置持久化各自作为独立检查项。

## 收尾和归属

正常完成后回到脚本窗口按 Enter。Launcher 通过 `CreateProcessW` 直接启动 Client；脚本只会把**仍存活且身份匹配**的该 Launcher 子进程纳入清理。身份同时绑定 PID、父 PID、同 API UTC 启动 ticks、可执行文件绝对路径和 SHA-256。关闭前重新核验；仅调用 `CloseMainWindow()` 并有界等待 5 秒，失败或身份变化时保留进程，不使用 `Stop-Process -Force`，也不扫描同名或“本轮之后启动”的并发客户端。

`run.json` 记录构建目录、端点、身份、关闭回执与截图名。`collectionStatus=finished`/exit 0 只说明采集脚本正常结束；`acceptanceStatus=not_evaluated` 一直表示结果仍待人工对照 GUI、服务事件和数据库证据。若 Launcher 已退出，子进程归属无法再可靠验证，脚本会保留客户端并在回执中体现；操作者按确切窗口自行关闭。
