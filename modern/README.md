# Moxian-Reborn Modern Source Tree

> 新代码遵循现代 C++17/20 规范，老代码保留原状。

## 结构

```
modern/
├── CMakeLists.txt              # 现代 CMake 工程入口
├── include/mxh/compat/         # 公共头（资源兼容层、协议抽象等）
├── src/                        # 实现
├── tests/                      # 单元测试（GoogleTest）
├── tools/
│   └── MoxianResourceExplorer/ # 资源浏览器 CLI 工具
└── docs/                       # 资源格式逆向文档
```

## 命名空间

- `mxh` — Moxian 项目根命名空间
- `mxh::compat` — 资源兼容层（读取老格式）

## 构建

```powershell
# 配置
cmake -S modern -B modern/build -G "Visual Studio 17 2022" -A Win32

# 构建
cmake --build modern/build --config Debug

# 测试
ctest --test-dir modern/build -C Debug --output-on-failure
```

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