// Item protocol hash. Synthetic cases feed the CMake hash script temp lists
// only. The real-header case reads the host tool's canonical list and does
// not assert numeric wire values.
#include "mxh/proto/protocol.hpp"
#include "mxh/proto/protocol_hash.hpp"
#include "protocol_hash_test_paths.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <stdio.h>
#define MXH_POPEN _popen
#define MXH_PCLOSE _pclose
#else
#include <stdio.h>
#include <sys/wait.h>
#define MXH_POPEN popen
#define MXH_PCLOSE pclose
#endif

namespace {

namespace fs = std::filesystem;

struct TempDir {
    fs::path path;
    explicit TempDir(const char* name) {
        std::random_device rd;
        path = fs::temp_directory_path() /
               (std::string("mxh-ph-") + name + "-" + std::to_string(rd()));
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
};

void WriteBytes(const fs::path& path, const std::string& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(static_cast<bool>(out)) << path.string();
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(static_cast<bool>(out)) << path.string();
}

std::string QuoteArg(const std::string& text) {
    std::string out = "\"";
    for (char ch : text) {
        if (ch == '"') {
            out += "\\\"";
        } else {
            out += ch;
        }
    }
    out += '"';
    return out;
}

struct CommandResult {
    int code = -1;
    std::string text;
};

CommandResult RunCommand(const std::vector<std::string>& args, bool redirect_stderr = false) {
    std::ostringstream command;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i != 0) {
            command << ' ';
        }
        command << QuoteArg(args[i]);
    }
    if (redirect_stderr) {
        command << " 2>&1";
    }
    CommandResult result;
    FILE* pipe = MXH_POPEN(command.str().c_str(), "r");
    if (pipe == nullptr) {
        result.text = "failed to start command";
        return result;
    }
    char buffer[512];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result.text += buffer;
    }
    const int closed = MXH_PCLOSE(pipe);
#if defined(_WIN32)
    result.code = closed;
#else
    result.code = WEXITSTATUS(closed);
    if (!WIFEXITED(closed)) {
        result.code = closed;
    }
#endif
    return result;
}

std::string Trim(std::string text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
        text.pop_back();
    }
    return text;
}

std::string HashList(const fs::path& list_path, const fs::path& hash_path) {
    const CommandResult ran = RunCommand({
        kMxhCmake,
        "-D",
        "MXH_CANON_INPUT=" + list_path.string(),
        "-D",
        "MXH_CANON_HASH_OUT=" + hash_path.string(),
        "-P",
        kMxhHashScript,
    }, true);
    EXPECT_EQ(ran.code, 0) << ran.text;
    std::ifstream in(hash_path, std::ios::binary);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return Trim(buffer.str());
}

bool IsHex64(const std::string& text) {
    if (text.size() != 64) {
        return false;
    }
    for (char ch : text) {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) {
            return false;
        }
    }
    return true;
}

}  // namespace

TEST(ProtocolHash, GeneratedHashIs64LowercaseHex) {
    EXPECT_TRUE(IsHex64(mxh::proto::kItemProtocolSha256));
    const TempDir dir("hex");
    const std::string hashed = HashList(kMxhCanonList, dir.path / "hash.txt");
    EXPECT_EQ(hashed, mxh::proto::kItemProtocolSha256);
}

TEST(ProtocolHash, LfAndCrlfNormalizeToSameHash) {
    const TempDir dir("lf");
    const fs::path lf = dir.path / "lf.txt";
    const fs::path crlf = dir.path / "crlf.txt";
    WriteBytes(lf, "PickupSyn=251\nBuySyn=19\n");
    WriteBytes(crlf, "PickupSyn=251\r\nBuySyn=19\r\n");
    const std::string lf_hash = HashList(lf, dir.path / "lf.hash");
    const std::string crlf_hash = HashList(crlf, dir.path / "crlf.hash");
    EXPECT_TRUE(IsHex64(lf_hash));
    EXPECT_EQ(lf_hash, crlf_hash);
}

TEST(ProtocolHash, ChangingOneEnumValueChangesHash) {
    const TempDir dir("value");
    const fs::path before = dir.path / "before.txt";
    const fs::path after = dir.path / "after.txt";
    WriteBytes(before, "BuySyn=19\nPickupSyn=251\n");
    WriteBytes(after, "BuySyn=19\nPickupSyn=254\n");
    const std::string before_hash = HashList(before, dir.path / "before.hash");
    const std::string after_hash = HashList(after, dir.path / "after.hash");
    EXPECT_TRUE(IsHex64(before_hash));
    EXPECT_TRUE(IsHex64(after_hash));
    EXPECT_NE(before_hash, after_hash);
}

TEST(ProtocolHash, CommentOnlyChangeDoesNotChangeHash) {
    const TempDir dir("comment");
    const fs::path plain = dir.path / "plain.txt";
    const fs::path commented = dir.path / "commented.txt";
    WriteBytes(plain, "PickupSyn=251\nBuySyn=19\n");
    WriteBytes(commented,
               "// comment only\n"
               "PickupSyn=251   \n"
               "\n"
               "# also ignored\n"
               "BuySyn=19\n");
    const std::string plain_hash = HashList(plain, dir.path / "plain.hash");
    const std::string commented_hash = HashList(commented, dir.path / "commented.hash");
    EXPECT_EQ(plain_hash, commented_hash);
}

TEST(ProtocolHash, RealHeaderCountMatchesAndNamesPickupAndBuy) {
    std::ifstream in(kMxhCanonList, std::ios::binary);
    ASSERT_TRUE(static_cast<bool>(in)) << kMxhCanonList;
    std::vector<std::string> names;
    bool saw_pickup = false;
    bool saw_buy = false;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.rfind("//", 0) == 0 || line[0] == '#') {
            continue;
        }
        const auto eq = line.find('=');
        ASSERT_NE(eq, std::string::npos) << line;
        const std::string name = line.substr(0, eq);
        names.push_back(name);
        if (name == "PickupSyn") {
            saw_pickup = true;
        }
        if (name == "BuySyn") {
            saw_buy = true;
        }
    }
    EXPECT_EQ(names.size(), mxh::proto::kItemProtocolCount);
    EXPECT_TRUE(saw_pickup);
    EXPECT_TRUE(saw_buy);
    std::cout << "ITEM_PROTOCOL_CANONICAL_1 " << (names.empty() ? "" : names[0]) << "\n";
    if (names.size() > 1) {
        std::cout << "ITEM_PROTOCOL_CANONICAL_2 " << names[1] << "\n";
    }
    if (names.size() > 2) {
        std::cout << "ITEM_PROTOCOL_CANONICAL_3 " << names[2] << "\n";
    }
    std::cout << "ITEM_PROTOCOL_SHA256 " << mxh::proto::kItemProtocolSha256 << "\n";
}

TEST(ProtocolHash, ScriptMatch) {
    const TempDir dir("match");
    const std::string hash(64, 'a');
    const fs::path map = dir.path / "map" / "protocol.manifest";
    const fs::path client = dir.path / "client" / "protocol.manifest";
    const std::string body = "item_protocol_sha256=" + hash + "\n";
    WriteBytes(map, body);
    WriteBytes(client, body);
    const CommandResult ran = RunCommand({kMxhPython, kMxhGatePy, "--map", map.string(), "--client", client.string()});
    EXPECT_EQ(ran.code, 0) << ran.text;
    EXPECT_EQ(Trim(ran.text), "PROTOCOL_MATCH item=" + hash);
    std::cout << ran.text;
}

TEST(ProtocolHash, ScriptMismatch) {
    const TempDir dir("mismatch");
    const std::string map_hash(64, 'a');
    const std::string client_hash(64, 'b');
    const fs::path map = dir.path / "map" / "protocol.manifest";
    const fs::path client = dir.path / "client" / "protocol.manifest";
    WriteBytes(map, "item_protocol_sha256=" + map_hash + "\n");
    WriteBytes(client, "item_protocol_sha256=" + client_hash + "\n");
    const CommandResult ran = RunCommand({kMxhPython, kMxhGatePy, "--map", map.string(), "--client", client.string()});
    EXPECT_NE(ran.code, 0);
    EXPECT_EQ(Trim(ran.text), "PROTOCOL_MISMATCH item=" + map_hash + "/" + client_hash);
    std::cout << ran.text;
}

TEST(ProtocolHash, ScriptMissingManifest) {
    const TempDir dir("missing");
    const std::string hash(64, 'c');
    const fs::path map = dir.path / "map" / "protocol.manifest";
    const fs::path client = dir.path / "client" / "protocol.manifest";
    const fs::path absent_map = dir.path / "absent-map" / "protocol.manifest";
    const fs::path absent_client = dir.path / "absent-client" / "protocol.manifest";

    const CommandResult both = RunCommand({kMxhPython, kMxhGatePy, "--map", absent_map.string(), "--client", absent_client.string()});
    EXPECT_NE(both.code, 0);
    EXPECT_EQ(Trim(both.text), "PROTOCOL_MISMATCH item=missing/missing");
    std::cout << both.text;

    WriteBytes(client, "item_protocol_sha256=" + hash + "\n");
    const CommandResult map_missing = RunCommand({kMxhPython, kMxhGatePy, "--map", absent_map.string(), "--client", client.string()});
    EXPECT_NE(map_missing.code, 0);
    EXPECT_EQ(Trim(map_missing.text), "PROTOCOL_MISMATCH item=missing/" + hash);
    std::cout << map_missing.text;

    WriteBytes(map, "item_protocol_sha256=" + hash + "\n");
    const CommandResult client_missing = RunCommand({
        kMxhPython, kMxhGatePy, "--map", map.string(), "--client", absent_client.string()});
    EXPECT_NE(client_missing.code, 0);
    EXPECT_EQ(Trim(client_missing.text), "PROTOCOL_MISMATCH item=" + hash + "/missing");
    std::cout << client_missing.text;
}
