# Moxian (DarkStory) 数据库架构总览

## 概述

墨香使用 **3 个独立数据库**（MSSQL），分别管理账号、游戏数据和日志。现代重构使用 **IDbAdapter** 抽象层，支持 SQLite（默认）和 MSSQL (ODBC) 双后端。

| 数据库 | 用途 | 备份文件 |
|--------|------|---------|
| `MHCMEMBER` | 账号/会员/角色基本信息 | `MHCMEMBER.bak` |
| `MHGAME` | 游戏世界数据（物品/技能/工会/PvP） | `MHGAME.bak` |
| `MHLOG` | 操作日志 | `MHLOG.bak` |

---

## 一、与原实现的交互方式

Legacy 服务端通过 **ODBC + 存储过程** 与数据库交互：

- 所有查询走 **EXEC dbo.MP_xxx** 存储过程
- 零内联 SQL（`SELECT`/`INSERT`/`UPDATE`/`DELETE` 语句在 C++ 代码中**不存在**）
- 三个服务端进程各自使用独立的 `DBThread.dll` 线程池，通过 `CDB::FreeQuery` 异步发送查询
- 结果通过回调函数 `R*` 返回（如 `RLoginCheckQuery`）

这决定了**数据库 Schema 的精确定义在存储过程内部**，而非 DDL 文件。以下 schema 推导自：
- 存储过程 `#define` 列表（`MapDBMsgParser.h`、`AgentDBMsgParser.h`、`DistributeDBMsgParser.h`）
- ODBC 结果列枚举（`enum` 定义在 parser 头文件中）
- 现代 SQLite schema（`MoxianDbTool/main.cpp` 中的 `moxian_schema_sql()`）

---

## 二、MHCMEMBER 数据库（账号与角色）

### 表: `chr_log_info`（账号登录信息）

| 列 | 类型 | 说明 |
|----|------|------|
| `id` / `UserIDX` | INTEGER PRIMARY KEY | 自增用户索引 |
| `pw` | TEXT | 密码 |
| `userlevel` | INTEGER DEFAULT 0 | 权限等级（2=管理员，0=普通） |
| `registerdate` | TEXT | 注册日期 |
| `lastlogindate` | TEXT | 最后登录日期 |
| `lastloginip` | TEXT | 最后登录 IP |
| `usepoint` | INTEGER DEFAULT 0 | 充值点券 |

**引用方**：DistributeServer (`Up_Member_CheckIn`, `Up_Member_CheckOut`)

### 表: `character_info`（角色基本信息）

| 列 | 类型 | 说明 |
|----|------|------|
| `charname` / `CharacterName` | TEXT PRIMARY KEY | 角色名 |
| `chrid` / `CharacterIDX` | INTEGER UNIQUE | 角色唯一索引 |
| `userid` / `UserIDX` | TEXT NOT NULL | 所属账号 |
| `character_data` | BLOB | 完整角色数据（序列化） |

**引用方**：AgentServer, MapServer, MurimNetServer

### 角色查询列枚举（`AgentDBMsgParser.h`）

查询 `MP_CHARACTER_SelectByUserIDX` 返回的列顺序：

| 索引 | 枚举名 | 说明 |
|------|--------|------|
| 0 | `eCL_ObjectID` | CharacterIDX |
| 1 | `eCL_StandIndex` | 站姿索引 |
| 2 | `eCL_ObjectName` | 角色名 |
| 3 | `eCL_BodyType` | 体型 |
| 4 | `eCL_HeadType` | 头型 |
| 5 | `eCL_Hat` | 帽子 |
| 6 | `eCL_Dress` | 衣服 |
| 7 | `eCL_shoes` | 鞋子 |
| 8 | `eCL_Weapon` | 武器 |
| 9 | `eCL_Grade` | 等级 |
| 10 | `eCL_Map` | 当前地图 |
| 11 | `eCL_Gender` | 性别 |
| 12 | `eCL_Height` | 身高 |
| 13 | `eCL_Width` | 体重 |
| 14 | `eCL_Stage` | 阶段 |
| 15 | `eCL_AuthKey` | 认证密钥 |

---

## 三、MHGAME 数据库（游戏数据）

### 表: `item_info`（物品）

| 列 | 类型 | 说明 |
|----|------|------|
| `itemid` | INTEGER PRIMARY KEY | 物品全局 ID |
| `owner_chr` | TEXT | 所属角色名 |
| `item_idx` | INTEGER | 物品模板索引 |
| `position` | INTEGER | 位置（背包/装备栏/仓库/工会仓） |
| `durability` | INTEGER | 耐久度 |
| `seal_info` | BLOB | 封印/强化信息 |

### 表: `munpa_info`（门派/工会）

| 列 | 类型 | 说明 |
|----|------|------|
| `munpaid` / `GuildIDX` | INTEGER PRIMARY KEY | 工会 ID |
| `munpaname` / `GuildName` | TEXT NOT NULL | 工会名 |
| `master_idx` | INTEGER | 门主角色索引 |
| `member_data` | BLOB | 成员序列化数据 |

### 表: `note_list`（纸条/邮件）

| 列 | 类型 | 说明 |
|----|------|------|
| `noteid` | INTEGER PRIMARY KEY | 消息 ID |
| `sender` | TEXT | 发送者 |
| `receiver` | TEXT | 接收者 |
| `message` | TEXT | 内容 |
| `senddate` | TEXT | 发送日期 |

---

## 四、MHLOG 数据库（日志）

### 表: `log_money`（货币日志）

| 列 | 类型 | 说明 |
|----|------|------|
| `logid` | INTEGER PRIMARY KEY AUTOINCREMENT | 日志 ID |
| `chrname` | TEXT | 角色名 |
| `amount` | INTEGER | 变动金额 |
| `reason` | TEXT | 原因 |
| `logtime` | TEXT | 时间戳 |

### 表: `log_item`（物品日志）

| 列 | 类型 | 说明 |
|----|------|------|
| `logid` | INTEGER PRIMARY KEY AUTOINCREMENT | 日志 ID |
| `chrname` | TEXT | 角色名 |
| `itemid` | INTEGER | 物品 ID |
| `action` | TEXT | 动作（获得/丢弃/交易） |
| `logtime` | TEXT | 时间戳 |

### 表: `log_chat`（聊天日志）

| 列 | 类型 | 说明 |
|----|------|------|
| `logid` | INTEGER PRIMARY KEY AUTOINCREMENT | 日志 ID |
| `chrname` | TEXT | 角色名 |
| `channel` | TEXT | 频道（一般/门派/组队/密语） |
| `message` | TEXT | 消息内容 |
| `logtime` | TEXT | 时间戳 |

---

## 五、存储过程清单

所有 DB 操作使用存储过程封装。以下按领域分类列出各存储过程及其调用方。

### 5.1 账号与登录（DistributeServer）

| 存储过程 | 说明 |
|----------|------|
| `Up_Member_CheckIn` (id, pw, authkey, type) | 登录验证 |
| `Up_Member_CheckOut` (userid) | 登出 |
| `Up_Ip_CheckIn` (ip, authkey) | IP 验证 |
| `Up_Ip_CheckInHK` (ip, authkey) | 香港 IP 验证 |
| `MP_CHARACTER_LoginInit` | 登录初始化 |
| `up_Server_ResetLoginMember` (servernum) | 重置在线人数 |
| `Up_GameLogOut` (userid) | 游戏登出 |
| `Up_GameLogOut_JP` (userid) | 日服登出 |

### 5.2 角色管理（AgentServer + MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_CHARACTER_SelectByUserIDX` (useridx, authkey) | 按账号查询角色列表 |
| `MP_CHARACTER_SelectByCharacterIDX` (chrid) | 按 ID 查角色详情 |
| `MP_CHARACTER_SelectByCharacterIDX_JP` (chrid) | 日服版 |
| `MP_CHARACTER_NameCheck` (name) | 检查角色名是否可用 |
| `MP_CHARACTER_DeleteCharacter` (chrid, serverno, ip) | 删除角色 |
| `MP_LoginCharacterSearchForName` (name, chrid) | 按名搜索登录角色 |
| `MP_CHARACTER_MapchangePointUpdate` (chrid, idx) | 更新地图传送点 |

### 5.3 角色详情（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_CHARACTER_KyungGong` (chrid) | 轻功信息 |
| `MP_CHARACTER_NaeGong` (chrid) | 内功信息 |
| `MP_CHARACTER_MugongInfo` (chrid) | 武功/技能信息 |
| `MP_CHARACTER_ItemSlotInfo_JP` (chrid) | 物品槽位信息 |
| `MP_CHARACTER_ItemInfo` (chrid) | 物品详情 |
| `MP_CHARACTER_ItemRareOptionInfo` (chrid) | 稀有属性信息 |
| `MP_CHARACTER_ItemOptionInfo` (chrid) | 物品选项信息 |
| `MP_CHARACTER_SkillInfo` (chrid) | 技能信息 |
| `MP_CHARACTER_UpdateExpFlag` (chrid) | 更新经验标记 |
| `MP_CHARACTER_HeroInfoUpdate` | 英雄信息更新 |
| `MP_CHARACTER_TotalInfoUpdate` | 总览信息更新 |
| `MP_CHARACTER_BadFameUpdate` | 恶名更新 |
| `MP_CHARACTER_SaveInfoBeforeLogOut` | 登出前保存 |
| `MP_CHARACTER_UpdateResetStatusPoint` (chrid, point) | 重置状态点 |

### 5.4 物品系统（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_ITEM_Update` | 物品更新 |
| `MP_ITEM_CombineUpdate` | 物品合成 |
| `MP_ITEM_MoveUpdate` | 物品移动（背包） |
| `MP_ITEM_MoveUpdatePyoguk` | 物品移动（仓库） |
| `MP_ITEM_MoveUpdateMunpa` | 物品移动（工会仓） |
| `MP_ITEM_Insert` | 物品创建 |
| `MP_ITEM_Delete` | 物品删除 |
| `MP_ITEM_RARE_OPTION_Insert` | 稀有属性创建 |
| `MP_ITEM_RARE_OPTION_Delete` | 稀有属性删除 |
| `MP_ITEM_OPTION_Insert` | 物品选项创建 |
| `MP_ITEM_OPTION_Delete` | 物品选项删除 |

### 5.5 武功系统（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_MUGONG_Update` | 武功更新 |
| `MP_MUGONG_MoveUpdate` | 武功移动 |
| `MP_MUGONG_Insert` | 武功习得 |
| `MP_MUGONG_Delete` | 武功遗忘 |

### 5.6 拍卖行（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_AUCTION_CHECK` (chrid) | 检查是否有拍卖 |
| `MP_AUCTION_SEARCH` (type, page) | 搜索拍卖 |
| `MP_AUCTION_SORT` | 排序 |
| `MP_AUCTION_REGISTER` (idx, amount, duedate, price, immediate, name) | 注册拍卖 |
| `MP_AUCTION_JOIN` (idx, price, name) | 参与竞拍 |
| `MP_AUCTION__REGISTER_CANCEL` (type, regidx, name) | 取消拍卖 |
| `MP_AUCTION_Regist` | 注册页计算 |
| `MP_AUCTION_PageCalculate` | 分页计算 |

### 5.7 组队系统（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_PARTY_Regist` | 注册队伍（地图服） |
| `MP_PARTY_PartyInfoByUserLogin` | 登录时获取队伍信息 |
| `MP_PARTY_CreateParty` | 创建队伍 |
| `MP_PARTY_BreakupParty` | 解散队伍 |
| `MP_PARTY_DelPartyidxinCharacterTB` | 清除角色队伍索引 |
| `MP_PARTY_UpdateMember` | 更新成员 |
| `MP_PARTY_AddMember` | 添加成员 |
| `MP_PARTY_DelMember` | 移除成员 |
| `MP_PARTY_ChangeMaster` | 更换队长 |

### 5.8 工会系统（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_GUILD_Create` | 创建工会 |
| `MP_GUILD_BreakUp` | 解散工会 |
| `MP_GUILD_DeleteMember` | 踢出成员 |
| `MP_GUILD_AddMember` | 加入成员 |
| `MP_GUILD_LoadGuild` | 加载工会信息 |
| `MP_GUILD_LoadNotice` | 加载公告 |
| `MP_GUILD_UpdateNotice` | 更新公告 |
| `MP_GUILD_LoadMember` | 加载成员列表 |
| `MP_GUILD_MarkRegist` | 注册徽章 |
| `MP_GUILD_MarkUpdate` | 更新徽章 |
| `MP_GUILD_LoadMark` | 加载徽章 |
| `MP_GUILD_LevelUp` | 工会升级 |
| `MP_GUILD_ChangeRank` | 变更成员等级 |
| `MP_GUILD_LoadItem` | 加载工会物品 |
| `MP_GUILD_GiveMemberNickName` | 设置成员别名 |
| `MP_GUILD_MoneyUpdate` | 工会资金更新 |
| `MP_GUILD_ItemOption_Info` | 工会物品选项 |
| `MP_GUILD_TRAINEE_Info` | 见习成员信息 |
| `MP_GUILD_TRAINEE_Insert` | 添加见习 |
| `MP_GUILD_TRAINEE_Delete` | 删除见习 |
| `MP_GUILD_AddStudent` | 添加弟子 |

### 5.9 商城系统（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_SHOPITEM_InvenInfo` | 商城背包信息 |
| `MP_SHOPITEM_ItemInfo` | 商城道具信息 |
| `MP_SHOPITEM_UseInfo` | 使用信息 |
| `MP_SHOPITEM_Using` | 道具使用 |
| `MP_SHOPITEM_Delete` | 删除 |
| `MP_SHOPITEM_Updatetime` | 更新时间 |
| `MP_SHOPITEM_UpdateParam` | 更新参数 |
| `MP_SHOPITEM_UpdateUseParam` | 更新使用参数 |
| `MP_SHOPITEM_GetItem` | 获取物品 |
| `MP_ITEM_MoveUpdateShop` | 物品移动（商城） |
| `MP_character_rename` | 角色改名 |
| `MP_SHOPITEM_CharChange` | 角色变更 |

### 5.10 好友与纸条（AgentServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_FRIEND_NotifyLogout` | 通知好友下线 |
| `MP_FRIEND_GetTargetIDX` (name, chrid) | 获取目标好友 ID |
| `MP_FRIEND_AddFriend` | 添加好友 |
| `MP_FRIEND_DeleteFriend` | 删除好友 |
| `MP_FRIEND_SendNote` | 发送纸条 |
| `MP_FRIEND_LoadNoteList` | 加载纸条列表 |
| `MP_FRIEND_ReadNote` | 阅读纸条 |
| `MP_FRIEND_DeleteNote` | 删除纸条 |

### 5.11 成名/恶名（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_FAME_CharacterUpdate` | 成名度更新 |
| `MP_BADFAME_CharacterUpdate` | 恶名度更新 |
| `MP_PK_CharacterUpdate` | PK 计数更新 |

### 5.12 地图与传送（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_MAP_BaseEconomy` (chrid) | 地图基础经济 |
| `MP_LogInMapInfo_Regist` (chrid, mapidx, etc) | 登录时注册地图信息 |
| `MP_LogInMapInfo_UnRegist` (chrid) | 登出时注销地图信息 |
| `MP_LoginMapInfo_MapUserUnRegist` (mapport) | 地图服注销 |
| `MP_MOVEPOINT_GetInfo` | 传送点信息 |
| `MP_MOVEPOINT_Insert` | 添加传送点 |
| `MP_MOVEPOINT_Update` | 更新传送点 |
| `MP_MOVEPOINT_Delete` | 删除传送点 |

### 5.13 泰坦系统（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_TITAN_WearItemInfo` (chrid) | 泰坦装备信息 |

### 5.14 雇佣/通缉（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_WANTED_LoadList` | 通缉列表 |
| `MP_WANTED_InfoByUserLogIn` | 登录时通缉信息 |
| `MP_WANTED_BuyRight` | 购买通缉权 |
| `MP_WANTED_Regist` | 注册通缉 |
| `MP_WANTED_GiveUpRight` | 放弃通缉权 |
| `MP_WANTED_Complete` | 通缉完成 |
| `MP_WANTED_Destroyed` | 通缉目标死亡 |
| `MP_WANTED_OrderList` | 排序列表 |

### 5.15 仓库（MapServer）

| 存储过程 | 说明 |
|----------|------|
| `MP_PYOGUK_Buy` | 购买仓库 |
| `MP_PYOGUK_MoneyUpdate` | 仓库资金更新 |
| `MP_PYOGUK_Info` | 仓库信息 |
| `MP_PYOGUK_ItemInfo` | 仓库物品信息 |
| `MP_PYOGUK_Titan_Endurance_Info` | 仓库泰坦耐久 |

---

## 六、现代 IDbAdapter 抽象层

### 架构

```
┌─────────────────────────────────────────┐
│              IDbAdapter                  │
│  (纯虚接口: connect/execute/query/txn)   │
├─────────────────┬───────────────────────┤
│  SqliteAdapter  │  MssqlOdbcAdapter     │
│  (默认, 零配置)  │  (可选, 原生兼容.bak) │
└─────────────────┴───────────────────────┘
```

### 核心接口 (见 `modern/include/mxh/db/db_adapter.hpp`)

| 方法 | 说明 |
|------|------|
| `connect(ConnectionConfig)` | 连接数据库 |
| `disconnect()` | 断开连接 |
| `execute(sql, params)` | 执行 INSERT/UPDATE/DELETE/DDL |
| `query(sql, params, ResultSet&)` | 执行 SELECT 并返回结果集 |
| `begin_transaction()` / `commit()` / `rollback()` | 事务支持 |

### 数据类型

- `Value` = `variant<monostate, int64_t, double, string, vector<uint8_t>>`（NULL 表示为 `monostate`）
- `ResultSet` = `{vector<string> columns, vector<Row> rows}`
- `Bind` = 预备语句参数（位置绑定）

### SQLite 适配器关键配置

```cpp
ConnectionConfig cfg;
cfg.backend = "sqlite";       // 选择后端
cfg.path    = "./moxian.db";  // 文件路径
```

连接后自动启用: `PRAGMA foreign_keys=ON; PRAGMA journal_mode=WAL;`

---

## 七、MSSQL → SQLite 迁移策略

### 已知差异

| 差异项 | MSSQL | SQLite |
|--------|-------|--------|
| 存储过程 | EXEC dbo.MP_xxx (@param, ...) | 不支持，需内联 SQL |
| 自增 | `IDENTITY(1,1)` | `AUTOINCREMENT` |
| 索引 | `CREATE INDEX ON` (schema-scoped) | `CREATE INDEX IF NOT EXISTS` |
| 二进制 | `VARBINARY(MAX)` | `BLOB` |
| 事务嵌套 | 支持 | 不支持（SAVEPOINT 模拟） |
| 批量 DML | `INSERT ... SELECT` | 逐一执行 |

### 现代代码的应对

1. **存储过程 → C++ 函数**：每个 `EXEC` 调用转化为 `IDbAdapter::execute()` 调用，SQL 语句在 C++ 中构建
2. **Schema 通过 `exec_multi()` 初始化**：在 `MoxianDbTool` 的 `init` 子命令中定义完整的 `CREATE TABLE` DDL
3. **三后端统一接口**：`IDbAdapter` 屏蔽差异，`MssqlOdbcAdapter` 供遗留兼容，`SqliteAdapter` 供现代开发
4. **MoxianDbTool 辅助**：提供 `init/exec/query/schema` 子命令，支持一键建库和数据查询

---

## 八、开发与运维

### 初始化 SQLite 数据库

```bash
# 使用 MoxianDbTool 初始化
mxh_db_tool init --db "sqlite;path=./moxian.db"

# 或使用 LoginServer 启动时初始化
mxh_login_server --db ./moxian.db --init-schema
```

### 还原 MSSQL 备份

```bash
# 将 .bak 还原到 SQL Server，然后使用 MssqlOdbcAdapter 连接
mxh_db_tool query --db "mssql_odbc;dsn=MoxianMHGAME;database=MHGAME" "SELECT * FROM chr_log_info"
```

### 数据导出/迁移

```bash
# 导出 MSSQL 表
mxh_db_tool query --db "mssql_odbc;..." "SELECT * FROM chr_log_info" > data.tsv

# 导入 SQLite
mxh_db_tool exec --db "sqlite;path=./moxian.db" ".import data.tsv chr_log_info"
```
