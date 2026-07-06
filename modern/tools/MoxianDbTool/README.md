# Moxian DB Tool

命令行工具：数据库初始化、迁移、查询。

## 用法

```bash
# 初始化 SQLite 数据库（创建表 + 默认数据）
mxh_db_tool init --db "sqlite;path=./moxian.db"

# 执行 SQL
mxh_db_tool exec --db "sqlite;path=./moxian.db" "SELECT * FROM chr_log_info"

# 从 MSSQL .bak 迁移到 SQLite
mxh_db_tool migrate --src-mssql-bak MHCMEMBER.bak --dst-sqlite ./moxian.db
```

## 构建

```powershell
cmake --build D:\Moxian\modern\build --config Release --target mxh_db_tool
```