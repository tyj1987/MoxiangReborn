#include "../support/playdh_env.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string message_of(const char* value) {
    try {
        mxh::test::playdh_dir_from_env_value(value);
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    return {};
}

class ScratchDir {
public:
    explicit ScratchDir(const char* name)
        : path_(std::filesystem::temp_directory_path() / name) {
        std::filesystem::remove_all(path_);
        std::filesystem::create_directories(path_);
    }
    ~ScratchDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }
    const std::filesystem::path& path() const { return path_; }

    ScratchDir(const ScratchDir&) = delete;
    ScratchDir& operator=(const ScratchDir&) = delete;

private:
    std::filesystem::path path_;
};

class WorkingDirectoryGuard {
public:
    explicit WorkingDirectoryGuard(const std::filesystem::path& next)
        : previous_(std::filesystem::current_path()) {
        std::filesystem::current_path(next);
    }
    ~WorkingDirectoryGuard() {
        std::error_code ec;
        std::filesystem::current_path(previous_, ec);
    }

    WorkingDirectoryGuard(const WorkingDirectoryGuard&) = delete;
    WorkingDirectoryGuard& operator=(const WorkingDirectoryGuard&) = delete;

private:
    std::filesystem::path previous_;
};

std::filesystem::path make_marked_playdh(const std::filesystem::path& root) {
    std::filesystem::create_directories(root / "Resource" / "Server");
    std::filesystem::create_directories(root / "Image" / "InterfaceScript");
    return root;
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

TEST(PlayDhEnv, E2EExistingEnvDirectorySkipsFallback) {
    ScratchDir scratch("mxh-e2e-playdh-env-used");
    const auto env_dir = scratch.path() / "from-env";
    std::filesystem::create_directories(env_dir);
    const auto fallback_base = scratch.path() / "fallback";
    const auto fallback_root = make_marked_playdh(
        fallback_base / "modern" / "data" / "PlayDH");

    const auto resolved = mxh::test::resolve_e2e_playdh_root(
        env_dir.string().c_str(), {fallback_base});
    EXPECT_EQ(std::filesystem::weakly_canonical(resolved),
              std::filesystem::weakly_canonical(env_dir));
    EXPECT_NE(std::filesystem::weakly_canonical(resolved),
              std::filesystem::weakly_canonical(fallback_root));
}

TEST(PlayDhEnv, E2EMissingEnvDirectoryDoesNotFallBack) {
    ScratchDir scratch("mxh-e2e-playdh-missing");
    const auto fallback_base = scratch.path() / "fallback";
    make_marked_playdh(fallback_base / "modern" / "data" / "PlayDH");
    const auto missing = (scratch.path() / "missing-dir").string();

    try {
        const auto resolved = mxh::test::resolve_e2e_playdh_root(
            missing.c_str(), {fallback_base});
        FAIL() << "expected MXH_PLAYDH_ROOT error, resolved " << resolved.string();
    } catch (const std::runtime_error& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("MXH_PLAYDH_ROOT"), std::string::npos);
        EXPECT_NE(message.find(missing), std::string::npos);
    }
}

TEST(PlayDhEnv, E2EEmptyEnvValueDoesNotFallBack) {
    ScratchDir scratch("mxh-e2e-playdh-empty");
    const auto fallback_base = scratch.path() / "fallback";
    make_marked_playdh(fallback_base / "modern" / "data" / "PlayDH");

    try {
        const auto resolved = mxh::test::resolve_e2e_playdh_root("", {fallback_base});
        FAIL() << "expected MXH_PLAYDH_ROOT error, resolved " << resolved.string();
    } catch (const std::runtime_error& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("MXH_PLAYDH_ROOT"), std::string::npos);
        EXPECT_NE(message.find("\"\""), std::string::npos);
    }
}

TEST(PlayDhEnv, E2EFileEnvValueDoesNotFallBack) {
    ScratchDir scratch("mxh-e2e-playdh-file");
    const auto file = scratch.path() / "not-a-dir";
    {
        std::ofstream out(file);
        ASSERT_TRUE(out);
    }
    const auto fallback_base = scratch.path() / "fallback";
    make_marked_playdh(fallback_base / "modern" / "data" / "PlayDH");
    const auto text = file.string();

    try {
        const auto resolved = mxh::test::resolve_e2e_playdh_root(
            text.c_str(), {fallback_base});
        FAIL() << "expected MXH_PLAYDH_ROOT error, resolved " << resolved.string();
    } catch (const std::runtime_error& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("MXH_PLAYDH_ROOT"), std::string::npos);
        EXPECT_NE(message.find(text), std::string::npos);
    }
}

TEST(PlayDhEnv, E2EUnsetValueUsesFallbackSearch) {
    ScratchDir scratch("mxh-e2e-playdh-fallback");
    const auto decoy = scratch.path() / "decoy";
    std::filesystem::create_directories(decoy);

    const auto modern_base = scratch.path() / "modern-base";
    const auto modern_root = make_marked_playdh(
        modern_base / "modern" / "data" / "PlayDH");
    make_marked_playdh(modern_base / "data" / "PlayDH");
    const auto modern_hit = mxh::test::resolve_e2e_playdh_root(
        nullptr, {decoy, modern_base});
    EXPECT_EQ(std::filesystem::weakly_canonical(modern_hit),
              std::filesystem::weakly_canonical(modern_root));

    const auto data_only = scratch.path() / "data-only";
    const auto data_root = make_marked_playdh(data_only / "data" / "PlayDH");
    const auto data_hit = mxh::test::resolve_e2e_playdh_root(
        nullptr, {decoy, data_only});
    EXPECT_EQ(std::filesystem::weakly_canonical(data_hit),
              std::filesystem::weakly_canonical(data_root));
}

TEST(PlayDhEnv, E2EUnsetValueWalksUpFromMapExecutable) {
    ScratchDir scratch("mxh-e2e-playdh-walk");
    const auto empty_cwd = scratch.path() / "empty-cwd";
    std::filesystem::create_directories(empty_cwd);
    const auto map_exe = scratch.path() / "out-of-tree" / "build" / "tools" /
                         "MoxianMapServer" / "mxh_map_server_CHINA.exe";
    std::filesystem::create_directories(map_exe.parent_path());
    const auto root = make_marked_playdh(
        scratch.path() / "modern" / "data" / "PlayDH");

    const WorkingDirectoryGuard cwd(empty_cwd);
    const auto resolved = mxh::test::search_e2e_playdh_from_map_exe(map_exe);
    ASSERT_FALSE(resolved.empty());
    EXPECT_EQ(std::filesystem::weakly_canonical(resolved),
              std::filesystem::weakly_canonical(root));

    if (std::getenv(mxh::test::kPlayDhRootEnv) == nullptr) {
        const auto via_wrapper = mxh::test::find_e2e_playdh_root_from_map_exe(map_exe);
        EXPECT_EQ(std::filesystem::weakly_canonical(via_wrapper),
                  std::filesystem::weakly_canonical(root));
    }
}
