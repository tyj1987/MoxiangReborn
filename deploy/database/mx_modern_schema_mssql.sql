-- mx_modern_schema_mssql.sql
--
-- Modern (C++17) Moxian server schema for SQL Server.
-- The modern servers skip --init-schema for non-SQLite backends; apply
-- this script out-of-band to the game database before starting the
-- three processes with --backend mssql_odbc.
--
-- Usage (LocalDB example):
--   sqlcmd -S "(localdb)\MSSQLLocalDB" -E -i mx_modern_schema_mssql.sql

IF DB_ID(N'Moxiang') IS NULL
BEGIN
    CREATE DATABASE [Moxiang];
END
GO

USE [Moxiang]
GO

IF NOT EXISTS (SELECT 1 FROM sys.tables WHERE name = N'chr_log_info')
BEGIN
    CREATE TABLE [dbo].[chr_log_info] (
        [id]         NVARCHAR(50)  NOT NULL PRIMARY KEY,
        [pw]         NVARCHAR(160) NOT NULL,
        [userlevel]  INT           NOT NULL DEFAULT 0
    );
END
GO

-- PBKDF2-SHA256 credentials require 118 characters. Upgrade databases
-- created by older releases without discarding legacy plaintext accounts.
IF COL_LENGTH(N'dbo.chr_log_info', N'pw') < 320
BEGIN
    ALTER TABLE [dbo].[chr_log_info]
        ALTER COLUMN [pw] NVARCHAR(160) NOT NULL;
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.tables WHERE name = N'character_info')
BEGIN
    CREATE TABLE [dbo].[character_info] (
        [chrid]        BIGINT        NOT NULL PRIMARY KEY,
        [charname]     NVARCHAR(50)  NOT NULL,
        [userid]       BIGINT        NOT NULL,
        [sex_type]     TINYINT       NOT NULL DEFAULT 0,
        [hair_type]    TINYINT       NOT NULL DEFAULT 0,
        [face_type]    TINYINT       NOT NULL DEFAULT 0,
        [body_type]    TINYINT       NOT NULL DEFAULT 0,
        [start_area]   INT           NOT NULL DEFAULT 0,
        [height]       FLOAT         NOT NULL DEFAULT 1.0,
        [width]        FLOAT         NOT NULL DEFAULT 1.0,
        [level]        INT           NOT NULL DEFAULT 1,
        [map_num]      INT           NOT NULL DEFAULT 0,
        [standing_idx] INT           NOT NULL DEFAULT 0
    );
END
GO

-- Test account used by the client E2E (id/pw = test/test).
IF NOT EXISTS (SELECT 1 FROM [dbo].[chr_log_info] WHERE [id] = N'test')
BEGIN
    INSERT INTO [dbo].[chr_log_info] ([id], [pw], [userlevel])
    VALUES (N'test', N'test', 2);
END
GO


-- M3 D-stage: modern player state (upsert target for BuySyn money persistence).
IF NOT EXISTS (SELECT 1 FROM sys.tables WHERE name = N'modern_player_state')
BEGIN
    CREATE TABLE [dbo].[modern_player_state] (
        [player_id]  BIGINT        NOT NULL PRIMARY KEY,
        [money]      BIGINT        NOT NULL DEFAULT 0,
        [level]      INT           NOT NULL DEFAULT 1,
        [exp]        BIGINT        NOT NULL DEFAULT 0,
        [updated_at] NVARCHAR(32)  NOT NULL
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes
               WHERE name = N'idx_modern_player_state_updated_at'
                 AND object_id = OBJECT_ID(N'modern_player_state'))
BEGIN
    CREATE NONCLUSTERED INDEX [idx_modern_player_state_updated_at]
        ON [dbo].[modern_player_state] ([updated_at]);
END
GO

-- M3 D-stage: modern player quest log (StartSyn Ok persistence target).
IF NOT EXISTS (SELECT 1 FROM sys.tables WHERE name = N'modern_player_quest_log')
BEGIN
    CREATE TABLE [dbo].[modern_player_quest_log] (
        [player_id]        BIGINT NOT NULL,
        [quest_id]         BIGINT NOT NULL,
        [state]            TINYINT NOT NULL DEFAULT 0,
        [accepted_time_ms] BIGINT NOT NULL DEFAULT 0,
        [updated_at]       NVARCHAR(32) NOT NULL,
        CONSTRAINT [pk_modern_player_quest_log] PRIMARY KEY CLUSTERED
            ([player_id], [quest_id])
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes
               WHERE name = N'idx_modern_player_quest_log_player'
                 AND object_id = OBJECT_ID(N'modern_player_quest_log'))
BEGIN
    CREATE NONCLUSTERED INDEX [idx_modern_player_quest_log_player]
        ON [dbo].[modern_player_quest_log] ([player_id]);
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.tables WHERE name = N'modern_account_status')
BEGIN
    CREATE TABLE [dbo].[modern_account_status] (
        [account_id] NVARCHAR(50) NOT NULL PRIMARY KEY,
        [login_blocked] INT NOT NULL DEFAULT 0,
        [reason] NVARCHAR(256) NOT NULL DEFAULT N'',
        [updated_at] DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME()
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.tables WHERE name = N'modern_account_identity')
BEGIN
    CREATE TABLE [dbo].[modern_account_identity] (
        [account_id] NVARCHAR(50) NOT NULL PRIMARY KEY,
        [user_idx] BIGINT NOT NULL UNIQUE
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.tables WHERE name = N'modern_gm_audit')
BEGIN
    CREATE TABLE [dbo].[modern_gm_audit] (
        [audit_id] BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY,
        [actor] NVARCHAR(64) NOT NULL,
        [target_account] NVARCHAR(50) NOT NULL,
        [action] NVARCHAR(32) NOT NULL,
        [reason] NVARCHAR(256) NOT NULL DEFAULT N'',
        [created_at] DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME()
    );
    CREATE INDEX [idx_modern_gm_audit_target]
        ON [dbo].[modern_gm_audit] ([target_account], [created_at]);
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.tables WHERE name = N'log_chat')
BEGIN
    CREATE TABLE [dbo].[log_chat] (
        [logid] BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY,
        [chrname] NVARCHAR(64) NOT NULL,
        [channel] NVARCHAR(32) NOT NULL,
        [message] NVARCHAR(512) NOT NULL,
        [logtime] DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME()
    );
    CREATE INDEX [idx_log_chat_time] ON [dbo].[log_chat] ([logtime]);
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.tables WHERE name = N'modern_live_event')
BEGIN
    CREATE TABLE [dbo].[modern_live_event] (
        [event_id] BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY,
        [event_type] NVARCHAR(32) NOT NULL,
        [title] NVARCHAR(128) NOT NULL,
        [config_json] NVARCHAR(MAX) NOT NULL,
        [starts_at] DATETIME2 NOT NULL,
        [ends_at] DATETIME2 NOT NULL,
        [enabled] INT NOT NULL DEFAULT 1,
        [created_by] NVARCHAR(64) NOT NULL,
        [created_at] DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME(),
        [updated_at] DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME()
    );
    CREATE INDEX [idx_modern_live_event_window]
        ON [dbo].[modern_live_event] ([enabled], [starts_at], [ends_at]);
END
GO

PRINT 'Moxiang modern schema ready';
GO
