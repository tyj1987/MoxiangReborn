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

### Bug C-4: [Lib]YHLibrary Strclass.cpp 残留 `<atlbase.h>` 死代码
- **症状**：编译报 `fatal error C1083: 无法打开包括文件: “atlbase.h”`
- **位置**：`墨香【源码】\[Lib]YHLibrary\Strclass.cpp:13`
- **根因**：legacy 代码早期用过 MFC/ATL，后来改 Win32-only，但 #include 没清掉。`Strclass.cpp` 整文件 grep 无任何 `CString / AtlThrow / CAtlWinModule / CSimpleArray` 等 ATL 符号使用 → 纯死 include。ATL/MFC 组件不在现代 BuildTools 安装里（默认只装 MSVC）。
- **现代方案**：删除该行。Phase 7.1 已处理（CMakeLists.txt 中记录）。MFC 类 `AllocSysString / SetSysString` 改成 `<oaidl.h>` 提供的 `BSTR` 类型。`_ASSERTE` 加 noop fallback。`_tcslen/_tcschr/_tcsinc/_ttoi` 等 generic-text mappings 显式 include `<tchar.h>`。
- **状态**：Phase 7.1 已迁移修复

### Bug C-5: [Lib]YHLibrary/HSEL.cpp 与 [Lib]HSEL/HSEL.cpp 不一致
- **症状**：单独构建 `YHLibrary.lib` 时得到 188KB；legacy prebuilt 是 400KB。两份 `HSEL.cpp` 的内容不完全相同（diff 1067 行）。
- **位置**：`墨香【源码】\[Lib]YHLibrary\HSEL.cpp` vs `墨香【源码】\[Lib]HSEL\HSEL.cpp`
- **根因**：历史遗留——开发者从某版本复制 `HSEL.cpp` 到 `YHLibrary/`，但两份独立演化；`HSEL_STREAM.cpp` 两边仍然一致（hash 相同）。
- **现代方案**：Phase 7.1 决定**保留** YHLibrary 内的 vendored 副本（不链接外部 `[Lib]HSEL.lib`，避免重复符号）。`YHLibrary.h` 公共 API 暴露 `CHSEL / CHSEL_STREAM`，调用方只链接 YHLibrary.lib 时行为是 self-contained。Phase 7.x 后续可考虑统一两份 source。
- **状态**：Phase 7.1 已记录，未尝试统一

### Bug C-6: [Lib]YHLibrary/HSEL.cpp 与 HSEL_STREAM.cpp 方法定义重复
- **症状**：链接报大量 `LNK4006: "private: void __cdecl CHSEL_STREAM::DESLeftEncode_Type_4" 已在 HSEL_STREAM.cpp.obj 中定义；已忽略第二个定义` 警告
- **位置**：`墨香【源码】\[Lib]YHLibrary\HSEL.cpp` + `HSEL_STREAM.cpp`
- **根因**：legacy 复制粘贴失误——`HSEL.cpp`（YHLibrary 内的 vendor 版）包含**完整**的 `CHSEL_STREAM` 实现，与 `HSEL_STREAM.cpp` 中相应方法完全重复。原 vcproj 编译时也报这些警告，靠"忽略第二个定义"取第一个。
- **现代方案**：保留现状以匹配 legacy ABI（不擅自删除）。Phase 7.1 build 警告保留为 `LNK4006`。
- **状态**：Phase 7.1 已迁移，未修改

### Bug C-7: [Lib]BaseNetwork StdAfx.h 残留 MFC + 旧 OLE 头
- **症状**：`fatal error C1083: 无法打开包括文件: “afx.h”` / `“ole2.h”`
- **位置**：`墨香【源码】\[Lib]BaseNetwork\stdafx.h`
- **根因**：legacy vcproj 设了 `UseOfMFC="0"` 但 stdafx.h 仍然 include `<afx.h>`（MFC 头）和 `<ole2.h>`（统一 OLE 头）。现代 BuildTools 不装 MFC/ATL 组件。
- **现代方案**：用 `<objbase.h>` 替代（提供 IUnknown / IClassFactory / COM 基础）。Phase 7.1 已处理。
- **状态**：Phase 7.1 已迁移修复

### Bug C-8: [Lib]BaseNetwork network.cpp 调用 `_CrtCheckMemory()` 无 ifdef 包裹
- **症状**：Release 构建报 `error C3861: "_CrtCheckMemory": 找不到标识符`
- **位置**：`墨香【源码】\[Lib]BaseNetwork\network.cpp:259/273/289/297`
- **根因**：`_CrtCheckMemory()` 只在 `_DEBUG` 定义时存在（CRT debug heap）。legacy 网络库用了 `_CrtCheckMemory()` 验证内存 + `_asm int 3` 触发调试中断，但**没有 `#ifdef _DEBUG` 包裹**。原 vcproj Release 配置链接必定失败，说明开发者实际上**只编译 Debug 版本**。
- **现代方案**：在 stdafx.h 加 fallback（`#ifdef NDEBUG inline int _CrtCheckMemory() { return 1; }`）。Phase 7.1 已处理。
- **状态**：Phase 7.1 已迁移修复

### Bug C-9: [Lib]BaseNetwork BaseNetworkDll.cpp `#define UNICODE` 与项目其余部分 MBCS 冲突
- **症状**：`error C2664: TCHAR[129] 转 LPOLESTR` / `const char[15] 转 LPTSTR`
- **位置**：`墨香【源码】\[Lib]BaseNetwork\BaseNetworkDll.cpp:1`
- **根因**：legacy BaseNetworkDll.cpp 文件首行 `#define UNICODE` 强制 UNICODE TCHAR model，但 vcproj CharacterSet 字段未设（默认 MBCS）。这是混用：此 TU 用宽字符 API（StringFromGUID2 / 注册表），其余 TU 用 ANSI API。现代 MSVC 默认 `/Zc:strictStrings` 拒绝隐式 `const char/wchar_t*` → `TCHAR*` 转换。
- **现代方案**：保留 `#define UNICODE`（注册表和 GUID 字符串处理确实需要宽字符），CMake 加 `/Zc:strictStrings-` 让 legacy `SetRegKeyValue(LPTSTR, L"literal", ...)` 转换兼容。
- **状态**：Phase 7.1 已迁移修复

### Bug C-10: [Lib]BaseNetwork 必须以 Win32 (x86) 编译
- **症状**：x64 编译报 `error C4235: 不支持在此结构上使用 __asm 关键字`
- **位置**：`墨香【源码】\[Lib]BaseNetwork\{network,create_index,switch_que}.cpp`
- **根因**：legacy IOCP 库使用 `__asm { int 3 }` / `__declspec(naked)` 进行 spinning locks + 调试中断。x64 MSVC **完全不支持**内联汇编。
- **现代方案**：Phase 7.1 POC 用 `-A Win32`（Visual Studio 17 2022 generator）保留 x86 编译。Phase 6/7 后续要把 `__asm` 用 `Interlocked*` / `__debugbreak()` 重写才能切到 x64。
- **状态**：Phase 7.1 已迁移（仅 x86）；Phase 6 待重写

### Bug C-11: [Lib]BaseNetwork network.cpp 直接 include `<mstcpip.h>` 但未先 include stdafx.h
- **症状**：`error C2375: WSAUnhookBlockingHook 重定义`（winsock.h / winsock2.h 冲突）
- **位置**：`墨香【源码】\[Lib]BaseNetwork\network.cpp:1`
- **根因**：network.cpp 只 include "define.h" / "network.h" 等，**没**显式 include "stdafx.h"（依赖头传递）。`<windows.h>` 通过传递链被拉入，间接 include `<winsock.h>`（WinSock 1.1）。然后 `<mstcpip.h>` 又触发 `<winsock2.h>`（WinSock 2.0）的 include。两个 WinSock 互斥。
- **现代方案**：在 network.cpp 第一行加 `#include "stdafx.h"`，并在 stdafx.h 用 `#define _WINSOCKAPI_` 阻止 `<windows.h>` 拉入 WinSock 1.1。
- **状态**：Phase 7.1 已迁移修复

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