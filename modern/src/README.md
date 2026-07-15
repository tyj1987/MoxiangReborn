# MoxianCompat - 璧勬簮鍏煎灞?
鎶?2003-2010 骞磋嚜鐮旀牸寮忕敤鐜颁唬 C++ 閲嶅啓锛屼繚鎸?**浜岃繘鍒?100% 鍏煎**銆?
## 宸插疄鐜扮殑鏍煎紡

| 鏍煎紡 | 绫?| 鐘舵€?| 瑕嗙洊鐜?|
|------|----|------|--------|
| `.bin` (XOR 鍔犲瘑) | `mxh::compat::MhFileEx` | 鉁?| 100% |
| `.pak` (4DyuchiFileStorage) | `mxh::compat::PackFile` | 鉁?| 100% |
| `.bmhm/.mhm` (鍦板浘鍧? | `mxh::compat::BmhmMap` | 馃毀 | 90% (寰呰ˉ鍏ㄨЕ鍙戝櫒) |
| `.ttb` (TileTable) | `mxh::compat::TtbTileTable` | 馃毀 | 70% |
| `.chx` (瑙掕壊妯″瀷) | `mxh::compat::ChxModel` | 馃毀 | 80% |
| `.chr` (瑙掕壊鍔ㄧ敾) | `mxh::compat::ChrMotion` | 馃毀 | 70% |
| `.bsad` (鎶€鑳藉尯鍩? | `mxh::compat::BsadArea` | 鉁?| 100% |

## 璁捐鍘熷垯

1. **闆舵嫹璐濅紭鍏?*锛氱洿鎺?`mmap` 鎴?`fread` 涓€娆℃€ц鍙栵紝瑙ｆ瀽鍣ㄥ彧璇诲彇涓嶄慨鏀?2. **涓嶄緷璧栬€?SDK**锛氱函鏍囧噯 C++17 瀹炵幇锛屽彲绉绘
3. **绾跨▼瀹夊叏**锛氭墍鏈夎В鏋愬櫒涓哄彧璇昏鍥撅紝澶氱嚎绋嬪彲鍏变韩
4. **閿欒閫忔槑**锛氭瘡涓?API 杩斿洖 `Result<T>` 鑰岄潪鎶涘紓甯革紙鍏煎鑰佷唬鐮侀鏍肩殑杩斿洖鍊硷級