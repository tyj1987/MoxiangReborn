add_executable(mxh_db_tests
    db_adapter_test.cpp
    sqlite_adapter_test.cpp
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
    SqliteAdapter.CreateTableAndInsert
    SqliteAdapter.SelectWithBindParameters
    SqliteAdapter.Transactions
    SqliteAdapter.NullAndBlobBindings
    SqliteAdapter.FileBasedPersistence
    RealResource.MonsterListIntoSqlite
    RealResource.PakFileListingToDb
)