add_executable(mxh_db_tests
    db_adapter_test.cpp
    sqlite_adapter_test.cpp
    mssql_odbc_adapter_test.cpp
    db_thread_test.cpp
    mssql_real_e2e_test.cpp
    real_resource_test.cpp
)

target_link_libraries(mxh_db_tests PRIVATE mxh_db mxh_compat gtest_main)

include(GoogleTest)
# See tests/unit/render/CMakeLists.txt for why we don't use gtest_discover_tests.
gtest_add_tests(TARGET mxh_db_tests
    DbAdapter.ConnectionConfigRoundTrip
    DbAdapter.MakeAdapterReturnsNonNullForSqlite
    DbAdapter.MakeAdapterReturnsNullForUnknown
    DbAdapter.EmptyBackendDefaultsToSqlite
    DbAdapter.SqliteFactoryReturnsSqliteAdapterConcreteType
    DbAdapter.FactoryBackendNameLookupIsCaseSensitive
    DbAdapter.FactoryReturnedAdapterIsNotConnected
    DbAdapter.MssqlFactoryReturnsMssqlAdapterConcreteType
    DbAdapter.MssqlFactoryAliasesRouteToMssqlAdapter
    SqliteAdapter.CreateTableAndInsert
    SqliteAdapter.SelectWithBindParameters
    SqliteAdapter.Transactions
    SqliteAdapter.NullAndBlobBindings
    SqliteAdapter.FileBasedPersistence
    SqliteAdapter.ExecMultiWithInsertOrIgnore
    MssqlOdbcAdapter.FactoryReturnsNonNull
    MssqlOdbcAdapter.FactoryAcceptsAliases
    MssqlOdbcAdapter.InitiallyNotConnected
    MssqlOdbcAdapter.ExecuteWithoutConnectionFails
    MssqlOdbcAdapter.QueryWithoutConnectionFails
    MssqlOdbcAdapter.BeginTransactionWithoutConnectionFails
    MssqlOdbcAdapter.CommitWithoutActiveTransactionFails
    MssqlOdbcAdapter.ConnectToInvalidServerFails
    MssqlOdbcAdapter.DisconnectIsIdempotent
    DbThread.AsyncExecuteQueryAndShutdown
    DbThread.AsyncOperationAfterShutdownFailsImmediately
    RealResource.MonsterListIntoSqlite
    RealResource.PakFileListingToDb
)
# Real SQL Server test skips unless MXH_MSSQL_E2E is configured.
add_test(NAME mssql_real_e2e_test COMMAND mxh_db_tests --gtest_filter=MssqlRealE2E.*)


