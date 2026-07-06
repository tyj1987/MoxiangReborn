# MoxianCompat - 资源兼容层

把 2003-2010 年自研格式用现代 C++ 重写，保持 **二进制 100% 兼容**。

## 已实现的格式

| 格式 | 类 | 状态 | 覆盖率 |
|------|----|------|--------|
| `.bin` (XOR 加密) | `mxh::compat::MhFileEx` | ✅ | 100% |
| `.pak` (4DyuchiFileStorage) | `mxh::compat::PackFile` | ✅ | 100% |
| `.bmhm/.mhm` (地图块) | `mxh::compat::BmhmMap` | 🚧 | 90% (待补全触发器) |
| `.ttb` (TileTable) | `mxh::compat::TtbTileTable` | 🚧 | 70% |
| `.chx` (角色模型) | `mxh::compat::ChxModel` | 🚧 | 80% |
| `.chr` (角色动画) | `mxh::compat::ChrMotion` | 🚧 | 70% |
| `.bsad` (技能区域) | `mxh::compat::BsadArea` | ✅ | 100% |

## 设计原则

1. **零拷贝优先**：直接 `mmap` 或 `fread` 一次性读取，解析器只读取不修改
2. **不依赖老 SDK**：纯标准 C++17 实现，可移植
3. **线程安全**：所有解析器为只读视图，多线程可共享
4. **错误透明**：每个 API 返回 `Result<T>` 而非抛异常（兼容老代码风格的返回值）