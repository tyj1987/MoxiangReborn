#include "mxh/tools/cli_playdh_root.hpp"

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
    void unset() {
#ifdef _WIN32
        _putenv_s(name_, "");
#else
        unsetenv(name_);
#endif
    }
    void set(const std::string& value) {
#ifdef _WIN32
        _putenv_s(name_, value.c_str());
#else
        setenv(name_, value.c_str(), 1);
#endif
    }
    ~EnvVarGuard() {
#ifdef _WIN32
        _putenv_s(name_, had_ ? previous_.c_str() : "");
#else
        if (had_) setenv(name_, previous_.c_str(), 1);
        else unsetenv(name_);
#endif
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

// Locates the checkout that contains the repo-tree PlayDH directory.
// Existence only: this does not open resource bytes. A missing tree fails
// the test. This search belongs to the test, not to the resolver.
std::filesystem::path find_full_checkout_root() {
    std::error_code ec;
    auto cursor = std::filesystem::absolute(std::filesystem::current_path(ec), ec);
    if (ec) return {};
    for (int depth = 0; depth < 8 && !cursor.empty(); ++depth) {
        const auto playdh = cursor / "modern" / "data" / "PlayDH";
        const auto tool = cursor / "modern" / "tools" / "dialog_children_type_scan" / "main.cpp";
        if (std::filesystem::is_directory(playdh, ec) &&
            std::filesystem::is_regular_file(tool, ec)) {
            return cursor;
        }
        const auto parent = cursor.parent_path();
        if (parent == cursor) break;
        cursor = parent;
    }
    return {};
}

std::filesystem::path canonical_or_self(const std::filesystem::path& path) {
    std::error_code ec;
    const auto canonical = std::filesystem::weakly_canonical(path, ec);
    if (ec) return path;
    return canonical;
}

}  // namespace

TEST(CliPlayDhRoot, UnsetDoesNotReadRepoTree) {
    ASSERT_EQ(std::getenv("MXH_PLAYDH_ROOT"), nullptr)
        << "run with MXH_PLAYDH_ROOT unset";
    const auto checkout = find_full_checkout_root();
    ASSERT_FALSE(checkout.empty())
        << "full checkout must contain modern/data/PlayDH";
    const auto repo_playdh = checkout / "modern" / "data" / "PlayDH";
    const WorkingDirectoryGuard cwd(checkout);

    const auto resolved = mxh::tools::resolve_cli_playdh_root({}, nullptr);
    EXPECT_NE(resolved.error.find(mxh::tools::kCliPlayDhRootMissing), std::string::npos)
        << resolved.error << " path=" << resolved.path.string();
    EXPECT_TRUE(resolved.path.empty()) << resolved.path.string();
    if (!resolved.path.empty()) {
        EXPECT_NE(canonical_or_self(resolved.path), canonical_or_self(repo_playdh));
    }
}

TEST(CliPlayDhRoot, SyntheticExplicitRootSucceeds) {
    ASSERT_EQ(std::getenv("MXH_PLAYDH_ROOT"), nullptr)
        << "run with MXH_PLAYDH_ROOT unset";
    const auto checkout = find_full_checkout_root();
    ASSERT_FALSE(checkout.empty())
        << "full checkout must contain modern/data/PlayDH";
    const auto repo_playdh = checkout / "modern" / "data" / "PlayDH";

    ScratchDir scratch("mxh-cli-playdh-root-explicit");
    const auto synthetic = scratch.path() / "playdh";
    std::filesystem::create_directories(synthetic);
    {
        std::ofstream out(synthetic / "marker.txt", std::ios::binary);
        ASSERT_TRUE(out);
        out << "synthetic\n";
    }

    const WorkingDirectoryGuard cwd(checkout);
    const auto resolved =
        mxh::tools::resolve_cli_playdh_root(synthetic.string(), nullptr);
    EXPECT_TRUE(resolved.error.empty()) << resolved.error;
    ASSERT_FALSE(resolved.path.empty());
    EXPECT_EQ(canonical_or_self(resolved.path), canonical_or_self(synthetic));
    EXPECT_NE(canonical_or_self(resolved.path), canonical_or_self(repo_playdh));
}

TEST(CliPlayDhRoot, SyntheticEnvSucceeds) {
    ASSERT_EQ(std::getenv("MXH_PLAYDH_ROOT"), nullptr)
        << "run with MXH_PLAYDH_ROOT unset";
    const auto checkout = find_full_checkout_root();
    ASSERT_FALSE(checkout.empty())
        << "full checkout must contain modern/data/PlayDH";
    const auto repo_playdh = checkout / "modern" / "data" / "PlayDH";

    ScratchDir scratch("mxh-cli-playdh-root-env");
    const auto synthetic = scratch.path() / "playdh";
    std::filesystem::create_directories(synthetic);
    {
        std::ofstream out(synthetic / "marker.txt", std::ios::binary);
        ASSERT_TRUE(out);
        out << "synthetic\n";
    }

    const WorkingDirectoryGuard cwd(checkout);
    EnvVarGuard env("MXH_PLAYDH_ROOT");
    env.set(synthetic.string());
    const auto resolved =
        mxh::tools::resolve_cli_playdh_root({}, std::getenv("MXH_PLAYDH_ROOT"));
    EXPECT_TRUE(resolved.error.empty()) << resolved.error;
    ASSERT_FALSE(resolved.path.empty());
    EXPECT_EQ(canonical_or_self(resolved.path), canonical_or_self(synthetic));
    EXPECT_NE(canonical_or_self(resolved.path), canonical_or_self(repo_playdh));
}
