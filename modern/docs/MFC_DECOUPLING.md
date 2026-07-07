# Phase 6.2 — MFC Decoupling Analysis

> Captured: 2026-07-08. Scope: `[Client]MH/` (930 files, 460 cpp + 470 h).

## Verdict: NO MFC DECOUPLING NEEDED

The Moxian client **never used MFC**. It was written from day one as a pure Win32 C++
application with a custom UI framework. The `MHClient.vcproj` has `UseOfMFC="0"` in
all 14 build configurations.

## Architecture

```
WinMain (MHClient.cpp:204)
  └── MyRegisterClass()  — WNDCLASSEX + RegisterClassEx (line 428)
  └── InitInstance()     — CreateWindow, setup engine
  └── Message Loop       — Manual PeekMessage/TranslateMessage/DispatchMessage
```

## Custom UI Framework (interface/)

All UI controls are custom subclasses of `cWindow` (no MFC inheritance):

```
cObject                            (interface/cObject.h)
  └── cWindow                      (interface/cWindow.h)
       ├── cDialog                 (modal dialogs)
       │    ├── cIconDialog
       │    ├── cGridDialog
       │    ├── cListDialog
       │    ├── cTabDialog
       │    └── cMsgBox
       ├── cButton
       ├── cEditBox                (text input)
       ├── cComboBox
       ├── cCheckBox
       ├── cList / cListCtrl
       ├── cStatic / cText
       ├── cGuage / cGuageBar      (progress bars)
       ├── cAni                    (animation)
       ├── cImage / cIcon
       └── cSpin                   (spinner control)
```

- **40 cpp + 40 h** files in `interface/`
- Rendering: Direct3D (via 4Dyuchi GX engine)
- Hit testing: custom per-widget implementation
- Keyboard/mouse events: custom dispatch via `cWindowManager`
- No message maps, no `DECLARE_DYNAMIC`, no MFC macros of any kind

## Global Singleton Pattern (USINGTON)

The client uses a macro-based global singleton registry:

```cpp
// Defined in [CC]Header/CommonDefine.h:8
#define USINGTON(className)         (&g_##className)
#define EXTERNGLOBALTON(className)  extern className g_##className;
#define GLOBALTON(className)        className g_##className;
```

90+ manager classes use this pattern. Example:
```cpp
#define DIRECTORYMGR  USINGTON(CDirectoryManager)   // → (&g_CDirectoryManager)
#define OBJECTMGR     USINGTON(CObjectManager)      // → (&g_CObjectManager)
```

This is a **custom** pattern, not MFC-related. The "C" prefix in class names
is Hungarian notation, not MFC inheritance.

## False Positives

The following look MFC-like but are custom classes:

| Name | Actual purpose | Looks like |
|------|---------------|------------|
| `CMapObject` | Map world object | MFC `CMap` |
| `CMapChange` | Map transition | MFC `CMap` |
| `CStringLoader` | String table loader | MFC `CString` |
| `CFileManager` | File manager (MHFile) | MFC `CFile` |
| `CListCtrl` (cWindow) | Custom list widget | MFC `CListCtrl` |

## No-MFC Confirmation Checklist

| Check | Result |
|-------|--------|
| `#include <afx` any .h/.cpp | **0 files** |
| `#include "afx` | **0 files** |
| `UseOfMFC="0"` in vcproj | **All 14 configs** |
| MFC class usage (CWinApp, CDialog, CWnd, CString) | **0** |
| MFC macros (BEGIN_MESSAGE_MAP, DECLARE_DYNAMIC) | **0** |
| MFC linker pragmas (mfc*.lib, nafxcw.lib) | **0** |
| `stdafx.h` includes MFC | **No** — pure `<windows.h>` + CRT |

## Effort Estimate

**Zero** — no MFC decoupling work required. The client is ready for CMake
migration as-is (with the other dependency replacements: d3dx8 → DX11,
wsock32 → ws2_32, HackShield/nProtect → stubs).
