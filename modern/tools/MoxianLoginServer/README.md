# Moxian LoginServer - Phase 4 demo server.

Listens on the configured port, accepts Moxian client connections, and
implements the Distribute phase of the login flow:

  Client → DistributeServer (this binary)
        → validates credentials in SQLite via IDbAdapter
        → returns AgentServer address

Usage:
  mxh_login_server --port 6001 --db "sqlite;path=./moxian.db" \
                    --agent-addr 127.0.0.1 --agent-port 7001