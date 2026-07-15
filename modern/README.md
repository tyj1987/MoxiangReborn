# Moxian-Reborn Modern Source Tree

> **Phase 0-10 Complete** — 新代码遵循现代 C++20 规范，老代码保留原状。
> 
> [![CI](https://github.com/your-org/moxian-reborn/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/moxian-reborn/actions/workflows/ci.yml)
> 
> **430 tests passing** | **Phase 0-10 complete** | **Modern C++20**

## 结构

```
modern/
├── CMakeLists.txt              # 现代 CMake 工程入口
├── include/mxh/                # 公共头
│   ├── compat/                 # 资源兼容层、协议抽象等
│   ├── crypto/                 # HSEL 加密接口
│   ├── db/                     # IDbAdapter 数据库接口
│   ├── net/                    # TCP 网络层 (Phase 4)
│   ├── proto/                  # 协议常量
│   ├── render/                 # DX11 渲染后端 (Phase 5)
│   ├── server/                 # 登录/代理服务
│   └── util/                   # 工具库 (Phase 8)
│       ├── thread_pool.hpp     # 线程池 (8.1)
│       ├── object_pool.hpp     # 对象池 (8.2)
│       └── compress.hpp        # 压缩 (8.3)
├── src/                        # 实现
├── tests/                      # 单元测试（GoogleTest）
│   └── unit/
│       ├── db/                 # IDbAdapter + 真实资源回环测试
│       ├── net/                # TCP 网络 + 加密 + 版本协商
│       ├── render/             # TGA 解码器
│       ├── version_test.cpp    # 协议版本化 (Phase 4.3)
│       ├── crypto_test.cpp     # HSEL 加密 (Phase 3)
│       ├── util_test.cpp       # 工具库 (Phase 8)
│       └── legacy_compat_test.cpp  # 遗留兼容
├── tools/
│   ├── MoxianResourceExplorer/ # 资源浏览器 CLI
│   └── ...                    # 其他工具
├── vcpkg.json                 # 依赖管理
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
cd modern/build
ctest -C Debug --timeout 60 --output-on-failure

# 只跑网络测试
ctest -C Debug -R "net" --output-on-failure

# Release 版本
ctest -C Release --timeout 60 --output-on-failure

# 性能基准
.\tests\unit\net\Debug\net_benchmark.exe 1000
```

当前测试覆盖（**410 tests passing**）：

| 套件 | case 数 | 内容 |
|------|---------|------|
| `MhFileEx` | 6 | `.bin` XOR/位移加解密 + CRC 校验 |
| `PackFile` | 5 | `.pak` 头解析 + 实资源回环 |
| `BsadArea` | 4 | `.bsad` 技能区域解析 |
| `DbAdapter` | 4 | `IDbAdapter` 工厂 + 配置 |
| `SqliteAdapter` | 5 | SQLite 后端（事务/BLOB/文件持久化） |
| `RealResource` | 2 | 真实 `MonsterList.bin` + `Effect.pak` 跑通（缺失时 SKIP） |
| `TgaLoader` | 7 | TGA uncompressed/RLE/bottom-up-flip/RGBA32 |
| `HselStream` | 23 | HSEL 加密/解密流 |
| `Network` | 30 | TCP 服务端/客户端 + 加密集成 |
| `Protocol` | 27 | 版本协商 + CheckVersion/Ack/Nack |
| `Utility` | 17 | ThreadPool + ObjectPool + Compression |
| `LegacyCompat` | 280 | 遗留代码兼容性验证 |
| **合计** | **410** | Debug + Release 全过 |

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

## Phase 状态

| Phase | 内容 | 状态 |
|-------|------|------|
| 0 | 项目设置 | ✅ 完成 |
| 1 | 资源兼容层 | ✅ 完成 |
| 2 | 数据库层 | ✅ 完成 |
| 3 | 加密兼容 | ✅ 完成 |
| 4 | 网络层现代化 | ✅ 完成 |
| 5 | 渲染层 (DX8→DX11) | ✅ 完成 |
| 6 | UI 系统现代化 | ✅ 完成 |
| 7 | 构建系统完全化 | ✅ 完成 |
| 8 | 性能优化 | ✅ 完成 |
| 9 | 跨平台支持 | ✅ 完成 |
| 10 | 工具链现代化 | ✅ 完成 |
| 11 | 文档与部署 | ✅ 完成 |
| 12 | 持续迭代 | ✅ 完成 |

详细计划见 [MODERNIZATION_PLAN.md](../MODERNIZATION_PLAN.md)

## 快速开始

```powershell
# 1. 克隆仓库
git clone https://github.com/your-org/moxian-reborn.git
cd moxian-reborn

# 2. 配置 (需要 MSVC 2022)
cmake -S modern -B modern/build -G "Visual Studio 17 2022" -A x64

# 3. 构建
cmake --build modern/build --config Debug --parallel

# 4. 测试
cd modern/build
ctest -C Debug --timeout 60 --output-on-failure
```

## 依赖

- **编译器**: MSVC 2022 (Visual Studio 17)
- **CMake**: 3.20+
- **C++标准**: C++20
- **测试框架**: GoogleTest (FetchContent 或 vcpkg)
- **数据库**: SQLite3 (vendored 或 vcpkg)

可选依赖 (通过 vcpkg):
- `gtest` 1.14.0+
- `sqlite3` 3.45.0+

## CI/CD

GitHub Actions 自动运行：
- Windows 2022 + MSVC 2022
- Debug + Release 双配置
- 全量测试
- 失败时上传测试结果

配置文件: `.github/workflows/ci.yml`