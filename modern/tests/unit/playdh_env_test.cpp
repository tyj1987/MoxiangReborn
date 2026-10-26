#include "../support/playdh_env.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

std::string message_of(const char* value) {
    try {
        mxh::test::playdh_dir_from_env_value(value);
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    return {};
}

}  // namespace

TEST(PlayDhEnv, UnsetValueLeavesSearchToTheCaller) {
    EXPECT_FALSE(mxh::test::playdh_dir_from_env_value(nullptr).has_value());
}

TEST(PlayDhEnv, EmptyValueNamesTheVariableAndPath) {
    const auto message = message_of("");
    EXPECT_NE(message.find("MXH_PLAYDH_ROOT"), std::string::npos);
    EXPECT_NE(message.find("\"\""), std::string::npos);
}

TEST(PlayDhEnv, MissingDirectoryNamesTheVariableAndPath) {
    const char* missing = "/tmp/mxh-playdh-root-does-not-exist";
    std::filesystem::remove_all(missing);
    const auto message = message_of(missing);
    EXPECT_NE(message.find("MXH_PLAYDH_ROOT"), std::string::npos);
    EXPECT_NE(message.find(missing), std::string::npos);
}

TEST(PlayDhEnv, FilePathIsNotADirectory) {
    const auto file = std::filesystem::temp_directory_path() / "mxh-playdh-env-not-a-dir";
    {
        std::ofstream out(file);
        ASSERT_TRUE(out);
    }
    const auto message = message_of(file.string().c_str());
    std::filesystem::remove(file);
    EXPECT_NE(message.find("MXH_PLAYDH_ROOT"), std::string::npos);
    EXPECT_NE(message.find(file.string()), std::string::npos);
}

TEST(PlayDhEnv, ExistingDirectoryIsUsed) {
    const auto dir = std::filesystem::temp_directory_path() / "mxh-playdh-env-exists";
    std::filesystem::create_directories(dir);
    const auto resolved = mxh::test::playdh_dir_from_env_value(dir.string().c_str());
    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(std::filesystem::weakly_canonical(*resolved),
              std::filesystem::weakly_canonical(dir));
    std::filesystem::remove_all(dir);
}

TEST(PlayDhEnv, ProcessEnvironmentMatchesValueParser) {
    const char* env = std::getenv(mxh::test::kPlayDhRootEnv);
    if (env == nullptr) {
        EXPECT_FALSE(mxh::test::playdh_dir_override().has_value());
        return;
    }
    std::error_code ec;
    if (std::filesystem::is_directory(env, ec)) {
        const auto got = mxh::test::playdh_dir_override();
        ASSERT_TRUE(got.has_value());
        EXPECT_EQ(std::filesystem::weakly_canonical(*got),
                  std::filesystem::weakly_canonical(env));
        return;
    }
    try {
        mxh::test::playdh_dir_override();
        FAIL() << "MXH_PLAYDH_ROOT was set to a missing directory and did not fail";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("MXH_PLAYDH_ROOT"), std::string::npos);
        EXPECT_NE(std::string(error.what()).find(env), std::string::npos);
    }
}
