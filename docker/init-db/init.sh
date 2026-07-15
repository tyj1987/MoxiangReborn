#!/bin/bash
# Moxian-Reborn Database Initialization Script
# This script runs when the db-init container starts

set -e

echo "Waiting for SQL Server to be ready..."
sleep 10

echo "Initializing Moxian database..."

/opt/mssql-tools/bin/sqlcmd -S $DB_HOST -U sa -P $SA_PASSWORD -Q "
IF NOT EXISTS (SELECT * FROM sys.databases WHERE name = 'MoxianDB')
BEGIN
    CREATE DATABASE MoxianDB;
    PRINT 'Database MoxianDB created successfully.';
END
ELSE
BEGIN
    PRINT 'Database MoxianDB already exists.';
END
"

echo "Database initialization completed."
