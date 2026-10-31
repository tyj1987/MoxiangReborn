#pragma once

// Temp directory for hand-written test bytes. remove_created() deletes only
// paths this object created. It never calls remove_all.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::test {

class SyntheticFixtureDir {
public:
    SyntheticFixtureDir() {
        std::random_device device;
        std::mt19937_64 rng(device());
        std::uniform_int_distribution<std::uint64_t> dist;
        std::ostringstream nonce;
        nonce << std::hex << dist(rng) << dist(rng);
        path_ = std::filesystem::temp_directory_path() /
                ("mxh-synthetic-fixture-" + nonce.str());
        std::error_code ec;
        if (std::filesystem::create_directory(path_, ec) && !ec) created_ = true;
    }

    ~SyntheticFixtureDir() { remove_created(); }

    SyntheticFixtureDir(const SyntheticFixtureDir&) = delete;
    SyntheticFixtureDir& operator=(const SyntheticFixtureDir&) = delete;

    bool created() const { return created_; }
    const std::filesystem::path& path() const { return path_; }

    // Creates parent directories under path() and writes `bytes`.
    // Returns an empty path when the write is refused.
    std::filesystem::path write_bytes(const std::filesystem::path& relative,
                                      std::string_view bytes) {
        if (!created_ || relative.empty() || relative.is_absolute()) return {};
        const auto file = (path_ / relative).lexically_normal();
        const auto rel = file.lexically_relative(path_);
        if (rel.empty() || *rel.begin() == "..") return {};
        std::error_code ec;
        if (std::filesystem::exists(file, ec)) return {};
        std::filesystem::path cursor = path_;
        for (const auto& part : relative.parent_path()) {
            cursor /= part;
            if (std::filesystem::is_directory(cursor, ec)) continue;
            if (!std::filesystem::create_directory(cursor, ec) || ec) return {};
            dirs_.push_back(cursor);
        }
        std::ofstream out(file, std::ios::binary);
        if (!out.is_open()) return {};
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!out.good()) return {};
        files_.push_back(file);
        return file;
    }

private:
    void remove_created() {
        if (!created_) return;
        std::error_code ec;
        for (auto it = files_.rbegin(); it != files_.rend(); ++it)
            std::filesystem::remove(*it, ec);
        for (auto it = dirs_.rbegin(); it != dirs_.rend(); ++it)
            std::filesystem::remove(*it, ec);
        std::filesystem::remove(path_, ec);
        files_.clear();
        dirs_.clear();
        created_ = false;
    }

    std::filesystem::path path_;
    std::vector<std::filesystem::path> files_;
    std::vector<std::filesystem::path> dirs_;
    bool created_ = false;
};

}  // namespace mxh::test
