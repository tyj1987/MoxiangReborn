CREATE LOGIN [GameSrv] WITH PASSWORD = 'w8j2f@Z0H7#Xl', DEFAULT_DATABASE = [MHGAME]
GO
USE [MHCMEMBER]
GO
CREATE USER [GameSrv] FOR LOGIN [GameSrv]
GO
ALTER ROLE [db_owner] ADD MEMBER [GameSrv]
GO
USE [MHGAME]
GO
CREATE USER [GameSrv] FOR LOGIN [GameSrv]
GO
ALTER ROLE [db_owner] ADD MEMBER [GameSrv]
GO
PRINT 'Database user setup complete'
