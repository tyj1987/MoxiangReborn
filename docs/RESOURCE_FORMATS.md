# Moxian 资源格式逆向文档

> 这是 Phase 1 资源兼容层的逆向文档。所有格式信息直接来自原代码 + 实测。

## 1. `.bin` — 加密二进制业务表

**用途**：游戏 80+ 业务数据（ItemList.bin、MonsterList.bin、SkillList.bin 等）。

**来源**：`墨香【源码】\[Tool]PackingMan\MHFileEx.cpp`

**结构**（按出现顺序）：

```
[12 字节]  MHFILE_HEADER { version:u32, type:u32, file_size:u32 }
[可选]     CRC1 (u8)  — 大多数实际文件缺失
[可选]     CRC2 (u8)
[N 字节]   加密载荷（N = file_size）
[可选]     尾部 CRC (u8)
```

**加密算法**（解压时）：

```cpp
for (i = 0; i < size; ++i) {
    data[i] -= (uint8_t)i;
    if (type > 0 && (i % type == 0)) {
        data[i] -= type;
    }
}
```

- `type == 0`：只做 `-= i`
- `type > 0`：在每 `type` 字节的整数倍位置上额外 `-= type`

**Header 字段**：
- `version`：`0x00000001`（最常见）或 `0x00000002`
- `type`：`0`、`1`、`2`、`3`、`4`
- `file_size`：载荷字节数

**Modern 实现**：`modern/src/mh_file_ex.cpp` + `modern/include/mxh/compat/mh_file_ex.hpp`

---

## 2. `.pak` — 资源包（4DyuchiFileStorage）

**用途**：模型/纹理/动画聚合（Effect.pak、Character.pak、Map.pak、Monster.pak、Npc.pak、Pet.pak、Titan.pak）。

**来源**：`墨香【源码】\4DyuchiFileStorage\PackFile.cpp` + `typedef.h`

**结构**：

```
[16 字节]   PACK_FILE_HEADER { total_size:u32, file_count:u32, version:u32, flag:u32 }

Per file:
  [32 字节]  PACK_FILE_DESC { total_size:u32, real_file_size:u32,
                              file_name_len:u32, file_data_offset:u32,
                              flag1:u32, flag2:u32, flag3:u32, flag4:u32 }
  [N 字节]   filename (no NUL terminator)
  [0-3 字节] padding to 4-byte alignment
  (data 在 file_data_offset 处单独存放，可不连续)
```

**关键字段**：
- `version`：`0x00000100`（最常见）或 `0x00000001`
- `file_name_len`：文件名长度（不含 NUL）
- `file_data_offset`：相对于 `.pak` 文件开头的偏移

**文件名规则**：
- 使用反斜杠 `\`（Windows 风格）
- 客户端查找时大小写不敏感

**Modern 实现**：`modern/src/pack_file.cpp` + `modern/include/mxh/compat/pack_file.hpp`

---

## 3. `.bmhm` / `.mhm` — 地图块

**用途**：地形几何 + 高度图 + Tile 索引 + 触发器。

**来源**：`墨香【源码】\4DyuchiGXMapEditor\TileSet.cpp` + 客户端 `[Client]MH/MHMap.cpp`

**Magic Header**（8 字节）：`7E CB 31 01 2A 00 00 00`

**结构**（推测，需补全）：

```
[8 字节]    Magic
[4 字节]    version
[4 字节]    width  (tile count X)
[4 字节]    height (tile count Z)
[4 字节]    tile_size (50)
[4 字节]    hfield_offset
[4 字节]    tile_offset
[4 字节]    trigger_offset
[16 字节]   reserved

[HField]    width*height × float32 (高度场)
[Tile]      4 × width*height 字节 (地块索引)
[Triggers]  NPC/跳跃点/登录点列表
```

**Modern 实现**：`modern/src/bmhm_map.cpp`（90% 完成，触发器解析待补）

---

## 4. `.ttb` — Tile Table

**用途**：与 `.bmhm` 配合，查地块纹理索引。

**结构**（待补全，原代码 `MHMap.cpp` 解析）：

```
[4 字节]    width (?)
[4 字节]    height (?)
[N 字节]    tile indices (u32 each, row-major)
```

**Modern 实现**：`modern/src/ttb_tile_table.cpp`（70% 完成）

---

## 5. `.chx` — 角色模型

**用途**：装备/角色外观模型。

**来源**：`墨香【源码】\MAXEXP\export.cpp`（3ds Max Biped/Physique 插件导出）

**结构**（版本相关）：

```
[32 字节]   Header { magic:u32, version:u32, mesh_count, bone_count,
                    material_count, vertex_count, index_count, reserved }
[vertices]  vertex_count × 12 字节 (xyz float32)
[normals]   vertex_count × 12 字节 (xyz float32)
[uvs]       vertex_count × 8 字节 (uv float32)
[indices]   index_count × 2-4 字节 (u16 or u32)
[mesh_defs] mesh_count × MeshDescriptor
[bone_defs] bone_count × BoneDescriptor
[materials] material_count × MaterialDescriptor
```

**3ds Max 版本限制**：原 MAXEXP 插件需要 **3ds Max 7-2017**（2018+ 移除了 Biped/Physique）。

**Modern 实现**：`modern/src/chx_model.cpp`（80% 完成）

---

## 6. `.chr` — 角色动画

**用途**：动作帧（待机、走、跑、攻击、技能）。

**结构**（待补全，原代码 `MOTION.CPP`）：

```
[Header]    magic:u32, version:u32, frame_count:u32, bone_count:u32, fps:u32
[bone_tracks]  bone_count × frame_count × BoneTransform
```

**Modern 实现**：`modern/src/chr_motion.cpp`（70% 完成）

---

## 7. `.bsad` — 技能区域

**用途**：技能命中范围（如 "9x9_Blank"、"13x13_Spikewall"）。

**结构**：

```
[8 字节]    Header { width:u16, height:u16, reserved:u32 }
[N 字节]    cells (N = width × height), each cell is 1 byte:
            0 = Empty
            1 = Hit
            2 = Block
```

**Modern 实现**：`modern/src/bsad_area.cpp`（100% 完成）

---

## 8. `.mhs` — 字符串索引

**用途**：多语言文本（韩文/中文/日文/英文）。

**Modern 实现**：TODO

---

## 9. 加密文件（nProtect / HackShield）

**加密狗 / 反外挂**：物理狗文件 `HShield\...` + 运行时 DLL。
**协议加密**：`[Lib]HSEL/` + `MHFile.cpp` 的 CRC 字段。

**状态**：已停止维护。建议现代实现使用 AES-256-GCM 替代，HSEL 接口层保留兼容。

---

## 10. 数据库

**类型**：Microsoft SQL Server
**备份文件**：`MHCMEMBER.bak` / `MHGAME.bak` / `MHLOG.bak`
**位置**：`墨香【客户端+服务端+工具】\DB.zip`（4.5 MB）

**三库分工**：
- `MHCMEMBER`：账号、角色基础信息（`chr_log_info` 表等）
- `MHGAME`：物品、工会、邮件、拍卖等业务
- `MHLOG`：操作日志

**Modern 实现**：保留 MSSQL；新增 `IDbAdapter` 抽象（Phase 2）。