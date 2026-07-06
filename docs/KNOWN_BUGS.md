# 已知 Bug 与陷阱（来自教程 4 + 源码阅读）

> 在现代化过程中遇到的原代码 bug 与陷阱。仅记录，不擅自修改——除非用户明确指示。

## 编译期问题

### Bug C-1: LOG 宏冲突
- **症状**：VS2003 下报 `"LOG" 宏的实参太多`、`"(" 有语法错误`
- **位置**：`[Lib]MHConsole\Console.cpp/.h`
- **根因**：游戏自定义 `void LOG(int LogLevel, char* strMsg, ...)` 与系统 `msplog.h` 的 LOG 函数宏冲突；系统 LOG 是函数宏无法重载
- **解决方案**：把所有 `LOG` 重命名为 `MLOG`；或在包含 Windows 头之前 `#undef LOG`
- **状态**：教程 4 已记录，未在源码修复

### Bug C-2: WS2_32 链接错误
- **症状**：`LNK2019: 无法解析的外部符号 __imp__closesocket@4 / __imp__socket@12`
- **位置**：`CBuddyAuth::~CBuddyAuth / ConnectToBuddyAuthServerAll` 等 9 处
- **根因**：未链接 Winsock 库 `WS2_32.lib`
- **解决方案**：项目属性 → Linker → Input → Additional Dependencies 加 `WS2_32.lib`

### Bug C-3: DirectX 8.1 SDK 路径
- **症状**：`d3dx8.h` 找不到 / `d3dx8.lib` 找不到
- **位置**：客户端主工程链接
- **根因**：DX8.1 SDK (2001) 不在 Visual Studio 默认路径
- **解决方案**：在 vcxproj 里手动指定 `IncludePath / LibraryPath`

## 运行时问题

### Bug R-1: HSEL 加密狗缺失
- **症状**：客户端/服务端启动时崩溃，找不到 `EhSvc.dll`
- **位置**：客户端 `MHClient.cpp` `HS_Init`
- **根因**：HSEL 物理加密狗已停产，无狗即不可运行
- **现代方案**：默认禁用加密；保留接口兼容

### Bug R-2: HackShield 反外挂下载失败
- **症状**：客户端卡在 "正在下载反外挂" 界面
- **位置**：客户端 `[Client]MHAutoPatch`
- **根因**：HackShield 服务器已下线
- **现代方案**：跳过反外挂初始化（默认禁用）

### Bug R-3: 800x600 硬编码窗口
- **位置**：`MHClient.cpp:489-495`
- **现象**：高 DPI 显示器模糊
- **现代方案**：HiDPI aware + 自适应窗口

### Bug R-4: 多语言宏遗漏
- **症状**：编译未定义 `_KOR_LOCAL_` 等宏时，字符编码乱码
- **位置**：散落各文件
- **现代方案**：抽 i18n 配置表

## 资源问题

### Bug F-1: .bin CRC 校验注释
- **位置**：`MHFile.cpp:182-204` (CheckCRC 函数)
- **现象**：CRC 校验被注释掉，安全校验失效
- **现代方案**：保留行为（原代码就是不校验）

### Bug F-2: 加密狗 DLL 缺失
- **位置**：`SWorking\` 下的 AntiCpSvr.dll / HackShield.crc
- **现象**：无法启动服务端
- **现代方案**：去除对这两个 DLL 的依赖

### Bug F-3: `MAX_MAP_NUM 37` 硬编码
- **位置**：`[Tool]Regen\DefineStruct.h`
- **现象**：实际资源已扩到 Map207，硬编码 37 限制工具
- **现代方案**：解硬编码，从配置或地图目录自动发现

## 性能问题

### Perf P-1: 单线程游戏循环
- **位置**：`MainGame.cpp:284` `Sleep(1)` 节流
- **现象**：CPU 利用率低
- **现代方案**：多线程（渲染/网络/逻辑分离）

### Perf P-2: 全局自旋锁
- **位置**：`GLOBALTON()` / `MAKESINGLETON()` 宏
- **现象**：多线程下不安全
- **现代方案**：替换为 `std::atomic` + thread_local

### Perf P-3: 字符串拼接
- **位置**：散落各 Manager（典型 2003 风格）
- **现象**：大量 `strcat` / `sprintf`
- **现代方案**：`std::string` + `std::format`

## 协议问题

### Bug P-1: 消息分发表硬编码大小
- **位置**：`Protocol.h` 末尾 `MP_MAX`
- **现象**：新增协议时必须更新
- **现代方案**：运行时注册表（保留 MP_MAX 作为兼容）

### Bug P-2: 加密可选
- **位置**：`#ifdef _CRYPTCHECK_` 宏
- **现象**：默认未启用加密
- **现代方案**：明确"明文/加密"双模式，不依赖宏

---

## 记录模板

每发现新 bug，用此模板追加：

```markdown
### Bug XXX-N: 简短描述
- **症状**：
- **位置**：文件:行号
- **根因**：
- **现代方案**：
- **状态**：未处理 / 教程记录 / 修复中 / 已修复
```