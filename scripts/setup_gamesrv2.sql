-- Reset GameSrv password and ensure mixed mode auth works
ALTER LOGIN [GameSrv] WITH PASSWORD = 'w8j2f@Z0H7#Xl', CHECK_POLICY = OFF, CHECK_EXPIRATION = OFF;
GO
ALTER LOGIN [GameSrv] ENABLE;
GO

-- Create database users and grant permissions
USE [MHGame];
GO
IF EXISTS (SELECT * FROM sys.database_principals WHERE name = 'GameSrv')
    DROP USER [GameSrv];
GO
CREATE USER [GameSrv] FOR LOGIN [GameSrv];
GO
EXEC sp_addrolemember 'db_owner', 'GameSrv';
GO

USE [MHCMEMBER];
GO
IF EXISTS (SELECT * FROM sys.database_principals WHERE name = 'GameSrv')
    DROP USER [GameSrv];
GO
CREATE USER [GameSrv] FOR LOGIN [GameSrv];
GO
EXEC sp_addrolemember 'db_owner', 'GameSrv';
GO

USE [MHLOG];
GO
IF EXISTS (SELECT * FROM sys.database_principals WHERE name = 'GameSrv')
    DROP USER [GameSrv];
GO
CREATE USER [GameSrv] FOR LOGIN [GameSrv];
GO
EXEC sp_addrolemember 'db_owner', 'GameSrv';
GO
