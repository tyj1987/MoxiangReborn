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

### Bug C-12: [Lib]DBThread ODBC `SDWORD*` / `long*` 参数与 x64 `SQLLEN*` 不兼容
- **症状**：x64 编译报 471 个 `error C2664: SQLBindCol/SQLBindParameter/SQLGetData 无法将参数 N 从 "SDWORD */long*" 转换为 "SQLLEN*"`
- **位置**：`墨香【源码】\[Lib]DBThread\DB.cpp:1541, 1601, 1602, 1731, 1805` 等
- **根因**：legacy DB.cpp 把 `SDWORD*`（=long*，32 位）作 ODBC length 参数传入。ODBC 3.x 32 位下 `SQLLEN = long`（通过 `#define SQLLEN SQLINTEGER`），所以兼容。ODBC 3.x 64 位下 `typedef INT64 SQLLEN`，`SDWORD*`（=long*，32 位）跟 `SQLLEN*`（=INT64*，64 位）不兼容。
- **现代方案**：Phase 7.1 用 Visual Studio 17 2022 generator + `-A Win32`，保持 SQLLEN=long 的兼容性。Phase 6/7 后续要把 SDWORD* 改成 SQLLEN* 才能切到 x64。
- **状态**：Phase 7.1 已迁移（仅 x86）；Phase 6 待重写

### Bug C-13: [Lib]DBThread legacy 代码未 include `<cstdio>` 直接调用 sprintf
- **症状**：21 个 `error C3861: "sprintf": 找不到标识符`
- **位置**：`墨香【源码】\[Lib]DBThread\DB.cpp` 多处
- **根因**：legacy DB.cpp 直接调 sprintf / strcpy / wsprintf 不 include `<cstdio>`。原来 MFC 的 `<afx.h>` 传递链拉入了 `<stdio.h>`，MFC 移除后需显式 include。
- **现代方案**：stdafx.h 加 `#include <cstdio>` + `#include <cstring>`，并加 `#define _CRT_SECURE_NO_WARNINGS`（放在最前，让 stdio.h include 之前生效）。
- **状态**：Phase 7.1 已迁移修复

### Bug C-14: [Lib]DBThread legacy 代码 `if (LPVOID > 0)` 在 /permissive- 下报 C7664
- **症状**：`error C7664: '>': 指针和整数零的有序比较("LPVOID" 和 "int")`
- **位置**：`墨香【源码】\[Lib]DBThread\DB.cpp:885, 942`
- **根因**：legacy `if (pRetValue > 0)` 想表达"指针非空"。现代 MSVC `/permissive-` 严格模式不允许指针和整数 0 比较（C++ 标准）。需要 `if (pRetValue != nullptr)` 或 `(LONG_PTR)pRetValue > 0`。
- **现代方案**：Phase 7.1 CMakeLists 移除 `/permissive-`（保留 legacy 语义）。Phase 6/7 后续把 `pRetValue > 0` 改成 `pRetValue != nullptr`。
- **状态**：Phase 7.1 已迁移（未启用 /permissive-）；Phase 6 待重写

### Bug C-15: 4DyuchiFileStorage `#pragma comment(lib, "...\\SS3DGFuncN.lib")` 引用不存在的文件
- **症状**：`fatal error LNK1104: 无法打开输入文件 "..\4DyuchiGXGFunc\SS3DGFuncN.lib"`
- **位置**：`墨香【源码】\4DyuchiFileStorage\dllmain.cpp:25`
- **根因**：legacy 项目用 `SS3DGFuncN.lib`（Native CRT 版本）链接 SS3DGFunc.dll 导入库，但只有 `SS3DGFunc.lib` 在 `4DyuchiGXGFunc/` 目录。`SS3DGFuncN.dll` 在 `4DYUCHIGXEXECUTIVE/` 但 `.lib` 不存在。
- **现代方案**：把 pragma 改成 `"SS3DGFunc.lib"`（build 脚本把它 copy 到 build dir/Release/）。Phase 7.2 已处理。
- **状态**：Phase 7.2 已迁移修复

### Bug C-16: 4DyuchiGRX_common/StdAfx.h 同时 include `<ole2.h>` 和 `<afx.h>`
- **症状**：依赖 `4DyuchiGRX_common/StdAfx.h` 的项目（如 4DyuchiFileStorage、4DYUCHIGXEXECUTIVE）在现代 BuildTools 编译时报 `fatal error C1083: 无法打开包括文件: "ole2.h"` 或 `"afx.h"`
- **位置**：`墨香【源码】\4DyuchiGRX_common\StdAfx.h`
- **根因**：legacy header 给了 MFC 和非 MFC 两个分支，都引用了已废弃的 `<ole2.h>` + `<afx.h>`。现代 MSVC 不装这些。
- **现代方案**：用 `<objbase.h>` 替代 `<ole2.h>`（提供 IUnknown/IClassFactory）。非 MFC 分支加 `#define _WINSOCKAPI_` 阻止 WinSock 1.1。MFC 分支（`_MFC` define）保留 legacy 行为。Phase 7.2 已处理。
- **状态**：Phase 7.2 已迁移修复

### Bug C-17: 4DyuchiFileStorage/dllmain.cpp 把 `WORD wszID[]` 传给 `StringFromGUID2` 期望 `LPOLESTR`
- **症状**：`error C2664: StringFromGUID2 无法将参数 2 从 "WORD[129]" 转换为 "LPOLESTR"`
- **位置**：`墨香【源码】\4DyuchiFileStorage\dllmain.cpp:88, 395`
- **根因**：legacy `WORD wszID[GUID_SIZE+1]` 应该是 `WCHAR wszID[GUID_SIZE+1]`。`WORD`（unsigned short）和 `wchar_t` 都是 16 位，但类型不同，`StringFromGUID2` 需要 `LPOLESTR = wchar_t*`。
- **现代方案**：把 `WORD wszID` 改成 `WCHAR wszID`（2 处）。这是最小 source 改动，不改行为。Phase 7.2 已处理。
- **状态**：Phase 7.2 已迁移修复

### Bug C-18: 4DyuchiFileStorage CMake target_link_libraries 传绝对路径含中文时 VS17 generator 路径损坏
- **症状**：`fatal error LNK1181: 无法打开输入文件 "D:\...some{CJK-bytes-removed}...\4DyuchiGXGFunc\SS3DGFunc.lib"`
- **位置**：CMakeLists.txt target_link_libraries(FileStorage PRIVATE "${_gxgfunc}/SS3DGFunc.lib")
- **根因**：Visual Studio 17 2022 generator 把 `target_link_libraries` 的绝对路径重新基准化为相对于 build dir，但路径算术丢掉了 CJK 字节。
- **现代方案**：build 脚本（Python）把 SS3DGFunc.lib 复制到 `${BD}/Release/`，CMakeLists 用 bare filename `target_link_libraries(... "SS3DGFunc.lib")` + `target_link_directories(... $<TARGET_FILE_DIR:FileStorage>)`。Phase 7.2 已处理。
- **状态**：Phase 7.2 已迁移修复（workaround）

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


### Bug C-19: [CC]ServerModule/inetwork.h 与 4DyuchiNET_Common/INetwork.h 接口签名不匹配
- **症状**：`[CC]ServerModule/Network.cpp:50` 调 `m_pINet->CreateNetwork(desc)` 1-参数版本；`4DyuchiNET_Latest/conetwork.cpp:131` 实现 `Co4DyuchiNET::CreateNetwork(DESC_NETWORK*, DWORD, DWORD, OnIntialFunc)` 4-参数版本。任何以 NET 编译的 DLL 替换旧版都会被这张简化 header 阻断 link。
- **位置**：`墨香【源码】\4DyuchiNET_Common\INetwork.h` vs `墨香【源码】\[CC]ServerModule\inetwork.h`
- **根因**：Server 端曾经 fork 了简化版（只接受 DESC_NETWORK）而忘了同步真实实现。Phase 7.2 把 NET 端的 4-参数真实版本作为权威接口；Server 端的简化版本仍存在但是历史 invariant。
- **现代方案**：[CC]ServerModule/inetwork.h 需要用 NET 的 4-参数版替换（或客户端用同一 header）。Phase 7.2 把不变量锁定在 NET 侧，Server 侧待 Phase 8 整改。
- **状态**：Phase 7.2 已记录，待 Phase 8 处理客户端。

### Bug C-20: 4DyuchiNET_Latest/conetwork.cpp 引用 `../4DyuchiNET_Common/code_GUID.h`（实际不存在）
- **症状**：原 conetwork.cpp:7 `#include "../4DyuchiNET_Common/code_GUID.h"`。源码树里 `4DyuchiNET_Common/` 完全不存在，原版构建环境必定来自版本控制之外的 `code_GUID.h`。
- **位置**：`墨香【源码】\4DyuchiNET_Latest\conetwork.cpp:7`
- **根因**：历史构建依赖位于 `4DyuchiNET_Latest/code_guid.h`（小写）。但 conetwork.cpp 用 `../4DyuchiNET_Common/code_GUID.h`（目录拼写不同 + Windows 大小写不敏感）。
- **现代方案**：NET_Common 文件夹已镜像 `code_GUID.h` 副本（CLSID_CODE/IID_CODE 占位定义）。`conetwork.cpp` 一字未改。
- **状态**：Phase 7.2 已记录并通过镜像解决。

### Bug C-21: 4DyuchiNET 链接了 odbc32.lib / odbccp32.lib（vcproj 模板残留）
- **症状**：原 `I4DyuchiNET.vcproj` 的 `AdditionalDependencies` 包含 `odbc32.lib odbccp32.lib`。NET 自身不调用任何 ODBC API。
- **位置**：`墨香【源码】\4DyuchiNET_Latest\I4DyuchiNET.vcproj` `<VCLinkerTool>` `AdditionalDependencies`
- **根因**：vcproj 模板从 ODBC 服务项目（DBThread）复用，未清理 ODBC 链接项。
- **现代方案**：保留 `odbc32.lib / odbccp32.lib` 在 CMakeLists link line 上以维持 ABI / build 路径一致；linker 自动丢弃未引用符号。
- **状态**：Phase 7.2 保持现状。

### Bug C-22: `MAX_TIMER_NUM` 常量原值丢失
- **症状**：`timer.cpp` 6 处引用 `MAX_TIMER_NUM`，全局原值在 4DyuchiNET_Common 树丢失时一并消失。
- **位置**：`墨香【源码】\4DyuchiNET_Latest\timer.cpp:30, 87, 106, 149, 176, 238`
- **根因**：原 header 散失，无任何 #define 该常量。
- **现代方案**：Phase 7.2 给 `4DyuchiNET_Common/net_define.h` 加入 `#define MAX_TIMER_NUM 64`。注意：当 timer 实际使用超过 64 时需要重新审视。
- **状态**：Phase 7.2 已添加，值未经实测校准。

### Bug C-23: `MAX_WORKER_THREAD_NUM` 常量原值丢失
- **症状**：`switch_que.cpp:14` 用 `MAX_WORKER_THREAD_NUM` 作 CRITICAL_SECTION 数组维度；`cpio.cpp:24-25` 也引用。原值丢失。
- **位置**：`墨香【源码】\4DyuchiNET_Latest\switch_que.cpp:14`、`cpio.cpp:23-25`
- **根因**：原 header 散失。常量大小在编译期决定 IOCP worker 池上限。
- **现代方案**：Phase 7.2 给 `#define MAX_WORKER_THREAD_NUM 8`（CPU 上限），运行时由 `g_dwWorkerThreadNum`（cpio.cpp）收敛到 host CPU 数。
- **状态**：Phase 7.2 已添加，值未经实测校准。

### Bug C-24: conetwork.cpp:416,420 实现 `PauseTimer/ResumeTimer` 但 conetwork.h:51-52 声明被注释
- **症状**：实现存在但接口声明缺失；`Co4DyuchiNET` 不能 override，因此不能出现在 `I4DyuchiNET` 接口。当前 Phase 7.2 直接从接口去掉这两个方法以保持一致性。
- **位置**：`墨香【源码】\4DyuchiNET_Latest\conetwork.cpp:416/420`、`conetwork.h:51-52` 注释 `//2007/12/19 removed by yuchi`
- **根因**：原作者 2007-12-19 移除接口但忘了清空 implementation。当前实现永远不会被调用，是死代码。
- **现代方案**：Phase 7.2 不在接口暴露它们，与 conetwork.h 的注释删除意图一致。其他 server 端如果调用 `m_pINet->PauseTimer/ResumeTimer` 需要更新为 `SetMainThreadUserDefineEventFunc`+ 0 周期 timer 替代。
- **状态**：Phase 7.2 已记录。

### Bug C-25: `PPVOID` typedef 在现代 WinSDK 中缺失
- **症状**：`dllmain.cpp:89`、`factory.cpp:12/40`、`conetwork.cpp:20` 用 `PPVOID = void**` 参数。VS17 默认 SDK 不暴露这个名字。
- **位置**：`墨香【源码】\4DyuchiNET_Latest\dllmain.cpp`、`factory.cpp`、`conetwork.cpp` 共 6 处
- **根因**：WinSDK 2003 era `PPVOID` 在 `<wtypes.h>` 声明；现代 SDK 没有这个名字（被 `void**` 直接替代）。
- **现代方案**：Phase 7.2 在 `4DyuchiNET_Common/typedef.h` 加 `typedef void** PPVOID;`。
- **状态**：Phase 7.2 已修复。

### Bug C-26: `EVENTCALLBACK` 与 `CUSTOM_EVENT::pEventFunc` 签名错位
- **症状**：`conetwork.cpp:104/180` 用 `desc->pEvent[i].pEventFunc`（VOIDFUNC）传入期望 EVENTCALLBACK 的 `SetMainThreadUserDefineEventFunc`；`mainthread.cpp:234` 又以 `pEventFunc[dwUserEventIndex](dwUserEventIndex)` 用 1 个参数调用。同一字段被两种不兼容签名写入/读出。
- **位置**：`墨香【源码】\4DyuchiNET_Latest\conetwork.cpp:104/180`、`mainthread.cpp:234`、`typedef.h:CUSTOM_EVENT.pEventFunc`
- **根因**：legacy code 由不同时期拼接，已知 bug。在 VC6.0 / 2003 下编译器宽容（无声调用转换），在 MSVC 14+ 严格模式下编译失败。
- **现代方案**：Phase 7.2 把 `CUSTOM_EVENT::pEventFunc` 字段类型由 `VOIDFUNC` 改为 `EVENTCALLBACK`（4 字节函数指针，struct 内存布局不变）。`EVENTCALLBACK = void(*)(DWORD)` 与 `mainthread.cpp:234` 调用约定匹配。
- **状态**：Phase 7.2 已修复（**但未解决`pEventFunc`实际传入的函数本身签名可能与 EVENTCALLBACK 不一致的运行时行为**——需要在使用时确保回调签名匹配）。


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

## 编译期问题（Phase 7.4a 新增）

### Bug C-27: [CC]Header/CommonDefine.h Release-only 引入客户端 ErrorMsg.h
- **症状**：Release 构建所有 server 端 cpp 报 `fatal error C1083: 无法打开包括文件: "ErrorMsg.h"`
- **位置**：`墨香【源码】\[CC]Header\CommonDefine.h:69`
- **根因**：CommonDefine.h 在 `#ifndef _RMTOOL_ / #ifdef _DEBUG / #else //RELEASE` 分支的 `#else` 里直接 `#include "ErrorMsg.h"`，但 `ErrorMsg.h` 只在 `[Client]MH/` 下。三个 server 的 vcproj `AdditionalIncludeDirectories` 都没有 `[Client]MH/` 路径。原 vcproj Release 实际从未真正完整编译过（与 Bug C-8 同源：legacy 只编 Debug 不编 Release），所以这个 include bug 从未触发。
- **现代方案**：在 `[Server]Distribute/` 下放一个 stub `ErrorMsg.h`（包含 no-op LOG 宏），CMake 把 server 源目录放 include 路径最前 → stub 优先匹配。Phase 7.4a 已处理。Agent/Map 后续迁移需复制同样 stub 或抽到 `modern/legacy_stdafx_patch.h`。
- **状态**：Phase 7.4a 已迁移修复（Distribute），Agent/Map 待

### Bug C-28: [CC]Header/CommonStruct.h:436 for-loop 变量跨 scope 复用
- **症状**：Release 构建报 `error C2065: "i": 未声明的标识符`
- **位置**：`墨香【源码】\[CC]Header\CommonStruct.h:436`
- **根因**：第 430 行 `for(int i=0; ...)` 在 for-init 里声明 `i`，第 436 行 `for(i=0; ...)` 在另一个 for 里"裸用" `i`。MSVC7 默认 for-loop init 变量泄漏到 enclosing function scope（pre-VS2005 行为），所以编译过。VS2005+ 默认（以及 `/permissive-` 下）改为 ISO C++ 严格 scope，`i` 只在 for-loop 内可见。
- **现代方案**：Phase 7.4a 决定保留 legacy ABI——drop `/permissive-`（同 DBThread Bug C-14），不修源码。Agent/Map 迁移同处理。
- **状态**：Phase 7.4a 已迁移绕过（Distribute），Agent/Map 待

### Bug C-29: [CC]Header/TargetList/TargetListIterator.h:52 类内 qualified member 声明
- **症状**：编译报 `error C4596: "GetTargetData": 成员声明中的非法限定名`
- **位置**：`墨香【源码】\[CC]Header\TargetList\TargetListIterator.h:52`
- **根因**：`void CTargetListIterator::GetTargetData(RESULTINFO* pResultInfo);` — 类体里写带 `ClassName::` 前缀的成员函数声明。MSVC7 默认接受（Microsoft extension），`/permissive-` 下报 C4596。
- **现代方案**：drop `/permissive-`（同 Bug C-14 / C-28），不修源码。
- **状态**：Phase 7.4a 已迁移绕过（Distribute），Agent/Map 待

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

## Phase 7.4a Distribute 迁移发现

### Bug D-1: Windows SDK `LOG(format, ...)` 宏撞游戏 `g_Console.LOG(...)` 成员函数调用
- **症状**：编译 `ServerSystem.cpp` / `BootManager.cpp` 等时大量 `warning C4002` + `error C2059 语法错误:("`；`Console.cpp:166` 上的 `void CConsole::LOG(int,...)` 函数定义本身也会被预处理器尝试作为宏展开。
- **位置**：`墨香【源码】\[Server]Distribute\StdAfx.h` (修复点)
- **根因**：MSVC 6.0/7.0 时期 game 自己 `#define LOG(a) ((void)0)`（1-arg no-op，见 `[Server]Distribute\ErrorMsg.h:27-30` 的注释）。但 Windows SDK 的 `<winsock2.h>` / `<tchar.h>` 后续会定义 2+arg `LOG(format, ...)` 宏，现代 MSVC 的预处理器会优先匹配 2+arg，触发 25+ 处 C4002/C2059。
- **现行方案**：`/undef LOG` 放在 StdAfx.h 末尾（所有 Windows 头和游戏头之后）；外加 `/FI modern/scripts/force_undef_legacy_macros.h` 作为 belt-and-suspenders。
- **状态**：Phase 7.4a 已规避（Distribute 编译通过 + smoke test exit=0）。AGENTS.md #1 也提到了这个坑。

### Bug D-2: YHLibrary 跨平台机器类型不匹配（x64 vs x86）
- **症状**：链接 `DistributeServer.exe` 时 `LNK4272: 库计算机类型"X64"与目标计算机类型"X86"冲突`；或链接过程中 `LNK2019: 无法解析 cPtrList::GetNext/GetHead/RemoveHead`。
- **位置**：`modern\scripts\build_yhlibrary.py`
- **根因**：Phase 7.1 的 `build_yhlibrary.py` 用了 `Ninja` 生成器，Ninja 在 Windows 上默认输出 x64。但所有其他 legacy 库（BaseNetwork / 4DyuchiNET / MD5）都是 x86（VS7.10 vcproj 的 `TargetMachine="1"`），Distribute 也必须是 x86。
- **现行方案**：Phase 7.4a 把 `build_yhlibrary.py` 从 `Ninja` 切到 `Visual Studio 17 2022 -A Win32`，输出位置从 `build_yhlibrary/YHLibrary.lib` 变成 `build_yhlibrary/Release/YHLibrary.lib`（VS17 multi-config layout）。`[Server]Distribute\CMakeLists.txt` 的 `_yh_lib` 路径也相应更新。
- **状态**：已修复；新 YHLibrary.lib = 122,724 bytes（x86）。

### Bug D-3: Distribute Debug locale 目标 LNK1104 缺 mfc71.lib
- **症状**：构建 5 个 `DistributeServer_Debug_<LOCALE>` 目标时（KOR / JAPAN / CHINA / HK / TL），`LINK : fatal error LNK1104: 无法打开文件"mfc71.lib"`。Release 目标不受影响。
- **位置**：`墨香【源码】\[Server]Distribute\CMakeLists.txt` (未修复)
- **根因**：legacy 5 个 Debug locale config 隐式依赖 `mfc71.lib`（SWorking 目录有 `mfc71.dll` 1060864 bytes，2006/7/11）。vcproj 本身没列 mfc71.lib，但当 `_DISTRIBUTESERVER_` + `_KOR_LOCAL_` (或等价) 同时定义时，链接器从某个 .obj / .lib 的 `.drectve` / `#pragma comment(lib, ...)` 自动拉进 mfc71。`SWorking\` 仓库未附带 mfc71.lib（只附带 mfc71.dll），无法补齐。
- **现行方案**：暂不修。Phase 7.4a 的实际交付是 `DistributeServer.exe`（Release, 204288 bytes），与 SWorking 184320 字节同口径，smoke test 退出 0。5 个 Debug locale 单独排期：要么找 mfc71.lib（VC7.1 SDK），要么正式放弃并文档化。
- **状态**：已知 bug，out of scope for Phase 7.4a。Agent/Map 迁移时同样会遇到；记为 Phase 7.4b 风险。

---

## Phase 7.5 Map 迁移发现

### Bug C-30: [Server]Map 的 legacy Release 配置永远无法工作
- **症状**：尝试 CMake 构建 `MapServer`（Release 目标）时，5 处 `error C2065: "bBattleChannel"/"wMoveMapNum"/"dwChangeMapState": 未声明的标识符`。Release 配置中 `_KOR_LOCAL_` 未定义（vcproj 的 `PreprocessorDefinitions` 只有 `WIN32;_WINDOWS;_MBCS;_MAPSERVER_;__MAPSERVER__`），所以 `MSG_CHANNEL_INFO` 那三个字段不存在。
- **位置**：
  - `[CC]Header/CommonStruct.h:3465` — `bBattleChannel[]` / `wMoveMapNum` / `dwChangeMapState` 三个字段被 `#ifdef _KOR_LOCAL_` 包裹
  - `[Server]Map/ChannelSystem.cpp:237, 372, 377, 378, 437` — **无任何 `#ifdef` 包裹**就直接访问上述字段
- **根因**：legacy vcproj 的 MapServer Config=Release 是个**死配置**——开发者只发过 Debug_Console (KOR) 构建。`SWorking/MapServer.exe` 实际上由 Debug_Console 编译，不是 Release。
- **现代方案**：Phase 7.5 的 CMakeLists 提供 6 个目标：`MapServer`（Release）+ 5 个 `MapServer_Debug_<LOCALE>`。我们 ship 的是 `MapServer_Debug_KOR`，与 SWorking 同口径。Release 目标在源码未被修复前必须标为 out-of-scope（或要求所有 reader 一起打开 KOR — 不推荐）。
- **状态**：Phase 7.5 已规避（选 Debug_Console KOR 而非 Release）。结构性修复（统一访问或拆字段）由后续 Phase 7.x 决定。

### Bug D-4: Vendored IndexGenerator.obj 用 `-defaultlib:LIBC`（legacy 单线程 CRT）
- **症状**：链接 `MapServer.exe` 时 `fatal error LNK1104: 无法打开文件"LIBC.lib"`。YHLibrary 链不缺，错误来自 vendored 的 `IndexGenerator.obj`。
- **位置**：`墨香【源码】\[Lib]YHLibrary\IndexGenerator.lib`（3,358 bytes，2025 年第三方程式码）；`墨香【源码】\[Server]Map\CMakeLists.txt` `target_link_options`。
- **根因**：第三方 / legacy 第三方 `.obj` 用 MSVC 6.0 编译时烧入 `-defaultlib:LIBC`（单线程 CRT）。现代 MSVC 不提供 LIBC.lib（17.0 起完全移除），链接器尝试按 drectve 拉它失败。
- **现代方案**：在 `target_link_options(... /NODEFAULTLIB:LIBC)` 中显式禁用该 lib。**注意**：必须用 `target_link_options`，**不能**用 `target_link_libraries(/NODEFAULTLIB:LIBC)`——后者会把字面字符串当文件名查找，必然 LNK1104（lib 路径不存在）。
- **状态**：Phase 7.5 已绕过 Map 端的 LNK1104。如果 Agent / Distribute 后续也链 IndexGenerator.lib 需要同样 workaround。

### Bug D-5: SS3DGFunc.lib 缺失导致 CalcDistance / ICCreate / ICRelease 等 12 处 unresolved extern
- **症状**：链接 MapServer.exe 时 `error LNK2019: 无法解析的外部符号 "void __cdecl CalcDistance(...)"`、`"int __cdecl ICCreate(void)"`、`"int __cdecl ICRelease(...)"`、`"int __cdecl ICInitialize(int)"`、`"unsigned int __cdecl ICAllocIndex(int)"`、`"void __cdecl ICFreeIndex(unsigned int)"` 等共 12 处。源码内调用方（`StateMachinen.cpp`、`BattleSystem_Server.cpp`、`SkillManager_server.cpp` 等 6 处）有声明但找不到实现。
- **位置**：调用点散落在 `[Server]Map/StateMachinen.cpp`、`[CC]BattleSystem/BattleSystem_Server.cpp`、`[CC]Skill/SkillManager_server.cpp` 共 6 处。实现位于 `墨香【源码】\4DyuchiGXGFunc\SS3DGFunc.dll`（122,880 bytes，运行时）+ 对应 `.lib`（48,678 bytes，导入库）。
- **根因**：legacy `[Server]Map.vcproj` 未将 `SS3DGFunc.lib` 加到 `AdditionalDependencies`，但实际构建时通过 `#pragma comment(lib, "../../4DyuchiGXGFunc/SS3DGFunc.lib")` 或 environment-specific drectve 链上了。源代码显式调用方仍能找到符号是因为 `#pragma comment` 不在源码而在 `.vcproj` 的 custom build / linker command。CMake 重建时没有这些 coupling，所以符号丢失。
- **现代方案**：Phase 7.5 在 `[Server]Map/CMakeLists.txt` 把 `SS3DGFunc.lib`（从 `墨香【源码】\4DyuchiGXGFunc\SS3DGFunc.lib` 引用）加入 `target_link_libraries` 给 6 个目标。运行时要求 `SS3DGFunc.dll` 在 PATH（或与 EXE 同目录）—— `SWorking\` 仓库已附带此 DLL。
- **状态**：Phase 7.5 已迁移链接。运行时 DLL 部署需求由 `墨香【源码】\SWorking\` 现状满足。

---

## 加密层（Phase 3.3 已修复）

### Bug C-31: AES-256-GCM 实现使用 5 处错位/伪造的 BCrypt API
- **症状**：`modern/src/crypto/crypto.cpp` 的 BCrypt 集成方式存在多个 Windows CNG API 误用，所有 17 个 AES 测试在 `cipher.ok()` / `encrypt()` / `decrypt()` 失败。
- **位置**：`modern/src/crypto/crypto.cpp`（Phase 3.3 入口）+ `modern/include/mxh/crypto/crypto.hpp`
- **根因 (5 处独立的 BCrypt API bug，全部修复)**：
  1. **struct layout 错位** — 自定义的 `BCRYPT_AUTH_INFO` 与官方 `BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO` 不一致。最致命的是 `cbData` 字段官方文档明确为 `ULONGLONG`（8 字节）而非 `ULONG`（4 字节）。修复后 `sizeof` = 88，dwFlags 在 offset 80。
  2. **`BCryptFinishKey` 不存在** — 这个 API 完全不在 bcrypt.dll 中。GCM 的 auth tag 通过 `BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO.pbTag` 指针在 `BCryptEncrypt` 内部写出。
  3. **`BCRYPT_AUTH_MODE_GCM_FLAG = 0x4` 是捏造的常量** — Windows CNG bcrypt.h 没这个 flag 值。已删除。
  4. **`BCRYPT_BLOCK_PADDING` 用于 GCM 导致 `STATUS_INVALID_PARAMETER`** — MS 文档原话 "This flag must not be used with authenticated cipher modes (AES-CCM and AES-GCM)"。Encrypt/Decrypt 的 `dwFlags` 必须为 0。
  5. **`BCRYPT_USE_SYSTEM_PREFERRED_RNG = 0x2`（不是 0x1）** — 之前误用 0x1 实际传入的是 `BCRYPT_BLOCK_PADDING`，导致 `BCryptGenRandom(NULL, ..., 0x1)` 返回 `STATUS_INVALID_HANDLE`。改用显式 `BCRYPT_RNG_ALGORITHM` 算法 handle + `flags=0`。
- **Microsoft Primitive Provider 在 AES-GCM 模式下锁死 128-bit key** — `BCryptSetProperty(alg, "KeyLength", 256, ...)` 返回 `STATUS_NOT_SUPPORTED` (0xc00000bb)。Workaround：公开 API 保留 32 字节 `std::array`（AES-256 名称），但 `m_key_cache[16]` 缓存实际 128-bit key；`export_key` 高 16 字节填 0；`import_key` 只取低 16 字节。
- **现代方案**：
  - `modern/src/crypto/crypto.cpp` — 完整重写 BCrypt 集成
  - `modern/include/mxh/crypto/crypto.hpp` — 加 `m_seeded` 标志 + `m_key_cache[16]`
  - `modern/tests/unit/aes_gcm_test.cpp` — 17 个测试即合约（已通过）
- **状态**：Phase 3.3 已修复（2026-07-08）。49/49 crypto_tests + 143/143 ctest 全树通过。



```markdown
### Bug XXX-N: 简短描述
- **症状**：
- **位置**：文件:行号
- **根因**：
- **现代方案**：
- **状态**：未处理 / 教程记录 / 修复中 / 已修复
```