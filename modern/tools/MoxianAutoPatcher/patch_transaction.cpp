#include "patch_transaction.hpp"

#include "patch_security.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace mxh::patch {
namespace fs = std::filesystem;

std::string Version::toString() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." +
           std::to_string(patch) + "." + std::to_string(build);
}

bool Version::operator<(const Version& other) const {
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
    if (patch != other.patch) return patch < other.patch;
    return build < other.build;
}

bool Version::operator==(const Version& other) const {
    return major == other.major && minor == other.minor &&
           patch == other.patch && build == other.build;
}

bool Version::operator!=(const Version& other) const {
    return !(*this == other);
}

Version Version::fromString(const std::string& str) {
    Version v;
    std::regex pattern(R"((\d+)\.(\d+)\.(\d+)\.(\d+))");
    std::smatch matches;
    if (std::regex_match(str, matches, pattern)) {
        try {
            v.major = std::stoi(matches[1].str());
            v.minor = std::stoi(matches[2].str());
            v.patch = std::stoi(matches[3].str());
            v.build = std::stoi(matches[4].str());
        } catch (const std::exception&) {
            // A remote manifest is untrusted input.  Treat overflow or
            // malformed numeric components as an invalid zero version;
            // callers can then reject it through their normal version
            // policy without terminating the updater process.
            return Version{};
        }
    }
    return v;
}

struct PatchTransaction::PatchFile {
    std::string path;
    std::string url;
    std::string sha256;
    std::uint64_t size = 0;
    bool isDiff = false;
    std::string basePath;
};

struct PatchTransaction::PatchManifest {
    Version version;
    std::string description;
    std::vector<PatchFile> files;
    std::vector<std::string> deleteFiles;
};

void PatchTransaction::setServer(const std::string& url) { serverUrl_ = url; }
void PatchTransaction::setGameDir(const std::string& dir) { gameDir_ = dir; }
void PatchTransaction::setManifest(const std::string& path) { manifestPath_ = path; }
void PatchTransaction::setManifestSha256(const std::string& value) { manifestSha256_ = value; }
void PatchTransaction::setHttpsDownloader(HttpsDownloader downloader) noexcept {
    httpsDownloader_ = downloader;
}
void PatchTransaction::setApplyObserver(ApplyObserver observer, void* user) noexcept {
    applyObserver_ = observer;
    applyObserverUser_ = user;
}

bool PatchTransaction::checkForUpdates() {
    std::cout << "Checking for updates..." << std::endl;
    std::cout << "  Server: " << serverUrl_ << std::endl;

    Version current = getCurrentVersion();
    std::cout << "  Current version: " << current.toString() << std::endl;

    PatchManifest manifest = getManifest();
    Version latest = manifest.version;
    std::cout << "  Latest version: " << latest.toString() << std::endl;

    if (latest < current || latest == current) {
        std::cout << "No updates available." << std::endl;
        return true;
    }

    std::cout << "Update available: " << current.toString() << " -> " << latest.toString() << std::endl;
    return true;
}

bool PatchTransaction::update() {
    std::cout << "Starting update..." << std::endl;

    PatchManifest manifest = getManifest();

    Version current = getCurrentVersion();
    if (manifest.version < current || manifest.version == current) {
        std::cout << "Already up to date." << std::endl;
        return true;
    }

    std::cout << "Updating from " << current.toString() << " to " << manifest.version.toString() << std::endl;
    std::cout << "  Files to update: " << manifest.files.size() << std::endl;
    std::cout << "  Files to delete: " << manifest.deleteFiles.size() << std::endl;

    std::uintmax_t required_bytes = 64u * 1024u * 1024u; // journal/staging headroom
    for (const auto& file : manifest.files) {
        required_bytes += file.size;
        const auto existing = fs::path(gameDir_) / file.path;
        std::error_code size_error;
        if (fs::is_regular_file(existing, size_error)) {
            required_bytes += fs::file_size(existing, size_error);
        }
    }
    std::error_code space_error;
    const auto available = fs::space(fs::path(gameDir_), space_error).available;
    if (space_error || available < required_bytes) {
        std::cerr << "Insufficient disk space for update (required="
                  << required_bytes << ", available=" << available << ")" << std::endl;
        return false;
    }

    std::set<std::string> completed_paths;
    const bool resuming = loadJournal(manifest.version, completed_paths) &&
                           fs::is_directory(fs::path(gameDir_) / "_backup");
    if (!resuming && !createBackup(manifest)) {
        std::cerr << "Failed to create backup" << std::endl;
        return false;
    }
    if (!resuming && !beginJournal(manifest.version)) {
        std::cerr << "Failed to create update journal" << std::endl;
        return false;
    }

    size_t completed_count = 0;
    size_t total = manifest.files.size();

    for (const auto& file : manifest.files) {
        std::cout << "  [" << (completed_count + 1) << "/" << total << "] " << file.path;

        if (completed_paths.count(file.path) != 0 &&
            mxh::patch::verify_file(fs::path(gameDir_) / file.path,
                                    file.size, file.sha256)) {
            std::cout << " - RESUME" << std::endl;
            ++completed_count;
            continue;
        }

        if (!downloadAndApplyPatch(file)) {
            std::cerr << " - FAILED" << std::endl;
            rollback();
            return false;
        }

        if (!appendJournal(file.path)) {
            std::cerr << " - JOURNAL FAILED" << std::endl;
            rollback();
            return false;
        }
        if (applyObserver_ != nullptr) {
            applyObserver_(file.path, applyObserverUser_);
        }
        std::cout << " - OK" << std::endl;
        ++completed_count;
    }

    for (const auto& path : manifest.deleteFiles) {
        fs::path fullPath = fs::path(gameDir_) / path;
        if (fs::exists(fullPath)) {
            std::error_code delete_error;
            if (!fs::remove(fullPath, delete_error) || delete_error) {
                std::cerr << "  Failed to delete " << path << ": "
                          << (delete_error ? delete_error.message() : "file was not removed")
                          << std::endl;
                if (!rollback()) {
                    std::cerr << "  CRITICAL: update rollback also failed" << std::endl;
                }
                return false;
            }
            std::cout << "  Deleted: " << path << std::endl;
        }
    }

    // Re-verify the complete post-update file set before publishing the
    // version marker.  Individual downloads are checked above, but a
    // final pass catches an external file replacement or a failed delete
    // while the update transaction is still reversible.
    for (const auto& file : manifest.files) {
        const auto target = fs::path(gameDir_) / file.path;
        if (!mxh::patch::verify_file(target, file.size, file.sha256)) {
            std::cerr << "Post-update verification failed: " << file.path << std::endl;
            if (!rollback()) {
                std::cerr << "CRITICAL: post-update rollback failed" << std::endl;
            }
            return false;
        }
    }
    for (const auto& path : manifest.deleteFiles) {
        std::error_code exists_error;
        if (fs::exists(fs::path(gameDir_) / path, exists_error) || exists_error) {
            std::cerr << "Post-update delete verification failed: " << path << std::endl;
            if (!rollback()) {
                std::cerr << "CRITICAL: post-update rollback failed" << std::endl;
            }
            return false;
        }
    }

    if (!updateVersionFile(manifest.version)) {
        std::cerr << "Failed to update version file" << std::endl;
        rollback();
        return false;
    }

    std::error_code journal_error;
    fs::remove(journalPath(), journal_error);
    if (journal_error) {
        std::cerr << "Failed to remove update journal: " << journal_error.message() << std::endl;
        return false;
    }

    std::cout << "Update completed successfully!" << std::endl;
    return true;
}

bool PatchTransaction::rollback() {
    std::cout << "Rolling back..." << std::endl;

    fs::path backupDir = fs::path(gameDir_) / "_backup";
    if (!fs::exists(backupDir)) {
        std::cerr << "No backup found" << std::endl;
        return false;
    }

    {
        std::ifstream created(backupDir / ".created");
        if (!created) {
            std::cerr << "Backup metadata is unreadable" << std::endl;
            return false;
        }
        std::string created_path;
        while (std::getline(created, created_path)) {
            if (mxh::patch::is_safe_relative_path(created_path)) {
                std::error_code error;
                fs::remove(fs::path(gameDir_) / created_path, error);
                if (error) {
                    std::cerr << "Failed to remove created file " << created_path
                              << ": " << error.message() << std::endl;
                    return false;
                }
            }
        }
        if (!created.eof()) {
            std::cerr << "Backup metadata read failed" << std::endl;
            return false;
        }
    }

    std::error_code iterator_error;
    fs::recursive_directory_iterator entries(backupDir, iterator_error);
    if (iterator_error) {
        std::cerr << "Cannot enumerate backup: " << iterator_error.message() << std::endl;
        return false;
    }
    for (const auto& entry : entries) {
        if (entry.is_regular_file()) {
            fs::path relative = fs::relative(entry.path(), backupDir);
            if (relative == ".created") continue;
            fs::path target = fs::path(gameDir_) / relative;

            std::error_code restore_error;
            fs::create_directories(target.parent_path(), restore_error);
            if (restore_error ||
                !fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing,
                               restore_error) || restore_error) {
                std::cerr << "Failed to restore " << relative << ": "
                          << (restore_error ? restore_error.message() : "copy failed")
                          << std::endl;
                return false;
            }
        }
    }

    std::error_code cleanup_error;
    fs::remove_all(backupDir, cleanup_error);
    if (cleanup_error) {
        std::cerr << "Failed to remove backup: " << cleanup_error.message() << std::endl;
        return false;
    }
    std::error_code journal_error;
    fs::remove(journalPath(), journal_error);
    if (journal_error) {
        std::cerr << "Failed to remove update journal: " << journal_error.message() << std::endl;
        return false;
    }

    std::cout << "Rollback completed" << std::endl;
    return true;
}

void PatchTransaction::printVersion() const {
    Version v = getCurrentVersion();
    std::cout << "Current version: " << v.toString() << std::endl;
}

fs::path PatchTransaction::journalPath() const { return fs::path(gameDir_) / ".update-journal"; }

bool PatchTransaction::loadJournal(const Version& version, std::set<std::string>& completed) const {
    std::ifstream input(journalPath(), std::ios::binary);
    if (!input) return false;
    std::string line;
    bool version_ok = false;
    while (std::getline(input, line)) {
        if (line.rfind("VERSION=", 0) == 0) {
            version_ok = Version::fromString(line.substr(8)) == version;
        } else if (line.rfind("DONE\t", 0) == 0 && version_ok) {
            const auto path = line.substr(5);
            if (mxh::patch::is_safe_relative_path(path) &&
                !mxh::patch::is_user_data_path(path)) completed.insert(path);
        }
    }
    return version_ok;
}

bool PatchTransaction::beginJournal(const Version& version) const {
    std::ofstream output(journalPath(), std::ios::binary | std::ios::trunc);
    if (!output) return false;
    output << "VERSION=" << version.toString() << "\n";
    output.flush();
    return static_cast<bool>(output);
}

bool PatchTransaction::appendJournal(const std::string& path) const {
    if (!mxh::patch::is_safe_relative_path(path) ||
        mxh::patch::is_user_data_path(path)) return false;
    std::ofstream output(journalPath(), std::ios::binary | std::ios::app);
    if (!output) return false;
    output << "DONE\t" << path << "\n";
    output.flush();
    return static_cast<bool>(output);
}

Version PatchTransaction::getCurrentVersion() const {
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

PatchTransaction::PatchManifest PatchTransaction::getManifest() const {
    if (manifestPath_.empty()) throw std::runtime_error("--manifest is required (refusing simulated update)");
    fs::path manifestFile = fs::path(manifestPath_);
    bool downloaded = false;
    if (manifestPath_.rfind("https://", 0) == 0) {
        manifestFile = fs::path(gameDir_) / ".manifest-download.tmp";
        std::error_code cleanup_error;
        fs::remove(manifestFile, cleanup_error);
        if (!httpsDownloader_) {
            throw std::runtime_error("HTTPS manifest transport requires Windows");
        }
        std::string error;
        if (!httpsDownloader_(manifestPath_, manifestFile, error)) {
            fs::remove(manifestFile, cleanup_error);
            throw std::runtime_error("HTTPS manifest download failed");
        }
        downloaded = true;
    } else if (manifestPath_.find("://") != std::string::npos &&
               manifestPath_.rfind("file://", 0) != 0) {
        throw std::runtime_error("manifest URL must use HTTPS or file://");
    } else if (manifestPath_.rfind("file://", 0) == 0) {
        manifestFile = fs::path(manifestPath_.substr(7));
    }
    struct Cleanup {
        fs::path path;
        bool enabled;
        ~Cleanup() { if (enabled) { std::error_code ignored; fs::remove(path, ignored); } }
    } cleanup{manifestFile, downloaded};
    if (manifestSha256_.size() != 64 ||
        mxh::patch::lower_hex(mxh::patch::sha256_file(manifestFile)) != mxh::patch::lower_hex(manifestSha256_))
        throw std::runtime_error("manifest SHA-256 verification failed");
    std::ifstream input(manifestFile);
    if (!input) throw std::runtime_error("cannot read manifest");
    PatchManifest manifest;
    std::string line;
    bool has_version = false;
    std::set<std::string> target_paths;
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
            try {
                file.size = std::stoull(fields[4]);
            } catch (...) {
                throw std::runtime_error("invalid patch file size in manifest");
            }
            if (file.sha256.size() != 64) throw std::runtime_error("invalid SHA-256 in manifest");
            if (!target_paths.insert(file.path).second)
                throw std::runtime_error("duplicate patch target in manifest");
            manifest.files.push_back(std::move(file));
        } else if (fields.size() == 2 && fields[0] == "DELETE") {
            if (!mxh::patch::is_safe_relative_path(fields[1])) throw std::runtime_error("unsafe delete path");
            if (mxh::patch::is_user_data_path(fields[1])) throw std::runtime_error("manifest may not delete user data");
            if (!target_paths.insert(fields[1]).second)
                throw std::runtime_error("duplicate patch target in manifest");
            manifest.deleteFiles.push_back(fields[1]);
        } else {
            throw std::runtime_error("invalid manifest record");
        }
    }
    if (!has_version) throw std::runtime_error("manifest has no VERSION");
    return manifest;
}

bool PatchTransaction::createBackup(const PatchManifest& manifest) const {
    fs::path backupDir = fs::path(gameDir_) / "_backup";

    if (fs::exists(backupDir)) {
        std::error_code remove_error;
        fs::remove_all(backupDir, remove_error);
        if (remove_error) return false;
    }

    std::error_code directory_error;
    fs::create_directories(backupDir, directory_error);
    if (directory_error) return false;

    std::ofstream created(backupDir / ".created", std::ios::binary);
    if (!created) return false;
    std::vector<std::string> paths = manifest.deleteFiles;
    for (const auto& file : manifest.files) paths.push_back(file.path);
    paths.push_back("MHVerInfo.ver");
    for (const auto& relative : paths) {
        const fs::path source = fs::path(gameDir_) / relative;
        if (fs::is_regular_file(source)) {
            const fs::path destination = backupDir / relative;
            std::error_code error;
            fs::create_directories(destination.parent_path(), error);
            if (error || !fs::copy_file(source, destination,
                                        fs::copy_options::overwrite_existing, error) || error) {
                created.close();
                std::error_code cleanup_error;
                fs::remove_all(backupDir, cleanup_error);
                return false;
            }
        } else {
            created << relative << '\n';
            if (!created) {
                created.close();
                std::error_code cleanup_error;
                fs::remove_all(backupDir, cleanup_error);
                return false;
            }
        }
    }
    created.flush();
    if (!created) {
        created.close();
        std::error_code cleanup_error;
        fs::remove_all(backupDir, cleanup_error);
        return false;
    }
    std::cout << "Backup created" << std::endl;
    return true;
}

bool PatchTransaction::downloadAndApplyPatch(const PatchFile& file) {
    fs::path targetPath = fs::path(gameDir_) / file.path;
    fs::path sourcePath;
    bool downloaded = false;
    if (file.url.rfind("file://", 0) == 0) {
        sourcePath = fs::path(file.url.substr(7));
    } else if (file.url.rfind("https://", 0) == 0) {
        sourcePath = targetPath;
        sourcePath += ".mxh-download";
        std::error_code cleanup_error;
        fs::remove(sourcePath, cleanup_error);
        if (!httpsDownloader_) {
            std::cerr << "HTTPS patch transport requires Windows: " << file.path << std::endl;
            return false;
        }
        std::string error;
        if (!httpsDownloader_(file.url, sourcePath, error)) {
            fs::remove(sourcePath, cleanup_error);
            std::cerr << "HTTPS patch download failed: " << file.path << std::endl;
            return false;
        }
        downloaded = true;
    } else {
        std::cerr << "patch URL must use HTTPS or file://: " << file.path << std::endl;
        return false;
    }
    if (!mxh::patch::verify_file(sourcePath, file.size, file.sha256)) {
        std::error_code cleanup_error;
        if (downloaded) fs::remove(sourcePath, cleanup_error);
        return false;
    }
    fs::create_directories(targetPath.parent_path());
    fs::path staged = targetPath;
    staged += ".mxh-new";
    std::error_code error;
    fs::copy_file(sourcePath, staged, fs::copy_options::overwrite_existing, error);
    if (error || !mxh::patch::verify_file(staged, file.size, file.sha256)) {
        fs::remove(staged, error);
        if (downloaded) fs::remove(sourcePath, error);
        return false;
    }
    if (downloaded) fs::remove(sourcePath, error);
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

bool PatchTransaction::updateVersionFile(const Version& version) const {
    fs::path verFile = fs::path(gameDir_) / "MHVerInfo.ver";

    std::vector<std::string> lines;

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

    // Write to a process-specific temporary file and publish atomically;
    // a torn version file must never make the next update choose the
    // wrong rollback path.
    fs::path temp = verFile;
    temp += ".tmp." + std::to_string(static_cast<unsigned long>(
#ifdef _WIN32
        GetCurrentProcessId()
#else
        0
#endif
    ));
    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    for (const auto& line : lines) {
        out << line << std::endl;
    }
    out.flush();
    out.close();
    if (!out) {
        std::error_code ignored;
        fs::remove(temp, ignored);
        return false;
    }
#ifdef _WIN32
    if (!MoveFileExW(temp.c_str(), verFile.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::error_code ignored;
        fs::remove(temp, ignored);
        return false;
    }
#else
    std::error_code error;
    fs::rename(temp, verFile, error);
    if (error) {
        fs::remove(temp, error);
        return false;
    }
#endif
    return true;
}

}  // namespace mxh::patch
