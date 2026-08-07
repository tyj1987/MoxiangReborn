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
        [pw]         NVARCHAR(50)  NOT NULL,
        [userlevel]  INT           NOT NULL DEFAULT 0
    );
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

PRINT 'Moxiang modern schema ready';
GO
