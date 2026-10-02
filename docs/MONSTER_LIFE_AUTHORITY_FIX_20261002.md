# Monster HP authority and Map17 follow-up

Base: `4f38103ccb6f16d95109101f0da0915a3f38a2d6`.

`MapHandler::apply_monster_damage` broadcasts absolute LifeNotify before its
caller sends SingleResult. CInGameState assigned that HP and then subtracted
the result damage again. HP5 followed by damage6 became local HP0, suppressing
normal targeting while the server monster remained alive. SingleResult now
only drives existing combat feedback/effects; MonsterAdd/LifeNotify own HP.
No damage formula, drop rule, server state or protocol changes.

## Regression evidence

`modern/tests/unit/client/monster_life_authority_test.cpp` adds five native
tests to mxh_client_tests: actual life/result order with hit/end feedback;
duplicates and old damage; result before authority; death with duplicate and
late results; removal plus late messages and same-ID authoritative respawn.
The former isolated-result HP test now waits for LifeNotify.

Cloud harness `/workspace/moxiang-audit-20261001/hp-authority/run.py` extracts
the unmodified production life/skill/remove handlers and target picker, runs
the new test source with a minimal non-Windows class shell. It models
MonsterAdd setup and stubs engine/UI/effect playback, so it is not full-client
integration. `red.log`: four failing tests, including observed HP0 vs expected
HP5 and missing target. `green.log`: 5/5 pass with feedback assertions intact.

Both native test files pass `g++ -std=c++20 -fsyntax-only` against actual
project headers. Full CInGameState compilation stops at missing Windows
`objbase.h`; no `modern/build` exists in this cloud workspace, so the prescribed
build/CTest command cannot execute. Governance and diff checks pass.

## Probe evidence changes / Windows rerun

Death confirmation requires the target's Death event, emitted by LifeNotify
on the alive-to-zero transition. Disappearance is logged but never promoted
to death. The natural drop must still reference that same target and match
the allowed source table; PickupAck, exact DBID/count and relog checks remain.
The shared deadline is unchanged; elapsed milliseconds and deadline status
are recorded on combat failure.

Combat mode writes `login.log`, `agent.log`, `map.log` in its unique evidence
directory. Each captures that child's stdout and stderr through a file handle;
STARTUPINFOEX restricts inheritance to that file and a NUL input handle.
Parent capture pipes are excluded; owned handles/attribute lists close on
success and exceptions. Failure to establish logging prevents spawning.
Map/Agent stdout already uses unitbuf; Login's buffered stdout can lose its
tail on forced cleanup, so these files are not a packet-completeness claim.
CreateProcess/handle behavior needs Windows verification.

ROG reported eight damage6 results, absolute HP47,41,35,29,23,17,11,5, no HP0
or explicit ObjectRemove, no drop/PickupAck/GameOutAck, player HP125. This is
consistent with the independently reproduced defect, but prior killed=1
does not prove server death. The live Map17 loop remains failed/unaccepted.

On the existing Windows checkout: rebuild `mxh_client_tests` and
`mxh_client_e2e`, run
`MonsterLifeAuthority.*:InGamePlayable.SkillResultWaitsForAuthoritativeTargetLifeBar`
and the full client suite. Then run the previously used isolated probe with
`--exercise-combat --map-number 17 --backend sqlite --timeout 12 --use-hsel`.
Collect result.json, all four logs and SQLite, verifying final authoritative
death, same-source natural drop, PickupAck and same DBID/count after relog.
Also check no server processes or parent capture pipe remain held at exit.
No claim of Unity visual acceptance follows from this probe.
