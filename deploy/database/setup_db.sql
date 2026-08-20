-- Run with sqlcmd -v ServiceIdentity="DOMAIN\\MoxianSvc".
-- The Windows identity is provisioned by the operator; this script stores no
-- password and grants only runtime DML access to the Moxiang database.

USE [master]
GO

IF SUSER_ID(N'$(ServiceIdentity)') IS NULL
BEGIN
    CREATE LOGIN [$(ServiceIdentity)] FROM WINDOWS;
END
GO

USE [Moxiang]
GO

IF DATABASE_PRINCIPAL_ID(N'$(ServiceIdentity)') IS NULL
BEGIN
    CREATE USER [$(ServiceIdentity)] FOR LOGIN [$(ServiceIdentity)];
END
GO

IF DATABASE_PRINCIPAL_ID(N'moxian_runtime') IS NULL
BEGIN
    CREATE ROLE [moxian_runtime];
END
GO

GRANT SELECT, INSERT, UPDATE, DELETE ON SCHEMA::[dbo] TO [moxian_runtime];
ALTER ROLE [moxian_runtime] ADD MEMBER [$(ServiceIdentity)];
GO

PRINT 'Moxiang least-privilege runtime identity ready';
GO
