#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

TEST(SqliteBackupScript, ContainsCommercialSafetyGates) {
    const auto script = std::filesystem::path(MXH_SOURCE_DIR).parent_path() /
                        "scripts" / "sqlite-backup.ps1";
    std::ifstream input(script, std::ios::binary);
    ASSERT_TRUE(input.is_open());
    const std::string text((std::istreambuf_iterator<char>(input)), {});
    EXPECT_NE(text.find("PRAGMA integrity_check"), std::string::npos);
    EXPECT_NE(text.find("VACUUM INTO"), std::string::npos);
    EXPECT_NE(text.find("Get-FileHash"), std::string::npos);
    EXPECT_NE(text.find("Restore target exists; pass -Force"), std::string::npos);
    EXPECT_NE(text.find(".restore-new"), std::string::npos);
}
