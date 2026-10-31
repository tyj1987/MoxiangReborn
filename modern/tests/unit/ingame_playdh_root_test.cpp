#include "ingame_playdh_root.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

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

class EnvVarGuard {
public:
    explicit EnvVarGuard(const char* name) : name_(name) {
        const char* current = std::getenv(name_);
        if (current != nullptr) {
            had_ = true;
            previous_ = current;
        }
    }
    void unset() { unsetenv(name_); }
    void set(const std::string& value) { setenv(name_, value.c_str(), 1); }
    ~EnvVarGuard() {
        if (had_) setenv(name_, previous_.c_str(), 1);
        else unsetenv(name_);
    }

    EnvVarGuard(const EnvVarGuard&) = delete;
    EnvVarGuard& operator=(const EnvVarGuard&) = delete;

private:
    const char* name_;
    bool had_ = false;
    std::string previous_;
};

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

// Locates the checkout that contains the repo-tree PlayDH copy. Fails the
// caller when that tree is absent. This is the test's own search, not the
// resolver under test.
std::filesystem::path find_full_checkout_root() {
    std::error_code ec;
    auto cursor = std::filesystem::absolute(std::filesystem::current_path(ec), ec);
    if (ec) return {};
    for (int depth = 0; depth < 8 && !cursor.empty(); ++depth) {
        const auto playdh = cursor / "modern" / "data" / "PlayDH";
        const auto marker = playdh / "Image" / "InterfaceScript" / "15.bin";
        if (std::filesystem::is_regular_file(marker, ec)) return cursor;
        const auto parent = cursor.parent_path();
        if (parent == cursor) break;
        cursor = parent;
    }
    return {};
}

}  // namespace

TEST(InGamePlayDhRoot, UnsetDoesNotReadRepoTree) {
    ASSERT_EQ(std::getenv("MXH_PLAYDH_ROOT"), nullptr)
        << "run with MXH_PLAYDH_ROOT unset";
    const auto checkout = find_full_checkout_root();
    ASSERT_FALSE(checkout.empty())
        << "full checkout must contain modern/data/PlayDH/Image/InterfaceScript/15.bin";
    const auto repo_playdh = checkout / "modern" / "data" / "PlayDH";
    const WorkingDirectoryGuard cwd(checkout);

    const auto resolved = mxh::client::resolve_ingame_playdh_root(std::nullopt, nullptr);
    EXPECT_NE(resolved.error.find(mxh::client::kInGamePlayDhRootMissing),
              std::string::npos)
        << resolved.error << " path=" << resolved.path.string();
    EXPECT_TRUE(resolved.path.empty()) << resolved.path.string();
    if (!resolved.path.empty()) {
        EXPECT_NE(std::filesystem::weakly_canonical(resolved.path),
                  std::filesystem::weakly_canonical(repo_playdh));
    }
}

TEST(InGamePlayDhRoot, SyntheticEnvSucceeds) {
    ASSERT_EQ(std::getenv("MXH_PLAYDH_ROOT"), nullptr)
        << "run with MXH_PLAYDH_ROOT unset";
    const auto checkout = find_full_checkout_root();
    ASSERT_FALSE(checkout.empty())
        << "full checkout must contain modern/data/PlayDH/Image/InterfaceScript/15.bin";
    const auto repo_playdh = checkout / "modern" / "data" / "PlayDH";

    ScratchDir scratch("mxh-ingame-playdh-root-synthetic");
    const auto synthetic = scratch.path() / "playdh";
    std::filesystem::create_directories(synthetic / "Image" / "InterfaceScript");
    {
        std::ofstream out(synthetic / "Image" / "InterfaceScript" / "15.bin",
                          std::ios::binary);
        ASSERT_TRUE(out);
        out << "synthetic\n";
    }

    const WorkingDirectoryGuard cwd(checkout);
    EnvVarGuard env("MXH_PLAYDH_ROOT");
    env.set(synthetic.string());
    const auto resolved = mxh::client::resolve_ingame_playdh_root(
        std::nullopt, std::getenv("MXH_PLAYDH_ROOT"));
    EXPECT_TRUE(resolved.error.empty()) << resolved.error;
    ASSERT_FALSE(resolved.path.empty());
    EXPECT_EQ(std::filesystem::weakly_canonical(resolved.path),
              std::filesystem::weakly_canonical(synthetic));
    EXPECT_NE(std::filesystem::weakly_canonical(resolved.path),
              std::filesystem::weakly_canonical(repo_playdh));
}
