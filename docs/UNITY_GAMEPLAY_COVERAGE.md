# Unity gameplay coverage baseline

2026-09-11. This is an inherited-work register, not a completion certificate.
Source baseline: `89a11b3653549bab7471f53f8f0aafb72e98ce2a`.
All Unity gameplay acceptance is pending until separately demonstrated in the
Editor and standalone Player. File existence and unit tests are not live integration.

| Scope | Existing implementation / reference | Automated evidence entry | Inherited gap |
|---|---|---|---|
| Login, character selection | `modern/src/client/CServerConnect.cpp`, `CCharSelect.cpp`; `modern/src/server/agent_handler.cpp` | client state and server handler tests | Unity real three-server lifecycle, rejection/reconnect and human selection |
| Character creation | `modern/src/client/CCharMake.cpp`, `CharMakeOptions.cpp`; `modern/src/ui/charmakedialog.cpp`; `agent_handler.cpp` | `modern/tests/unit/client/cchar_make_state_test.cpp`, `char_make_options_test.cpp` | full appearance/equipment preview, persistence and human input |
| Movement, combat, skills, drops, pickup, inventory | `modern/src/client/CInGameState.cpp`; `modern/src/server/map_handler.cpp` | `docs/VERIFICATION_MATRIX.md` combat/drop/relogin E4 and movement entries | all skill/item variants, visible labels, collisions, abnormal paths, two human clients |
| Map loading/travel | `modern/src/map_change_catalog.cpp`; `modern/src/client/GameStateStubs.cpp` CMapChange; `GameLoadingCoordinator.cpp`; `agent_handler.cpp` | `modern/tests/unit/map_change_catalog_test.cpp`, `client/game_state_stubs_test.cpp`, `server/server_handler_test.cpp` | Map10→12 route evidence does not prove all scenes/UI or exact collision; human travel pending |
| Quests | `modern/src/server/quest_runtime_adapter.cpp`, `quest_manager.cpp`, `quest_script_loader.cpp`; client CInGameState; quest dialogs | server quest runtime/manager tests and client input tests | resource-backed multi-step NPC completion, reward collection and human GUI |
| NPC shops | `modern/src/server/npc_shop.cpp`, `map_handler.cpp`; `modern/src/ui/citemshopdialog.cpp` | npc_shop, dealitem_parser, server_handler and client input tests | full catalog/errors and human GUI |
| Player trade/exchange | `modern/src/ui/cdealdialog.cpp`, `cexchangedialog.cpp`; `modern/include/mxh/services/ITradeService.hpp`; `modern/include/mxh/server/exchange_manager.hpp` | UI/exchange/agent_exchange/service unit tests | **Not integrated**: no verified live TradeService binding or client/MapHandler exchange route |
| Guild core | `modern/src/server/map_handler.cpp` create/invite/accept/disband; `modern/src/ui/cGuildDialog.cpp`; CInGameState | server_handler, agent_guild and guild UI tests | cross-process/map/relogin and full human action matrix |
| Guild extensions | `modern/src/server/agent_guild*.cpp`, `guild_manager.cpp` | isolated guild unit/data-plane tests | union, field war, warehouse and rank live integration must be checked individually |
| Paid shop | `modern/src/portal/shop_routes.cpp`; shop_item_manager | portal shop routes and item effect tests | order/payment/delivery/account entitlement chain unverified; separate from NPC shops; no paid service purchase authorized |
| MurimNet PvP | `modern/src/server/murimnet_runtime.cpp` and room/channel/player/wire/crypt; `agent_murimnet.cpp` | murimnet runtime and agent side-effect tests | **Client blocked**: `modern/src/client/GameStateStubs.hpp` CMurimNet remains stub; lobby/session/results/persistence unverified |
| Chat, social, Boss, economy and remaining active UI | original client/server source; modern CInGameState and active dialog registry | existing verification matrix and unit directories | granular original-behavior inventory still required; no implicit exclusion |

Each expanded row must identify the original reference, live client host, server
authority, UI root, automated evidence, real user evidence, resource dependencies
and blocking reason. Original E5 remains legacy comparison; modern remaster art
uses a separate quality acceptance. `VERIFICATION_MATRIX.md` G4–G9 are Partial,
G10/G11 are Not started at this baseline. This register does not upgrade them.
