# VM112 original r18 isolated test stage

This candidate targets only `C:\Moxiang\original\stage-r18-20261009` on VM112
(`192.168.2.244`). Its temporary Login/Agent/Map ports are
`36001/37001/38001`; the running r17 stage retains `16001/17001/18001`.
The database is the separate `MoxiangOriginalR18Test_20261009` copy on CT930.
Do not point this package at the current original or Unity database.

`deploy/scripts/start_original_vm112.ps1` defaults to PlanOnly. An explicit
`-Start` first verifies the complete stage file manifest and exact canonical
PlayDH file set/hash, the VM112 address, all three assigned ports, protected
ODBC configuration identity/TLS, and the read-only schema v3 writer contract.
It neither creates nor migrates a database. Provisioning and two-pass migration
of the new copy require a separate reviewed maintenance sequence on PVE.

The r18 writer contract requires four nullable, checked, no-default original
base attributes; four nullable, checked, no-default progression fields; and a
trusted constrained vitals table. An old/partial player row is rejected, never
silently initialized from defaults. CharacterMake creates the new progression
and pending vitals rows in its own transaction; the first Map GameIn completes
the exact Lv1 seed in a second transaction. Existing r17 characters are not
backfilled or moved into this test database.

The present source slice covers monster damage, authoritative death, one-level
experience/point accrual, and bounded Lv1 PresentSpot revival with persisted
vitals. Revisioned r18 sessions temporarily reject item use/move/buy/sell/
discard and quest start/end; sessions with active quests reject skill use
before MP spend. The old 20-byte ground-drop message and automatic quest kill
progress are not emitted for these sessions. Natural ground drop, click pickup,
equipment changes, quest writes, and higher-level revival still require their
own writer/protocol work and are not accepted by this stage.

The test stage may use Map10 solely for schema/protocol probes; Map10 has
high-level monsters and is not a verified Lv1 birth route. Historical CN
CreateCharacter procedure evidence points at persisted Map17/LoginPoint2017,
and that value is not traced to an original install file. The default
StartArea stays the agent's `default_map_num`. Register Map17 on 38004 with
`-EnableMap17` only when StartArea is configured to 17. The switch defaults
off, `--start-area` is not passed, and the three listeners stay
36001/37001/38001. An occupied 38004 refuses the start; there is no fallback
port. An unregistered configured StartArea fails startup and character
creation instead of falling back to Map10. Characters already saved on Map10
still enter Map10. TYJ-155 still owns `docs/CLEAN_MACHINE_DEPLOY.md`. Do not
adjust damage or item probabilities to compensate.
