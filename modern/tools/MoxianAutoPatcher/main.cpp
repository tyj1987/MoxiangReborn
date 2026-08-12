// MoxianAutoPatcher - Modern auto-update tool for Moxian game
//
// Replaces legacy AutoPatchToolWin32 with a modern C++ application.
// Supports:
//   - Checking for updates via HTTPS
//   - Downloading patches with progress
//   - Applying binary diffs (bsdiff/bspatch)
//   - Verifying file integrity (SHA-256)
//   - Rollback on failure
//
// Usage:
//   MoxianAutoPatcher [--check] [--update] [--rollback]
//   MoxianAutoPatcher --server <url> [--version <ver>]
//
// Update flow:
//   1. Check server for latest version
//   2. Download patch manifest (JSON)
//   3. Download patch files
//   4. Verify checksums
//   5. Apply patches (binary diff or full replacement)
//   6. Update version file

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <chrono>
#include <iomanip>
#include <functional>

#include "patch_security.hpp"

namespace fs = std::filesystem;

// ============================================================================
// Version Information
// ============================================================================

struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int build = 0;

    std::string toString() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." +
               std::to_string(patch) + "." + std::to_string(build);
    }

    bool operator<(const Version& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        if (patch != other.patch) return patch < other.patch;
        return build < other.build;
    }

    bool operator==(const Version& other) const {
        return major == other.major && minor == other.minor &&
               patch == other.patch && build == other.build;
    }

    bool operator!=(const Version& other) const {
        return !(*this == other);
    }

    static Version fromString(const std::string& str) {
        Version v;
        std::regex pattern(R"((\d+)\.(\d+)\.(\d+)\.(\d+))");
        std::smatch matches;
        if (std::regex_match(str, matches, pattern)) {
            v.major = std::stoi(matches[1]);
            v.minor = std::stoi(matches[2]);
            v.patch = std::stoi(matches[3]);
            v.build = std::stoi(matches[4]);
        }
        return v;
    }
};

// ============================================================================
// Patch Manifest
// ============================================================================

struct PatchFile {
    std::string path;           // Relative path
    std::string url;            // Download URL
    std::string sha256;         // Expected SHA-256
    uint64_t size = 0;          // File size
    bool isDiff = false;        // Binary diff patch
    std::string basePath;       // Base file for diff
};

struct PatchManifest {
    Version version;
    std::string description;
    std::vector<PatchFile> files;
    std::vector<std::string> deleteFiles;
};

// ============================================================================
// Auto Patcher Core
// ============================================================================

class MoxianAutoPatcher {
public:
    MoxianAutoPatcher() = default;
    ~MoxianAutoPatcher() = default;

    // Set server URL
    void setServer(const std::string& url) {
        serverUrl_ = url;
    }

    // Set game directory
    void setGameDir(const std::string& dir) {
        gameDir_ = dir;
    }

    void setManifest(const std::string& path) { manifestPath_ = path; }
    void setManifestSha256(const std::string& value) { manifestSha256_ = value; }

    // Check for updates
    bool checkForUpdates() {
        std::cout << "Checking for updates..." << std::endl;
        std::cout << "  Server: " << serverUrl_ << std::endl;

        // Read current version
        Version current = getCurrentVersion();
        std::cout << "  Current version: " << current.toString() << std::endl;

        // Fetch latest version from server
        // In real implementation, this would make HTTP request
        // For now, simulate with local file
        PatchManifest manifest = getManifest();
        Version latest = manifest.version;
        std::cout << "  Latest version: " << latest.toString() << std::endl;

        if (latest < current || latest == current) {
            std::cout << "No updates available." << std::endl;
            return false;
        }

        std::cout << "Update available: " << current.toString() << " -> " << latest.toString() << std::endl;
        return true;
    }

    // Download and apply updates
    bool update() {
        std::cout << "Starting update..." << std::endl;

        // Get manifest
        PatchManifest manifest = getManifest();

        // Check if update is needed
        Version current = getCurrentVersion();
        if (manifest.version < current || manifest.version == current) {
            std::cout << "Already up to date." << std::endl;
            return true;
        }

        std::cout << "Updating from " << current.toString() << " to " << manifest.version.toString() << std::endl;
        std::cout << "  Files to update: " << manifest.files.size() << std::endl;
        std::cout << "  Files to delete: " << manifest.deleteFiles.size() << std::endl;

        // Create backup
        if (!createBackup(manifest)) {
            std::cerr << "Failed to create backup" << std::endl;
            return false;
        }

        // Download and apply patches
        size_t completed = 0;
        size_t total = manifest.files.size();

        for (const auto& file : manifest.files) {
            std::cout << "  [" << (completed + 1) << "/" << total << "] " << file.path;

            if (!downloadAndApplyPatch(file)) {
                std::cerr << " - FAILED" << std::endl;
                rollback();
                return false;
            }

            std::cout << " - OK" << std::endl;
            completed++;
        }

        // Delete removed files
        for (const auto& path : manifest.deleteFiles) {
            fs::path fullPath = fs::path(gameDir_) / path;
            if (fs::exists(fullPath)) {
                fs::remove(fullPath);
                std::cout << "  Deleted: " << path << std::endl;
            }
        }

        // Update version file
        if (!updateVersionFile(manifest.version)) {
            std::cerr << "Failed to update version file" << std::endl;
            rollback();
            return false;
        }

        std::cout << "Update completed successfully!" << std::endl;
        return true;
    }

    // Rollback to previous version
    bool rollback() {
        std::cout << "Rolling back..." << std::endl;

        fs::path backupDir = fs::path(gameDir_) / "_backup";
        if (!fs::exists(backupDir)) {
            std::cerr << "No backup found" << std::endl;
            return false;
        }

        {
            std::ifstream created(backupDir / ".created");
            std::string created_path;
            while (std::getline(created, created_path)) {
                if (mxh::patch::is_safe_relative_path(created_path)) {
                    std::error_code error;
                    fs::remove(fs::path(gameDir_) / created_path, error);
                }
            }
        }

        // Restore files from backup
        for (const auto& entry : fs::recursive_directory_iterator(backupDir)) {
            if (entry.is_regular_file()) {
                fs::path relative = fs::relative(entry.path(), backupDir);
                if (relative == ".created") continue;
                fs::path target = fs::path(gameDir_) / relative;

                fs::create_directories(target.parent_path());
                fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing);
            }
        }

        // Remove backup
        fs::remove_all(backupDir);

        std::cout << "Rollback completed" << std::endl;
        return true;
    }

    // Print current version
    void printVersion() const {
        Version v = getCurrentVersion();
        std::cout << "Current version: " << v.toString() << std::endl;
    }

private:
    // Read current version from MHVerInfo.ver
    Version getCurrentVersion() const {
        fs::path verFile = fs::path(gameDir_) / "MHVerInfo.ver";
        if (!fs::exists(verFile)) {
            return Version{0, 0, 0, 0};
        }

        std::ifstream file(verFile);
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("VERSION=") == 0) {
                return Version::fromString(line.substr(8));
            }
        }

        return Version{0, 0, 0, 0};
    }

    PatchManifest getManifest() const {
        if (manifestPath_.empty()) throw std::runtime_error("--manifest is required (refusing simulated update)");
        if (manifestSha256_.size() != 64 ||
            mxh::patch::lower_hex(mxh::patch::sha256_file(manifestPath_)) != mxh::patch::lower_hex(manifestSha256_))
            throw std::runtime_error("manifest SHA-256 verification failed");
        std::ifstream input(manifestPath_);
        if (!input) throw std::runtime_error("cannot read manifest");
        PatchManifest manifest;
        std::string line;
        bool has_version = false;
        while (std::getline(input, line)) {
            if (line.empty() || line[0] == '#') continue;
            if (line.rfind("VERSION=", 0) == 0) {
                manifest.version = Version::fromString(line.substr(8));
                has_version = true;
                continue;
            }
            std::vector<std::string> fields;
            std::istringstream row(line);
            std::string field;
            while (std::getline(row, field, '\t')) fields.push_back(field);
            if (fields.size() == 5 && fields[0] == "FILE") {
                if (!mxh::patch::is_safe_relative_path(fields[1])) throw std::runtime_error("unsafe patch target path");
                PatchFile file;
                file.path = fields[1]; file.url = fields[2]; file.sha256 = fields[3];
                file.size = std::stoull(fields[4]);
                if (file.sha256.size() != 64) throw std::runtime_error("invalid SHA-256 in manifest");
                manifest.files.push_back(std::move(file));
            } else if (fields.size() == 2 && fields[0] == "DELETE") {
                if (!mxh::patch::is_safe_relative_path(fields[1])) throw std::runtime_error("unsafe delete path");
                manifest.deleteFiles.push_back(fields[1]);
            } else {
                throw std::runtime_error("invalid manifest record");
            }
        }
        if (!has_version) throw std::runtime_error("manifest has no VERSION");
        return manifest;
    }

    // Create backup of current files
    bool createBackup(const PatchManifest& manifest) const {
        fs::path backupDir = fs::path(gameDir_) / "_backup";

        // Remove old backup if exists
        if (fs::exists(backupDir)) {
            fs::remove_all(backupDir);
        }

        // Create new backup
        fs::create_directories(backupDir);

        std::ofstream created(backupDir / ".created", std::ios::binary);
        std::vector<std::string> paths = manifest.deleteFiles;
        for (const auto& file : manifest.files) paths.push_back(file.path);
        paths.push_back("MHVerInfo.ver");
        for (const auto& relative : paths) {
            const fs::path source = fs::path(gameDir_) / relative;
            if (fs::is_regular_file(source)) {
                const fs::path destination = backupDir / relative;
                fs::create_directories(destination.parent_path());
                fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
            } else {
                created << relative << '\n';
            }
        }
        std::cout << "Backup created" << std::endl;
        return true;
    }

    // Download and apply a single patch
    bool downloadAndApplyPatch(const PatchFile& file) {
        fs::path targetPath = fs::path(gameDir_) / file.path;
        fs::path sourcePath = file.url.rfind("file://", 0) == 0
            ? fs::path(file.url.substr(7)) : fs::path(file.url);
        if (!mxh::patch::verify_file(sourcePath, file.size, file.sha256)) return false;
        fs::create_directories(targetPath.parent_path());
        fs::path staged = targetPath;
        staged += ".mxh-new";
        std::error_code error;
        fs::copy_file(sourcePath, staged, fs::copy_options::overwrite_existing, error);
        if (error || !mxh::patch::verify_file(staged, file.size, file.sha256)) {
            fs::remove(staged, error);
            return false;
        }
#ifdef _WIN32
        if (!MoveFileExW(staged.c_str(), targetPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            fs::remove(staged, error);
            return false;
        }
#else
        fs::rename(staged, targetPath, error);
        if (error) return false;
#endif
        return true;
    }

    // Update MHVerInfo.ver with new version
    bool updateVersionFile(const Version& version) const {
        fs::path verFile = fs::path(gameDir_) / "MHVerInfo.ver";

        std::vector<std::string> lines;

        // Read existing file
        if (fs::exists(verFile)) {
            std::ifstream in(verFile);
            std::string line;
            while (std::getline(in, line)) {
                if (line.find("VERSION=") == 0) {
                    lines.push_back("VERSION=" + version.toString());
                } else {
                    lines.push_back(line);
                }
            }
        } else {
            lines.push_back("VERSION=" + version.toString());
        }

        // Write updated file
        std::ofstream out(verFile);
        if (!out.is_open()) {
            return false;
        }

        for (const auto& line : lines) {
            out << line << std::endl;
        }

        return true;
    }

    std::string serverUrl_;
    std::string gameDir_ = ".";
    std::string manifestPath_;
    std::string manifestSha256_;
};

// ============================================================================
// Command Line Interface
// ============================================================================

void printUsage() {
    std::cout << "MoxianAutoPatcher - Modern auto-update tool for Moxian game" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  MoxianAutoPatcher [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --check           Check for updates only" << std::endl;
    std::cout << "  --update          Download and apply updates" << std::endl;
    std::cout << "  --rollback        Rollback to previous version" << std::endl;
    std::cout << "  --version         Show current version" << std::endl;
    std::cout << "  --server <url>    Set update server URL" << std::endl;
    std::cout << "  --manifest <path> Verified local manifest (required for check/update)" << std::endl;
    std::cout << "  --manifest-sha256 <hex> Trusted manifest digest (required)" << std::endl;
    std::cout << "  --dir <path>      Set game directory" << std::endl;
    std::cout << "  --help, -h        Show this help" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  MoxianAutoPatcher --check --server https://update.moxian.com" << std::endl;
    std::cout << "  MoxianAutoPatcher --update --dir C:\\Moxian" << std::endl;
    std::cout << "  MoxianAutoPatcher --rollback" << std::endl;
}

int main(int argc, char* argv[]) {
    MoxianAutoPatcher patcher;

    bool checkOnly = false;
    bool update = false;
    bool rollback = false;
    bool showVersion = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--check") {
            checkOnly = true;
        } else if (arg == "--update") {
            update = true;
        } else if (arg == "--rollback") {
            rollback = true;
        } else if (arg == "--version") {
            showVersion = true;
        } else if (arg == "--server" && i + 1 < argc) {
            patcher.setServer(argv[++i]);
        } else if (arg == "--dir" && i + 1 < argc) {
            patcher.setGameDir(argv[++i]);
        } else if (arg == "--manifest" && i + 1 < argc) {
            patcher.setManifest(argv[++i]);
        } else if (arg == "--manifest-sha256" && i + 1 < argc) {
            patcher.setManifestSha256(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }

    try {
    if (showVersion) {
        patcher.printVersion();
        return 0;
    }

    if (rollback) {
        return patcher.rollback() ? 0 : 1;
    }

    if (checkOnly) {
        return patcher.checkForUpdates() ? 0 : 1;
    }

    if (update) {
        return patcher.update() ? 0 : 1;
    }

    // Default: check and update
    if (patcher.checkForUpdates()) {
        return patcher.update() ? 0 : 1;
    }

    return 0;
    } catch (const std::exception& error) {
        std::cerr << "FATAL: " << error.what() << std::endl;
        return 2;
    }
}
