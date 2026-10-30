# Item 协议兼容性边界

更新：2026-10-09。当前 modern 客户端与服务端配对运行，不等于与原始 Item 协议兼容。原资源字节对齐、原玩法语义、网络兼容分别验收。本页固定已核验的差异，不启用新的运行协议，也不改变原始头文件。

## 固定参考

参考为本机保留的 `reference/legacy-source/4dddd9a6`。目录名是保留节点标识，不代表已证明某个官方发行版本。

| 文件 | SHA-256 |
|---|---|
| `[CC]Header/Protocol.h` | `f0a0c0678bab16b7787bf273d97f31660820e37db6bb3f4f9dec23e104d0e656` |
| `[CC]Header/CommonStruct.h` | `5b485d298868a87294de7ebfc7a64919c69a754d9f9a44422251bf1947524625` |
| `[CC]Header/CommonStructMS.h` | `c8e04da9b213bf426e8e80b3dbe44d356ddbafae8c1d4d7a1ae4326f047ae169` |
| `[Server]Map/ItemManager.cpp` | `fee5a7ef8800be38dab6f959e00f4f93172c38498b59d6b6ec17e5c4454d7170` |
| `[Client]MH/ItemManager.cpp` | `2634e373675c92dc53618796a35042903e49fb489e812e623a57e3c47cff6d5b` |

本轮逐文件核验确认，参考 Protocol.h 和服务端 ItemManager 与原版候选保留的原文件相同。完整原客户端 ABI 构建尚未通过，本表不扩大为全部源码、资源或历史版本一致。

## 通知结构与含义

原 `MP_ITEM_MONSTER_OBTAIN_NOTIFY` 使用 `ITEMOBTAINARRAY`。在 `MSGBASE` 后，其负载是 `ItemNum:u8 + wObtainCount:u16 + ITEMBASE[ItemNum]`，长度为 **3 + 22×ItemNum** 字节。`wObtainCount` 是获得数量，不能当成数组记录数。

| ITEMBASE 偏移 | 字节 | 字段 |
|---:|---:|---|
| 0 | 4 | dwDBIdx |
| 4 | 2 | wIconIdx |
| 6 | 2 | Position |
| 8 | 4 | Durability |
| 12 | 4 | RareIdx |
| 16 | 2 | QuickPosition |
| 18 | 4 | ItemParam |

上述布局按原 Windows 固定宽类型和 pack(1) 得出。原头部还受 `_CRYPTCHECK_` 影响；不得把负载尺寸直接当成完整网络包尺寸，也不得忽略外层 framing。

原服务端通过物品分配及数据库返回记录构造通知；客户端按背包 Position 添加或更新物品，播放英雄获得特效。这条路径不创建可点击的地面物品。旧代码的部分已有堆叠更新可提前回调，不能把它描述成所有分支均具备事务提交后确认。

当前 modern 将同名通知解释为 20 字节的地面对象（对象、怪物、物品、数量及坐标），并使用独立 Pickup 请求。这是另一套语义。原参考中的普通 `MP_ITEM_DROP` 和 `AddItemObject` 已被注释，本轮没有找到该版本有效的普通地面点击拾取闭环；这不代表所有历史墨香版本都没有该玩法。

## 编号冲突

原 `YY_MOVE` 三个枚举属于注释，不占用编号。以去除注释后的完整枚举计算：

| 消息 | 原始编号 | 当前 modern 编号 |
|---|---:|---:|
| MoveSyn/Ack/Nack | 16/17/18 | 16/17/18 |
| BuySyn/Ack/Nack | 19/20/21 | 22/23/24 |
| PickupSyn/Ack/Nack | 无对应有效原消息 | 19/20/21 |
| SellSyn/Ack/Nack | 22/23/24 | 25/26/27 |
| Money / MoneyError | 28/29 | 31/32 |
| AppearanceChange / ObtainMoney | 30/31 | 33/34 |
| MonsterObtainNotify | 32 | 35 |

原编号 35 为 `MP_ITEM_UPGRADE_FAILED_ACK`。不能只调整某个编号，或单边部署整体重编号。原头文件、现有已部署 modern 协议和回滚包在迁移期间分别保留。

## 实施与验收门禁

1. MonsterList 字段 48–58 和纯掉落决策先独立实现与测试。Money、NoItem、三张表及最大次数分别处理；不因解析修复自动启用死亡路径。
2. 建立独立的原 Item profile 与数组 codec，枚举验证由固定原参考提取，避免两个硬编码表互相证明。测试零条、单条、多条、错误计数、截断和旧 20 字节地面负载。
3. 原语义的直接授予需核对收件人、堆叠、容量、容器和金额；权威写入成功后再通知，失败不得发送成功。数据库用例只在 PVE 执行，金额不能伪造为物品。
4. 用户计划中的点击拾取继续作为待交付能力。如果保留 modern 地面交互，必须有明确的扩展协议及版本边界，不能占用原 Buy 编号或将其冒充原 MonsterObtainNotify。原版 1:1 结论另需准确版本的证据。
5. 双端迁移、原 framing/宏、部署契约与回退验证完成后才切换运行。正常 GUI 击杀、获得或拾取、背包及重登持久化分别验收；纯测试不能代替这些结果。

当前 `Resource/MonsterDropItemList.bin` 与固定 SWorking 参考表字节不同，不可交换为同版概率。当前同名 `Resource/Server/MonsterDropItemList.bin` 未通过掉落表解析；强制解码得到 `$Group` 前缀不足以证明完整 AI 语法，继续拒绝作为掉落表回退。

详细证据：本轮审计目录 `original-integration/drop-protocol-compatibility-review.md`。可玩状态及剩余门禁见 [PLAYABLE_STATUS.md](PLAYABLE_STATUS.md) 与 [VERIFICATION_MATRIX.md](VERIFICATION_MATRIX.md)。
