#include <gtest/gtest.h>

#include "../../tools/MoxianAutoPatcher/patch_security.hpp"

TEST(PatchSecurity, AcceptsOnlyContainedRelativePaths) {
    EXPECT_TRUE(mxh::patch::is_safe_relative_path("bin/client.exe"));
    EXPECT_TRUE(mxh::patch::is_safe_relative_path("Data/Map/12.bin"));
    EXPECT_FALSE(mxh::patch::is_safe_relative_path("../outside.exe"));
    EXPECT_FALSE(mxh::patch::is_safe_relative_path("Data/../outside.exe"));
    EXPECT_FALSE(mxh::patch::is_safe_relative_path("C:/Windows/file"));
    EXPECT_FALSE(mxh::patch::is_safe_relative_path("/absolute/file"));
}

TEST(PatchSecurity, Sha256AndSizeVerificationUseRealBytes) {
    const auto path = std::filesystem::temp_directory_path() / "mxh_patch_security.txt";
    {
        std::ofstream output(path, std::ios::binary);
        output << "abc";
    }
    EXPECT_EQ(mxh::patch::sha256_file(path),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    EXPECT_TRUE(mxh::patch::verify_file(path, 3,
        "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD"));
    EXPECT_FALSE(mxh::patch::verify_file(path, 4,
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    std::filesystem::remove(path);
}
