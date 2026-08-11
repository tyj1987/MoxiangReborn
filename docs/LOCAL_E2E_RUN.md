# Local End-to-End Run (Phase B.2.5)

> Status: 2026-08-11 verified PASS on SQLite + MSSQL LocalDB.

This document records the fully-external end-to-end run that proves the modern stack works locally. The 3 modern servers spawn in separate processes on 127.0.0.1, and the headless E2E client (mxh_client_e2e --no-spawn) connects and walks all 5 protocol steps (login / charselect / charcreate / relist / gamein).

## What was verified

Both backends produced 5/5 PASS on Phase B.2.5 (login / charselect / charcreate / relist / gamein). The MSSQL run wrote real rows into chr_log_info (test/test) and character_info (5+ rows including chrid 240366, 412303, 945025, 953712, 1117800).

Query the DB after running:

'''powershell
& "C:\\Program Files\\Microsoft SQL Server\\Client SDK\\ODBC\\170\\Tools\\Binn\\SQLCMD.EXE" -S "(localdb)\\MSSQLLocalDB" -d Moxiang -E -Q "SELECT * FROM chr_log_info; SELECT TOP 5 * FROM character_info"
'''

## Manual reproduction

### SQLite path (simplest, no LocalDB needed)

'''powershell
# 1. Login server
$login = Start-Process "C:\\moxiang\\modern\\build\\tools\\MoxianLoginServer\\Debug\\mxh_login_server.exe" -ArgumentList "--port","16001","--backend","sqlite","--db","C:\\moxiang\\modern\\scratch\\e2e_local\\login.db","--agent-addr","127.0.0.1","--agent-port","17001","--init-schema","--legacy" -RedirectStandardOutput "C:\\moxiang\\modern\\scratch\\e2e_local\\login.out" -RedirectStandardError "C:\\moxiang\\modern\\scratch\\e2e_local\\login.err" -PassThru

# 2. Agent server
$agent = Start-Process "C:\\moxiang\\modern\\build\\tools\\MoxianAgentServer\\Debug\\mxh_agent_server_CHINA.exe" -ArgumentList "--port","17001","--backend","sqlite","--db","C:\\moxiang\\modern\\scratch\\e2e_local\\agent.db","--legacy","--map-server","127.0.0.1:18001" -RedirectStandardOutput "C:\\moxiang\\modern\\scratch\\e2e_local\\agent.out" -RedirectStandardError "C:\\moxiang\\modern\\scratch\\e2e_local\\agent.err" -PassThru

# 3. Map server
$map = Start-Process "C:\\moxiang\\modern\\build\\tools\\MoxianMapServer\\Debug\\mxh_map_server_CHINA.exe" -ArgumentList "--port","18001","--backend","sqlite","--db","C:\\moxiang\\modern\\scratch\\e2e_local\\map.db","--map","12","--legacy" -RedirectStandardOutput "C:\\moxiang\\modern\\scratch\\e2e_local\\map.out" -RedirectStandardError "C:\\moxiang\\modern\\scratch\\e2e_local\\map.err" -PassThru

# 4. Verify ports listening
Test-NetConnection 127.0.0.1 -Port 16001 -InformationLevel Quiet
Test-NetConnection 127.0.0.1 -Port 17001 -InformationLevel Quiet
Test-NetConnection 127.0.0.1 -Port 18001 -InformationLevel Quiet

# 5. Run client E2E (--no-spawn skips spawning, connects to OUR servers)
& "C:\\moxiang\\modern\\build\\tools\\MoxianClientE2E\\Debug\\mxh_client_e2e.exe" --no-spawn --backend sqlite --db "C:\\moxiang\\modern\\scratch\\e2e_local\\login.db" --timeout 30

# 6. Cleanup
Stop-Process -Id $login.Id, $agent.Id, $map.Id -Force
'''

### MSSQL LocalDB path (production-shaped)

'''powershell
# 0. Make sure LocalDB is running
sqllocaldb start MSSQLLocalDB
$DbKv = "backend=mssql_odbc;host=(localdb)\\MSSQLLocalDB;database=Moxiang;encrypt=no;trust_server_certificate=yes;"

# 1-3. Same Start-Process pattern as SQLite but with mssql_odbc backend
# (replace sqlite with mssql_odbc, drop --init-schema from server flags,
#  drop per-server --db, share )

# 4. Run client E2E with --init-schema to bootstrap the modern schema
& "C:\\moxiang\\modern\\build\\tools\\MoxianClientE2E\\Debug\\mxh_client_e2e.exe" --no-spawn --backend mssql_odbc --init-schema --db $DbKv --timeout 30

# 5. Cleanup
Stop-Process -Id $login.Id, $agent.Id, $map.Id -Force
'''

## Expected output (excerpt)

`
[e2e] --no-spawn: assuming servers are already up (login:16001, agent:17001, map:18001)
[e2e] [1/5] OK: LoginAck received, user_idx=1 agent=127.0.0.1:17001
[e2e] [2/5] OK: CharacterListAck received, 5 valid slot(s)
[e2e] [3/5] OK: character created, agent re-sent CharacterListAck
[e2e] [4/5] OK: created character present (chrid=240366, 5 valid slot(s))
[e2e] [5/5] OK: GameInAck received, player_id=240366 name=... level=1 map=12 life=100/100
[e2e] Phase B.2.5 e2e: all 5 protocol steps passed (login/charselect/charcreate/relist/gamein)
`

## Architecture

| Process | Role | Port | DB arg | Flags |
|---|---|---|---|---|
| mxh_login_server.exe | login + auth | 16001 | sqlite;path=X OR mssql_odbc kv | --legacy --agent-addr 127.0.0.1 --agent-port 17001 |
| mxh_agent_server_CHINA.exe | char ops | 17001 | same | --legacy --map-server 127.0.0.1:18001 |
| mxh_map_server_CHINA.exe | map | 18001 | same | --legacy --map 12 |
| mxh_client_e2e.exe | headless E2E | 16001/17001/18001 | shared | --no-spawn --backend X --db Y |

## Notes

- --legacy is REQUIRED for the headless client (4DyuchiNET framing).
- --init-schema auto-creates schema for SQLite; for MSSQL it is delegated to mxh_client_e2e --init-schema.
- The E2E client hardcodes ports 16001/17001/18001 and account test/test.
- For sustained runs use --timeout 60.

## See also

- modern/tools/MoxianClientE2E/main.cpp (E2E state machine + 5-step driver)
- modern/include/mxh/client/ (CLoginState / CCharSelectState / CInGameState)
- scripts/clean-deploy.ps1 (M6-A prerequisite: clean-machine deploy)
