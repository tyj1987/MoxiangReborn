# Moxian-Reborn Documentation

补充技术文档。

## 目录

- [RESOURCE_FORMATS.md](./RESOURCE_FORMATS.md) - 资源格式逆向文档（持续更新）
- [PROTOCOL.md](./PROTOCOL.md) - 网络协议文档（待生成）
- [DATABASE_SCHEMA.md](./DATABASE_SCHEMA.md) - 数据库 schema（待生成）
- [KNOWN_BUGS.md](./KNOWN_BUGS.md) - 已知 bug 与陷阱（来自教程 4 + 阅读源码）

## 资源格式概览

| 格式 | 扩展名 | 用途 | 实现 | 完成度 |
|------|--------|------|------|--------|
| 加密二进制 | `.bin` | 业务数据（80+ 个文件） | `modern/src/mh_file_ex.cpp` | 100% |
| 资源包 | `.pak` | 模型/纹理/动画聚合 | `modern/src/pack_file.cpp` | 100% |
| 地图块 | `.bmhm` / `.mhm` | 地形几何 + 触发器 | `modern/src/bmhm_map.cpp` | 90% |
| Tile Table | `.ttb` | 地块纹理索引 | `modern/src/ttb_tile_table.cpp` | 70% |
| 角色模型 | `.chx` | 装备外观 | `modern/src/chx_model.cpp` | 80% |
| 角色动画 | `.chr` | 动作帧 | `modern/src/chr_motion.cpp` | 70% |
| 技能区域 | `.bsad` | 命中范围 | `modern/src/bsad_area.cpp` | 100% |
| 字符串索引 | `.mhs` | 多语言文本 | TODO | 0% |

## 当前进度

见 `/MODERNIZATION_PLAN.md` 第 5 节。