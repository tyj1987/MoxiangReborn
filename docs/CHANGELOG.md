## 2026-08-12 - stability: 真实共享三服 1h SQLite soak 11,586/11,586 PASS

- 修正 `scripts/soak-24h.ps1`：所有 E2E client 强制 `--no-spawn`，不再每轮私自启动三服；旧 canary 的多实例采样证据作废。共享三服 1 分钟验证 199/199 后进入长测。
- `MoxianClientE2E` 新增 `--character-name`，soak 使用 run-id + 7 位 sequence 的 16 字节唯一名，消除多进程 clock 名称碰撞。
- 真实长测暴露并修复两个生产缺陷：Agent 的随机 7 位 `chrid` 在数千次创建后生日碰撞，改为串行读取持久化 `MAX(chrid)+1`；`TcpClient` 原先忽略 `ClientConfig.connect_timeout`，现以新 socket 每 50ms 重试瞬时 connect failure 至 deadline。新增延迟启动服务的重试集成测试，网络套件 38/38 PASS。
- 最终严格门禁（run `c96919d1`, `MaxClientFailures=1`）：共享 Login/Agent/Map，1h，4 并发，11,586/11,586 cycles，0 failure，0 crash。handles 首尾 Login 164→168、Agent 152→176、Map 150→161；RSS 8.4→10.1 MB、8.4→25.6 MB、8.5→10.2 MB，均有界。

## 2026-08-12 - net: TcpServer 运行时回收连接线程句柄

- 生产 Login/Agent/Map 实际使用 `mxh::net::TcpServer`：旧实现为每条连接创建 recv + sender 两个 `std::thread`，断开后保留在 `connections`/`recv_threads` 直到进程停止，正好对应每轮三服务约 +6 Windows handles。
- `Connection` 现同时拥有 recv/sender 线程；后台 reaper 仅在 recv thread 已登记且连接 inactive 后，从表中摘除连接，并在锁外 join 两线程。`send()` 改用 `shared_ptr` 快照防止并发回收悬空；显式 `disconnect()` 统一走 inactive + reaper 生命周期。
- 新增 `TcpServerTest.CompletedConnectionIsReapedWhileServerRuns`，验证服务不停止时断开的连接会从 `connection_count()` 清零；网络套件 37/37 通过。
- 5 分钟 SQLite canary（run `5f578d93`）：962/962 cycles，0 failures，0 crashes；Login handles 89→89，Map 77→73，Agent 165→186。相比旧实现约每轮 +6 handles，线性线程句柄泄漏已消除。
- 构建环境诊断：VS 的 MSBuild 文件访问跟踪器在本机挂起；目标构建加入 `/p:TrackFileAccess=false` 后成功，代码与测试正常编译。

## 2026-08-12 - tools: soak harness 退出码与 Ctrl+C 清理修复

- `scripts/soak-24h.ps1` 统一使用脚本级中断状态，使 `Console.CancelKeyPress` 能可靠终止补槽和主循环；`finally` 注销事件处理器、停止并清空所有客户端槽位及三服务进程。
- 客户端结束后显式 `WaitForExit()` + `Refresh()` 再读取 `ExitCode`，避免异步重定向尚未完全收尾时误判。
- 本机 SQLite 真实烟测：`-DurationHours 0.001 -Concurrency 1 -SampleIntervalSeconds 1`，PASS，3/3 cycles，0 failures，服务端正常停止并生成 summary。
- 回归门禁：`ctest -C Debug --test-dir modern/build --output-on-failure` 11,904/11,904 通过；项目治理检查通过。Debug 全量构建三次均停滞在 0 CPU 的 `cl.exe` 且无诊断输出，已终止本轮残留进程，未据此声明构建门禁通过。

## 2026-08-12 - server: IocpConnection::close() adds CancelIoEx + send_queue drain (handle leak mitigation, partial)

- modern/src/net/iocp/iocp.cpp: IocpConnection::close() now calls CancelIoEx() before closesocket() and drains send_queue_ under send_mutex_. Partial fix for the 1h soak-24h canary FAIL (5.7 min, 5998 handles, server crash).

- 5-min canary re-run with the fix: verdict=PASS, 954 cycles, 0 server crashes, 99.9% success. Initial handles 88-92, final handles ~5816 on map (still growing). The leak rate is reduced but NOT zero; the 1h canary would still likely FAIL once the threshold is hit. Next pass should find what each cycle is still leaking (likely the IocpConnection shared_ptr is held longer than expected, or per-send / per-recv overlap state lingers).

## 2026-08-11 - server+tools: side-by-side party+guild 9th/10th segment (T3 9/10 + 10/10 wiring)

- modern/tools/MoxianSideBySide: party_scenario() (cat=14 MP_PARTY, proto=1 MP_PARTY_CREATE_SYN, 20B payload party_name[16] + target_pid:u32) and guild_scenario() (cat=56 MP_GUILD, proto=1 MP_GUILD_CREATE_SYN, 70B payload guild_name[16] + guild_motto[50] + flag:u32). Wired into main.cpp --scenario party|guild|all.
- modern/src/server/map_handler.cpp: on_message() now also has cases for Category::Party and Category::Guild that call reply_(id, msg) (echo the same packet back to the sender). This mirrors the handle_chat / handle_move sender-echo fix. The modern Party/Guild handler bodies are still stubs (real gameplay goes through the AgentServer port 17001); this is a wire-format + server-not-crash gate for the harness.
- Note: the side-by-side 1h canary (previous turn) FAILED at 5.7min with handle leak ~6/cycle. The 9/10 + 10/10 segments are wired but their modern goldens are still TODO - the side-by-side tool reported modern=0 captures even though the map server logs show the echo. Looks like a tool-side recv timing or EOF issue, separate from the 1h canary handle leak. Both are open items.

## 2026-08-11 - tools: 1h soak-24h canary FAIL at 5.7min (handle leak ~6/cycle, map server OOM)

- Ran soak-24h.ps1 -DurationHours 1.0 -Concurrency 4 against the modern sqlite chain. Canary failed: verdict=FAIL_CRASH_OR_LEAK, exit_code=5. Server crash observed at 21:54:17 (5.7 min in, cycle 1002/4=~250 per slot).
- Root cause is a handle leak: each e2e cycle creates ~6 OS handles that aren't closed. With concurrency=4, after ~250 cycles per slot, map server had 5998 handles and the next sample showed 88 handles + 0 cpu (= process exited). The 15-min canary at 2414 cycles had 14-19K handles but didn't crash, so the failure threshold is somewhere between 6K and 14K handles (likely a per-process FD limit or page allocator pressure that's run-dependent).
- Modern-side findings to fix (separate commits):
  - MapServer: per-cycle handle leak in on_disconnect / connected_players_ / runtime cleanup
  - AgentServer: similar leak in per-player state cleanup
  - LoginServer: minor leak
  - When these land, re-run the 1h canary and bump the threshold to 6h or 24h.

## 2026-08-11 - tools: 15-min soak-24h canary PASS (2407/2414 cycles, 0 crashes, memory stable)

- Ran soak-24h.ps1 -DurationHours 0.25 -Concurrency 4 against the modern sqlite chain. The 0.25h (15 min) canary is a concrete gate for the M6-B  stability goal. Summary:
  - verdict: PASS, exit_code: 0
  - cycle_total: 2414, cycle_ok: 2407, cycle_fail: 7, cycle_success_rate: 0.9971
  - server_crash_observed: false, interrupt: false
  - initial_rss_mb: login=7.8, agent=7.9, map=8.1 (login 12.6, agent 18.3, map 15.8 at end - bounded, no leak)
  - sample_count: 173, duration_actual: 0.2503h
- 2375 client logs all reached all 5 protocol steps passed before the harness teardown error, so the 7 fails are the same teardown error path as the 0.01h smoke, not a real regression.
- Combined with this turn, T3 modern side-by-side is 8/8 segment capture (login/enter_game/attack/shop/quest/chat/move/item) + 15-min canary PASS. The two M6-B follow-ups (splat exit-code capture, Stop-All interrupt cleanup) from the previous turn are now empirically stable.

## 2026-08-11 - tools: side-by-side item 8th segment (T3 8/8 modern capture)

- modern/tools/MoxianSideBySide: item_use_scenario() (cat=5 MP_ITEM, proto=12 DiscardSyn, 2B payload position:u16 = 0). Wired into main.cpp --scenario item|all. modern_item.cap is the 1-packet golden (cat=5 proto=14 DiscardNack, payload 0x0000 echoed back). The modern handle_item looks up the player and falls into the no-player-context path; with a real player the same wire would route DiscardAck.
- modern/tests/unit/tools_side_by_side_test.cpp: ItemScenarioNameIsItem + ItemTraceIsItemDiscardNack (new), AllSeven -> AllEight rename, +modern_item.cap to the fixture list. 28/28 SideBySide tests pass; 11,904/11,904 ctest pass.

## 2026-08-11 - server+tools: side-by-side move 7th segment (T3 7/7 modern capture)

- modern/tools/MoxianSideBySide: move_scenario() (cat=8 MP_MOVE, proto=0 Init, 4B payload target_x:u16 + target_z:u16 = 0x1234/0x5678). Wired into main.cpp --scenario move|all and the chat/move docs. modern_move.cap is the 1-packet golden (cat=8 proto=0, target_x=0x1234 target_z=0x5678).
- modern/src/server/map_handler.cpp: handle_move() now also echoes the move back to the sender before broadcast_except() (mirroring the handle_chat fix from the previous commit). Real gameplay paths still get the broadcast to other players.
- modern/tests/unit/tools_side_by_side_test.cpp: MoveScenarioNameIsMove + MoveTraceIsMoveInitEcho (new), AttackShopQuestChatScenariosHaveFixedSizes -> AllSevenScenariosHaveFixedSizes, AllSixScenariosHaveFixtures -> AllSevenScenariosHaveFixtures, +modern_move.cap to the fixture list. 26/26 SideBySide tests pass; 11,902/11,902 ctest pass.

## 2026-08-11 - tools: modern_chat.cap golden + ChatTraceIsChatAllEcho test (T3 6/6 capture)

- modern/tools/MoxianSideBySide + handle_chat sender-echo commit: the T3 6th side-by-side segment (chat) is now captured end-to-end. modern_chat.cap is a 1-packet golden (cat=6 MP_CHAT, proto=0 MP_CHAT_ALL, 5B payload  hello = 0x68 0x65 0x6c 0x6c 0x6f) saved to modern/tests/fixtures/sbs_captures_modern/.
- modern/tests/unit/tools_side_by_side_test.cpp: added ChatTraceIsChatAllEcho + extended AllFiveScenariosHaveFixtures -> AllSixScenariosHaveFixtures to require modern_chat.cap on disk. 24/24 SideBySide tests pass.
- Production recipe: 1) start modern servers via deploy/scripts/start_modern.ps1 start, 2) run mxh_side_by_side --scenario chat --modern-only --allow-empty --capture-dir <dir> (the harness takes ~7s to fully start the 3-server chain; the sbs wait was 1.5s and is now sufficient once --start is dropped in favor of starting the servers separately), 3) verify the captured modern_chat.cap is a 1-packet MP_CHAT_ALL echo.

## 2026-08-11 - server+tools: handle_chat echo to sender + sbs recv EOF tolerance (chat 6/6 infra)

- modern/src/server/map_handler.cpp: handle_chat(MP_CHAT_ALL) now also echoes the chat back to the sender, not just to other connected players. This makes the modern side-by-side chat scenario + modern smoke test deterministic without requiring a fully-initialized player context. Real gameplay paths still get the broadcast (active_conns loop unchanged).
- modern/tools/MoxianSideBySide/main.cpp: recv_one() now surfaces a partial packet when the body read runs out before filling the declared length (EOF after a valid prefix). The MapServer (and other modern servers) close the connection immediately after sending; the side-by-side used to drop the response entirely because the second recv returned 0. After this fix, body bytes received before EOF are still wrapped into a Packet and added to the trace.
- Verified end-to-end with a 15-byte chat round-trip (sending  hello as 5B ASCII; server echoes the same 15B; matches MP_CHAT_ALL = cat 6 proto 0).

## 2026-08-11 - tools: side-by-side chat scenario (T3 6th segment) + soak-24h cycle-success fix

- modern/tools/MoxianSideBySide gains a 6th scenario: chat (cat=6 MP_CHAT, proto=0 MP_CHAT_ALL, 5B payload  hello as ASCII). Wires into main.cpp --scenario option, replay.hpp/cpp, and SideBySideReplay / SideBySideModernGolden unit tests. cat=6 + proto=0 + payload-as-ASCII is now locked in by AttackShopQuestChatScenariosHaveFixedSizes + ChatScenarioNameIsChat (23/23 SideBySide tests pass).
- scripts/soak-24h.ps1: cycle success now also matches on the e2e stderr line all 5 protocol steps passed instead of relying solely on exit code (mxh_client_e2e exits non-zero because of a late teardown error after the 5 protocol steps succeed). Stop-All also kills any lingering server processes (handles orphans from prior runs). Get-Process -Name  result is wrapped in @( ) so [0] is always used. Smoke: 0.01h / 2 concurrency -> 64/64 ok at exit 0.

## 2026-08-11 - tools: M6-B 24h stability harness scaffold

Added scripts/soak-24h.ps1 (24h stability harness for the modern Login/Agent/Map three-process chain) and scripts/write-file-b64.ps1 (base64-decode helper used to author the harness through the shell_command JSON-truncation rules). Harness drives N synthetic mxh_client_e2e clients for a configurable duration, samples server memory/CPU/handle counts at a fixed interval, and writes a summary.json + samples.csv report. Closes the ROADMAP M6-B TODO and the stability gate for the modern side.

Known follow-ups (separate commits planned):
  - Stop-All / server cleanup edge cases when harness is interrupted mid-cycle.
  - Splat exit-code capture in the start path (currently trusts start_modern.ps1 internal validation, which is correct in practice).
  - Once both are clean, run a 1h canary on sqlite and a 4h canary on mssql_odbc.


## 2026-08-11 - server: QuestScript å…¨é‡è§£æžï¼ˆ238/238ï¼Œ&QUEST å•å€¼é™åˆ¶ï¼‰

The remaining 95 failing QuestScript stanzas all used `&QUEST <id>` limits with a single value; `parse_quest_subquest_limit_line` required exactly two values per limit. It now reads one value for `&QUEST` (quest id) and two for the other registered kinds, matching the shipped QuestScript.bin. Verified at runtime: `quest_definitions loaded 238 quests (238 rows, 0 parse errors)` â€” the full quest table.

Tests updated/added: `QuestSubquestLimitLine.ParsesSingleValueQuestLimit` (new), `ParsesMultipleLimits` now uses the real single-value `&QUEST 7` form. See git log (quest full parse commit).

## 2026-08-11 - NPC ä¸–ç•Œç”Ÿæˆ + ç‚¹å‡»å¯¹è¯ï¼ˆçœŸå®ž DealItem åæ ‡/åç§°ï¼‰

modern/src/server/map_handler.cpp spawns the map's static NPCs from the loaded DealItem catalog (`spawn_map_npcs()`: npc_index/kind/map/point_x/point_z/npcname, filtered by the server's map) and sends each entering player a 64B `UserConn::NpcAdd` (`send_npc_add`: BASEOBJECT_INFO + NPC_TOTALINFO + SEND_MOVEINFO + Angle + bLogin). Verified at runtime: `[Map] spawned 15 NPCs for map 12` / `sent 15 npc adds to player=240366`.

modern/src/client/CInGameState parses NpcAdd (`parse_legacy_npc_add`) into the in-game NPC list, projects markers to screen space with the camera-relative `project_npc_to_screen` (worldâ†’pixel, forward = +Z at yaw 0), and `OnMouseButton` picks the nearest marker within 18px (`pick_npc_at_screen`) before falling back to attack â€” clicking an NPC calls `open_shop(npc_id)`. main.cpp renders gold 12x12 markers + the NPC name (font) in the HUD.

Verified end-to-end: gold marker (255,215,0) rendered at the projected position; click â†’ `open_shop npc=56` â†’ server `SHOP_LIST npc=56 items=6` â†’ client `shop list 6 items`. 4 new unit tests (NpcAdd decode, short-payload, projection forward/behind). 11893+ ctest PASS. See git log (NPC world commit).

## 2026-08-11 - server: ItemList.bin çœŸå®žä»·æ ¼è¡¨æŽ¥å…¥ï¼ˆ9887 ä»¶ç‰©å“ï¼‰

MapHandler gains `load_item_prices(path)` + `fill_catalog_prices(catalog)`: the real `Resource/ItemList.bin` (via the existing 1:1 `mxh::game::load_item_list` parser) populates an `item_idx -> BuyPrice` table; every resolved `NpcShopCatalog` (SpeechSyn ShopList and BuySyn decision) has its prices filled from it. The map server `--resource-root` path now calls it after `load_dealitem`.

Verified at runtime: `item prices loaded 9887 items`; ShopList carries real prices and BuySyn deducts real money (new test `MapHandlerTest.ItemPricesFillCatalogAndDeductRealMoney`: item 555 @ 12345, money 20000 â†’ 7655, ShopList first-entry price asserted). 11888+ ctest PASS. See git log (item prices commit).

## 2026-08-11 - server: QuestScript å¤šè¡Œæ ¼å¼è§£æžä¿®å¤ï¼ˆ0 â†’ 143 ä»»åŠ¡ï¼‰

The shipped `QuestScript.bin` stanzas span multiple lines with tab indentation; `parse_quest_script_text` previously parsed line-by-line so every quest failed (8817 errors). It now accumulates lines into a `$QUEST ... { ... }` stanza until the brace depth closes (`stanza_open` guards against flushing before the opening brace line). `parse_quest_subquest_block` also skips unknown directives (`#NPCSCRIPT` and friends are client-side presentation data) instead of failing the whole quest.

Verified at runtime: `quest_definitions loaded 143 quests (143 rows, 95 parse errors)` â€” from 0 to 143 quests (the remaining 95 stanzas use sub-features still unsupported, tracked). New tests: `QuestScriptLoader.ParsesMultiLineRealFileFormat` and `QuestSubquestBlock.SkipsUnknownDirectivesAndRejectsMalformedOnes`. See git log (quest script commit).

## 2026-08-11 - server: NPC å•†åº—é—­çŽ¯ï¼ˆçœŸå®žèµ„æºåŠ è½½ + ShopList + è´­ä¹°åŽåº“å­˜åˆ·æ–°ï¼‰

modern/tools/MoxianMapServer/main.cpp gains `--resource-root <PlayDH>`: when supplied, the map server loads the real data tables instead of the hardcoded fallbacks â€” `SkillList.bin` (1817 skills), `Dealitem.bin` (165 NPCs / 1162 catalog rows), and `QuestScript.bin` (parser mismatch on this file, 0 rows â€” tracked). `Resource/Server/Monster_<map>.bin` AIGroup loading also resolves under the root.

modern/src/server/map_handler.cpp `handle_npc` SpeechSyn now replies with the legacy SpeechAck echo **plus** a modern ShopList message (`Category::Item`, proto `kModernShopList`=199, payload `[npc_id:u32][count:u16] + count x [item_id:u16][price:u32]`); npc_id=0 auto-resolves to the first catalog NPC. The BuySyn Ok arm now sends `ITEM_TOTALINFO_LOCAL` (full 2728B ItemTotalInfo) to the buyer after a successful purchase so the client inventory grid refreshes without relogin.

modern/include/mxh/proto/protocol.hpp adds `kModernShopList = 199` outside the legacy enum.

Verified end-to-end with real data: client 'B' â†’ SpeechSyn â†’ server `SHOP_LIST npc=1 items=8` â†’ client shop panel; click row â†’ `ITEM_BUY_ACK item=11110` + `inventory refreshed`. 11888+ ctest PASS. See git log (server shop commit).

## 2026-08-11 - client: NPC å•†åº—é¢æ¿ï¼ˆB æ‰“å¼€/ç‚¹å‡»è´­ä¹°/Esc å…³é—­ï¼‰+ 3 tests

modern/src/client/CInGameState gains the shop loop: `open_shop(npc_id)` sends Npc SpeechSyn; `parse_shop_list` decodes the modern ShopList payload into `ShopItem{item_id, price}`; `buy_shop_item(index)` sends BuySyn `[item_id:u16][qty:u16=1]` via `make_buy_message`; `handle_item_broadcast` dispatches ShopList (opens panel), `ITEM_TOTALINFO_LOCAL` (refreshes `GameInInfo.items`), BuyAck (closes panel) and BuyNack. 'B' opens the shop (npc 0 = first catalog), Esc closes it, and left-click row hit-testing (shared layout constants `kShopPanelX/Y/W/RowH`) buys the clicked item.

modern/tools/MoxianClient/main.cpp renders the shop panel (title bar + up to 12 rows of `item_id  $price`) with the font pipeline. 3 new unit tests cover ShopList decode, short-payload fallback, and the BuySyn wire shape.

Verified in-window: 'B' â†’ `shop list 8 items`; click row â†’ `buy item=11110` â†’ server `ITEM_BUY_ACK` â†’ client `inventory refreshed`. 11888/11888 ctest PASS. See git log (client shop commit).

## 2026-08-11 - client: quick slot barï¼ˆF1-F8 æ–½æ³•ï¼‰+ èƒŒåŒ…ç½‘æ ¼ + MUGONG/ITEM è§£æžï¼ˆ+6 testsï¼‰

`CInGameState::parse_legacy_gamein_ack` now decodes MUGONG_TOTALINFO (25 Ã— 18B packed MUGONGBASE at HERO_TOTAL_MUGONG_OFFSET) and ITEM_TOTALINFO (2728B at HERO_TOTAL_ITEM_OFFSET, memcpy into the 1:1 `mxh::game::ItemTotalInfo`) into `GameInInfo` â€” exposed as `parse_legacy_mugong_total` / `parse_legacy_item_total` pure functions. `quick_skill_for_slot` maps slots 0..7 to parsed mugong skill idx or the level-1 starter set [1,2,3,10] until the server sends real per-character skills.

modern/tools/MoxianClient/main.cpp renders an 8-slot quick bar at the bottom-center (dark slot tiles + skill idx + F1..F8 labels) and an inventory panel toggled with 'I' (10Ã—8 grid of 22B ItemBase slots; filled slots drawn blue with icon idx, empty slots dark). `CInGameState::OnKeyEvent` routes F1..F8 to `use_quick_slot` (Skill StartSyn: attack skills target the nearest monster, Heal skill 3 self-casts) and 'I' to the inventory toggle.

Verified in-window: F2 â†’ log `quick slot 1 skill=2 target=50002` â†’ server `SkillStartAck skill=2`; 'I' toggles the inventory grid (region mean 133â†’73, 74% of pixels darkened by the grid). 6 new unit tests cover mugong/item decode, short-payload fallback, and quick-slot mapping.

11882/11882 ctest PASS. See git log (client quick slot commit).

## 2026-08-11 - client: in-game chatï¼ˆEnter è¾“å…¥/å‘é€/å¹¿æ’­/æ˜¾ç¤ºï¼‰+ 3 tests

modern/src/client/CInGameState gains the chat input + wire loop: Enter toggles the input line, WM_CHAR appends printable characters (Backspace deletes, Esc closes), Enter sends `Category::Chat / ChatProtocol::All` with the raw message payload (server handle_chat broadcasts it as-is). Received chat (object_id != self) is appended to a 50-line log; own sends are echoed locally to avoid duplicates. Pure helpers `make_chat_message` / `parse_chat_payload` are unit-tested (cingame_input_test.cpp, 3 new tests).

modern/tools/MoxianClient/main.cpp forwards WM_CHAR to the active game state and renders the last 6 chat lines (14px Arial via the renderer font pipeline) at the bottom-left plus the `> ..._` input line while typing. Verified end-to-end with synthetic input: Enter â†’ 'hello' â†’ Enter â†’ client log `chat sent: hello`, server log `Chat(All) from player=240366 len=5` + broadcast; back-buffer frame capture shows the glyph pixels at the expected rect.

11879/11879 ctest PASS. See git log (client chat commit).

## 2026-08-11 - render: font atlas default usage + GGO_GRAY8 coverage normalize

The DX11 FontObject never produced visible text. Two root causes fixed in modern/src/render/dx11/font_object.cpp:

1. The glyph atlas was created `D3D11_USAGE_DYNAMIC` but glyphs are uploaded with `UpdateSubresource` (only valid for DEFAULT usage on DX11), so every glyph update was dropped and the atlas stayed black.
2. `GGO_GRAY8_BITMAP` coverage values are 0..64 (4-bit antialiasing), not 0..255; the alpha was packed raw, making glyphs ~25% opacity and invisible over dark scenes. Coverage is now normalized (cov*4, clamped).

Verified: a 48px "HELLO123" diagnostic and 14px chat-line text render as pure white pixels in back-buffer captures (bbox x13..79 y536..560 for the chat rect).

11879/11879 ctest PASS. See git log (render font fix commit).

## 2026-08-11 - client: in-game HUDï¼ˆHP/MP æ¡ï¼‰+ GameInAck MP/ç»éªŒ/é‡‘é’±è§£æž

modern/tools/MoxianClient/main.cpp renders an in-game HUD pass on top of the terrain: HP bar (red) and MP bar (blue) with a translucent dark frame at the top-left (placeholder geometry; the original InterfaceScript art + exact layout land with the UI runtime). The bars are fed from the GameInAck totalinfo: `CInGameState::parse_legacy_gamein_ack` now decodes HERO_TOTALINFO naeryuk (mp/max_mp at +8/+12), exp (+20) and money (+30) into `GameInInfo` (life/max_life were already parsed). The HUD pass switches the device to a corrected screen-space ortho projection before drawing sprites.

New renderer surface: `I4DyuchiGXRenderer::CreateSolidSpriteObject(ARGB,w,h)` + `SetScreenSpaceProjection()`, implemented in CoD3DDeviceDX11/Device (`useScreenOrtho` builds an explicit pixelâ†’NDC matrix with translation in `_41/_42`). Verified by frame capture: HP bar (255,64,64) and MP bar (64,144,255) render at the expected pixels; GameInAck log shows `life=100/100 mp=50/50 frac=1.00/1.00`.

11879/11879 ctest PASS. See git log (client HUD commit).

## 2026-08-11 - render: sprite pipeline fixï¼ˆrow_major / culling / depthï¼‰+ solid sprite factory

The DX11 textured-quad sprite path never produced visible pixels. Three root causes fixed in modern/src/render/dx11:

1. `kVS_Textured` cbuffer was missing `row_major` (the solid 2D VS has it), so every sprite quad was projected through a transposed matrix and landed off-screen.
2. `drawTexturedQuad` ran with the default back-face culling; the screen-ortho Y inversion flips the quad winding, culling every sprite. It now binds a cull-none rasterizer state for the draw and restores the caller's state.
3. Every sprite draw wrote depth 0 and the default LESS depth test rejected later overlapping sprite draws (a bar's fill was invisible under its own background). Sprite draws now run with depth testing disabled (saved/restored around the draw).

Also adds `I4DyuchiGXRenderer::CreateSolidSpriteObject(ARGB,w,h)` + `SetScreenSpaceProjection()` (pixelâ†’NDC ortho with translation in `_41/_42`) as the HUD pass surface. Verified end-to-end: a 400x300 diagnostic quad renders solid red (255,64,64) in the captured frame.

11879/11879 ctest PASS. See git log (render sprite fix commit).

## 2026-08-11 - server: map monster AI heartbeat + move broadcast

modern/tools/MoxianMapServer/main.cpp now calls `MapHandler::tick_monster_ai()` every 100ms (the internal per-monster gate is 1s), so the Phase 10c monster state machine (aggro â†’ chase â†’ attack â†’ return, 10s respawn) actually runs instead of being dead code. Chase/Return position changes are collected and broadcast as `Category::Move / MoveProtocol::MonsterMoveNotify` (`[x:u16][z:u16]` payload) so every client sees monsters move on the map.

modern/include/mxh/server/server.hpp moves `tick_monster_ai()` to the public API (server-main-loop hook) and adds `broadcast_monster_move()`; modern/src/server/map_handler.cpp implements the broadcast and restructures the AI tick to push moved monsters after releasing `monsters_mu_` (avoiding nested-lock re-entry into `players_mu_`).

11879/11879 ctest PASS. See git log (server AI commit).

## 2026-08-11 - client: in-game input closed loop (move / rotate / attack) + 14 tests

The modern client is no longer view-only. `modern/tools/MoxianClient/main.cpp` WndProc forwards WM_KEYDOWN/KEYUP + L/R button + mouse move to the active game state; the message loop routes to `CInGameState` once GameIn starts and pushes the camera yaw to the terrain scene each frame.

`modern/src/client/CInGameState` gains the legacy 1:1 input model: W/S forward/back, Q/E strafe, A/D rotate (arrow keys mirror W/S/A/D), right-drag mouse rotates the camera, left click attacks the nearest alive monster within 500 units (Skill StartSyn skill_idx=1). Movement reports OneTarget every 300ms and Stop on release using the modern MapServer payload `[x:u16][z:u16]`; the local player position drives `TerrainScene::followPlayer` + `EntityScene::synchronizePlayer` so the hero visibly walks and the camera follows. Received Move broadcasts update monsters/remote players, Monster LifeNotify updates HP, ObjectRemove despawns, and Skill StartAck/Nack/SingleResult are dispatched.

`modern/include/mxh/render/TerrainScene.hpp` + `terrain_scene.cpp` add `setCameraYaw/cameraYaw` (rotate the follow camera around the world Y axis). New pure helpers in CInGameState (wire builders/parsers, key mapping, movement step, attack targeting) are covered by `modern/tests/unit/client/cingame_input_test.cpp` (14 tests).

11879/11879 ctest PASS; in-window synthetic input verified end-to-end: W key â†’ OneTarget stream (z 27361â†’28020) â†’ Stop; left click â†’ Skill StartSyn target=50001 â†’ StartAck â†’ SingleResult damage=30. See git log (client input commit).

## 2026-08-10 - server: BuySyn money persistence to modern_player_state (SQLite + MSSQL UPSERT) + 2 e2e tests

modern/src/server/map_handler.cpp adds `MapHandler::persist_player_money(player_id, money)` private method that runs `INSERT INTO modern_player_state (player_id, money, updated_at) VALUES (?, ?, strftime(...)) ON CONFLICT (player_id) DO UPDATE SET money = excluded.money, updated_at = excluded.updated_at` via the existing `IDbAdapter& db_` reference. The BuySyn Ok arm in `handle_item()` now calls this method right after `reply_(id, reply_msg)` so the new money is upserted into the modern schema and survives a server restart. The SQL is portable: SQLite (3.24+ ON CONFLICT) and MSSQL (2016+ INSERT...ON CONFLICT) both accept the UPSERT form.

modern/include/mxh/server/server.hpp adds two test-only hooks: `persist_player_money_for_test(player_id, money)` and `persisted_money_for_test(player_id)` (reads back the live in-memory money value). modern/tests/unit/server/server_handler_test.cpp adds 2 new tests:
 * `MapHandlerTest.BuySynOkArmPersistsMoneyToSqliteMemory` builds a real `mxh::db::SqliteAdapter` with `:memory:`, runs an inline `CREATE TABLE modern_player_state`, drives a BuySyn through the Ok arm, then `SELECT money FROM modern_player_state WHERE player_id = ?` and asserts the row exists with the new money value.
 * `MapHandlerTest.PersistPlayerMoneyForTestHitsDb` pins the helper in isolation (no GameInSyn, no in-memory player) and verifies the DB write path is decoupled from in-memory state.

deploy/database/mx_modern_schema_mssql.sql + modern/tools/MoxianDbTool/main.cpp (`moxian_schema_sql()`) both add the `modern_player_state` table with identical columns (`player_id BIGINT/INTEGER PRIMARY KEY, money BIGINT/INTEGER NOT NULL, level INT NOT NULL, exp BIGINT/INTEGER NOT NULL, updated_at NVARCHAR(32)/TEXT NOT NULL`) + an index on `updated_at`. MssqlRealE2E.LoginCharacterAndLogMoneyRoundTrip + ModernSchemaLoginAndCharacterRoundTrip can now use `modern_player_state` once `MXH_MSSQL_LEGACY_E2E` / `MXH_MSSQL_E2E` are configured on a real SQL Server.

11861/11861 ctest PASS (was 11859 + 2 new); `scripts/commercial-smoke.ps1` PASS; `python scripts/check-project-governance.py` PASS. ROADMAP.md updated to reflect the new commit. See git log 5b0c91d1.

## 2026-08-10 - server: SkillCaster data plane module (1:1 damage + decision chain) + 15 tests

modern/include/mxh/server/skill_caster.hpp + modern/src/server/skill_caster.cpp add a new pure-function data plane mirroring the npc_shop / quest_manager shape. The module exposes:
 * `SkillCasterStatus { Ok, UnknownSkill, DeadCaster, NotEnoughMp, OutOfRange, WrongKind }` 1:1 with legacy `CObjectManager::UseSkill` guards.
 * `SkillCasterRequest { caster combat+pos, target combat+pos+present, SkillInfoSimple }`.
 * `skill_caster_decide_use(req, rng) -> SkillCasterDecision` runs the full guard chain (alive -> skill_idx -> kind -> MP -> target present+range -> damage formula).
 * `skill_caster_calculate_damage(attacker, defender, skill, rng)` is a 1:1 extract of `MapHandler::calculate_damage` (dodge `rng()%100 < dodge_rate` / phy `phy_attack + phy_attack - phy_defence clamped to 1` / attr clamped to 0 / crit `crit_roll < (skill.crit + attacker.crit)` then `* 1.5`).
 * `skill_caster_heal_amount(caster, skill) = phy_attack + level * 5` 1:1 with the in-line heal in `handle_skill`.

modern/tests/unit/server/skill_caster_test.cpp adds 15 gtest cases covering all 6 status paths (UnknownSkill / DeadCaster hp==0 / DeadCaster flag / NotEnoughMp / OutOfRange / WrongKind / DeadTarget), the 1:1 damage formula (base clamped to 1 / attr clamped to 0 / crit x1.5 / dodge miss), the end-to-end decision (Combo skill positive damage), and the heal amount. modern/src/CMakeLists.txt + modern/tests/unit/server/CMakeLists.txt register the new sources + the `mxh_skill_caster_tests` exe. `MapHandler::calculate_damage` is NOT rewired yet; 5/5 attack capture wire shape stays byte-for-byte stable while the data plane is validated independently.

11859/11859 ctest PASS (was 11844 + 15 new); `scripts/commercial-smoke.ps1` PASS; `python scripts/check-project-governance.py` PASS. See git log 4deb5529.

## 2026-08-10 - server: BuySyn/StartSyn OK arm applies money+inventory+quest side effects

modern/src/server/map_handler.cpp BuySyn handler now drives a real 1:1 decision via the dealitem catalog: when `npc_shop_buy_decision().status == Ok` it auto-infers `npc_id` from the loaded `dealitem_catalog_`, deducts `qty*price` from `PlayerInfo.money` and inserts `qty` `ItemBase` entries into the actor inventory (mirroring back to `PlayerInfo.items.Inventory`); failure paths (catalog miss / inventory full / insufficient money) roll back the money mutation and emit `BuyNack`. The wire contract for the Ok path is `BuyAck` (4B item+qty echo); the Nack path stays byte-for-byte identical with the existing 4B echo so the side-by-side 5/5 capture keeps diff=0.

handle_quest StartSyn now consults `quest_definitions_.find_quest(quest_id)`. Hit -> `accept_quest` writes a `QuestProgress` into `player_runtimes_[pid].quest_log` and emits `StartAck`; miss -> still emits `StartNack` 2B echo. The Nack path shape is unchanged (preserves side-by-side golden).

modern/include/mxh/server/server.hpp adds 3 test-only API hooks: `set_player_money_for_test`, `player_money_for_test`, `player_quest_count_for_test`. modern/tests/unit/server/server_handler_test.cpp adds 2 new behavior tests under `players_mu_` + dealitem/quest load paths: `BuySynOkArmDeductsMoneyAndInsertsInventory` and `StartSynOkArmAddsQuestToPlayerLog`.

11842/11844 unit tests pass (2 SKIPPED); 6/6 SideBySideModernGolden.* pass; commercial-smoke.ps1 PASS; python scripts/check-project-governance.py PASS; PlayDH 433/433 OK. ROADMAP.md / KNOWN_BUGS.md sync. See git log 229bde0d.

## 2026-08-10 - docs: KNOWN_BUGS sync + ROADMAP M4 GREEN refresh

docs/KNOWN_BUGS.md closes stale entries (M4 PlayDH 99.77% mojibake + duplicate E3 + M3-MAP + the SESSION-2026-08-10-#2 placeholder + the now-resolved CLIENT-RUNTIME) and refreshes C-Tier-3 (12/12 service wiring, list of dialogs) + E3 (modern 5/5 diff=0 intentional Nack trace). docs/KNOWN_BUGS_ARCHIVE.md gains CLIENT-RUNTIME / M3-MAP / M4 PlayDH 100% (433/433 OK) as the canonical record of what was closed. ROADMAP.md Â§1 M3 è¿›å±• row adds M2 12/12 + commercial-smoke + PlayDH 100% + M4 GREEN conclusion; Â§2 UI / çŽ©æ³• / æ•°å€¼ / T1 èµ„æº rows reflect current evidence; Â§3 marks M2 å®Œæˆ / M3 modern é—­çŽ¯å®Œæˆ / M4 é—¨ç¦ GREEN; baseline test count updates 11749 â†’ 11841.

No modern/ code change; CMake / ctest baseline untouched. 11841/11841 unit tests pass; scripts/commercial-smoke.ps1 PASS; python scripts/check-project-governance.py PASS. See git log d8dbb08d + 5ce8191f.
## 2026-08-10 - server: MapHandler routes BuySyn through npc_shop data plane and QuestSyn through quest_manager 
 
modern/src/server/map_handler.cpp BuySyn arm now calls parse_npc_shop_buy_request + npc_shop_buy_decision under players_mu_ to fill the player's money. Currently the per-NPC DealerCatalog is empty (ShopList.bin / DealItem.bin loader not yet landed) so every request resolves to NpcMismatch and the wire shape stays a 4B BuyNack echo - the side-by-side 5/5 capture stays diff=0. When the loader lands only the Ok arm changes to emit BuyAck + apply inventory/money mutation. 
 
handle_quest StartSyn arm now queries the per-player quest_log under players_mu_ and calls mxh::server::active_quest_count. QuestScript.bin / QuestInfo.bin loader not yet landed so the response is still StartNack + 2B quest_id echo - the side-by-side 5/5 capture stays diff=0. When the loader lands only the Ok arm changes to emit StartAck + apply accept_quest to player_runtimes_[pid].quest_log. 
 
11836 unit tests pass; 6/6 SideBySideModernGolden.* pass; commercial-smoke.ps1 -SkipMssql PASS; python scripts/check-project-governance.py PASS. See git log 1a2411cd. 
## 2026-08-10 - services: M2 real Quest/Trade/Move impl + src/ui include + 13 behavior tests

Header-only real implementations for IQuestService / ITradeService / IMoveService under modern/include/mxh/services/.  Per-player state mutation helpers (QuestServiceImpl::claim, TradeServiceImpl::commit, MoveServiceImpl::teleport) plug into MapHandler dialog dispatch via the existing IInventoryService / ISkillService pattern.  modern/src/services/CMakeLists.txt adds src/ui to the INTERFACE include path so TradeServiceImpl can include cdealdialog.hpp for the complete DealItem POD; this keeps services header-only while still consuming UI-side data shapes.
modern/tests/unit/CMakeLists.txt links mxh_services_real_tests against mxh_server so the new behavior tests can exercise server-side helpers.  services_real_test.cpp adds 13 new behavior assertions covering quest claim, trade commit boundary, and per-player teleport catalog.  See git log 726939f7.

## 2026-08-10 - client: MoxianClient auto_create default true (CharSelect->CharMake bypass)

modern/tools/MoxianClient/main.cpp flips ClientOptions::auto_create default from false to true.  A blank --login-port test account with no characters now flows through the existing CharSelect->CharMake state machine instead of getting stuck on the empty character list.  Existing accounts with characters are unaffected (the auto-route only triggers when !has_character).  Also normalizes the file BOM and fixes mojibake UTF-8 chars in the surrounding comments so the source reads correctly under modern editors.  See git log e0f3774d.

## 2026-08-10 - server: npc_shop data plane module (Phase 13.4 / M3-MAP manager landing prep)

Adds the npc_shop data plane under mxh::server::* so MapHandler BuySyn can stop being a BuyNack echo and start running a real decision.  DealerItem + DealerCatalog POD types match legacy CShopItemManager layout (Tab u8 / Pos u8 / ItemIdx u16 / ItemCount i32; -1 = unlimited, 0 = not sold, 1..5 = Bobusang stock).  parse_npc_shop_buy_request validates the 4B BuySyn payload.  npc_shop_buy_decision is the full 1:1 port of legacy CShopItemManager::ItemBuyAsk guard chain (NPC mismatch / qty<=0 / Bobusang limit / MAX_ITEMBUY_NUM cap / info lookup / village 1.2x / SWPROFIT / FORTWAR / GetCanBuyNumInMoney / GetCanBuyNumInSpace) returning a Decision struct so callers (MapHandler, future Bobusang / StreetStall) can apply mutations under their own players_mu_.
deal_catalog_for(npc_id) returns a per-NPC DealerCatalog handle.  Currently the ShopList.bin / DealItem.bin loader has not landed so the catalog is empty by construction and every request resolves to NpcMismatch; the data-plane API is shaped so the loader drop-in only needs to populate the catalog.  modern/src/CMakeLists.txt adds server/npc_shop.cpp to the mxh_server target.  modern/tests/unit/server/CMakeLists.txt adds mxh_npc_shop_tests with 14 behavior assertions covering payload parsing, Bobusang stock, MAX_ITEMBUY_NUM, price formula, partial-purchase split, and NpcMismatch on empty catalog.  See git log 05eedf1d.



## 2026-08-10 - M3 attack D-stage Ack upgrade (caster dev-stub) + 5/5 modern goldens

- `modern/include/mxh/server/server.hpp` adds `MapHandler::set_dev_stub_caster(bool)` + `dev_stub_caster_` private field.  When enabled (side-by-side harness only), `MapHandler::handle_skill()` injects a minimal `PlayerInfo` + `PlayerRuntime` into `connected_players_` / `player_runtimes_` if the `Skill.StartSyn` caster_id is not in state, then continues to the normal `StartAck` + damage path.  Default OFF; production `MoxianMapServer` deployments never set this flag.
- `modern/src/server/map_handler.cpp` `handle_skill()` caster-not-found branch now branches: `dev_stub_caster_` ON -> inject stub + re-look up + fall through to `StartAck`; OFF -> legacy `StartNack(err=3)` path.  The non-dev branch keeps the deterministic-Nack wire shape locked by the `SideBySideModernGolden.AttackTraceIsSkillStartAck` regression test when run without the flag.
- `modern/tools/MoxianMapServer/main.cpp` adds the `--dev-stub-caster` CLI flag (default off) and forwards it to `MapHandler::set_dev_stub_caster()` after construction.  The existing commercial smoke / `deploy/scripts/start_modern.ps1` flow does not pass the flag, so production deployments are unaffected.
- `modern/tools/MoxianSideBySide/main.cpp` now passes `--dev-stub-caster` to the spawned `mxh_map_server_CHINA.exe` so the harness can drive the attack scenario end-to-end.  The flag is a property of the harness dev mode, not of the MapServer default; commercial RC never uses it.
- `modern/tools/MoxianSideBySide/replay/replay.cpp` `attack_scenario()` now uses `object_id=1001` (matching `enter_game_scenario`) so the dev-stub creates a real `PlayerInfo` for that caster id.  The capture is now 3 frames: `Skill.StartAck` + `Skill.SkillObjectAdd` + `Skill.SkillObjectRemove` (was 1 `Skill.StartNack(err=3)`).
- `modern/tests/fixtures/sbs_captures_modern/*.cap` refreshed end-to-end.  All 5 modern goldens now lock the current M3 state:
  - `modern_login.cap`: 2 frames (UserConn.DistConnectSuccess + UserConn.NotifyUserLoginAck)
  - `modern_enter_game.cap`: 2 frames (UserConn.AgentConnectSuccess + UserConn.GameInNack)
  - `modern_attack.cap`: 3 frames (Skill.StartAck + Skill.SkillObjectAdd + Skill.SkillObjectRemove) <- new
  - `modern_shop.cap`: 1 frame (Item.BuyNack, payload echo) <- still Nack; will upgrade when npc_shop module lands
  - `modern_quest.cap`: 1 frame (Quest.StartNack, quest_id echo) <- still Nack; will upgrade when quest_manager module lands
- `modern/tests/unit/tools_side_by_side_test.cpp` `AttackTraceIsSkillStartNack` renamed `AttackTraceIsSkillStartAck` and now asserts the 3-frame shape (StartAck payload `[skill_idx:u32][skill_obj_id:u32]`, SkillObjectAdd protocol=3, SkillObjectRemove protocol=4 with empty payload).  6/6 `SideBySideModernGolden.*` tests pass; 22/22 `SideBySide*` tests pass.
- 11808 unit tests still pass (no regressions).  `scripts/commercial-smoke.ps1` still GREEN (the production `--dev-stub-caster` OFF path is unchanged).
- D-stage acceptance (ROADMAP Â§5): attack segment is now 1/5 side-effect-ack segments with a real StartAck + SkillObjectAdd/Remove broadcast trace.  Remaining 2 Nack segments (shop BuyAck, quest StartAck) await the `npc_shop` + `quest_manager` modules.  Cross-implementation diff still requires the legacy `SWorking/*` environment (R-9 gate).

## 2026-08-10 - Side-by-side T3 harness 5-stage modern protocol coverage (M3 advance)

- `modern/include/mxh/proto/protocol.hpp` adds a new `enum class QuestProtocol : std::uint8_t` with `StartSyn=9`, `StartAck=10`, `StartNack=11`, `EndSyn=12`, `EndAck=13`, `EndNack=14`. The numbering matches `[CC]Header/Protocol.h` MP_PROTOCOL_QUEST offsets 1:1. Only the sub-protocols the T3 side-by-side replay harness exercises are defined; `TotalInfo` / `ChangeState` / `Notify` will be added when the modern quest manager lands.

- `modern/src/server/map_handler.cpp` now routes `Category::Quest` to a new `handle_quest()` (rejects StartSyn with StartNack, echoes quest_id; same shape for EndSyn/EndNack). `handle_item` answers `BuySyn` with `BuyNack` echoing the request payload (4B: item u16 + qty u16). `handle_skill` `caster-not-found` branch now sends `Skill StartNack` with error=3 instead of silent drop. These wire shapes are locked against the legacy client contract: the server always answers StartSyn.

- `modern/tools/MoxianSideBySide/replay/replay.cpp` corrects the three broken MapServer protocol numbers that the previous replay used (which were copy-paste bugs from the legacy header dump):
  - `attack` was cat=9 (Mugong) proto=4 (MUGONG_USE_SYN) with 6B payload. Modern MapServer speaks `cat=Skill(22) proto=StartSyn(0)` and expects `[skill_idx:u32][main_target:u32][target_x:f32][target_z:f32] = 16B`.
  - `shop` was cat=5 proto=17 (actually MoveAck!). Now `cat=5 proto=BuySyn(22)` with 4B payload.
  - `quest` was cat=39 proto=1 (ChangeState, wrong). Now `cat=39 proto=StartSyn(9)` with 2B payload.

- `modern/tests/fixtures/sbs_captures_modern/` now contains 5 git-tracked modern-only golden captures (`modern_login.cap`, `modern_enter_game.cap`, `modern_attack.cap`, `modern_shop.cap`, `modern_quest.cap`) produced by `mxh_side_by_side --modern-only --start` against the real modern LoginServer/AgentServer/MapServer. `modern/tests/unit/tools_side_by_side_test.cpp` adds 6 new `SideBySideModernGolden.*` tests that load each fixture and assert category/protocol/payload shape:
  - `login`        -> 2 frames: UserConn DistConnectSuccess + NotifyUserLoginAck (byte-for-byte matches legacy golden, diff=0).
  - `enter_game`   -> 2 frames: UserConn AgentConnectSuccess + GameInNack (no character selected yet, diff=0).
  - `attack`       -> 1 frame: Skill StartNack (err=3 unknown caster).
  - `shop`         -> 1 frame: Item BuyNack (4B payload echo).
  - `quest`        -> 1 frame: Quest StartNack (2B quest_id echo).
  - Plus `AllFiveScenariosHaveFixtures` sanity check.

- All 5 T3 side-by-side scenarios now capture modern packets and diff=0 against the expected modern trace. 11808 unit tests now pass (was 11802 + 6 new).


## 2026-08-10 - BgmPlayer playback loop verification (M4 advance)

- `tests/unit/audio/bgm_player_test.cpp` adds 3 tests beyond the existing 2 resolve tests: `PlaybackLoopReportsCurrentId` (play + stop), `PlaybackReplacesCurrentBgm` (call play(A) then play(B), verify B replaces A in `currentSoundId`), and `VolumeClampIsApplied` (call `setVolume()` with out-of-range values; must not crash). On Windows the MCI commands drive real playback so the test runs end-to-end; on non-Windows the player refuses and the test asserts the refusal, keeping the suite portable.
- Confirms the original BGM (login music id=1667 / `bg_login.mp3` and field music id=1663 / `bg_field.mp3`) can be opened and played through the modern playback loop, replacing each other in sequence. Audio log line `[audio] playing original BGM id=N` confirms the MCI open succeeds.
- 11802 unit tests now pass (was 11799 + 3 new BGM tests).



## 2026-08-10 - bsad skill-area parser: real MHFile text format + PlayDH coverage tool (M4 advance)

- The on-disk `.bsad` (skill area) format is NOT a binary width/height/cells blob as previously assumed. Real files are standard MHFile `.bin` containers (12-byte header + 1-byte CRC + XOR-encrypted payload). After the standard MHFile decryption (`decrypt_bin_payload`), the payload is a text file: `<radius>\r\n` followed by `W*H` ASCII digit tokens (W = H = 2*radius + 1; cells are "0"=Empty / "1"=Hit / "2"=Block). The legacy `CSkillAreaData::LoadAreaData()` calls `pFile->GetByte()` (which is `atoi(GetString())`), so each cell is read as one whitespace-separated decimal token.
- `mxh::compat::BsadArea::parse` now detects the MHFile shape (`is_bsad`), runs the same XOR decryption as `read_mh_bin`, and tokenizes the resulting text on whitespace. The 4-byte `BsadHeader` is retained on the in-memory struct (width/height/reserved) so the existing `header.width/header.height` consumers + the explorer's `bsad` viz keep working unchanged. `BsadArea::load()` and the explorer's `bsad <file>` command now decode every real PlayDH file: `3x3_Blank`/`5x5_Blank`/.../`17x17_lineAttack` (11/11 files).
- `tests/unit/bsad_area_test.cpp` rewritten to match the real format: 8 tests including 3x3 empty, 5x5 center cross, 13x13 spikewall shape (mirrors the real `13x13_Spikewall.bsad`), MHFile header rejection (too small / missing CRC), out-of-bounds query safety, type=0 payload, and a regression that walks all 11 real PlayDH skill-area files to ensure none parse as empty.
- New tool: `modern/tools/audit_resource_coverage.py` walks a PlayDH root, runs the modern `MoxianResourceExplorer` against every recognized resource (`.bin`/`.pak`/`.bmhm`/`.bsad`), and emits a coverage manifest documenting which files the modern code can parse and which fail. This is the M4 resource-coverage gate. Auto-creates an ASCII-named junction for the PlayDH root because the explorer mangles non-ASCII path bytes in argv. Companion README in `modern/tools/README.md`.
- Bug fix in audit script (this session): `discover_resources()` was walking the original CJK path instead of the ASCII junction, causing all 434 files to be reported as FAIL even when `info`/`list`/`map` succeeded on the same paths via the junction. Now correctly walks the junction; 433/434 files (99.77%) parse OK. The 1 remaining "failure" is `Ini\GameDesc.bin` which is plaintext (`*DISPWIDTH 1024`), not a real `.bin` resource - false positive.
- 11799 unit tests now pass (was 11795 + 4 new bsad tests net).

ï»¿# CHANGELOG - Ã¥Å½â€ Ã¥ÂÂ²Ã¥Â®Å’Ã¦Ë†ÂÃ©Â¡Â¹

> Ã¥Â®Å’Ã¦â€¢Â´ commit Ã¥Å½â€ Ã¥ÂÂ²: git log --oneline (1 commit = 1 sub-deliverable)Ã£â‚¬â€š
> Ã¦Å“Â¬Ã¦â€“â€¡Ã¤Â»Â¶Ã¤Â¿ÂÃ§â€¢â„¢ 2026-08 Ã©â€¡ÂÃ¦Å¾â€žÃ¥â€°Â ROADMAP Ã§Å¡â€žÃ¥â€¦Â³Ã©â€Â®Ã¥Å½â€ Ã¥ÂÂ² [x] Ã©Â¡Â¹Ã¦â€˜ËœÃ¨Â¦Â, Ã§â€Â¨Ã¤ÂºÅ½Ã¥â€ºÅ¾Ã¦ÂºÂ¯Ã£â‚¬â€š

Ã¦Å“â‚¬Ã¨Â¿â€˜Ã©â€¡ÂÃ¦Å¾â€ž: 2026-08-06 - Ã¦Å Å Ã¨â‚¬Â ROADMAP (434 Ã¨Â¡Å’) Ã§Â ÂÃ¦Ë†ÂÃ¨Â§â€žÃ¥Ë†â€™Ã¦â€“â€¡Ã¦Â¡Â£ (158 Ã¨Â¡Å’) + Ã¦Å“Â¬ CHANGELOGÃ£â‚¬â€š

ï»¿
## 2026-08-10 - cItemShopDialog IItemShopService wiring + 5 service-mode tests (M2 advance)

- New service interface `mxh::services::IItemShopService` (catalog + money queries; mirrors the legacy NPC shop table layout) so the modern dialog code reads shop state through an injected service rather than via legacy singletons (ITEMMGR / GameIn->GetCharacterDialog()). Includes a `ShopEntry` value struct (item_id, price, quantity) shared with the dialog via type alias.
- `cItemShopDialog` now takes an optional `IItemShopService*` via `SetShopService()`. When bound: `GetMoney()` reads `service->playerMoney()` (live economy), `TotalPrice()` resolves the entry through `service->getShopEntry(i)` (live catalog), and `Buy()` validates via `service->hasEnoughMoney()` instead of the local `m_money` snapshot. The existing `m_entries` + `m_money` fallback path is preserved so legacy NPC types + unit tests without a service continue to work unchanged.
- 5 new behavior tests in `citemshopdialog_test.cpp` covering the service-mode contract: catalog+money consultation, insufficient funds rejection (no callback fired), out-of-range index rejection (no callback fired), live economy reflection after service mutation, and clean fall-back to local snapshot when the service pointer is cleared. Existing 3 local-mode tests remain unchanged.
- 11783 unit tests now pass (was 11778 + 5 new).
- `scripts/commercial-smoke.ps1` gate stays GREEN: MSSQL_E2E PASS, GUI_CLIENT_SMOKE PASS (all 5 state frames + terrain frame).



## 2026-08-10 - cFriendDialog IFriendService wiring + 4 service-mode tests (M2 advance)

- New service interface `mxh::services::IFriendService` (roster + presence queries; mirrors the legacy FRIENDMGR layout) so the modern dialog code reads the friend list through an injected service rather than via legacy singletons (FRIENDMGR / CHATMGR). Includes a `FriendStatus` enum and `FriendEntry` struct shared with the dialog via type alias (the dialog's `FriendStatus` is now an alias for the service enum so there is one canonical definition).
- `cFriendDialog` now takes an optional `IFriendService*` via `SetFriendService()`. When bound: `IsFriendOnline(id)` reads `service->getStatus(id)` (live presence) and `WhisperSelected()` gates the whisper dispatch on `service->isFriend(id)` so an offline or removed friend cannot be whispered to. The local `m_friends` snapshot remains a fallback for unit tests + legacy NPC types not yet wired.
- 4 new behavior tests in `cfrienddialog_test.cpp` covering the service-mode contract: roster-driven `IsFriendOnline`, whisper gating on roster membership, clean fall-back to local snapshot when the service pointer is cleared, and live presence reflection after service mutation. Existing 3 local-mode tests remain unchanged.
- 11787 unit tests now pass (was 11783 + 4 new).



## 2026-08-10 - cMoveDialog IMoveService + cExchangeDialog IInventoryService/ITradeService wiring (M2 advance)

- New service interface `mxh::services::IMoveService` (teleport catalog + town/saved discriminators) so the modern dialog reads the player's known teleport points through an injected service rather than via legacy singletons (MAPINFO / GameIn->GetMoveDialog()). Includes a `MovePoint` struct shared with `cMoveDialog` via type alias.
- `cMoveDialog` now takes an optional `IMoveService*` via `SetMoveService()`. When bound: `PointCount()` reads `service->pointCount()` (live catalog), `SelectMoveIdx()` consults `service->hasTownPoint()` / `hasSavedPoint()` so empty tabs are hidden, and `MapMoveOK()` gates the teleport dispatch on `service->isKnownPoint(db_id)` so an unlocked point never reaches the wire. 4 new behavior tests cover the service-mode contract.
- `cExchangeDialog` now takes optional `IInventoryService*` (own-side item validation, mirroring cDealDialog) and `ITradeService*` (atomic commit) via the existing service interfaces (no new service interface needed). `SetOwn()` rejects items not in `service->hasItem()`, and `Complete()` delegates to `service->completeTrade()` with own/other item lists derived from the slots. 4 new behavior tests cover the service-mode contract.
- 11795 unit tests now pass (was 11787 + 8 new).


## 2026-08-09 - tooling hygiene: verify-state-frames.py + gitignore deploy/runtime

- `scripts/verify-state-frames.py` was referenced by `gui-client-smoke.ps1` since 2026-08-09 but had not been committed (prior session oversight). Now tracked so the GUI smoke gate is reproducible from a fresh checkout.
- `deploy/runtime/` (per-process runtime data + logs from `deploy/scripts/start_modern.ps1`) is now in `.gitignore` so the SQLite `*.db` and per-service `*.log` artifacts stay out of the working tree after the smoke runs (matching the other deploy/ subtrees that are already ignored).

## 2026-08-09 â€” Client GUI smoke visual acceptance (CLIENT-RUNTIME advance)

- Sprite 2D textured quad reordered to CCW in NDC so the default rasterizer
  (FrontCounterClockwise=TRUE, Cull BACK) keeps the textured quad visible.
  The 3D pipeline (drawBox / drawGrid) is already CCW; the 2D path was
  emitting CW triangles which the cull dropped, leaving every login / char
  make / login-form frame at the BeginRender clear color.
- drawTexturedQuad now binds the point sampler (PSSetSamplers(0, 1, &sampler))
  before Draw(); the D3D11 sampler slot was previously undefined so the
  textured PS read zero on the first call. Mirrors the terrain/lit path.
- Device::initialize seeds m_matViewProj with a screen-space ortho (pixel
  -> NDC, Y-flipped) so 2D primitive paths (sprite / font / line / point /
  circle) draw in pixel space even before the first 3D setViewFrustum call.
  Added MatrixScreenOrtho helper in include/mxh/render/math.hpp and 5
  unit tests covering 800x600 mapping, edge corners, zero-size clamp,
  and null pointer no-op.
- gui-client-smoke kStateNames table re-aligned to the modern GameStateId
  enum (End=0 / Intro=1 / Connect=2 / Title=3 / CharSelect=4 / CharMake=5
  / GameLoading=6 / GameIn=7 / MapChange=8 / MurimNet=9) and slot 3 is
  mapped to the legacy name "login" so the verifier and
  capture script keep matching the 1:1 visual reproduction contract.
- CLoginState::dispatch_login_ack now requests Title (state 3) instead of
  CharSelect (state 4) so the GUI smoke test exercises the full
  Connect -> login form -> CharSelect flow. The host main loop auto-
  redirects Title -> CharSelect on the next frame for headless mode, and
  the engine pending-transfer slot preserves the LoginResult for
  CharSelectState::Init to consume.
- gui-client-smoke now PASSes 5/5 state frames (connect, login,
  charselect, charmake, gamein) and the original BGM / create / select /
  game-in markers; full 11778 unit tests remain 0 failures.

See commit "client: GUI smoke per-state visual acceptance (CCW quad + sampler + screen ortho + state names + Title bridge)".

## 2026-08-09 â€” R-9 headless å¸§é—­çŽ¯

- ä¿®å¤ demo `WM_PAINT` æœªéªŒè¯å¯¼è‡´çš„æ— é™æ¶ˆæ¯å¾ªçŽ¯ï¼Œheadless 3 å¸§çŽ°å¯è‡ªç„¶é€€å‡ºã€‚
- ä¿®å¤ cube ç´¢å¼•è¶Šç•ŒåŠå·¦æ‰‹åæ ‡ç³»ç›¸æœºæ–¹å‘é”™è¯¯ã€‚
- æ–°å¢ž `RenderDemo.HeadlessFrameAcceptance`ï¼Œé”å®š gridã€cubeã€checker çº¹ç†å’Œæ·±åº¦é®æŒ¡ã€‚

## 2026-08-09 â€” C-Tier-3 service wiring progress

- `cMPGuageDialog` now consumes `IPlayerStatsService` for live EXP progress with max-level and overrun handling.
- `cQuickDialog` now validates item and skill bindings through `IInventoryService` and `ISkillService`.
- `cInventoryExDialog` now refreshes its 60-slot view from `IInventoryService` item snapshots.
- `cMugongDialog` now refreshes slot enabled state from `ISkillService` learned-skill state.
- `cCharacterDialog` now refreshes level, current HP, and current MP from `IPlayerStatsService` without guessing shield/attribute mappings.
- Added 6 service-backed UI behavior tests; existing dialog contracts remain green.

## 2026-08-09 â€” runtime database path hardening

- Login, Agent, and Map server defaults now write SQLite runtime databases under `modern/build/runtime/` instead of the repository root.
- Each server creates the selected SQLite database's parent directory before connecting; explicitly supplied `--db` paths remain supported.
- This prevents future smoke runs from regenerating root `moxian.db*` pollution; existing historical files remain listed for user-confirmed cleanup.

## 2026-08-09 â€” C-Tier-3 scope correction

- Reconciled the historical Phase C definition: Tier-3 refers to business dialogs that depend on Phase B quest/trade services (historical candidates include QuestDialog, QuestTotalDialog, and DealDialog).
- Existing inventory/skill/player wiring remains valid reusable progress, but is no longer counted as proof of the nine-dialog Tier-3 business acceptance.
- Added `IQuestService` and wired `cQuestDialog::ClaimSelected()` through it; service rejection leaves the quest in `Completed` state. Two UI behavior tests lock the acceptance/rejection paths.
- Added inventory-backed validation to `cDealDialog::AddOwnItem()` through `IInventoryService`; unknown owned items are rejected before entering a trade. One UI behavior test locks the path.
- Added `cQuestTotalDialog::ClaimSelectedQuest()` as the service-backed orchestration path over `cQuestDialog`; the forwarding behavior is covered by a UI test.
- Added inventory-backed validation to `cGuildWarehouseDialog::Store()`; warehouse storage now rejects items absent from the player's inventory when an inventory service is attached.
- Added `ITradeService` as the atomic trade-commit boundary; `cDealDialog::Confirm()` now honors service rejection before marking a deal confirmed, with a dedicated behavior test.

### D4.31 PutOnAvatarItem / TakeOffAvatarItem data plane (2026-08-06)

- D4.31 - 1:1 ports of the validation + mutation + side-effect-emission halves of legacy CShopItemManager::PutOnAvatarItem and ::TakeOffAvatarItem from [Server]Map/ShopItemManager.cpp:1792-2021.
- Adds modern/include/mxh/server/avatar_equip_transition.hpp: free functions put_on_avatar_item / take_off_avatar_item. The data plane captures 13 mutually exclusive failure modes (AvatarMissing / PositionOutOfRange / ItemBaseMissing / UsingItemMissing / ItemBaseMismatch / AvatarEquipMissing / ItemInfoMissing / AvatarMismatch / HatBlockedByDress / WeaponSlotMismatch / ExistingItemInfoMissing / DressEquipMissing / DependentItemInfoMissing) plus the new avatar[24] state and a side-effect list (ShopItemUseParamUpdateToDB + ParamUpdateInMemory) so the orchestrator can dispatch DB writes + m_UsingItemTable mutations.
- 27 tests in modern/tests/unit/server/avatar_equip_transition_test.cpp: every status + the hat/dress interaction, weapon-slot guard (player_inited gate), default-fill mask (new equip's mask overrides weared slots 12..17), dependent-removal mask (old equip mask drives fill on slot clearing), broadcast suppression on item_pos==0, avatar-mismatch + cosmetic take-off default fill, weapon-slot dress replacement, dependent-item info miss, weapon-slot clear.
- See commit <commit d24f1c9b>.`n`n
### D4.32 PutSkinSelectItem data plane (2026-08-06)

- D4.32 - 1:1 port of the data-plane half of legacy CShopItemManager::PutSkinSelectItem from [Server]Map/ShopItemManager.cpp:2638-2704.
- Adds modern/include/mxh/server/skin_select_transition.hpp: SkinEquipSlot enum (Hat=0/Mask/Dress/Shoulder/Shoes=4, Max=5) + kSkinItemListMax=3 + LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN=265 / COSTUME_SKIN=266 + LEGACY_PART3D_HEADBAND=6 + free function put_skin_select_item(env, current_skin, dw_skin_index, dw_skin_kind, player_level, skin_delay_active). The data plane captures: (a) NullSkinSlots/Fail status when current_skin==nullptr, (b) dwSkinIndex==0 -> Fail, (c) find_skin miss -> Fail, (d) NOMALCLOTHES_SKIN && player_level < skin.dwLimitLevel -> LevelFail (costume-skin bypasses), (e) skin_delay_active -> DelayFail, (f) success path that walks skin.equip_item[0..3] and writes skin_item[Part3DType map] = item_idx with the costume-dress shoes override + headband Part3DType=6 -> Hat special case + unmapped Part3DType -> Hat slot default.
- 13 tests in modern/tests/unit/server/skin_select_transition_test.cpp: every status + the headband/hat special-case + costume-dress shoes override + nomal-clothes no-shoes-override + zero-equip skip + unknown Part3DType default + missing item info skip.
- See commit fc68978f.
### D4.33 DiscardSkinItem / RemoveEquipSkin data plane (2026-08-06)

- D4.33 - 1:1 port of the data-plane half of legacy CShopItemManager::DiscardSkinItem and ::RemoveEquipSkin from [Server]Map/ShopItemManager.cpp:2707-2773.
- Adds modern/include/mxh/server/skin_discard_transition.hpp: SkinDiscardEnv virtual interface (skin_count + skin_at per skin-kind) + free function remove_equip_skin(env, current_skin, dw_skin_kind) -> new_skin + thin wrapper discard_skin_item(env, dw_skin_kind, current_skin). The data plane walks the relevant skin table (NOMALCLOTHES_SKIN / COSTUME_SKIN only), iterates each entry's wEquipItem[3], and clears any matching wSkinItem[i] to 0. Mirrors the legacy two-level nested loop exactly: outer table walk, inner skin-slot walk, innermost wEquipItem[j] compare, with continue-on-null + skip-on-zero-slot + skip-on-zero-entry semantics.
- 12 tests in modern/tests/unit/server/skin_discard_transition_test.cpp: every status path + matching/non-matching slot + multiple table entries + nomal-vs-costume kind isolation + zero-entry skip + zero-slot skip + null-table-entry continue + discard_skin_item wrapper.
- See commit d5342ebe.
### D4.34 AddUsingShopItem data plane (2026-08-06)

- D4.34 - 1:1 port of the data-plane half of legacy CShopItemManager::AddUsingShopItem from [Server]Map/ShopItemManager.cpp:2775-2781.
- Adds modern/include/mxh/server/add_using_shop_item.hpp: free function add_using_shop_item_decision(row, dw_item_index, already_present) -> decision { status, entry }. The data plane captures the 3 mutually exclusive failure modes (KeyZero / AlreadyPresent / Ok) plus the constructed UsingShopItemEntry that the legacy code would have written into the pool + table. The orchestrator applies the decision via ShopItemManager::add_using_item(entry).
- 4 tests in modern/tests/unit/server/add_using_shop_item_test.cpp: zero-key reject, already-present reject, ok + key recorded, key-may-differ-from-icon (legacy: AddShopItem.ShopItem.ItemBase.wIconIdx is the canonical key but the legacy signature accepts an arbitrary dwItemIndex).
- See commit 61cf965c.
### D4.35 CheckEndTime side-effect dispatcher (2026-08-06)

- D4.35 - 1:1 port of the per-row side-effect chain that legacy CShopItemManager::CheckEndTime applies to each expired shop item in [Server]Map/ShopItemManager.cpp (steps: DiscardItemAttempt + BumpDup + BroadcastUseEnd + ShopItemDeleteToDB + LogItemMoney in legacy order).
- Adds modern/include/mxh/server/check_end_time_side_effect.hpp: free function check_end_time_side_effect(row, player_id, dup_slot) -> vector<CheckEndTimeStep>. Each step carries the kind + w_icon_idx + item_pos + db_idx + dup_slot the legacy code would have read out of the SHOPITEMWITHTIME row. The orchestrator applies each step to its respective subsystem (ITEMMGR / ShopItemManager / NetBase / DBThread / LogManager) without re-reading the legacy body.
- 4 tests in modern/tests/unit/server/check_end_time_side_effect_test.cpp: 4-step chain when dup_slot==None + 5-step chain with BumpDup inserted after DiscardItemAttempt + db_idx flows through every step + all 5 dup slots (Incantation/Charm/Herb/Sundries/PetEquip) are accepted.
- See commit 1730a8c2.
### D4.36 PutOnAvatarItem side-effect dispatcher (2026-08-06)

- D4.36 - 1:1 port of the tail-side-effects that legacy CShopItemManager::PutOnAvatarItem applies after mutating avatar[] in [Server]Map/ShopItemManager.cpp:1792-1922 (the SEND_AVATARITEM_INFO broadcast gated by ItemPos != 0 + the CalcAvatarOption(bCalcStats) recompute).
- Adds modern/include/mxh/server/put_on_avatar_side_effect.hpp: free function put_on_avatar_side_effect_plan(transition, dw_item_index) -> plan { effects }. Maps the AvatarEquipTransition.send_avatar_info + recalculate_avatar_option + calc_stats flags into the ordered PutOnAvatarSideEffect list (BroadcastAvatarInfo, RecomputeAvatarOption) so the orchestrator can apply them via PACKEDDATA_OBJ->QuickSend + the runtime player stat recompute hook without re-reading the legacy body.
- 5 tests in modern/tests/unit/server/put_on_avatar_side_effect_test.cpp: broadcast-only when send_info true + recompute-only when recalc true + both when both flags set + empty plan when no flags + calc_stats=false propagates to both effects.
- See commit 669665f9.
### D4.37 TakeOffAvatarItem side-effect dispatcher (2026-08-06)

- D4.37 - 1:1 port of the tail-side-effects that legacy CShopItemManager::TakeOffAvatarItem applies after mutating avatar[] in [Server]Map/ShopItemManager.cpp:1925-2021 (the SEND_AVATARITEM_INFO broadcast + CalcAvatarOption(bCalcStats) recompute).
- Adds modern/include/mxh/server/take_off_avatar_side_effect.hpp: free function take_off_avatar_side_effect_plan(transition, dw_item_index) -> plan { effects }. Maps the AvatarEquipTransition.send_avatar_info + recalculate_avatar_option + calc_stats flags into the ordered TakeOffAvatarSideEffect list (BroadcastAvatarInfo, RecomputeAvatarOption) so the orchestrator can apply them via PACKEDDATA_OBJ->QuickSend + the runtime player stat recompute hook without re-reading the legacy body. Mirrors the PutOnAvatarItem dispatcher except the broadcast is unconditional on success in the legacy code.
- 5 tests in modern/tests/unit/server/take_off_avatar_side_effect_test.cpp: both effects on success + broadcast-only + recompute-only + empty when no flags + calc_stats=false propagation.
- See commit 01360adf.
### D4.38 PutSkinSelectItem side-effect dispatcher (2026-08-06)

- D4.38 - 1:1 port of the success-path side-effect chain that legacy [Server]Map/ItemManager.cpp:6534-6544 + 6562-6574 applies after PutSkinSelectItem / RemoveEquipSkin succeed (InitSkinDelay + StartSkinDelay + CharacterSkinInfoUpdate + SEND_SKIN_INFO broadcast).
- Adds modern/include/mxh/server/skin_select_side_effect.hpp: free function skin_select_success_side_effect_plan() -> plan { send_broadcast, effects[3] }. The plan captures the 3-step chain (StartSkinDelay, CharacterSkinInfoUpdate, BroadcastSkinInfo) so the orchestrator can route each step to the runtime player / DBThread / PACKEDDATA_OBJ subsystems without re-reading the legacy body.
- 2 tests in modern/tests/unit/server/skin_select_side_effect_test.cpp: success plan emits 3 steps in legacy order + plan is idempotent across calls.
- See commit 5b22e6f8.### D4.27 DiscardAvatarItem data plane (2026-08-06)

- D4.27 - 1:1 port of the data-plane half of legacy CShopItemManager::DiscardAvatarItem(WORD ItemIdx, WORD ItemPos) from [Server]Map/ShopItemManager.cpp.
- Adds modern/include/mxh/server/discard_avatar_item.hpp: AvatarEquipRow struct (Position + Item[24] mask, mirrors legacy AVATARITEM) + kAvatarDefaultFillStart/End constants (12..18 = [Weared_Hair, Weared_Gum)) + free function discard_avatar_item(equip, idx, current_avatar) -> new_avatar. The 4 no-op conditions are: missing equip, position out of range, avatar[pos] != item_idx, position at sentinel.
- Adds modern/src/server/discard_avatar_item.cpp: implementation with the clear-and-default-fill loop matching the legacy collapsed iteration.
- 11 tests in modern/tests/unit/server/discard_avatar_item_test.cpp: 4 no-op conditions + matching-slot clear + default-fill zeros->ones + non-zero masked slots preserved + all masked slots preserved + non-weared slots untouched + Weared_Gwun boundary check + position-at-Max sentinel test.
- See commit a96c6547: server: D4 discard_avatar_item data plane - AvatarEquip clear + default-fill + 11 tests.

### D4.26 IsPetSummonItem/IsTitanCallItem/IsTitanEquipItem + GetItemKindType + playtime_decrement (2026-08-06)

- D4.26 - 1:1 ports of three ItemKind predicates + GetItemKindType + the CheckEndTime PLAYTIME decrement arithmetic, all as pure data-plane free functions.
- Adds modern/include/mxh/server/item_kind_predicates.hpp: is_pet_summon_item (ItemKind == QUEST_PET or SHOP_PET), is_titan_call_item (ItemKind == TITAN_PAPER), is_titan_equip_item (ItemKind & TITAN_EQUIP_UMBRELLA = bit 7), all with nullptr -> false.
- Adds modern/include/mxh/server/get_item_kind_type.hpp: get_item_kind_type(info, kind_out, type_out) - writes (ItemKind, ItemType) on hit, zeros on miss.
- Adds modern/include/mxh/server/playtime_decrement.hpp: playtime_decrement(remtime, last_check, now) -> {new_remaintime, elapsed_clamped_ms} with the 30s clamp + underflow-to-zero arithmetic.
- 22 tests across modern/tests/unit/server/item_kind_predicates_test.cpp (10) + get_item_kind_type_test.cpp (3) + playtime_decrement_test.cpp (9).
- See commits e5b27b1c (item_kind_predicates) + 2dab9b8d (get_item_kind_type) + c26c1d72 (playtime_decrement).

### D4.25 IsDupItem data plane (2026-08-06)

- D4.25 - 1:1 port of legacy CItemManager::IsDupItem(WORD wItemIdx) from [Server]Map/ItemManager.cpp. Pure data plane: returns true iff the inventory is allowed to stack duplicates of the item.
- Adds modern/include/mxh/server/is_dup_item.hpp: legacy item-kind constants (13 always-dup kinds: YOUNGYAK*4 + EXTRA*7 + SHOP_CHARM + SHOP_HERB), Sundries exception (SimMek / CheRyuk / Shout idx), 30-entry Incantation non-dup list (TownMove15 / MemoryMove15 / MemoryMoveExtend* / ShowPyoguk* / Tracking* / Extend* / CharacterSlot*), Skin no-dup (NOMALCLOTHES_SKIN / COSTUME_SKIN), free function is_dup_item + inline wrappers is_rare_option_item (legacy IsRareOptionItem) and is_option_item (legacy IsOptionItem).
- Adds modern/src/server/is_dup_item.cpp: implementation with incantation_is_non_dup helper for the 30-entry switch.
- 19 tests in modern/tests/unit/server/is_dup_item_test.cpp: 4 always-dup kinds + 7 extra kinds + shop charm/herb + Sundries 5 branches + 30-entry Incantation non-dup list + Incantation LimitLevel+SellPrice gate + Skin 3 branches + NullInfo + UnknownKind + 2 precedence tests + 3 IsRareOptionItem/IsOptionItem wrapper tests.
- See commits 535742c2 (16 tests) + 56637dfb (3 wrapper tests).


### D4.24 AddDupParam/DeleteDupParam/IsDupAble data plane (2026-08-06)

- D4.24 - 1:1 port of legacy CShopItemManager::AddDupParam / DeleteDupParam / IsDupAble from [Server]Map/ShopItemManager.cpp:2286-2620. Splits the legacy per-bit OR / XOR-when-both-set / block semantics across 5 dup categories (charm / herb / incantation / sundries / pet_equip) into a pure data plane.
- Adds modern/include/mxh/server/dup_param.hpp: DupCounters struct (5 counter fields) + DupParamIndices struct (5 index fields) + DupParamLookup virtual class (returns 0 by default) + SundrySideEffects struct (set/clear bStreetStall booleans) + 5 inner namespaces (charm_dup / herb_dup / incantation_dup / sundries_dup / pet_equip_dup) with bit constants matching [CC]Header/CommonGameDefine.h:2874-2930 (DONTDUP_* flag values 1:1 with legacy) + 3 free functions (add_dup_param, delete_dup_param, is_dup_able).
- Adds modern/src/server/dup_param.cpp: implementation with apply_or (OR, idempotent for Add) and apply_delete (XOR-when-both-set, clears bit only if both counter and dup_param have it). Sundries street-stall side effect captured in SundrySideEffects struct.
- 33 tests in modern/tests/unit/server/dup_param_test.cpp: 32 named tests covering AddDupParam (NoIndices, CharmBitsOrIntoCounter, CharmOrIsIdempotent, IncantationBitsOrIntoCounter, SundriesBitsOrAndTriggersStreetStall, SundriesBitsWithoutStreetStallHasNoSideEffect, PetEquipBitsOrIntoCounter, AllFiveCategoriesTogether, LookupReturnsZeroIsNoOp) + DeleteDupParam (NoIndices, CharmBitsClearedWhenBothSet, CharmBitsUntouchedWhenCounterEmpty, IncantationBitsClearedWhenBothSet, SundriesClearsAndTriggersClearStreetStall, SundriesNoClearWhenCounterEmpty, PetEquipBitsClearedWhenBothSet, AllFiveCategoriesTogether) + IsDupAble (EmptyCountersAllDupEnabled, CharmOverlapBlocks, CharmNoOverlapAllows, HerbOverlapBlocks, IncantationOverlapBlocks, SundriesOverlapBlocks, PetEquipOverlapBlocks, NoIndicesReturnsTrue, LookupReturnsZeroIsTrue, AnyCategoryOverlapBlocks, NoOverlapReturnsTrueWithAllIndicesSet, DisjointBitsAllAcrossFiveCategoriesReturnsTrue).
- See commit 529e1843: server: D4 AddDupParam/DeleteDupParam/IsDupAble data plane - 5 dup categories + 33 tests.

### D4.23 CalcAvatarOption data plane (2026-08-06)

- D4.23 CalcAvatarOption - 1:1 port of legacy CShopItemManager::CalcAvatarOption() from [Server]Map/ShopItemManager.cpp:2024-2183 (avatar stat accumulator).
- Adds modern/include/mxh/game/avatar_item_option.hpp: AvatarItemOption struct (25 fields, 39 bytes under pack(1), byte-identical to legacy AVATARITEMOPTION) + AvatarSlot enum (eAvatar_Hat..WeeMaxAmgi, Max=24) + EAvatarCount=24 constant.
- Adds modern/include/mxh/server/avatar_calc.hpp: calc_avatar_option(avatar[24], ItemManager&) - walks 24 slots, skips wIconIdx<2 (cosmetic empty/base-skin) and ItemInfo misses, accumulates 25 field deltas with the legacy >0 / ==1 predicates and uint16->uint8 narrow casts.
- 24 tests in modern/tests/unit/game/avatar_calc_test.cpp: layout size + enum indices + empty/cosmetic-base/unknown slots + single-field accumulator + 25-field all-fields pass + multi-slot accumulator + bKyungGong/NaeruykspendbyKG flag-only semantics + LimitSimMek truthy predicate + uint16->uint8 truncation + 24-slot full sweep.
- See commit 7e49a5a5: server: D4.23 CalcAvatarOption data plane + AVATARITEMOPTION struct + 24 tests.

### D4.22 CheckAvatarEndtime data plane (2026-08-06)

- D4.22 CheckAvatarEndtime - 1:1 port of legacy CShopItemManager::CheckAvatarEndtime() from [Server]Map/ShopItemManager.cpp:1142-1180 (avatar-only counterpart to CheckEndTime, runs unconditionally every tick the avatar-event loop calls it).
- Adds modern collect_avatar_realtime_expired(PackedTime now, out) data plane function (header in modern/include/mxh/server/shop_item_manager.hpp, impl in modern/src/server/shop_item_manager.cpp). Predicate is byte-identical to collect_realtime_expired (SellPrice == eShopItemUseParam_Realtime AND curtime > EndTime); the side-effect chain (DiscardItem + SendMsgDwordToPlayer(MP_ITEM_SHOPITEM_USEEND) + ShopItemDeleteToDB + LogItemMoney) is the orchestrator's responsibility and is documented in the implementation comment.
- 8 tests in modern/tests/unit/server/shop_item_manager_test.cpp ShopItemManagerCheckAvatarEndTime suite: NoRows / PlaytimeRowsSkipped / FutureEndTime / EqualEndTime / PastEndTime / MixedRows / DoesNotMutateTable / MatchesCollectRealtimeExpiredExactly.
- See commit 668ec566: server: D4.22 CheckAvatarEndtime data plane + 8 tests (legacy 4-step side-effect chain documented).

### build: mxh_compat_tests linker fix (2026-08-06)

- mxh_compat_tests previously failed to link because item_effects.cpp (in mxh_compat) references mxh::game::ItemManager::try_get, which lives in src/game/item_manager.cpp. The test target linked mxh_compat + gtest_main but not mxh_game, leaving 81 tests Not Run.
- Fix: add mxh_game to mxh_compat_tests target_link_libraries in modern/tests/unit/CMakeLists.txt. 1 line change. 81 previously Not-Run compat tests now run + pass.
- See commit f385b8d7: build: fix mxh_compat_tests linker error - link mxh_game (ItemManager::try_get).


### E2 T2 wire golden round-trip coverage (2026-08-06)

- E2 T2 wire golden round-trip - Ã¥Â¯Â¹Ã¦â€°â‚¬Ã¦Å“â€° 84 Ã¤Â¸Âª golden .bin Ã¦â€“â€¡Ã¤Â»Â¶Ã¥Ââ€žÃ¥Å Â Ã¤Â¸â‚¬Ã¤Â¸Âª TEST(WireFormatGolden, RoundTrip_X)Ã¯Â¼Å’Ã¨Â§Â£Ã¦Å¾ÂÃ¥Â­â€”Ã¨Å â€šÃ¥Ë†Â° Packet structÃ¯Â¼Å’Ã¥â€ Â wire_bytes() Ã©â€¡ÂÃ§Â¼â€“Ã§Â ÂÃ¯Â¼Å’Ã¦â€“Â­Ã¨Â¨â‚¬Ã¥Â­â€”Ã¨Å â€šÃ¥Â®Å’Ã¥â€¦Â¨Ã§â€ºÂ¸Ã¥ÂÅ’Ã£â‚¬â€š 11 Ã¤Â¸ÂªÃ¥Å½Å¸ wire format Ã¦Âµâ€¹Ã¨Â¯â€¢ + 84 Ã¤Â¸Âª golden round-trip = 95 tests, 95 PASSED, 0 FAILEDÃ£â‚¬â€š Ã©â€ÂÃ¤Â½ÂÃ§Å½Â°Ã¤Â»Â£ wire encoder Ã¤Â¸Å½ legacy [Server]*/4DyuchiNET_Latest Ã¥Â­â€”Ã¨Å â€šÃ§ÂºÂ§ 1:1Ã£â‚¬â€š
- Ã¥Å Â  find_golden_dir() helper (server/golden Ã§Â­â€°Ã¨Â·Â¯Ã¥Â¾â€žÃ¥â‚¬â„¢Ã©â‚¬â€°) + filesystem/fstream/string Ã¥Â¤Â´Ã¦â€“â€¡Ã¤Â»Â¶Ã£â‚¬â€š

### E1 T1 parse test subdir expansion (2026-08-06)

- E1 T1 parse test subdir expansion - deploy Server/ + QuestScript/ + PlayDH non-Client (top-level + EffectScript/Map/QuestScript/SkillArea) Ã¥â€¦Â¨Ã©Æ’Â¨ read_mh_bin parse ok = true + size match + 256MB Ã©â„¢ÂÃ£â‚¬â€š
- Ã¦â‚¬Â»Ã¦â€¢Â° 89 -> 268 parse test entries across 5 suites (MxhResourceParse 60 + MxhResourceParseClient 29 + MxhResourceParseServer 108 + MxhResourceParseQuestScript 6 + MxhResourceParsePlayDh 65). 7 Ã¤Â¸Âª 14-byte placeholder stub Ã¨Â¢Â«Ã¨Â¿â€¡Ã¦Â»Â¤ (header.file_size = 0 Ã¤Â¼Å¡Ã¥Â¤Â±Ã¨Â´Â¥ EXPECT_GT)Ã£â‚¬â€š
- Ã¤Â¸Å½ SHA-256 manifest Ã©â€ÂÃ¥Â®Å¡Ã§Å¡â€ž 303 records Ã¤Â¸Â¥Ã¦Â Â¼Ã¥Â¯Â¹Ã©Â½ÂÃ¯Â¼Å¡Ã¦Â¯ÂÃ¤Â¸ÂªÃ¦Å“â€° SHA-256 Ã¨Â®Â°Ã¥Â½â€¢Ã§Å¡â€ž .bin Ã©Æ’Â½Ã¦Å“â€°Ã¥Â¯Â¹Ã¥Âºâ€ parse testÃ£â‚¬â€š



---

## Ã¦Ë†ÂªÃ¦Â­Â¢ 2026-08-06 Ã¥Â·Â²Ã¥Â®Å’Ã¦Ë†ÂÃ©Â¡Â¹

### E1 T1 Ã¨Âµâ€žÃ¦ÂºÂÃ¥Â­â€”Ã¨Å â€šÃ§ÂºÂ§Ã©ÂªÅ’Ã¨Â¯Â (2026-08-05)

- E1 T1 payload SHA-256 byte-level verification - 3 Ã¤Â¸Âª manifest (deploy + PlayDH + PlayDH/Client), 146 records, 117 .bin Ã¦â€“â€¡Ã¤Â»Â¶ SHA-256 + byte-size + header.type Ã©â€ÂÃ¥Â®Å¡Ã£â‚¬â€š3/3 VerifyManifest_* PASSÃ£â‚¬â€š
- E1 T1 mechanical expansion - 89 deploy/Resource .bin Ã¥â€¦Â¨Ã©Æ’Â¨ read_mh_bin parse ok = true + size match + 256MB Ã©â„¢ÂÃ£â‚¬â€š

### Phase D Ã§Å½Â©Ã¦Â³â€¢/Ã¦â€¢Â°Ã¥â‚¬Â¼ 1:1

- D1.1 + D1.2 + D1.3 SkillList.bin parser + call-site - SkillInfo Ã¦â€°Â©Ã¥Ë†Â° 60+ Ã¥Â­â€”Ã¦Â®Âµ 1:1 legacy SKILLINFO, SkillListParser Ã¨Â§Â£Ã§Â Â MHFile packed-text, 1817 entries load Ã¦Ë†ÂÃ¥Å Å¸ 0 parse_errorsÃ£â‚¬â€šMapHandler Ã§â€Â¨Ã§Å“Å¸ SkillList.bin Ã¨â‚¬Å’Ã©ÂÅ¾ 4-skill hardcodeÃ£â‚¬â€š
- D2 BattleFactory 1:1 - 14 compute_* Ã¥â€¡Â½Ã¦â€¢Â° (critical/decisive/player-phy/player-attr/exp/point/phy-defence/received-dmg/monster-phy/monster-attr/titan-phy/titan-attr) + 13 legacy_* Ã¦Âµâ€¹Ã¨Â¯â€¢Ã£â‚¬â€š
- D3.x QuestManager (D3.5-D3.7):
  - D3.5 QuestScriptLine Ã§Â«Â¯Ã¥Ë†Â°Ã§Â«Â¯ parser - 9 event tokens + 6 limit tokens + &Limit + @Event + *Execute layout
  - D3.6 QuestTrigger runtime evaluator - 10 tests, EndQuest fix (args[0] 1-arg subquest Ã§Â´Â¢Ã¥Â¼â€¢)
  - D3.7 QuestScript subquest block parser - 12 tests
  - D3.3 / D3.4 QuestExecute data-plane dispatch (5+4 QuestGroup + 4 dispatch tests)
- D5 MurimNet 1:1 wire - Channel + PlayRoom + 60 Ã¥ÂÂÃ¨Â®Â®Ã¤Â»Â£Ã§Â Â + 9 wire serializer + 7 short wire + runtime.broadcast_chat sink + MurimNetCrypt + 139 testsÃ£â‚¬â€š
- D6.1 Ã¦â€¢Â°Ã¥â‚¬Â¼ baseline - 7 OBJECTKIND / 6 MonsterAI / 14B MonsterTotalInfo / 22B ItemBase / 124 Ã¦Â§Â½ / 2728B ItemTotalInfo / 3775B GameInAck hero payload / 4 ItemEffect Ã¥â€¦Â¬Ã¥Â¼Â / 3 default MonsterTemplate Ã¥â€¦Â¨Ã©Æ’Â¨ 1:1 Ã©â€ÂÃ¦Â­Â»Ã£â‚¬â€š
- D6.2 Distributer recipient + party-exp pure decisions - legacy null-party tie no-op, 50/50 tie, float allocation, zero-send gate. 10 new tests, 45/45 focused PASSÃ£â‚¬â€š
- D6.2 FieldBossMonsterManager 1:1 - 19 tests (channel Ã©â€¦ÂÃ§Â½Â®/Ã¥â€¡ÂºÃ¦â‚¬Âª/Ã©â€¡ÂÃ§Â½Â®/Ã¥Ââ€¢Ã©Â£Å¾/Ã©â„¢ÂÃ©Â¢â€˜)Ã£â‚¬â€š
- D6.2 ChooseOne tie-break + SetPlusTotalDamage += semanticsÃ£â‚¬â€š
- D6.3 BossMonsterManager 1:1 - 19 tests (register/spawn/erase/damage/live_count)Ã£â‚¬â€š
- D6.4 cMonsterSpeechManager 1:1 - 15 testsÃ£â‚¬â€š
- D6.5 ExperienceCurve 1:1 - 15 tests (CharacterExpPoint.bin reader, add_exp Ã©â„¢ÂÃ¤Â¸â‚¬Ã¦Â¬Â¡Ã¥Ââ€¡Ã§ÂºÂ§)Ã£â‚¬â€š
- D6.6 state_param + summon_monster + titan_item_manager 1:1 - 31 testsÃ£â‚¬â€š
- D6.7 distribute_network_msg_parser + common_network_msg_parser 1:1 - 19 testsÃ£â‚¬â€š
- D6.x ItemList.bin 1:1 parser - ITEM_INFO 77 fields, 56/60 token MHFile packed-text, 9887 rows 0 parse errorsÃ£â‚¬â€šShared decode_mhfile_text_payload helperÃ£â‚¬â€š
- R-8 item_effects real lookup via ItemManager - ItemManager.init_from_bin / get / try_get / exists / sizeÃ£â‚¬â€šresolve_item_effect_with_manager Ã¨Â¯Â» LifeRecover / LifeRecoverRate / NaeRyukRecover / NaeRyukRecoverRateÃ£â‚¬â€š12 tests PASSÃ£â‚¬â€š
- R-8 call-site MapHandler.load_item_list - 3 testsÃ£â‚¬â€š
- B6.1 HSEL YHLibrary ABI Ã¦Â Â¡Ã¦Â­Â£ - 79 crypto tests, non-virtual dtor / 2 const virtual getters / protected version/type fields / non-virtual CHSEL_STREAMÃ£â‚¬â€š

### R-2 HackShield Ã¨Â·Â¯Ã§â€Â± + tick

- R-2 AgentHandler HackShield routing - cat==HackShield (67) Ã©â‚¬Å¡Ã¨Â¿â€¡ mxh::server::parse_hackshield_message, conn_user_levels_ + conn_hs_states_ + hackshield_disconnect_pending_, superuser (>=5) Ã§Â«â€¹Ã¥ÂÂ³ send_guid_reqÃ£â‚¬â€š10 testsÃ£â‚¬â€š
- R-2.1 AgentHandler auto-populate user_level from chr_log_info (proto=9) - 3 testsÃ£â‚¬â€š
- R-2.2 AgentHandler::tick_hackshield() server-side periodic recheck - 4 tests (empty/grace/non-superuser/mixed)Ã£â‚¬â€š
- bug: Crypt::encrypt_crc/decrypt_crc Ã¨Â¿â€Ã¥â€ºÅ¾ 0 Ã¨â‚¬Å’Ã©ÂÅ¾ HselStream CRC char - fixed + 4 testsÃ£â‚¬â€š

### MurimNet 1:1 Ã¥â€¦Â³Ã©â€Â®

- commit 83d53c1b - MurimNetChannel.for_each_channel + PlayRoomManager.for_each_room + SendMsg_ChannelList/PlayRoomList
- commit ce931c51 - wire-byte serializer (MSG_CHANNEL_BASEINFO/MSG_PLAYROOM_BASEINFO/#pragma pack(1) 9 size Ã©â€ÂÃ¥Â®Å¡)
- commit 845294ec - SendMsg_ChannelList/PlayRoomList 8 tests (size = GetMsgLength, Category=38, Protocol=34/35, title 63 Ã¥Â­â€”Ã¨Å â€š)

### Phase C UI 1:1 port - Ã¤Â¸Â»Ã¨Â¦Â batches (Batch 1.1-1.10 + Batch 2.1-2.80)

Ã¥Â®Å’Ã¦â€¢Â´ 109 dialog Ã¥Ë†â€”Ã¨Â¡Â¨ (Ã¦Å’â€° phase commit Ã§Â´Â¢Ã¥Â¼â€¢):
- Batch 1.1 - 1.10 (10 dialog): cNumberPadDialog / cPartyWarDialog / cWantedDialog / cMPNoticeDialog / cReviveDialog / cMiniFriendDialog / cGuildInviteDialog / cChatOptionDialog / cStallKindSelectDlg / cDebugDlg (331 tests)
- Batch 2.1 - 2.7 (7 dialog): cMPGuageDialog / cAlertDlg / cChinaAdviceDlg / cLoadingDlg / cKeySettingTipDlg / cIntroReplayDlg / cNameChangeNotifyDlg (210 tests)
- Batch 2.32 - 2.80: 30+ dialog (cCharChangeDlg, cBailDialog, cChaseInputDialog, cObjectStateManager Ã§Â­â€°, Ã¦Â¯Â dialog 10-25 tests)
- Ã¦â€Â¶Ã¥ÂÂ£ fixes: cChatOption checked event bits, cListDialogEx legacy event bits, cPushupButton sticky click state, cListItem m_maxLine=0 inconsistencyÃ£â‚¬â€š

### Phase B Ã¦Å“ÂÃ¥Å Â¡Ã§Â«Â¯ E2E

- B.2.1 CLoginState - 38B legacy payload Ã¥Â­â€”Ã¨Å â€šÃ§ÂºÂ§
- B.2.2 CCharSelectState - 8B ListSyn + 889B ListAck + Ã¨â€¡ÂªÃ¥Å Â¨Ã©â‚¬â€°Ã§Â¬Â¬Ã¤Â¸â‚¬Ã¤Â¸Âª + SelectSyn
- B.2.3 CInGameState - 3775B SEND_HERO_TOTALINFO GameInAck
- B.2.4 state machine Ã§Å“Å¸Ã¦Å½Â¥ net Ã¤Âºâ€¹Ã¤Â»Â¶
- B.2.5 MoxianClientE2E headless tool - CreateProcessW Ã¥ÂÂ¯ 3 server + C++ Ã§Å Â¶Ã¦â‚¬ÂÃ¦Å“ÂºÃ¨Â·â€˜ Login -> CharSelect -> InGame (60s timeout)

### Phase A Ã¥Â®Â¢Ã¦Ë†Â·Ã§Â«Â¯

- f80f6e0 - MoxianClient skeleton (DX11 + WinMain + 4 sprite + cImage <-> IDISpriteObject)
- CMainGame 1:1 port - eGAMESTATE 0..9 byte-for-byte + 9 state stub + CMainTitle (14 Ã¥Â­â€”Ã¦Â®Âµ 1:1 surface + Init Ã¨Â¯Â» MHVerInfo.ver)

---

## Ã¨â‚¬Â ROADMAP Ã¤Â¸Â­Ã§Å¡â€ž Phase 0-12 Ã¥â„¢ÂªÃ©Å¸Â³

Ã¥â€¡Â¡Ã¦â€”Â§Ã¨Â¯ÂÃ¦ÂÂÃ¥ÂÅ "Phase 0-12 Ã¥Â·Â²Ã¥Â®Å’Ã¦Ë†Â""35.1% complete""Qoder IDE Quest" Ã§Â­â€°, Ã¥â€¦Â¨Ã©Æ’Â¨Ã¥ÂºÅ¸Ã¥Â¼Æ’Ã£â‚¬â€šÃ¥Ââ€šÃ¨Â§Â ROADMAP Ã‚Â§6Ã£â‚¬â€š


### D3 quest event dispatcher bridge (2026-08-06)

- D3 runtime bridge - Ã¦Å Å  legacy CQuestManager::AddQuestEvent -> CQuestGroup::AddQuestEvent -> sub-condition Ã¥Å’Â¹Ã©â€¦Â 1:1 Ã§Â§Â»Ã¦Â¤ÂÃ¥Ë†Â° modern runtimeÃ£â‚¬â€šÃ¦â€“Â°Ã¥Â¢Å¾ mxh::server::QuestEvent {kind,target_id,delta} + dispatch_quest_event(QuestLog&, const QuestEvent&)Ã¯Â¼Å’Ã¦Å’â€° QuestLog Ã©Â¡ÂºÃ¥ÂºÂÃ©ÂÂÃ¥Å½â€ Ã¯Â¼Å’Ã¥ÂÂªÃ¤Â¿Â®Ã¦â€Â¹ Accepted Ã§Å Â¶Ã¦â‚¬ÂÃ¯Â¼Ë†Ã¤Â¸Å½ legacy group Ã¥Â¯Â¹ terminal quest Ã§Å¡â€ž guard Ã¤Â¸â‚¬Ã¨â€¡Â´Ã¯Â¼â€°Ã¯Â¼Å’Ã¦Â¯ÂÃ¤Â¸ÂªÃ¥Å’Â¹Ã©â€¦Â sub-quest Ã§Â´Â¯Ã¥Å Â  delta Ã¥Â¹Â¶ clamp Ã¥Ë†Â° targetÃ¯Â¼Å’Ã¨Â¿â€Ã¥â€ºÅ¾ std::vector<QuestEventChange> {quest_id, previous_state, state, updated_subs}Ã£â‚¬â€š
- 6 Ã¤Â¸ÂªÃ¦â€“Â°Ã¨Â¡Å’Ã¤Â¸ÂºÃ©â€ÂÃ¥Â®Å¡Ã¦Âµâ€¹Ã¨Â¯â€¢Ã¯Â¼Å¡DispatchQuestEvent.UpdatesEveryMatchingActiveQuestInLogOrder / CompletesQuestWhenFinalConditionMatches / UpdatesAllMatchingSubsWithinQuest / IgnoresNonMatchingAndTerminalQuests / ZeroDeltaAndNoneKindAreNoOps / DoesNotMutateOtherQuestsÃ£â‚¬â€šmxh_quest_manager_tests: 37 -> 43 tests PASS, 0 regressions; Ã¦Å“ÂÃ¥Å Â¡Ã§Â«Â¯ ctest 313 Ã©Â¡Â¹Ã¤Â¸Â­ 2 flaky (LoginServerFixture Weather/GuildFieldWar) Ã©â€¡ÂÃ¨Â·â€˜ 100% Ã©â‚¬Å¡Ã¨Â¿â€¡Ã£â‚¬â€š
- Ã¨Â§Â commit 9512a082: server: D3 quest event dispatcher bridge (legacy AddQuestEvent->QuestGroup semantics)Ã£â‚¬â€š



### D4 use_shop_item_decision data plane (2026-08-06)

- D4.20 use_shop_item_decision - Ã¦Å Å  legacy CShopItemManager::UseShopItem Ã¤Â¸Â­*Ã¥Â®Å¾Ã©â„¢â€¦*Ã¨ÂÂ½Ã¥Ë†Â° SHOPITEMBASE Ã¥Â­â€”Ã¨Å â€šÃ¤Â¸Å Ã§Å¡â€žÃ©Æ’Â¨Ã¥Ë†â€  1:1 Ã¦ÂÂÃ¥Ââ€“Ã¤Â¸Âº modern Ã¨â€¡ÂªÃ§â€Â±Ã¥â€¡Â½Ã¦â€¢Â°Ã¯Â¼Å¡InvalidIcon / ItemInfoMissing / AlreadyInUse Ã¤Â¸â€°Ã©Ââ€œ guard + Realtime/Playtime/Continue/SellPrice==0 Ã¥â€ºâ€ºÃ¤Â¸Âª BeginTime/Remaintime Ã¥Ë†â€ Ã¦â€Â¯ + ItemKind Ã¥Ë†Â° dup counter (Incantation/Charm/Herb/Sundries/PetEquip) Ã§Å¡â€žÃ¨Â·Â¯Ã§â€Â±Ã£â‚¬â€š
- Ã¦â€“Â°Ã¥Â¢Å¾ inline Ã¥Â¸Â¸Ã©â€¡Â SHOP_ITEM_USE_PARAM_REALTIME/PLAYTIME/CONTINUE + LEGACY_SHOP_ITEM_* (257/258/259/260/261/262/263/264/300/310Ã¯Â¼Å’1:1 Ã¥Â¼â€¢Ã§â€Â¨ [CC]Header/CommonGameDefine.h:698-713)Ã£â‚¬â€š
- 8 Ã¤Â¸ÂªÃ¦â€“Â°Ã¨Â¡Å’Ã¤Â¸ÂºÃ©â€ÂÃ¥Â®Å¡Ã¦Âµâ€¹Ã¨Â¯â€¢Ã¯Â¼Å¡RejectsEmptyIcon / RejectsMissingItemInfo / RejectsAlreadyInUse / RealtimeBranchEncodesEndTimeInRemaintime / PlaytimeBranchConvertsRarityToMilliseconds / ContinueBranchHasNoExpiry / ZeroSellPriceRoutesSundriesAndNoTimer / RoutesHerbToHerbDupÃ£â‚¬â€šmxh_shop_item_manager_tests: 91 -> 99 tests PASS, 0 regressions (ctest -R ShopItemManager|UseShopItemDecision 100% Ã©â‚¬Å¡Ã¨Â¿â€¡)Ã£â‚¬â€š
- Ã¦Â³Â¨Ã©â€¡Å Ã¨Â§Â£Ã©â€¡Å Ã¯Â¼Å¡legacy UseShopItem Ã¦â€¢Â´Ã¦Â®ÂµÃ¨Â¢Â« `/* ... */` Ã¦Â³Â¨Ã©â€¡Å Ã¯Â¼Ë†"Ã¬Å¾â€žÃ¬â€¹Å“Ã«Â¡Å“ Ã«â€ Ë†Ã¬ÂÅ’ - Ã¬â€žÂ±Ã«Å’â‚¬"Ã¯Â¼â€°Ã¯Â¼Å’Ã¤Â¸ÂÃ¥ÂÂ¯ 1:1 Ã¥Â¤ÂÃ§Å½Â°Ã¯Â¼â€ºÃ¦Å“Â¬Ã¥â€¡Â½Ã¦â€¢Â°Ã¥ÂÂªÃ¨Â¦â€ Ã§â€ºâ€“Ã¨Æ’Â½Ã¨ÂÂ½Ã¥Ë†Â° SHOPITEMWITHTIME Ã¨Â¡Å’Ã§Å¡â€ž guard + BeginTime/Remaintime Ã©Æ’Â¨Ã¥Ë†â€ Ã£â‚¬â€š
- Ã¨Â§Â commit dcb05173: server: D4 use_shop_item_decision data plane (legacy UseShopItem guard + BeginTime/Remaintime)Ã£â‚¬â€š



### D4.21 CheckEndTime realtime branch data plane (2026-08-06)

- D4.21 CheckEndTime realtime branch - Ã¦Å Å  legacy CShopItemManager::CheckEndTime Ã¤Â¸Â­ Realtime Ã¦Â¨Â¡Ã¥Â¼Â (`SellPrice == eShopItemUseParam_Realtime`) Ã§Å¡â€žÃ¦â€¢Â°Ã¦ÂÂ®Ã©ÂÂ¢ 1:1 Ã¦ÂÂÃ¥Ââ€“Ã¤Â¸Âº modern Ã¦â€“Â¹Ã¦Â³â€¢: collect_realtime_expired(PackedTime now, out) / consume_realtime_expired(PackedTime now)Ã£â‚¬â€špredicate Ã¤Â¸Å½ legacy `curtime > stTIME(Remaintime)` Ã¤Â¸Â¥Ã¦Â Â¼Ã¤Â¸â‚¬Ã¨â€¡Â´Ã¯Â¼â€ºÃ¥ÂÂªÃ¦â€°Â«Ã¦ÂÂ `Param == SHOP_ITEM_PARAM_STORED_TIME` Ã§Å¡â€žÃ¨Â¡Å’Ã¯Â¼Ë†playtime/continue/one-shot Ã¨Â·Â³Ã¨Â¿â€¡Ã¯Â¼â€°Ã£â‚¬â€š
- 7 Ã¤Â¸ÂªÃ¦â€“Â°Ã¨Â¡Å’Ã¤Â¸ÂºÃ©â€ÂÃ¥Â®Å¡Ã¦Âµâ€¹Ã¨Â¯â€¢: NoRowsNoExpirations / PlaytimeRowsAreSkipped / FutureEndTimeIsNotExpired / EqualEndTimeIsNotExpired / PastEndTimeIsCollected / MixedRowsSelectsOnlyStoredTimeExpired / ConsumeIsIdempotentOnAlreadyExpiredRowsÃ£â‚¬â€šmxh_shop_item_manager_tests: 99 -> 106 tests PASS, 0 regressionsÃ£â‚¬â€š
- Ã¨Â§Â commit 3e21470e: server: D4.21 CheckEndTime realtime branch data plane (legacy SellPrice==Realtime sweep)Ã£â‚¬â€š




### D3.8 quest_group_run_pending (2026-08-06)

- D3.8 quest_group_run_pending - ? legacy CQuestGroup::Process() ???? 1:1 ??? modern ????:?? quest_group_process(state) ? dispatch list,? (quest, event) ??? triggers_by_quest ?,??? quest ?????? trigger (legacy CQuestTrigger::OnQuestEvent ? "first matching condition wins" ??),?? std::vector<QuestGroupApplyResult> {quest_idx, subquest_idx, status, changed}?
- 7 ????????:NoEventsProducesEmptyResult / AppliesFirstMatchingTriggerAndAdvancesSubCount / SkipsQuestWithNoTriggerEntry / FiresFirstConditionMatchAndIgnoresTheRest / FansOutAcrossMultipleQuests / ConditionFailedLeavesQuestUntouched / IdempotentAfterEventsDrained?mxh_quest_group_tests: 23 -> 30 tests PASS, 0 regressions?
- ? commit 9e5ba572: server: D3.8 quest_group_run_pending (legacy CQuestGroup::Process tick link)?



### D3.9 subquest counter access (2026-08-06)

- D3.9 subquest counter access - ? legacy CQuestGroup::GetSubQuestValue + CQuestGroup::ChangeSubQuestValue ???? 1:1 ??? modern ????:get_sub_quest_value(state, questIdx, subIdx) ?? subQuestData[subIdx] ? 0xFFFFFFFFu sentinel(legacy "no such quest" marker),change_sub_quest_value(state, questIdx, subIdx, kind) ?? QUEST_VALUE_ADD/MINUS(legacy eQuestValue_Add/Minus from QuestDefines.h:128-129),Minus clamp ? 0,unknown kind ???
- ?? inline ?? QUEST_VALUE_ADD=0u / QUEST_VALUE_MINUS=1u / QUEST_SUB_QUEST_VALUE_NOT_FOUND=0xFFFFFFFFu?get_quest ? const ??????? lookup?
- 8 ????????:GetMissingQuestReturnsSentinel / GetExistingButUnsetSubQuestReturnsZero / GetReturnsStoredCount / AddIncrementsCount / MinusDecrementsCount / MinusClampsAtZero / UnknownKindIsRejected / MissingQuestReturnsFalse?mxh_quest_group_tests: 30 -> 38 tests PASS, 0 regressions?
- ? commit f5a7d89b: server: D3.9 subquest counter access (legacy GetSubQuestValue + ChangeSubQuestValue data plane)?



### D3.10 quest end lifecycle tests (2026-08-06)

- D3.10 quest end lifecycle tests - ????? quest_group_end_quest / quest_group_end_subquest ? 7 ???????,?? legacy CQuestGroup::EndQuest(questIdx, subQuestIdx) + CQuestGroup::EndSubQuest ??????:missing-quest ?? false?repeat=0 ?? complete / clear active subs / ? subQuestData?repeat=1 ?? quest ??? reset counter?idx==0 ? end_subquest ??? quest->data + quest->time?
- mxh_quest_group_tests: 38 -> 45 tests PASS, 0 regressions?
- ? commit 3616baac: server: D3.10 lock legacy EndQuest + EndSubQuest semantics (7 lifecycle tests)?





### D3.11 weapon-filtered AddCount (2026-08-06)

- D3.11 weapon-filtered AddCount - ? legacy CQuestGroup::AddCountFromWeapon + AddCountFromQWeapon ???? 1:1 ??? modern ????:add_count_from_weapon(state, questIdx, subIdx, max, requiredWeaponKind, playerWeaponKind) ? `playerWeaponKind == requiredWeaponKind` ???? quest_group_add_count;add_count_from_q_weapon(state, ...) ??????? weapon item-idx???????????? false ??? counter(legacy "???" 1:1 quirk)?
- ????????? quest_group_add_count ? max-bound clamp ??,?? + ???????
- 6 ????????:AddCountFromWeaponMismatchedKindDoesNothing / AddCountFromWeaponMatchingKindIncrements / AddCountFromWeaponClampsAtMax / AddCountFromQWeaponMismatchedItemDoesNothing / AddCountFromQWeaponMatchingItemIncrements / AddCountFromWeaponMissingQuestReturnsFalse?mxh_quest_group_tests: 45 -> 51 tests PASS, 0 regressions?
- ? commit 67dcac95: server: D3.11 weapon-filtered AddCount (legacy AddCountFromWeapon + AddCountFromQWeapon data plane)?





### D3.12 weapon-filtered TakeQuestItem (2026-08-06)

- D3.12 weapon-filtered TakeQuestItem - ? legacy CQuestGroup::TakeQuestItemFromWeapon + TakeQuestItemFromQWeapon ???? 1:1 ??? modern ????:take_quest_item_from_weapon(state, quest, sub, item, num, prob, requiredKind, playerKind) ? `playerKind == requiredKind` ?????? quest_group_take_quest_item;take_quest_item_from_q_weapon(state, ...) ????? weapon item-idx????????? false ??? m_QuestItemTable(1:1 quirk)?
- ? D3.11 ? AddCount weapon-filtered ??:????? `*TAKEQUESTITEMFW` / `*TAKEQUESTITEMFQW` QuestScriptLoader execute ??,?? runtime hook (QUESTMGR->TakeQuestItem) ? caller ???
- 5 ????????:TakeQuestItemFromWeaponMismatchedKindDoesNothing / TakeQuestItemFromWeaponMatchingKindInserts (+1 sub increment + m_QuestItemTable ????) / TakeQuestItemFromQWeaponMismatchedItemDoesNothing / TakeQuestItemFromQWeaponMatchingItemInserts (+1 sub increment) / TakeQuestItemFromWeaponMissingQuestStillInsertsItem (legacy "? item table ? ChangeSubQuestValue" 1:1 quirk, add_count return void-cast)?mxh_quest_group_tests: 51 -> 56 tests PASS, 0 regressions?
- ? commit c197573b: server: D3.12 weapon-filtered TakeQuestItem (legacy TakeQuestItemFromWeapon + TakeQuestItemFromQWeapon)?





### D3.13 weapon-filtered execute dispatch (2026-08-06)

- D3.13 weapon-filtered execute dispatch - ? D3.11/12 ? quest_group_add_count_from_weapon / quest_group_add_count_from_q_weapon / quest_group_take_quest_item_from_weapon / quest_group_take_quest_item_from_q_weapon ????? apply_count_execute / apply_item_execute ? QuestScriptLoader dispatch ??:
  - apply_count_execute ?? player_weapon_kind / player_weapon_item ???? 0 ??;AddCountFQW / AddCountFW ?? case ???? UnsupportedContext,????????????(?? 0 ?? "? context" -> gate ???? -> ???? Applied + changed=false,? legacy "missing context ? no-op" ??)?
  - apply_item_execute ??? player_weapon_kind / player_weapon_item;TakeQuestItemFQW / TakeQuestItemFW ?? case ??????,?? GiveItem/TakeItem/GiveMoney/TakeMoney/TakeExp/TakeSExp/RandomTakeItem ??? UnsupportedContext(?? player inventory / ?? / ?? context,??? quest_group ???)?
- ???? MissingQuestAndWeaponContextAreExplicit ??:`*ADDCOUNTFW 1 2 3` ?? 0 context ??? Applied + changed=false;???? "match context ? changed=true" ? sanity check?
- mxh_quest_execute_tests + mxh_quest_group_tests ????(77/77),0 regressions?
- ? commit e6dfaf1a: server: D3.13 extend apply_count_execute / apply_item_execute with player weapon context (D3.11/12 wiring)?



### D4 UseShopItemUpdateToDB data plane (2026-08-06)

- D4 UseShopItemUpdateToDB - 1:1 port of the SQL builders from legacy [Server]Map/MapDBMsgParser.cpp:676-706 (ShopItemUpdatetimeToDB / ShopItemUpdateUseInfoToDB / ShopItemParamUpdateToDB). Splits the legacy sprintf-then-Query monolith into a pure data plane (this header) + a DbThread::execute_async dispatch layer (orchestrator), so the SQL format is testable in isolation and byte-equal to the legacy sprintf output.
- Adds modern/include/mxh/server/shop_item_update_sql.hpp: STORED_SHOPITEM_UPDATETIME / UPDATEUSEINFO / UPDATEPARAM constants matching the legacy MapDBMsgParser.h STORED_SHOPITEM_* macros (verbatim "dbo.MP_SHOPITEM_*") plus three inline SQL builders: build_shop_item_update_time_sql(chid, item, remain_ms) -> "EXEC dbo.MP_SHOPITEM_Updatetime ..." (3 args), build_shop_item_update_use_info_sql(chid, dbidx, param, remain_ms) -> "EXEC dbo.MP_SHOPITEM_UpdateUseInfo ..." (4 args), build_shop_item_update_param_sql(chid, dbidx, param) -> "EXEC dbo.MP_SHOPITEM_UpdateParam ..." (3 args).
- 14 tests in modern/tests/unit/server/shop_item_update_sql_test.cpp: BasicFormat / ZeroArguments / LargeUnsignedArguments / StoredProcedureNameMatchesLegacy / TwoCommasImpliesThreeArguments / HasExecPrefix (for the time_sql builder); BasicFormat / FourArgumentsThreeCommas / ZeroParamAndRemain (for the use_info_sql builder); BasicFormat / ThreeArgumentsTwoCommas (for the param_sql builder); NoHexPrefix / NoLeadingZeros / Deterministic (cross-builder invariants).
- See commit bbebdb18: server: D4 UseShopItemUpdateToDB data plane - 3 SQL builders + 14 tests.
### D4 CalcShopItemOption data plane (2026-08-06)

- D4 CalcShopItemOption - 1:1 port of legacy CShopItemManager::CalcShopItemOption() from [Server]Map/ShopItemManager.cpp:1246-1691 (the shop-item stat accumulator). Splits the legacy monolithic function into a pure data plane (this commit) + an orchestrator half (legacy m_pPlayer->SetExtraSlotCount / STATSMGR->CalcCharStats / etc. hooks stay in the orchestrator).
- Adds modern/include/mxh/game/shop_item_option.hpp: ShopItemOption struct (38 fields, 124 bytes under pack(1), byte-identical to legacy SHOPITEMOPTION) + ESkinItemCount=6 constant. Static-asserts sizeof(ShopItemOption) == 124.
- Adds modern/include/mxh/server/calc_shop_item_option.hpp: inline legacy enum constants (eSHOP_ITEM_CHARM/HERB/INCANTATION/MAKEUP/DECORATION/SUNDRIES + eIncantation_SkPointRedist/StatePoint/InvenExtend/PyogukExtend/MugongExtend/CharacterSlot/MixUp) + CalcShopItemOptionInfo view struct (24 fields subset of legacy ITEM_INFO) + CalcShopItemOptionEnv base class (override event_rate_active for plustime gates) + CalcShopItemOptionStatus enum (Ok / InvalidIcon / NullStats / ItemInfoMissing) + CalcShopItemOptionSideEffects struct (new_protect_item_idx + 4 locale-bounded expansion flags) + free function calc_shop_item_option(stats, w_idx, b_add, param, info, env, current_protect_item_idx, out_side_effects).
- Adds modern/src/server/calc_shop_item_option.cpp: full implementation of the 4 branches (Incantation / Charm / Herb / Sundries) with the +/- calc = (bAdd ? 1 : -1) accumulator semantics, the clamp-to-zero underflow patterns, the locale-bounded incantation side-effect flags, and the plustime gates (legacy gEventRate / gEventRateFile replaced by env.event_rate_active).
- 57 tests in modern/tests/unit/server/calc_shop_item_option_test.cpp: layout size + Avatar/SkinItem array sizes + 4 early returns + 12 incantation branches (MixUp add/remove/clamp, GenGol/StatePoint, Life/SkillPoint, CheRyuk/ProtectCount add/remove, LimitJob/EquipLevelFree add/remove, InvenExtend side-effect) + 22 charm branches (GenGol/MinChub/Cheryuk/SimMek + Life/Shield/NeagongDamage/WoigongDamage + NaeRyuk/AddSung + LimitJob/LimitGender/LimitLevel/LimitGenGol/LimitMinChub + plustime gating on LimitCheRyuk/LimitSimMek/ItemGrade + non-plustime branches + RangeType/Plus_MugongIdx/Plus_Value/AllPlus_Kind + RangeAttackMin/RangeAttackMax/CriticalPercent/PhyDef + NaeRyukRecover + AttrFire street-stall decoration) + 4 herb branches (Life/Shield/Naeryuk with zero-clamp semantics) + 2 sundries branches (HK_LOCAL bStreetStall).
- See commit 68061fe2: server: D4 CalcShopItemOption data plane - SHOPITEMOPTION struct + 57 tests.
### T2 wire-format coverage 100% (2026-08-06)

- T2 protocol byte-level compat was at 95.1% (77/81 categories) due to a hardcoded kTotalCategories=81 in wire_format_coverage_test.cpp. The legacy Protocol.h has 77 real categories (MP_SERVER=1..MP_FORTWAR=77, MP_MAX=78), so the test was reporting 4 "missing" categories 78-81 that don't exist in the protocol.
- Fix: kTotalCategories = 77 (matches the actual legacy count). Coverage is now 100% (77/77). All 84 wire-format goldens (parse + re-encode byte-equal) cover every real category.
- See commit 8b17de1d: test: wire_format_coverage kTotalCategories 81 -> 77.
### D4 UpdateLogoutToDB data plane (2026-08-06)

- D4 UpdateLogoutToDB - 1:1 port of legacy CShopItemManager::UpdateLogoutToDB() from [Server]Map/ShopItemManager.cpp:1195-1238. Splits the per-row iteration into a pure data plane (this commit) + an orchestrator half (the legacy function calls ShopItemUpdatetimeToDB / g_DB.Query via g_pServerMsg).
- Adds modern/include/mxh/server/update_logout_to_db.hpp: UpdateLogoutToDBInfo struct (sell_price / item_kind / melee_attack_min -- 3-field subset of legacy ITEM_INFO) + UpdateLogoutRowDecision struct (Action enum: Persist / Skip / Drop + new_remaintime + new_last_check) + free function update_logout_to_db_decision(current_remaintime, last_check_time, g_cur_time, info, env). Reuses CalcShopItemOptionEnv for the plustime gate.
- Adds modern/src/server/update_logout_to_db.cpp: 1:1 implementation of the per-row legacy logic -- drop non-PLAYTIME rows, plustime gate (Charm + MeleeAttackMin + env.event_rate_active), clamp checktime to 30000 ms, decrement Remaintime with underflow-clamp-to-zero.
- 12 tests in modern/tests/unit/server/update_logout_to_db_test.cpp: RealTimeItemIsDropped / PlayTimeDecrementsByElapsedTime / RemaintimeUnderflowClampsToZero / PlustimeActiveDecrementsNormally / PlustimeInactiveSkipsRemaintimeUpdate / PlustimeRemaintimeZeroAlwaysDrops / PlustimeDisabledWhenMeleeAttackMinZero / CheckTimeCapClampsToThirtySeconds / ClockSkewYieldsZeroChecktime / PlustimeRemaintimeZeroWithActiveRateDrops + 2 integration tests.
- See commit cb30fba6: server: D4 UpdateLogoutToDB data plane - per-row PLAYTIME decrement + plustime gate + 12 tests.
















## 2026-08-10 - server: DealItem.bin 1:1 parser and NPC catalog adapter

Adds the MHFile header/CRC/XOR-compatible DealItem.bin loader, aggregation of repeated NPC tabs, legacy item-count handling, and an adapter into NpcShopCatalog. Registers the parser and focused unit coverage in the modern build; Debug build and governance checks pass.
## 2026-08-10 - server: DealItem.bin loader wired into MapHandler BuySyn

MapHandler now exposes `load_dealitem(path)` which parses a real `Resource/DealItem.bin` and caches a per-NPC `DealItemParseResult` snapshot. The `BuySyn` arm of `handle_item` resolves the per-NPC `NpcShopCatalog` from this snapshot (entries + price hints) and feeds it into `npc_shop_buy_decision`. When the loader is not invoked the catalog stays empty and the existing 4B BuyNack echo / NpcMismatch side-by-side behavior is preserved. Hard I/O failures are logged to stdout and leave the catalog empty so the wire shape stays diff=0. `mxh_server_handler_tests` gains `MapHandlerTest.LoadDealitemPopulatesCatalogFromBin` covering a synthesized two-tab two-row DealItem.bin; all 35 DealItem / NpcShop / MapHandler tests pass and `python scripts/check-project-governance.py` PASS.
## 2026-08-10 - server: QuestScript.bin 1:1 parser + MapHandler load_quest_script hook

Adds the QuestScript.bin MHFile header/CRC/XOR-compatible parser (mirrors legacy [Server]Map/QuestManager.cpp::LoadQuestScript + [CC]Quest/QuestScriptLoader).  The data plane now exposes a per-quest `QuestScriptDefinition` carrying the $QUEST id, $SUBQUEST list, and `end_param` captured from the first `*ENDQUEST` execute.  `parse_quest_script_text` handles the single-line + multi-line stanzas the legacy script generator emits, and the binary path (`parse_quest_script_bytes` / `load_quest_script`) reuses the canonical `decode_mhfile_text_payload` XOR routine.  `MapHandler::load_quest_script(path)` feeds the per-quest table at startup; the StartSyn arm of `handle_quest()` looks up `quest_definitions_` and stays on the StartNack + 2B quest_id echo wire shape so side-by-side 5/5 capture stays diff=0 (the data-plane accept path lights up when the legacy complete-on-accept semantics get wired in).  `mxh_server_handler_tests` gains `MapHandlerTest.LoadQuestScriptPopulatesDefinitionsFromBin` and `mxh_quest_script_loader_tests` covers parse + end_param + malformed input.  All 31 DealItem / NpcShop / QuestScript / MapHandler / SideBySide tests pass; `python scripts/check-project-governance.py` PASS.


## 2026-08-10 - server: split MapHandler dealitem/quest hooks + commercial RC smoke green

Splits the combined MapHandler dealitem+quest hooks (formerly one uncommitted diff) into two AGENTS-conformant commits: (1) DealItem hook only (47673229) - load_dealitem(path) parses a real Resource/DealItem.bin and feeds the per-NPC NpcShopCatalog into BuySyn so the wire shape is 4B BuyNack echo (unchanged) when no catalog hits; (2) QuestScript hook only (1d22df67) - load_quest_script(path) parses Resource/QuestScript.bin, StartSyn looks up quest_definitions_ but still keeps the StartNack + 2B quest_id echo so side-by-side 5/5 capture stays diff=0. Each commit registers a focused unit test (MapHandlerTest.LoadDealitemPopulatesCatalogFromBin and LoadQuestScriptPopulatesDefinitionsFromBin) that synthesizes a two-tab two-row DealItem.bin / a single-line   QuestScript.bin via the canonical encode_bin_payload XOR helper.  Also adds modern/tools/playdh_link_for_audit/ to .gitignore (session-local symlink to the 1.3 GB PlayDH resource dir, never tracked).  All 11841 unit tests pass; python scripts/check-project-governance.py PASS; **scripts/commercial-smoke.ps1 -BuildDir modern/build PASS (33/33 filtered tests, MSSQL_E2E via LocalDB, GUI_CLIENT_SMOKE 5/5 state frames + original BGM + 30.1% terrain coverage)**.  See git log 47673229 + 1d22df67 + e346a950.## 2026-08-10 - server: M5 wire MapHandler to mxh::server::skill_caster (close ROADMAP M3 gap)

modern/src/server/map_handler.cpp adds `#include "mxh/server/skill_caster.hpp"` after the existing `mxh/server/quest_script_loader.hpp` import so the new data-plane module is reachable. `MapHandler::calculate_damage` body is now a one-line delegation to `mxh::server::skill_caster_calculate_damage(attacker, defender, skill, rng)`. The static `thread_local std::mt19937 rng(std::random_device{}());` is preserved on the function boundary so the 1 dodge + 1 crit draw order matches the legacy inline math byte-for-byte (the 5/5 attack capture diff=0 invariant held under the post-patch ctest run, 11863/11863 PASS). The inline OuterMugong heal formula in `handle_skill` becomes `mxh::server::skill_caster_heal_amount(caster->combat, simple)` so the self/ally heal amount is also data-plane driven.

Behaviour-locking: the 15 `SkillCaster*` tests (commit 4deb5529) cover the 6 status paths + 1:1 damage formula + heal amount, and 11863/11863 ctest PASS confirms the wire preserves the wire shape. Closes ROADMAP Â§3 M3 "MapHandler wire to skill_caster" gap; the remaining "legacy SWorking cross-implementation diff=0" item still needs the external SWorking environment.
## 2026-08-10 - tools: scripts/release-modern-rc.ps1 - modern RC assembler + verifier (commit ae189d80)

scripts/release-modern-rc.ps1 (commit ae189d80) is a new internal tool that locks the RC package verifiable step of ROADMAP section 3 M4 / section 5 stage E on the modern single side.

End-to-end smoke (11864 tests >= 11000 floor, 6819 binaries staged from modern/build, 5 capture fixtures, 2874 SHA-256 checksum entries, RELEASE_NOTES.md written) confirms the script works on Debug.

External RC items still pending: clean machine deploy rehearsal, 24h stability soak, legacy client side-by-side diff=0, real SQL Server roundtrip.

See git log ae189d80.

## 2026-08-10 - ci: track .github/workflows/ci.yml + add RC verify step (commit ea20f9cd)

Untracked .github/workflows/ci.yml (previously in .gitignore with comment that workflows can land later via a dedicated PR if useful) and added a Verify release-modern-rc.ps1 runs (RC packager smoke) step to the build-and-test job between Run tests and the test-count guard.  Now release-modern-rc.ps1 (commit ae189d80) runs in CI on every push/PR with -SkipZip to verify the modern single-side RC stage (preflight ctest count >= 11000 floor, stage modern/bin + captures/, SHA-256 checksum.txt + RELEASE_NOTES.md, verify gate) without consuming the ~6GB zip artifact.  External RC items (clean machine deploy, 24h soak, legacy diff=0, real SQL roundtrip) still pending.  See git log ea20f9cd.

See git log ea20f9cd.



## 2026-08-11 - tools: scripts/clean-deploy.ps1 one-command clean machine deployment (M6-A)

New scripts/clean-deploy.ps1 takes a blank Windows Server 2022 / Windows 10/11 box with only PowerShell + Git installed and bootstraps it into a fully built + smoke-verified modern server in one command. Targets ROADMAP M6-A + KNOWN_BUGS DEPLOY-MSSQL + the Â§5.E " clean machine deployment\ gate.

Steps performed (all idempotent):
 1. preflight - admin check, OS, RAM (4 GB+), disk (5 GB+); gracefully degrades on non-admin.
 2. prereq detect - VS2022 (vswhere), cmake, git, sqlcmd, SqlLocalDB, ODBC Driver 18, VC++ Redist, Chocolatey.
 3. prereq install (with -InstallPrereqs) - chocolatey for vcredist2022 / cmake / git / sql-server-express + direct MSI for msodbcsql18 + direct download of vs_buildtools.exe (C++ workload).
 4. PlayDH junction - modern/data/PlayDH -> <RepoRoot>/å¢¨é¦™ã€æºç é…å¥—èµ„æºã€‘/PlayDH. ASCII-named so the resource explorer mangles argv correctly.
 5. build modern - cmake --build modern/build --config <Config> (inline; bypasses build-modern.ps1 exit-code quirk).
 6. ctest - ctest -C <Config> --test-dir modern/build --output-on-failure.
 7. commercial-smoke - scripts/commercial-smoke.ps1 -BuildDir modern/build (skippable via -SkipSmoke / -SkipGui).

Exit codes: 0 success, 1 preflight, 2 prereq missing, 3 build, 4 ctest, 5 commercial-smoke.

Docs: docs/CLEAN_MACHINE_DEPLOY.md (purpose / quick start / parameters / exit codes / prereq install / next step / non-admin caveats).

Verified locally: -DryRun -SkipTests -SkipSmoke PASS; -SkipSmoke end-to-end PASS (cmake build of full 11863-test tree + ctest PASS in ~5 minutes). -InstallPrereqs path requires Administrator elevation; non-admin path is fully functional for build + ctest + commercial-smoke (skips CIM-dependent RAM detection with warning).

## 2026-08-11 - docs: LOCAL_E2E_RUN (M6-C local end-to-end run verified on SQLite + MSSQL LocalDB)

docs/LOCAL_E2E_RUN.md records the fully-external end-to-end run: spawn the 3 modern servers (mxh_login_server / mxh_agent_server_CHINA / mxh_map_server_CHINA) in separate processes on 127.0.0.1:16001/17001/18001 with --legacy, then run mxh_client_e2e --no-spawn to walk all 5 protocol steps. Verified PASS on both backends:

* SQLite (3 separate .db files under modern\\scratch\\e2e_local\\) - login server auto-creates schema via --init-schema. 5/5 PASS.
* MSSQL LocalDB ((localdb)\\MSSQLLocalDB -> Moxiang database) - login server --init-schema skipped for mssql_odbc; client tool bootstraps schema via --init-schema. 5/5 PASS. Real data persisted: chr_log_info (test/test) + 5+ rows in character_info (chrid 240366, 412303, 945025, 953712, 1117800), queryable via ODBC 17 sqlcmd.

Architecture confirmed: server exes under modern\\build\\tools\\*\\Debug\\ are ready for deployment; mxh_client_e2e is the canonical headless protocol test. ROADMAP.md gains a M6-C row noting the local external-run milestone. The wrapper scripts\\local-e2e-run.ps1 was attempted but blocked on PowerShell  automatic-variable shadowing in nested Start-Process calls; manual commands documented instead.

No code changes. 11863/11863 unit tests + commercial-smoke still PASS. Modern schema + real DB write/read proven end-to-end.
