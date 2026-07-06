# Moxian-Reborn Modern Source Tree

> 新代码遵循现代 C++17/20 规范，老代码保留原状。

## 结构

```
modern/
├── CMakeLists.txt              # 现代 CMake 工程入口
├── include/mxh/compat/         # 公共头（资源兼容层、协议抽象等）
├── src/                        # 实现
│   ├── render/                 # DX11 渲染后端（Phase 5）
│   ├── db/                     # IDbAdapter (Phase 2)
│   ├── net/                    # I4DyuchiNET (Phase 4)
│   ├── crypto/                 # HSEL 替代 (Phase 3)
│   ├── server/                 # 登录/代理服务 (Phase 0-4)
│   ├── log/                    # MLOG 兼容宏 (Phase 0)
│   └── proto/                  # 协议常量 (Phase 0)
├── tests/                      # 单元测试（GoogleTest）
│   └── unit/
│       ├── db/                 # IDbAdapter + 真实资源回环测试
│       ├── net/                # 异步 socket
│       └── render/             # TGA 解码器等 CPU 侧测试
├── tools/
│   └── MoxianResourceExplorer/ # 资源浏览器 CLI 工具
└── docs/                       # 资源格式逆向文档
```

## 命名空间

- `mxh` — Moxian 项目根命名空间
- `mxh::compat` — 资源兼容层（读取老格式）

## 构建

```powershell
# 配置（用 Visual Studio 2022 x64）
cmd /c "call C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && cmake -S modern -B modern/build -G 'Visual Studio 17 2022' -A x64"

# 增量构建
cmd /c "call C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && cmake --build modern/build --config Debug --parallel"

# 全量构建（先清干净）
cmd /c "call C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && cmake --build modern/build --config Debug --target clean && cmake --build modern/build --config Debug"
```

## 测试

```powershell
# 跑全部 GoogleTest
cmd /c "call C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && ctest --test-dir modern/build/tests -C Debug --output-on-failure"

# 只跑 TGA 解码器测试
cmd /c "call C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && ctest --test-dir modern/build/tests/unit/render -C Debug --output-on-failure"

# Release 版本同样可测（与 Debug 输出对齐）
cmd /c "call C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && ctest --test-dir modern/build/tests -C Release --output-on-failure"
```

当前测试覆盖：

| 套件 | case 数 | 内容 |
|------|---------|------|
| `MhFileEx` | 6 | `.bin` XOR/位移加解密 + CRC 校验 |
| `PackFile` | 5 | `.pak` 头解析 + 实资源回环 |
| `BsadArea` | 4 | `.bsad` 技能区域解析 |
| `DbAdapter` | 4 | `IDbAdapter` 工厂 + 配置 |
| `SqliteAdapter` | 5 | SQLite 后端（事务/BLOB/文件持久化） |
| `RealResource` | 2 | 真实 `MonsterList.bin` + `Effect.pak` 跑通（缺失时 SKIP） |
| `TgaLoader` | 7 | TGA uncompressed/RLE/bottom-up-flip/RGBA32 |
| **合计** | **33** | Debug + Release 全过 |

## 编码规范

- C++17 起步，允许 C++20 特性（`concepts`, `ranges`, `coroutines`）
- 头文件 `.hpp`，源文件 `.cpp`
- 头文件 `#pragma once`
- 命名空间 `mxh::xxx`
- 类名 `PascalCase`
- 函数/变量 `snake_case`
- 成员变量 `snake_case_`（尾下划线）
- 常量 `kPascalCase`
- 不用匈牙利命名法（仅兼容老代码时）