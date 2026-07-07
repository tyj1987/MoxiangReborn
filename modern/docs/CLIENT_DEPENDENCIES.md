# Phase 6.1 — Client Dependency Audit

> Captured: 2026-07-08. Scope: `[Client]MH/` only (excludes MHAutoPatch / Selupdate).

## File Counts

| Type | Count |
|------|-------|
| `.cpp` files | 460 |
| `.h` files | 470 |
| **Total** | **930** |

### By Subdirectory

| Directory | cpp | h | Purpose |
|-----------|-----|---|---------|
| `[Client]MH/` (root) | ~360 | ~380 | Core game logic, UI dialogs, networking |
| `interface/` | 40 | ~40 | Custom cWindow UI framework |
| `Effect/` | 39 | ~30 | Visual effects system |
| `Engine/` | 11 | ~10 | 4Dyuchi GX wrapper (GraphicEngine) |
| `Audio/` | 4 | ~3 | Miles Sound System wrapper |
| `input/` | 4 | ~3 | Keyboard/mouse/IME input |
| `CommonImageFile/` | 0 | ~4 | Image format headers |
| `FreeImage/` | 0 | 20+ | FreeImage headers (vendored) |

## Dependency Matrix

### 1. MFC Status: ✅ NO MFC

**`UseOfMFC="0"` in MHClient.vcproj.** The client uses a custom UI framework:

- `interface/cWindow.h` — base window class (pure Win32 GDI)
- `interface/cWindowManager.h` — window manager singleton
- `interface/cEditBox.h` — text input widget
- `interface/cTextArea.h` — multi-line text display
- `interface/cFont.h` — bitmap font rendering
- `interface/cMsgBox.h` — modal message dialog
- `interface/cListBox.h` — scrollable list
- `interface/cButton.h` — button widget
- `interface/cScrollBar.h` — scrollbar widget
- `interface/cTabControl.h` — tab control
- `interface/cProgressBar.h` — progress bar

**MFC decoupling effort: ZERO** — no MFC to remove.

### 2. Rendering: DirectX 8.1 → DX11 (Phase 5)

| Dependency | Source | Replacement |
|-----------|--------|-------------|
| `d3dx8.lib` | DX8.1 SDK | DirectX 11 (Phase 5 `mxh_render`) |
| `I4DyuchiGXExecutive` | `4DyuchiGXExecutive.dll` (COM) | `mxh_render` DX11 backend |
| `I4DyuchiGXRenderer` (75 methods) | `4DYUCHIGX_RENDER.dll` (COM) | Phase 5: **75/75 implemented, 99/99 tests** |
| `CoD3DDevice` | Legacy DX8 device wrapper | Phase 5 `device.cpp` |
| `SS3DGFunc.dll` | `4DyuchiGXGFunc/` (naked x86 ASM) | Keep as vendored DLL (CalcDistance, IC*) |

**Key insight**: Phase 5's `I4DyuchiGXRenderer` interface (75 methods) is a **1:1 mirror** of the legacy COM interface. The client's Engine/GraphicEngine.cpp calls through this interface. Plugging in the modern DX11 backend is a **link-time swap** — no source changes needed in client code.

### 3. Audio: Miles Sound System → OpenAL Soft

| Dependency | Source | Replacement |
|-----------|--------|-------------|
| `Audio/SoundLib.lib` | Vendored Miles SDK | OpenAL Soft (Phase 6.3) |
| `MHAudioManager.cpp` | Client wrapper (~4 cpp) | Wrap as `mxh::audio::AudioEngine` |
| `mss.h`, `AIL_*` | Miles API | OpenAL API (`alSourcePlay`, etc.) |

**Scope**: ~4 audio files, all in `Audio/` subdirectory. Well-encapsulated.

### 4. Image: FreeImage → DirectXTex / stb_image

| Dependency | Source | Replacement |
|-----------|--------|-------------|
| `FreeImage/` headers | Vendored (20+ headers) | DirectXTex (Phase 5 has DDS writer) |
| `FreeImage.lib` | Vendored | stb_image for TGA/BMP/JPG/PNG decode |
| `MHImage.h/cpp` | Client wrapper | Wrap as `mxh::image::ImageLoader` |

**Scope**: ~2-3 image files. Phase 5 already has TGA decoder + DDS writer.

### 5. Anti-Cheat: HackShield / nProtect → Stubs

| Dependency | Guard Macro | Status |
|-----------|-------------|--------|
| HackShield | `_HACK_SHIELD_` | Stub out — AHNLab EOL since 2015 |
| nProtect GameGuard | `_NPROTECT_` | Stub out — INCA Internet EOL |
| nProtect ggsrv25 | `Debug_HK` only (Agent) | Already documented (Bug D-6) |

**Scope**: ~2 files (`HackShieldManager.cpp`, `NProtectManager.cpp`). Guarded by `#ifdef` — simply don't define the macros and they compile to empty TUs.

### 6. Video: Windows Media / DirectShow → libavcodec

| Dependency | Source | Replacement |
|-----------|--------|-------------|
| `wmstub.lib` | Windows Media SDK | libavcodec / SDL video |
| `strmiids.lib` | DirectShow | libavcodec |
| `vfw32.lib` | Video for Windows | Drop (legacy AVI playback) |

**Scope**: ~1-2 files for intro/logo video playback. Can be stubbed for first build.

### 7. Network: wsock32.lib → WS2_32.lib

| Dependency | Source | Replacement |
|-----------|--------|-------------|
| `wsock32.lib` | WinSock 1.1 | `WS2_32.lib` (WinSock 2) |
| `BaseNetwork.dll` | `[Lib]BaseNetwork/` (COM) | Phase 4 `mxh_net` Asio backend |
| `4DyuchiNET.dll` | `4DyuchiNET_Latest/` (IOCP) | Phase 4 `mxh_net` |

**Scope**: ~5 network files. Phase 4 already has working TcpServer/TcpClient.

### 8. Crypto: HSEL → OpenSSL AES (Phase 3)

| Dependency | Source | Replacement |
|-----------|--------|-------------|
| `HSEL.lib` | `[Lib]HSEL/` (dongle) | Phase 3.2 `mxh::crypto::HselStream` (32/32 PASS) |
| AES channel | — | Phase 3.3 `mxh::crypto::Aes256GcmCipher` (17/17 PASS) |

### 9. Locale Matrix

From the vcproj and MHClient.cpp:

| Locale | Macro(s) | Version string | Active? |
|--------|----------|----------------|---------|
| Korea | `_KOR_LOCAL_` (default) | `NDSC08070301` | Default |
| China | `_CHINA_LOCAL_` | `OPEN06041301` | Yes |
| Japan | `_JP_LOCAL_` | `JOPN06032901` | Yes |
| Hong Kong | `_HK_LOCAL_` | `HKCV07020501` | Yes |
| Taiwan | `_TW_LOCAL_` + `TAIWAN_LOCAL` | `TWCV06112701` | Yes |
| Thailand | `_TL_LOCAL_` | `TLCV06051001` | Yes |

### 10. Additional Linker Dependencies

From MHClient.vcproj `AdditionalDependencies`:

```
wmstub.lib          — Windows Media stub
ComCtl32.lib        — Common Controls (Win32)
strmiids.lib        — DirectShow GUIDs
vfw32.lib           — Video for Windows
Audio/SoundLib.lib  — Miles Sound System
imm32.lib           — Input Method Editor
odbc32.lib          — ODBC database
odbccp32.lib        — ODBC installer
Winmm.lib           — Windows Multimedia (timeGetTime)
SS3DGFunc.lib       — 4Dyuchi geometry functions
d3dx8.lib           — DirectX 8.1 helper library
yhlibrary.lib       — Legacy utility library
IndexGenerator.lib  — Vendored YHLibrary index generator
wsock32.lib         — WinSock 1.1
dinput8.lib         — DirectInput 8
```

## Build Strategy

### Minimal First Build

For an initial CMake build, skip:
1. HackShield → stub (`#ifndef _HACK_SHIELD_`)
2. nProtect → stub (`#ifndef _NPROTECT_`)
3. Video playback → stub (`wmstub.lib`, `strmiids.lib`, `vfw32.lib`)
4. Miles Sound → stub (`SoundLib.lib` → no-op AudioEngine)
5. FreeImage → use Phase 5 TGA decoder + stb_image

### CMake Recipe (from vcproj)

```cmake
# Client target: ~460 cpp files
# UseOfMFC = 0
# CharacterSet = 2 (MBCS)
# Include: ., [Lib]YHLibrary, Audio, [CC]BattleSystem, [CC]Skill, [CC]Ability, [CC]Header
# Link: ws2_32, ComCtl32, imm32, odbc32, odbccp32, Winmm, SS3DGFunc, YHLibrary, IndexGenerator, dinput8
# Defines: WIN32, _WINDOWS, _MBCS, _MHCLIENT_, _CRYPTCHECK_, _FILE_BIN_
# Locale: default _KOR_LOCAL_ (like server builds)
# SubSystem: 2 (Windows GUI)
# Output: MHClient-Modern.exe
```

## Risk Assessment

| Risk | Level | Mitigation |
|------|-------|------------|
| d3dx8.lib not available | **LOW** | Replace with Phase 5 DX11 backend (already done) |
| 4Dyuchi COM DLLs needed | **LOW** | Phase 5 provides 1:1 interface replacement |
| 460 cpp compilation time | **MEDIUM** | Use /MP (multi-processor compilation) |
| Winsock 1.1 → 2 migration | **LOW** | Change `wsock32.lib` → `ws2_32.lib` |
| Locale #ifdef conflicts | **MEDIUM** | Same pattern as server — pick default locale first |
| cWindow GDI rendering | **MEDIUM** | Pure Win32 GDI, no MFC, but depends on Win32 window management |
| MBCS encoding | **MEDIUM** | Same as servers — do NOT pass /utf-8 |
| Miles Sound replaced | **LOW** | Well-encapsulated in Audio/ subdirectory |
