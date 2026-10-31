#include <gtest/gtest.h>

#include "../../tools/MoxianAutoPatcher/patch_rehearsal.hpp"
#include "../../tools/MoxianAutoPatcher/patch_security.hpp"
#include "../../tools/MoxianAutoPatcher/patch_transaction.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef MXH_REPO_ROOT
#error "MXH_REPO_ROOT is required"
#endif

namespace fs = std::filesystem;

namespace {

std::string read_bytes(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void write_bytes(const fs::path& path, const std::string& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(static_cast<bool>(output)) << path;
    output << bytes;
}

std::vector<std::string> scratch_names(const fs::path& root) {
    std::vector<std::string> names;
    std::error_code error;
    if (!fs::is_directory(root, error) || error) return names;
    for (fs::recursive_directory_iterator it(root, error), end; it != end && !error; it.increment(error)) {
        const std::string name = it->path().filename().generic_string();
        if (mxh::patch::is_updater_scratch_name(name)) names.push_back(it->path().generic_string());
    }
    return names;
}

std::string expected_pass_log() {
    mxh::patch::TreeManifest manifest;
    for (const auto& file : mxh::patch::standard_installed_files()) {
        manifest.files.emplace(std::string(file.path),
                                mxh::patch::sha256_bytes(file.bytes.data(), file.bytes.size()));
    }
    return "PATCH_REHEARSAL UPDATE OK files=" +
           std::to_string(mxh::patch::standard_update_files().size()) +
           " sha256=" + manifest.digest() + "\n"
           "PATCH_REHEARSAL ROLLBACK OK restored=" + std::to_string(manifest.files.size()) + "\n"
           "PATCH_REHEARSAL LEFTOVERS 0\n"
           "PATCH_REHEARSAL PASS\n";
}

struct RemoveTree {
    fs::path path;
    ~RemoveTree() {
        std::error_code error;
        fs::remove_all(path, error);
    }
};

struct MidUpdate {
    fs::path game;
    int calls = 0;
    std::string relative;
    std::string bytes;
};

void on_apply(const std::string& relative, void* user) {
    auto* mid = static_cast<MidUpdate*>(user);
    ++mid->calls;
    if (mid->calls != 1) return;
    mid->relative = relative;
    mid->bytes = read_bytes(mid->game / relative);
}

class PatchRehearsal : public ::testing::Test {
protected:
    fs::path make_temp() {
        const auto path = fs::temp_directory_path() /
            ("mxh-rehearsal-test-" + std::to_string(++sequence_) + "-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(path);
        owned_.push_back(path);
        return path;
    }

    void TearDown() override {
        for (const auto& path : owned_) {
            std::error_code error;
            fs::remove_all(path, error);
        }
    }

    std::vector<fs::path> owned_;

private:
    static int sequence_;
};

int PatchRehearsal::sequence_ = 0;

}  // namespace

TEST_F(PatchRehearsal, UpdateSuccess) {
    EXPECT_EQ(mxh::patch::sha256_bytes("", 0),
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(mxh::patch::sha256_bytes("abc", 3),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    const fs::path root = make_temp();
    const fs::path abc = root / "abc.txt";
    write_bytes(abc, "abc");
    EXPECT_EQ(mxh::patch::sha256_file(abc), mxh::patch::sha256_bytes("abc", 3));

    mxh::patch::RehearsalLayout layout;
    std::string error;
    ASSERT_TRUE(mxh::patch::prepare_standard_rehearsal(root, layout, error)) << error;
    for (const auto& file : mxh::patch::standard_installed_files()) {
        EXPECT_EQ(mxh::patch::sha256_file(layout.game / file.path),
                  mxh::patch::sha256_bytes(file.bytes.data(), file.bytes.size()))
            << file.path;
    }

    mxh::patch::PatchTransaction transaction;
    transaction.setGameDir(layout.game.string());
    transaction.setManifest(mxh::patch::rehearsal_file_url(layout.manifest));
    transaction.setManifestSha256(layout.manifest_sha256);
    ASSERT_TRUE(transaction.update());

    for (const auto& file : mxh::patch::standard_update_files()) {
        const fs::path installed = layout.game / file.path;
        EXPECT_EQ(read_bytes(installed), file.bytes) << file.path;
        EXPECT_EQ(mxh::patch::sha256_file(installed),
                  mxh::patch::sha256_bytes(file.bytes.data(), file.bytes.size()))
            << file.path;
    }
    EXPECT_EQ(read_bytes(layout.game / "logs" / "client.log"), "keep-this-log");
    const std::string version = read_bytes(layout.game / "MHVerInfo.ver");
    EXPECT_NE(version.find("VERSION=1.0.0.1"), std::string::npos) << version;
    EXPECT_NE(version.find("CHANNEL=rehearsal"), std::string::npos) << version;
    EXPECT_FALSE(fs::exists(layout.game / "bin" / "client.dat.mxh-new"));
    EXPECT_FALSE(fs::exists(layout.game / ".update-journal"));
    EXPECT_TRUE(fs::is_directory(layout.game / "_backup"));
}

TEST_F(PatchRehearsal, RollbackRestoresBytesAndHashes) {
    ASSERT_EQ(mxh::patch::standard_update_files().size(), 3u);
    ASSERT_EQ(mxh::patch::standard_installed_files().size(), 4u);

    const fs::path root = make_temp();
    mxh::patch::RehearsalLayout layout;
    std::string error;
    ASSERT_TRUE(mxh::patch::prepare_standard_rehearsal(root, layout, error)) << error;
    const auto before = mxh::patch::capture_tree_manifest(layout.game);
    ASSERT_EQ(before.files.size(), 4u);
    for (const auto& entry : before.files) EXPECT_EQ(entry.second.size(), 64u) << entry.first;

    const auto result = mxh::patch::execute_rehearsal(layout);
    const auto after = mxh::patch::capture_tree_manifest(layout.game);
    EXPECT_TRUE(result.manifest_exact);
    EXPECT_TRUE(result.pass);
    EXPECT_EQ(after, before);
    EXPECT_EQ(result.sha256, before.digest());
    EXPECT_EQ(result.text, expected_pass_log());
    EXPECT_EQ(result.restored, 4);
    EXPECT_EQ(result.leftovers, 0);
    for (const auto& file : mxh::patch::standard_installed_files()) {
        EXPECT_EQ(read_bytes(layout.game / file.path), file.bytes) << file.path;
        EXPECT_EQ(mxh::patch::sha256_file(layout.game / file.path), after.files.at(std::string(file.path)))
            << file.path;
    }
    EXPECT_FALSE(fs::exists(layout.game / "bin" / "extra.dat"));
    EXPECT_FALSE(fs::exists(layout.game / "_backup"));
    EXPECT_FALSE(fs::exists(layout.game / ".update-journal"));
    EXPECT_TRUE(scratch_names(root).empty());
}

TEST_F(PatchRehearsal, InterruptedUpdateRestoresManifest) {
    const fs::path root = make_temp();
    const fs::path game = root / "game";
    const fs::path payload = root / "payload";
    const std::string old_a = "old-a-bytes";
    const std::string new_a = "new-a-bytes";
    const std::string old_b = "old-b-bytes";
    const std::string kept = "keep-this-log";
    write_bytes(game / "bin" / "a.dat", old_a);
    write_bytes(game / "bin" / "b.dat", old_b);
    write_bytes(game / "logs" / "client.log", kept);
    write_bytes(game / "MHVerInfo.ver", "VERSION=1.0.0.0\nCHANNEL=rehearsal\n");
    write_bytes(payload / "a.dat", new_a);
    const auto before = mxh::patch::capture_tree_manifest(game);

    const std::string hash_a = mxh::patch::sha256_file(payload / "a.dat");
    const fs::path missing = payload / "missing-b.dat";
    ASSERT_FALSE(fs::exists(missing));
    std::string manifest_text =
        "VERSION=2.0.0.0\n"
        "FILE\tbin/a.dat\t" + mxh::patch::rehearsal_file_url(payload / "a.dat") + "\t" + hash_a + "\t" +
        std::to_string(new_a.size()) + "\n"
        "FILE\tbin/b.dat\t" + mxh::patch::rehearsal_file_url(missing) +
        "\t0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\t" +
        std::to_string(old_b.size()) + "\n";
    const fs::path manifest = root / "manifest.txt";
    write_bytes(manifest, manifest_text);

    MidUpdate observed;
    observed.game = game;
    mxh::patch::PatchTransaction transaction;
    transaction.setGameDir(game.string());
    transaction.setManifest(mxh::patch::rehearsal_file_url(manifest));
    transaction.setManifestSha256(mxh::patch::sha256_file(manifest));
    transaction.setApplyObserver(&on_apply, &observed);
    EXPECT_FALSE(transaction.update());

    EXPECT_EQ(observed.calls, 1);
    EXPECT_EQ(observed.relative, "bin/a.dat");
    EXPECT_EQ(observed.bytes, new_a);
    const auto after = mxh::patch::capture_tree_manifest(game);
    EXPECT_EQ(after, before);
    EXPECT_EQ(read_bytes(game / "bin" / "a.dat"), old_a);
    EXPECT_EQ(read_bytes(game / "bin" / "b.dat"), old_b);
    EXPECT_EQ(read_bytes(game / "logs" / "client.log"), kept);
    EXPECT_EQ(mxh::patch::sha256_file(game / "bin" / "a.dat"), before.files.at("bin/a.dat"));
    EXPECT_EQ(mxh::patch::sha256_file(game / "bin" / "b.dat"), before.files.at("bin/b.dat"));
    EXPECT_EQ(mxh::patch::count_updater_leftovers(root), 0);
    EXPECT_FALSE(fs::exists(game / "_backup"));
    EXPECT_FALSE(fs::exists(game / ".update-journal"));
    EXPECT_FALSE(fs::exists(game / "bin" / "a.dat.mxh-new"));
}

TEST_F(PatchRehearsal, NoLeftoverTmpFiles) {
    const fs::path root = make_temp();
    mxh::patch::RehearsalLayout layout;
    std::string error;
    ASSERT_TRUE(mxh::patch::prepare_standard_rehearsal(root, layout, error)) << error;
    const auto result = mxh::patch::execute_rehearsal(layout);
    ASSERT_TRUE(result.pass) << result.text;
    EXPECT_EQ(result.leftovers, 0);
    EXPECT_TRUE(scratch_names(root).empty());
    EXPECT_FALSE(fs::exists(layout.game / "_backup"));
    EXPECT_FALSE(fs::exists(layout.game / ".update-journal"));
    EXPECT_FALSE(fs::exists(layout.game / "bin" / "client.dat.mxh-new"));
    EXPECT_FALSE(fs::exists(layout.game / "MHVerInfo.ver.tmp.0"));
    EXPECT_FALSE(fs::exists(layout.game / ".manifest-download.tmp"));

    const fs::path interrupted = make_temp();
    const fs::path game = interrupted / "game";
    write_bytes(game / "bin" / "a.dat", "old-a-bytes");
    write_bytes(game / "MHVerInfo.ver", "VERSION=1.0.0.0\n");
    const fs::path source = interrupted / "new-a.dat";
    write_bytes(source, "new-a-bytes");
    const fs::path manifest = interrupted / "manifest.txt";
    write_bytes(manifest,
                "VERSION=2.0.0.0\nFILE\tbin/a.dat\t" + mxh::patch::rehearsal_file_url(source) +
                    "\taaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\t" +
                    std::to_string(std::string("new-a-bytes").size()) + "\n");
    mxh::patch::PatchTransaction transaction;
    transaction.setGameDir(game.string());
    transaction.setManifest(mxh::patch::rehearsal_file_url(manifest));
    transaction.setManifestSha256(mxh::patch::sha256_file(manifest));
    EXPECT_FALSE(transaction.update());
    EXPECT_EQ(mxh::patch::count_updater_leftovers(interrupted), 0);
    EXPECT_TRUE(scratch_names(interrupted).empty());
    EXPECT_FALSE(fs::exists(game / "_backup"));
    EXPECT_FALSE(fs::exists(game / "bin" / "a.dat.mxh-new"));
    EXPECT_FALSE(fs::exists(game / "bin" / "a.dat.mxh-download"));
}

TEST_F(PatchRehearsal, HashMismatchFailure) {
    const fs::path root = make_temp();
    const fs::path game = root / "game";
    write_bytes(game / "bin" / "client.dat", "old-client-bytes");
    write_bytes(game / "logs" / "client.log", "keep-this-log");
    write_bytes(game / "MHVerInfo.ver", "VERSION=1.0.0.0\n");
    const auto before = mxh::patch::capture_tree_manifest(game);
    const fs::path source = root / "new-client.dat";
    write_bytes(source, "new-client-bytes");
    const fs::path manifest = root / "manifest.txt";
    write_bytes(manifest,
                "VERSION=3.0.0.1\nFILE\tbin/client.dat\t" + mxh::patch::rehearsal_file_url(source) +
                    "\t0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\t" +
                    std::to_string(std::string("new-client-bytes").size()) + "\n");

    MidUpdate observed;
    observed.game = game;
    mxh::patch::PatchTransaction transaction;
    transaction.setGameDir(game.string());
    transaction.setManifest(mxh::patch::rehearsal_file_url(manifest));
    transaction.setManifestSha256(mxh::patch::sha256_file(manifest));
    transaction.setApplyObserver(&on_apply, &observed);
    EXPECT_FALSE(transaction.update());
    EXPECT_EQ(observed.calls, 0);
    EXPECT_EQ(mxh::patch::capture_tree_manifest(game), before);
    EXPECT_EQ(read_bytes(game / "bin" / "client.dat"), "old-client-bytes");
    EXPECT_EQ(mxh::patch::sha256_file(game / "bin" / "client.dat"), before.files.at("bin/client.dat"));
    EXPECT_EQ(mxh::patch::count_updater_leftovers(root), 0);
    EXPECT_FALSE(fs::exists(game / "_backup"));

    mxh::patch::PatchTransaction bad_manifest;
    bad_manifest.setGameDir(game.string());
    bad_manifest.setManifest(mxh::patch::rehearsal_file_url(manifest));
    bad_manifest.setManifestSha256(
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    try {
        bad_manifest.update();
        FAIL() << "manifest digest mismatch must throw";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("manifest SHA-256 verification failed"), std::string::npos)
            << error.what();
    }
    EXPECT_EQ(mxh::patch::capture_tree_manifest(game), before);
    EXPECT_FALSE(fs::exists(game / "_backup"));
    EXPECT_FALSE(fs::exists(game / ".update-journal"));
}

TEST_F(PatchRehearsal, RejectsPlayDhTarget) {
    const fs::path root = make_temp();
    const fs::path cases[] = {
        root / "PlayDH",
        root / "playdh" / "client",
        root / "PLAYDH" / "client",
        root / "a" / "PlayDh" / "b",
    };
    for (const auto& path : cases) {
        const auto rejected = mxh::patch::reject_rehearsal_target(path);
        ASSERT_TRUE(rejected.has_value()) << path;
        EXPECT_EQ(rejected->reason, mxh::patch::RehearsalRejectReason::PlayDh) << rejected->message;
        mxh::patch::RehearsalLayout layout;
        std::string error;
        EXPECT_FALSE(mxh::patch::prepare_standard_rehearsal(path, layout, error)) << path;
        EXPECT_NE(error.find("PlayDH"), std::string::npos) << error;
        EXPECT_FALSE(fs::exists(path)) << path;
    }

    const auto relative = mxh::patch::reject_rehearsal_target(fs::path("PlayDH") / "client");
    ASSERT_TRUE(relative.has_value());
    EXPECT_EQ(relative->reason, mxh::patch::RehearsalRejectReason::PlayDh);

    const auto allowed = mxh::patch::reject_rehearsal_target(root / "playdh-notes" / "client");
    EXPECT_FALSE(allowed.has_value());

    fs::create_directories(root / "PlayDH");
    const fs::path alias = root / "alias-to-playdh";
    std::error_code link_error;
    fs::create_directory_symlink(root / "PlayDH", alias, link_error);
    ASSERT_FALSE(link_error) << link_error.message();
    const auto via_link = mxh::patch::reject_rehearsal_target(alias / "game");
    ASSERT_TRUE(via_link.has_value()) << alias;
    EXPECT_EQ(via_link->reason, mxh::patch::RehearsalRejectReason::PlayDh) << via_link->message;
    mxh::patch::RehearsalLayout layout;
    std::string error;
    EXPECT_FALSE(mxh::patch::prepare_standard_rehearsal(alias / "game", layout, error));
    EXPECT_FALSE(fs::exists(root / "PlayDH" / "game"));
}

TEST_F(PatchRehearsal, RejectsRepositoryTarget) {
    const fs::path repo{MXH_REPO_ROOT};
    ASSERT_TRUE(fs::exists(repo / ".git") || fs::exists(repo.parent_path() / ".git"));

    const fs::path probe = repo / "rehearsal-reject-probe";
    owned_.push_back(probe);
    fs::create_directories(probe);
    const auto rejected = mxh::patch::reject_rehearsal_target(probe);
    ASSERT_TRUE(rejected.has_value()) << probe;
    EXPECT_EQ(rejected->reason, mxh::patch::RehearsalRejectReason::InsideRepository) << rejected->message;

    mxh::patch::RehearsalLayout layout;
    std::string error;
    EXPECT_FALSE(mxh::patch::prepare_standard_rehearsal(probe, layout, error));
    EXPECT_NE(error.find("repository working tree"), std::string::npos) << error;
    EXPECT_TRUE(fs::is_empty(probe));

    const fs::path missing = repo / "rehearsal-reject-missing";
    RemoveTree missing_guard{missing};
    ASSERT_FALSE(fs::exists(missing));
    EXPECT_FALSE(mxh::patch::prepare_standard_rehearsal(missing, layout, error));
    EXPECT_FALSE(fs::exists(missing));

    std::ostringstream out;
    std::ostringstream err;
    const int code = mxh::patch::run_patch_rehearsal_cli(out, err, probe);
    EXPECT_NE(code, 0);
    EXPECT_NE(err.str().find("repository working tree"), std::string::npos) << err.str();
    EXPECT_EQ(out.str().find("PATCH_REHEARSAL PASS"), std::string::npos) << out.str();
    for (const auto& entry : fs::directory_iterator(probe)) {
        EXPECT_FALSE(entry.path().filename().string().rfind("mxh-patch-rehearsal-", 0) == 0) << entry.path();
    }

    const fs::path link_root = make_temp();
    const fs::path link = link_root / "into-repo";
    std::error_code link_error;
    fs::create_directory_symlink(repo, link, link_error);
    ASSERT_FALSE(link_error) << link_error.message();
    RemoveTree child_guard{repo / "rehearsal-link-child"};
    const auto via_link = mxh::patch::reject_rehearsal_target(link / "rehearsal-link-child");
    ASSERT_TRUE(via_link.has_value());
    EXPECT_EQ(via_link->reason, mxh::patch::RehearsalRejectReason::InsideRepository) << via_link->message;
    EXPECT_FALSE(mxh::patch::prepare_standard_rehearsal(link / "rehearsal-link-child", layout, error));
    EXPECT_FALSE(fs::exists(repo / "rehearsal-link-child"));
}

TEST_F(PatchRehearsal, RejectsImplicitTarget) {
    const auto empty = mxh::patch::reject_rehearsal_target({});
    ASSERT_TRUE(empty.has_value());
    EXPECT_EQ(empty->reason, mxh::patch::RehearsalRejectReason::Empty);
    EXPECT_NE(empty->message.find("explicit absolute path"), std::string::npos);

    for (const char* path : {".", "game", "relative/tree"}) {
        const auto rejected = mxh::patch::reject_rehearsal_target(fs::path(path));
        ASSERT_TRUE(rejected.has_value()) << path;
        EXPECT_EQ(rejected->reason, mxh::patch::RehearsalRejectReason::NotAbsolute) << path;
        EXPECT_NE(rejected->message.find("explicit absolute path"), std::string::npos) << path;
    }

    mxh::patch::RehearsalCliArgs conflict;
    conflict.rehearsal = true;
    conflict.has_directory = true;
    const auto message = mxh::patch::rehearsal_cli_conflict(conflict);
    ASSERT_TRUE(message.has_value());
    EXPECT_NE(message->find("does not accept a target path"), std::string::npos);

    mxh::patch::RehearsalCliArgs clean;
    clean.rehearsal = true;
    EXPECT_FALSE(mxh::patch::rehearsal_cli_conflict(clean).has_value());
}

TEST_F(PatchRehearsal, PassIgnoresRollbackExitCode) {
    mxh::patch::RehearsalLogInput matched;
    matched.update_ok = true;
    matched.rollback_exit_code = false;
    matched.manifest_exact = true;
    matched.files = 3;
    matched.restored = 4;
    matched.leftovers = 0;
    matched.sha256 = std::string(64, 'a');
    EXPECT_TRUE(mxh::patch::rehearsal_should_pass(matched));
    const std::string pass_text = mxh::patch::format_rehearsal_log(matched);
    EXPECT_NE(pass_text.find("PATCH_REHEARSAL ROLLBACK OK restored=4"), std::string::npos) << pass_text;
    EXPECT_NE(pass_text.find("PATCH_REHEARSAL PASS"), std::string::npos) << pass_text;
    EXPECT_EQ(pass_text.find("PATCH_REHEARSAL FAIL"), std::string::npos) << pass_text;

    mxh::patch::RehearsalLogInput mismatched = matched;
    mismatched.rollback_exit_code = true;
    mismatched.manifest_exact = false;
    mismatched.restored = 3;
    EXPECT_FALSE(mxh::patch::rehearsal_should_pass(mismatched));
    const std::string fail_text = mxh::patch::format_rehearsal_log(mismatched);
    EXPECT_NE(fail_text.find("PATCH_REHEARSAL ROLLBACK MISMATCH restored=3"), std::string::npos) << fail_text;
    EXPECT_NE(fail_text.find("PATCH_REHEARSAL FAIL"), std::string::npos) << fail_text;
    EXPECT_EQ(fail_text.find("PATCH_REHEARSAL PASS"), std::string::npos) << fail_text;

    const fs::path root = make_temp();
    mxh::patch::RehearsalLayout layout;
    std::string error;
    ASSERT_TRUE(mxh::patch::prepare_standard_rehearsal(root, layout, error)) << error;
    const auto before = mxh::patch::capture_tree_manifest(layout.game);
    const auto result = mxh::patch::execute_rehearsal(layout);
    ASSERT_TRUE(result.pass) << result.text;
    ASSERT_TRUE(result.manifest_exact);
    EXPECT_EQ(mxh::patch::capture_tree_manifest(layout.game), before);

    mxh::patch::PatchTransaction again;
    again.setGameDir(layout.game.string());
    EXPECT_FALSE(again.rollback());
    EXPECT_EQ(mxh::patch::capture_tree_manifest(layout.game), before);
    EXPECT_EQ(mxh::patch::count_updater_leftovers(root), 0);

    mxh::patch::RehearsalLogInput after_false_exit;
    after_false_exit.update_ok = true;
    after_false_exit.rollback_exit_code = false;
    after_false_exit.manifest_exact = true;
    after_false_exit.files = result.files;
    after_false_exit.restored = mxh::patch::restored_file_count(
        before, mxh::patch::capture_tree_manifest(layout.game));
    after_false_exit.leftovers = 0;
    after_false_exit.sha256 = before.digest();
    EXPECT_TRUE(mxh::patch::rehearsal_should_pass(after_false_exit));
    EXPECT_NE(mxh::patch::format_rehearsal_log(after_false_exit).find("PATCH_REHEARSAL PASS"), std::string::npos);
}

TEST_F(PatchRehearsal, CliCleansOwnTempDir) {
    const fs::path parent = make_temp();
    write_bytes(parent / "sentinel.txt", "keep-sentinel");
    fs::create_directories(parent / "not-ours");
    write_bytes(parent / "not-ours" / "keep.txt", "keep");

    std::ostringstream out;
    std::ostringstream err;
    const int code = mxh::patch::run_patch_rehearsal_cli(out, err, parent);
    EXPECT_EQ(code, 0) << out.str() << err.str();
    EXPECT_EQ(err.str(), "");
    EXPECT_EQ(out.str(), expected_pass_log()) << out.str();
    EXPECT_EQ(read_bytes(parent / "sentinel.txt"), "keep-sentinel");
    EXPECT_EQ(read_bytes(parent / "not-ours" / "keep.txt"), "keep");

    int children = 0;
    for (const auto& entry : fs::directory_iterator(parent)) {
        ++children;
        EXPECT_NE(entry.path().filename().string().rfind("mxh-patch-rehearsal-", 0), 0u) << entry.path();
    }
    EXPECT_EQ(children, 2);
}
