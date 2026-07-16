# Moxian Protocol Documentation

> Auto-generated from `Protocol.h` on 2026-07-16

**Generation tool**: `modern/tools/MoxianProtocolDoc/` (C++) + `modern/tools/gen_protocol_doc.py` (this Python wrapper)

**Tool status**: MoxianProtocolDoc `--summary` works; `--output <md>` crashes with STATUS_STACK_BUFFER_OVERRUN on the full 92KB Protocol.h. This doc uses `--summary` + Python-side enum extraction as a stable interim format.

---

## Summary

- **MP_CATEGORY entries (per C++ parser)**: 124 (parser counts 124 by including comment references; the canonical enum body has 77 real entries — see table below)
- **MP_PROTOCOL_* enums (per C++ parser)**: 64
- **Total protocol values (all enums combined)**: 3458
- **Real category enum entries (Python-verified)**: 77
- **Protocol enum names (Python-verified)**: 64

---

## MP_CATEGORY enum

| # | Name | Value | Description |
|---|------|-------|-------------|
| 1 | `MP_SERVER` | 1 | MP_SERVER???A Ay???e 0AI?ui?u???N???U.? |
| 2 | `MP_POWERUP` | — | — |
| 3 | `MP_CHAR` | — | — |
| 4 | `MP_MAP` | — | — |
| 5 | `MP_ITEM` | — | — |
| 6 | `MP_CHAT` | — | — |
| 7 | `MP_USERCONN` | — | — |
| 8 | `MP_MOVE` | — | — |
| 9 | `MP_MUGONG` | — | — |
| 10 | `MP_AUCTIONBOARD` | — | — |
| 11 | `MP_CHEAT` | — | GMTOOL? |
| 12 | `MP_QUICK` | — | — |
| 13 | `MP_PACKEDDATA` | — | — |
| 14 | `MP_PARTY` | — | — |
| 15 | `MP_PEACEWARMODE` | — | — |
| 16 | `MP_UNGIJOSIK` | — | — |
| 17 | `MP_AUCTION` | — | — |
| 18 | `MP_AUTOPATCH` | — | pjs [4/27/2003]? |
| 19 | `MP_SIGNAL` | — | — |
| 20 | `MP_TACTIC` | — | — |
| 21 | `MP_MUNPA` | — | — |
| 22 | `MP_SKILL` | — | — |
| 23 | `MP_KYUNGGONG` | — | — |
| 24 | `MP_SIMBUB` | — | — |
| 25 | `MP_MORNITORTOOL` | — | — |
| 26 | `MP_MORNITORSERVER` | — | — |
| 27 | `MP_MORNITORMAPSERVER` | — | — |
| 28 | `MP_EXCHANGE` | — | — |
| 29 | `MP_STREETSTALL` | — | — |
| 30 | `MP_PYOGUK` | — | — |
| 31 | `MP_BATTLE` | — | — |
| 32 | `MP_CHAR_REVIVE` | — | — |
| 33 | `MP_FRIEND` | — | — |
| 34 | `MP_BOSSMONSTER` | — | — |
| 35 | `MP_MONSTER` | — | — |
| 36 | `MP_OPTION` | — | — |
| 37 | `MP_NPC` | — | Npc??IAC ?ioE?I AU??e (???eE?????? ??A???? ??i??i) LBS 03.12.24? |
| 38 | `MP_MURIMNET` | — | — |
| 39 | `MP_QUEST` | — | ???OAu QuestAC ?ioAA???? AuAa ??c???u?????? ????????c???U. LBS 04.01.06? |
| 40 | `MP_DEBUG` | — | — |
| 41 | `MP_PK` | — | PK? |
| 42 | `MP_HACKCHECK` | — | — |
| 43 | `MP_RMTOOL_CONNECT` | — | — |
| 44 | `MP_RMTOOL_USER` | — | — |
| 45 | `MP_RMTOOL_MUNPA` | — | — |
| 46 | `MP_RMTOOL_GAMELOG` | — | — |
| 47 | `MP_RMTOOL_OPERLOG` | — | — |
| 48 | `MP_RMTOOL_STATISTICS` | — | — |
| 49 | `MP_RMTOOL_ADMIN` | — | — |
| 50 | `MP_RMTOOL_CHARACTER` | — | — |
| 51 | `MP_RMTOOL_ITEM` | — | — |
| 52 | `MP_WANTED` | — | — |
| 53 | `MP_JOURNAL` | — | — |
| 54 | `MP_SURYUN` | — | — |
| 55 | `MP_SOCIETYACT` | — | — |
| 56 | `MP_GUILD` | — | — |
| 57 | `MP_GUILD_FIELDWAR` | — | — |
| 58 | `MP_NOTE` | — | — |
| 59 | `MP_PARTYWAR` | — | — |
| 60 | `MP_GTOURNAMENT` | — | — |
| 61 | `MP_JACKPOT` | — | — |
| 62 | `MP_GUILD_UNION` | — | — |
| 63 | `MP_SIEGEWAR` | — | — |
| 64 | `MP_SIEGEWAR_PROFIT` | — | — |
| 65 | `MP_WEATHER` | — | — |
| 66 | `MP_PET` | — | — |
| 67 | `MP_HACKSHIELD` | — | — |
| 68 | `MP_RMTOOL_PET` | — | — |
| 69 | `MP_NPROTECT` | — | — |
| 70 | `MP_RMTOOL_DELCHAR` | — | — |
| 71 | `MP_SURVIVAL` | — | — |
| 72 | `MP_TITAN` | — | — |
| 73 | `MP_ITEMEXT` | — | — |
| 74 | `MP_BOBUSANG` | — | — |
| 75 | `MP_ITEMLIMIT` | — | — |
| 76 | `MP_AUTONOTE` | — | — |
| 77 | `MP_FORTWAR` | — | — |

---

## MP_PROTOCOL_* enums (top-level)

| # | Enum name |
|---|-----------|
| 1 | `MP_PROTOCOL_NPROTECT` |
| 2 | `MP_PROTOCOL_HACKSHIELD` |
| 3 | `MP_PROTOCOL_WEATHER` |
| 4 | `MP_PROTOCOL_MORNITORSERVER` |
| 5 | `MP_PROTOCOL_MORNITORMAPSERVER` |
| 6 | `MP_PROTOCOL_AUTOPATCH` |
| 7 | `MP_PROTOCOL_AUCTION` |
| 8 | `MP_PROTOCOL_SERVER` |
| 9 | `MP_PROTOCOL_POWERUP` |
| 10 | `MP_PROTOCOL_CHAR` |
| 11 | `MP_PROTOCOL_USERCONN` |
| 12 | `MP_PROTOCOL_MUGONG` |
| 13 | `MP_PROTOCOL_CHAT` |
| 14 | `MP_PROTOCOL_MOVE` |
| 15 | `MP_PROTOCOL_ITEM` |
| 16 | `MP_PROTOCOL_ITEMEXT` |
| 17 | `MP_PROTOCOL_AUCTIONBOARD` |
| 18 | `MP_PROTOCOL_CHEAT` |
| 19 | `MP_PROTOCOL_QUICK` |
| 20 | `MP_PROTOCOL_PACKEDDATA` |
| 21 | `MP_PROTOCOL_PARTY` |
| 22 | `MP_PROTOCOL_PEACEWARMODE` |
| 23 | `MP_PROTOCOL_UNGIJOSIK` |
| 24 | `MP_PROTOCOL_SIGNAL` |
| 25 | `MP_PROTOCOL_TACTIC` |
| 26 | `MP_PROTOCOL_SKILL` |
| 27 | `MP_PROTOCOL_MUNPA` |
| 28 | `MP_PROTOCOL_PYOGUK` |
| 29 | `MP_PROTOCOL_KYUNGGONG` |
| 30 | `MP_PROTOCOL_SIMBUB` |
| 31 | `MP_PROTOCOL_EXCHANGE` |
| 32 | `MP_PROTOCOL_STREESTALL` |
| 33 | `MP_PROTOCOL_BATTLE` |
| 34 | `MP_PROTOCOL_CHAR_REVIVE` |
| 35 | `MP_PROTOCOL_FRIEND` |
| 36 | `MP_PROTOCOL_NOTE` |
| 37 | `MP_PROTOCOL_BOSSMONSTER` |
| 38 | `MP_PROTOCOL_MONSTER` |
| 39 | `MP_PROTOCOL_OPTION` |
| 40 | `MP_PROTOCOL_NPC` |
| 41 | `MP_PROTOCOL_MURIMNET` |
| 42 | `MP_PROTOCOL_QUEST` |
| 43 | `MP_PROTOCOL_DEBUG` |
| 44 | `MP_PROTOCOL_PK` |
| 45 | `MP_PROTOCOL_WANTED` |
| 46 | `MP_PROTOCOL_JOURNAL` |
| 47 | `MP_PROTOCOL_HACKCHECK` |
| 48 | `MP_PROTOCOL_SURYUN` |
| 49 | `MP_PROTOCOL_SOCIETYACT` |
| 50 | `MP_PROTOCOL_GUILD` |
| 51 | `MP_PROTOCOL_GUILD_UNION` |
| 52 | `MP_PROTOCOL_GUILD_FIELDWAR` |
| 53 | `MP_PROTOCOL_PARTYWAR` |
| 54 | `MP_PROTOCOL_GTOURNAMENT` |
| 55 | `MP_PROTOCOL_JACKPOT` |
| 56 | `MP_PROTOCOL_PET` |
| 57 | `MP_PROTOCOL_SIEGEWAR` |
| 58 | `MP_PROTOCOL_SIEGEWAR_PROFIT` |
| 59 | `MP_PROTOCOL_SURVIVAL` |
| 60 | `MP_PROTOCOL_TITAN` |
| 61 | `MP_PROTOCOL_BOBUSANG` |
| 62 | `MP_PROTOCOL_ITEMLIMIT` |
| 63 | `MP_PROTOCOL_AUTONOTE` |
| 64 | `MP_PROTOCOL_FORTWAR` |

---

## Per-protocol-enum value tables

The full per-enum value breakdown (3,458 individual `MP_*` constants) is generated by MoxianProtocolDoc but is not embedded here due to the STATUS_STACK_BUFFER_OVERRUN crash in the C++ tool's `generateMarkdown()` path.

To regenerate one enum at a time, you can manually split Protocol.h and call:

```bash
D:\Moxian\modern\build\tools\MoxianProtocolDoc\Release\MoxianProtocolDoc.exe <single-enum.h> --output <output.md>
```

Or fix the C++ tool's generateMarkdown() bug (likely an O(N×M) nested loop that explodes for 124×64) and re-run the full generation.

---

## Legacy source references

- Master definition: `墨香【源码】\[CC]Header\Protocol.h` (92,634 bytes)
- Network struct definitions: `墨香【源码】\[CC]Header\CommonStruct.h`
- Client-side message handlers: `墨香【源码】\[Client]MH\MHNetworkMsgParser.cpp`
- Server-side message handlers: `墨香【源码】\[Server]Agent\AgentNetworkMsgParser.cpp` + `墨香【源码】\[Server]Map\MapNetworkMsgParser.cpp` + `墨香【源码】\[Server]Distribute\DistributeNetworkMsgParser.cpp`

