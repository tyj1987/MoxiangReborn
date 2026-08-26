-- Supply GameSrvPassword through a protected SQLCMD variable or secret-aware
-- deployment wrapper; credentials must never be committed to this template.
ALTER LOGIN [GameSrv] WITH PASSWORD = '$(GameSrvPassword)', CHECK_POLICY = OFF, CHECK_EXPIRATION = OFF;
GO
ALTER LOGIN [GameSrv] ENABLE;
GO
USE [MHGame];
GO
IF NOT EXISTS (SELECT * FROM sys.database_principals WHERE name = 'GameSrv')
BEGIN
    CREATE USER [GameSrv] FOR LOGIN [GameSrv];
END
GO
EXEC sp_addrolemember 'db_owner', 'GameSrv';
GO
USE [MHCMEMBER];
GO
IF NOT EXISTS (SELECT * FROM sys.database_principals WHERE name = 'GameSrv')
BEGIN
    CREATE USER [GameSrv] FOR LOGIN [GameSrv];
END
GO
EXEC sp_addrolemember 'db_owner', 'GameSrv';
GO
USE [MHLOG];
GO
IF NOT EXISTS (SELECT * FROM sys.database_principals WHERE name = 'GameSrv')
BEGIN
    CREATE USER [GameSrv] FOR LOGIN [GameSrv];
END
GO
EXEC sp_addrolemember 'db_owner', 'GameSrv';
GO
