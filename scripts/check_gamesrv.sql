-- Check if GameSrv login exists and is enabled
SELECT name, type_desc, is_disabled FROM sys.server_principals WHERE name = 'GameSrv';
GO

-- Check database user mappings
USE [MHGame];
SELECT dp.name AS DatabaseUser, sp.name AS LoginName 
FROM sys.database_principals dp 
LEFT JOIN sys.server_principals sp ON dp.sid = sp.sid 
WHERE sp.name = 'GameSrv';
GO
