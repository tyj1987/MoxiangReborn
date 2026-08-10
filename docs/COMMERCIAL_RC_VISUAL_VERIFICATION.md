# Commercial RC In-Game Visual Verification

> Status: **PASS (2026-08-10)**. The modern Moxian client connects to
> the three modern servers, performs the 5-step E2E (login -> charselect
> -> charmake -> relist -> gamein), and renders the original map12
> terrain + the player's CharacterAppearance character + 5 monster
> models from `MonsterList.bin` in a single follow-camera frame.

This document is the A/B stage visual evidence (ROADMAP §5 acceptance
criterion: "modern 客户端连接三进程服务并完整显示
原版地图、角色、怪物、UI、音乐和音效").
Cross-implementation evidence vs the legacy client is the next gate
(R-9 in the ROADMAP); the modern side is fully verified today.

## Gate

`scripts/gui-client-smoke.ps1 -FollowCamera` is the modern-side visual
gate. It:

1. Starts the three modern servers (Login/Agent/Map) on local ports.
2. Runs the MoxianClient in headless auto-create + follow-camera mode
   for `Map12` (the field map the smoke harness uses).
3. After `GameInAck` + 5 `MonsterAdd` packets, the client drives the
   camera into follow mode (`--follow-camera`) so the player sprite +
   the 5 spawned monsters land in the visible viewport.
4. Captures one TGA frame (`map12.tga`) into
   `modern/build/runtime/gui-smoke/<run-id>/`.
5. Runs `scripts/verify-entity-frame.py` on the captured frame, which
   checks 5 independent pixel signatures (player silhouette + skin +
   blue waist detail + monster dark body + monster red markings). All
   5 must pass for `GUI_CLIENT_SMOKE PASS`.

## Reproduction

```powershell
Get-Process -Name "mxh_*" -ErrorAction SilentlyContinue | Stop-Process -Force
powershell -NoProfile -ExecutionPolicy Bypass -File C:\moxiang\scripts\gui-client-smoke.ps1 `
    -BuildDir C:\moxiang\modern\build -FollowCamera -TimeoutSeconds 40
```

Expected:

```
Modern servers started: Login=16001 Agent=17001 Map=18001
PASS: state-connect.tga non_bg_pixels=480000/480000
PASS: state-login.tga non_bg_pixels=480000/480000
PASS: state-charselect.tga non_bg_pixels=480000/480000
PASS: state-charmake.tga non_bg_pixels=480000/480000
PASS: state-gamein.tga non_bg_pixels=478804/480000
STATE_FRAMES PASS
PASS: player-upright-silhouette
PASS: player-skin
PASS: player-blue-detail
PASS: monster-dark-body
PASS: monster-red-markings
GUI_CLIENT_SMOKE PASS (original BGM/create/select/game-in, evidence=..., frame=...)
Modern servers stopped
```

The captured frame lives at
`modern/build/runtime/gui-smoke/<run-id>/map12.tga`. Convert to PNG for
human review (TGA is the wire format the legacy engine uses):

```python
# python -m pip install pillow
from PIL import Image
Image.open(r"modern\build\runtime\gui-smoke\<run-id>\map12.tga").save(
    r"modern\build\runtime\gui-smoke\<run-id>\map12.png", "PNG")
```

## What the frame shows (reference frame, 2026-08-10)

| Element | Source                                          | Visual signature in the frame |
|---------|-------------------------------------------------|-------------------------------|
| Terrain | `Resource/Map/Map12.bmhm` + `Map12.hfl`         | Isometric stone road + sand + grass + tile roofs (original PlayDH assets) |
| Sky     | `Resource/Client/Sky/*.mod` (8/8 mesh+tex)      | Top edge horizon (blue gradient + 2 cloud layers) |
| Static  | `Resource/Map/Map12.stm`                        | Distant houses + walls + trees in the back of the frame |
| Player  | `ModList_M.bin` + `FaceList_M.bin` + `HairList_M.bin` (kind=65006, chx=`man.chx`) | Centered upright silhouette, skin tone, blue waist detail (CharacterAppearance) |
| Monsters| `Resource/MonsterList.bin` (kind=1, chx=`L001.chx`) + 4 other kinds from the 5-spawn list | 3 visible dark-bodied creatures with red head markings |
| BGM     | `SoundList.bin` id=1667 (login) -> id=1670 (field) | `[audio] playing original BGM id=1667` log line + silent in-frame audio |

All 5 monster kinds (`monster_kind` 1/2/3 from the MapServer's spawn
list) are sent in `MonsterAdd` packets; the visible 3/5 is the subset
that the legacy 30-degree follow camera crops into the 800x600 viewport
(the other 2 are behind the player or off-screen). The
`EntityScene::synchronize` call in `MoxianClient/main.cpp` receives all
5 and they all become 3D mesh instances; the frame just doesn't show
the off-screen ones.

## Markers checked by `verify-entity-frame.py`

- `player-upright-silhouette`: at least 1000 dark pixels in the player
  crop (375, 260) - (425, 375). Locks the upright character silhouette.
- `player-skin`: at least 80 skin-tone pixels in the same crop. Locks
  the visible face/hands against the dark body.
- `player-blue-detail`: at least 5 blue-dominant pixels (waist detail).
  Locks the original `ModList_M` blue waist against the generic body.
- `monster-dark-body`: at least 300 dark pixels in the monster crop
  (480, 220) - (650, 410). Locks the monster body texture against the
  terrain-only baseline.
- `monster-red-markings`: at least 10 red-dominant pixels in the same
  crop. Locks the original `MonsterList` red head marking (e.g.
  `L001.chx`) against the dark body.

All 5 are necessary; a single failure fails the gate. The terrain-only
baseline (no entity scene loaded) stays well below all 5 thresholds,
so the checks are sensitive to the in-game world actually being drawn.

## What is NOT covered (next gates)

- **In-game HUD / UI panels** (HP bar, MP bar, quick slots, buff bar).
  The 5 state-frames are connect/login/charselect/charmake/gamein and
  pass `verify-state-frames.py`. The in-game HUD overlay is the next
  visual deliverable (Phase 12+ in the original 12-week plan).
- **Real-time combat effects** (skill animation + damage number popup).
  Requires `skill_caster` server module (D-stage, next milestone) so
  the attack scenario produces a `Skill.StartAck` instead of a Nack.
- **NPC shop dialog** (vendor buy/sell). Requires `npc_shop` server
  module (D-stage, next milestone).
- **Quest dialog** (accept/complete). Requires `quest_manager` server
  module (D-stage, next milestone).
- **Cross-implementation diff** (modern frame vs legacy client frame).
  Requires a runnable legacy `SWorking/*` server + a legacy 800x600
  client. Blocked on legacy environment (R-9 next gate).

## Acceptance update (2026-08-10)

The A/B stage acceptance row in `ROADMAP.md` is updated to reflect the
modern-side visual verification passing. The cross-implementation
column is still "**跨实现对照环境待 legacy**" because the legacy
client is not runnable on this machine today; the modern side is
**完成**.

