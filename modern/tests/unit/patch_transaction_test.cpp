#include <gtest/gtest.h>

#include "../../tools/MoxianAutoPatcher/patch_security.hpp"
#include "../../tools/MoxianAutoPatcher/patch_transaction.hpp"

#include <chrono>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>

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

std::string file_url(const fs::path& path) {
    return "file://" + fs::absolute(path).generic_string();
}

std::map<std::string, std::string> snapshot(const fs::path& root) {
    std::map<std::string, std::string> files;
    std::error_code error;
    for (fs::recursive_directory_iterator it(root, error), end; it != end && !error; it.increment(error)) {
        if (error || !it->is_regular_file(error)) continue;
        files.emplace(it->path().lexically_relative(root).generic_string(), read_bytes(it->path()));
    }
    return files;
}

std::string expect_throw_update(mxh::patch::PatchTransaction& transaction) {
    try {
        transaction.update();
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    return {};
}

class PatchTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = fs::temp_directory_path() /
            ("mxh-patch-tx-" + std::to_string(++sequence_) + "-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        game_ = root_ / "game";
        payload_ = root_ / "payload";
        fs::create_directories(game_);
        fs::create_directories(payload_);
    }

    void TearDown() override {
        std::error_code error;
        fs::remove_all(root_, error);
    }

    fs::path write_payload(const std::string& name, const std::string& bytes) {
        const auto path = payload_ / name;
        write_bytes(path, bytes);
        return path;
    }

    fs::path write_manifest(const std::string& text) {
        const auto path = root_ / "manifest.txt";
        write_bytes(path, text);
        return path;
    }

    mxh::patch::PatchTransaction transaction_for(const fs::path& manifest) {
        mxh::patch::PatchTransaction transaction;
        transaction.setGameDir(game_.string());
        transaction.setManifest(file_url(manifest));
        transaction.setManifestSha256(mxh::patch::sha256_file(manifest));
        return transaction;
    }

    std::string file_row(const std::string& relative, const fs::path& source) const {
        const auto bytes = read_bytes(source);
        return "FILE\t" + relative + "\t" + file_url(source) + "\t" +
               mxh::patch::sha256_file(source) + "\t" + std::to_string(bytes.size());
    }

    fs::path root_;
    fs::path game_;
    fs::path payload_;

private:
    static int sequence_;
};

int PatchTransactionTest::sequence_ = 0;

}  // namespace

TEST_F(PatchTransactionTest, PortableSha256MatchesKnownVectors) {
    const std::pair<std::string, std::string> vectors[] = {
        {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
        {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
        {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
         "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
        {std::string(70, 'a'),
         "6bd5e5034855a11241f0dee8fc72850ffd9955b28347a86428b5fa19119f6ad0"},
        {std::string(100, 'a'),
         "2816597888e4a0d3a36b82b83316ab32680eb8f00f8cd3b904d681246d285a0e"},
    };
    int index = 0;
    for (const auto& vector : vectors) {
        const auto path = payload_ / ("vector-" + std::to_string(index++));
        write_bytes(path, vector.first);
        EXPECT_EQ(mxh::patch::sha256_file(path), vector.second) << path;
        EXPECT_TRUE(mxh::patch::verify_file(path, vector.first.size(), vector.second));
    }
}

TEST_F(PatchTransactionTest, VersionOrderingAndNotNewerLeavesTreeUntouched) {
    EXPECT_TRUE(mxh::patch::Version::fromString("1.0.0.9") <
                mxh::patch::Version::fromString("1.0.0.10"));
    EXPECT_TRUE(mxh::patch::Version::fromString("1.0.0.0") <
                mxh::patch::Version::fromString("1.0.1.0"));
    EXPECT_TRUE(mxh::patch::Version::fromString("1.0.0.0") <
                mxh::patch::Version::fromString("1.1.0.0"));
    EXPECT_TRUE(mxh::patch::Version::fromString("1.0.0.0") <
                mxh::patch::Version::fromString("2.0.0.0"));
    EXPECT_FALSE(mxh::patch::Version::fromString("1.2.0.0") <
                 mxh::patch::Version::fromString("1.1.9.9"));
    EXPECT_EQ(mxh::patch::Version::fromString("1.2.3.4"),
              mxh::patch::Version::fromString("1.2.3.4"));
    EXPECT_EQ(mxh::patch::Version::fromString("1.2.3"), mxh::patch::Version{});
    EXPECT_EQ(mxh::patch::Version::fromString("999999999999999999999.0.0.0"),
              mxh::patch::Version{});
    EXPECT_EQ(mxh::patch::Version::fromString("1.2.3.4").toString(), "1.2.3.4");

    write_bytes(game_ / "bin" / "client.dat", "installed-bytes");
    write_bytes(game_ / "MHVerInfo.ver", "VERSION=2.0.0.0\nCHANNEL=test\n");
    const auto replacement = write_payload("replacement.dat", "should-not-land");
    const auto before = snapshot(game_);
    for (const char* version : {"2.0.0.0", "1.9.9.9"}) {
        const auto manifest = write_manifest(
            std::string("VERSION=") + version + "\n" + file_row("bin/client.dat", replacement) + "\n");
        auto transaction = transaction_for(manifest);
        EXPECT_TRUE(transaction.update()) << version;
        EXPECT_EQ(snapshot(game_), before) << version;
        EXPECT_FALSE(fs::exists(game_ / "_backup")) << version;
    }
}

TEST_F(PatchTransactionTest, UpdateAndRollbackRoundTrip) {
    write_bytes(game_ / "bin" / "client.dat", "old-client-bytes");
    write_bytes(game_ / "bin" / "obsolete.dat", "obsolete-bytes");
    write_bytes(game_ / "logs" / "client.log", "keep-this-log");
    write_bytes(game_ / "MHVerInfo.ver", "VERSION=1.0.0.0\nCHANNEL=test\n");
    const auto before = snapshot(game_);

    const auto client = write_payload("client.dat", "new-client-bytes");
    const auto added = write_payload("added.dat", "brand-new-file");
    const auto manifest = write_manifest(
        "VERSION=1.4.0.2\n" +
        file_row("bin/client.dat", client) + "\n" +
        file_row("bin/added.dat", added) + "\n" +
        "DELETE\tbin/obsolete.dat\n");
    auto transaction = transaction_for(manifest);
    ASSERT_TRUE(transaction.update());

    EXPECT_EQ(read_bytes(game_ / "bin" / "client.dat"), "new-client-bytes");
    EXPECT_EQ(read_bytes(game_ / "bin" / "added.dat"), "brand-new-file");
    EXPECT_FALSE(fs::exists(game_ / "bin" / "obsolete.dat"));
    EXPECT_EQ(read_bytes(game_ / "logs" / "client.log"), "keep-this-log");
    EXPECT_NE(read_bytes(game_ / "MHVerInfo.ver").find("VERSION=1.4.0.2"), std::string::npos);
    EXPECT_NE(read_bytes(game_ / "MHVerInfo.ver").find("CHANNEL=test"), std::string::npos);

    ASSERT_TRUE(transaction.rollback());
    EXPECT_EQ(snapshot(game_), before);
    EXPECT_FALSE(fs::exists(game_ / "_backup"));
    EXPECT_FALSE(fs::exists(game_ / ".update-journal"));
}

TEST_F(PatchTransactionTest, BadFileDigestLeavesTreeUntouched) {
    write_bytes(game_ / "bin" / "client.dat", "old-client-bytes");
    write_bytes(game_ / "logs" / "client.log", "keep-this-log");
    write_bytes(game_ / "MHVerInfo.ver", "VERSION=1.0.0.0\n");
    const auto before = snapshot(game_);
    const auto payload = write_payload("client.dat", "new-client-bytes");
    const auto bytes = read_bytes(payload);
    const auto manifest = write_manifest(
        "VERSION=3.0.0.1\n"
        "FILE\tbin/client.dat\t" + file_url(payload) +
        "\t0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\t" +
        std::to_string(bytes.size()) + "\n");
    auto transaction = transaction_for(manifest);
    EXPECT_FALSE(transaction.update());
    EXPECT_EQ(snapshot(game_), before);
    EXPECT_FALSE(fs::exists(game_ / "_backup"));
    EXPECT_FALSE(fs::exists(game_ / ".update-journal"));
    EXPECT_FALSE(fs::exists(game_ / "bin" / "client.dat.mxh-new"));
}

TEST_F(PatchTransactionTest, RejectsDotDotAndAbsoluteTargets) {
    write_bytes(game_ / "bin" / "client.dat", "old-client-bytes");
    write_bytes(game_ / "MHVerInfo.ver", "VERSION=1.0.0.0\n");
    const auto before = snapshot(game_);
    const auto payload = write_payload("client.dat", "new-client-bytes");
    const char* rows[] = {
        "FILE\t../outside.dat\t",
        "FILE\t/tmp/outside.dat\t",
        "FILE\tbin/../../outside.dat\t",
    };
    for (const char* row : rows) {
        const auto manifest = write_manifest(
            std::string("VERSION=9.0.0.1\n") + row + file_url(payload) +
            "\t0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\t1\n");
        auto transaction = transaction_for(manifest);
        const auto message = expect_throw_update(transaction);
        EXPECT_NE(message.find("unsafe patch target path"), std::string::npos) << row << message;
        EXPECT_EQ(snapshot(game_), before) << row;
        EXPECT_FALSE(fs::exists(game_ / "_backup")) << row;
    }
}

TEST_F(PatchTransactionTest, RejectsUserDataDeletes) {
    write_bytes(game_ / "logs" / "client.log", "keep-this-log");
    write_bytes(game_ / "screenshots" / "last.tga", "shot");
    write_bytes(game_ / "Moxian" / "settings.json", "{}");
    write_bytes(game_ / "MHVerInfo.ver", "VERSION=1.0.0.0\n");
    const auto before = snapshot(game_);
    const char* deletes[] = {
        "logs/client.log",
        "screenshots/last.tga",
        "Moxian/settings.json",
        "_backup/old.dat",
        ".update-journal",
    };
    for (const char* path : deletes) {
        const auto manifest = write_manifest(
            std::string("VERSION=9.0.0.1\nDELETE\t") + path + "\n");
        auto transaction = transaction_for(manifest);
        const auto message = expect_throw_update(transaction);
        EXPECT_NE(message.find("manifest may not delete user data"), std::string::npos)
            << path << " " << message;
        EXPECT_EQ(snapshot(game_), before) << path;
        EXPECT_FALSE(fs::exists(game_ / "_backup")) << path;
    }
}

TEST_F(PatchTransactionTest, ManifestDigestMismatchLeavesTreeUntouched) {
    write_bytes(game_ / "bin" / "client.dat", "old-client-bytes");
    write_bytes(game_ / "MHVerInfo.ver", "VERSION=1.0.0.0\n");
    const auto before = snapshot(game_);
    const auto payload = write_payload("client.dat", "new-client-bytes");
    const auto manifest = write_manifest(
        "VERSION=4.0.0.1\n" + file_row("bin/client.dat", payload) + "\n");
    mxh::patch::PatchTransaction transaction;
    transaction.setGameDir(game_.string());
    transaction.setManifest(file_url(manifest));
    transaction.setManifestSha256(
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    const auto message = expect_throw_update(transaction);
    EXPECT_NE(message.find("manifest SHA-256 verification failed"), std::string::npos) << message;
    EXPECT_EQ(snapshot(game_), before);
    EXPECT_FALSE(fs::exists(game_ / "_backup"));
    EXPECT_FALSE(fs::exists(game_ / ".update-journal"));
}

TEST_F(PatchTransactionTest, JournalResumeThenRollbackRestoresPreUpdateBytes) {
    const std::string old_a = "old-a-bytes";
    const std::string old_b = "old-b-bytes";
    const std::string new_a = "new-a-bytes";
    const std::string new_b = "new-b-bytes";
    write_bytes(game_ / "bin" / "a.dat", old_a);
    write_bytes(game_ / "bin" / "b.dat", old_b);
    write_bytes(game_ / "logs" / "client.log", "keep-this-log");
    write_bytes(game_ / "MHVerInfo.ver", "VERSION=1.0.0.0\nCHANNEL=test\n");
    const auto before = snapshot(game_);

    const auto backup = game_ / "_backup";
    write_bytes(backup / "bin" / "a.dat", old_a);
    write_bytes(backup / "bin" / "b.dat", old_b);
    write_bytes(backup / "MHVerInfo.ver", read_bytes(game_ / "MHVerInfo.ver"));
    write_bytes(backup / ".created", "");
    write_bytes(game_ / "bin" / "a.dat", new_a);
    write_bytes(game_ / ".update-journal", "VERSION=2.0.0.0\nDONE\tbin/a.dat\n");

    const auto source_a = write_payload("a.dat", new_a);
    const auto source_b = write_payload("b.dat", new_b);
    const auto manifest = write_manifest(
        "VERSION=2.0.0.0\n" +
        file_row("bin/a.dat", source_a) + "\n" +
        file_row("bin/b.dat", source_b) + "\n");
    fs::remove(source_a);

    auto transaction = transaction_for(manifest);
    ASSERT_TRUE(transaction.update());
    EXPECT_EQ(read_bytes(game_ / "bin" / "a.dat"), new_a);
    EXPECT_EQ(read_bytes(game_ / "bin" / "b.dat"), new_b);
    EXPECT_EQ(read_bytes(game_ / "logs" / "client.log"), "keep-this-log");
    EXPECT_NE(read_bytes(game_ / "MHVerInfo.ver").find("VERSION=2.0.0.0"), std::string::npos);
    EXPECT_EQ(read_bytes(backup / "bin" / "a.dat"), old_a);

    ASSERT_TRUE(transaction.rollback());
    EXPECT_EQ(snapshot(game_), before);
}
