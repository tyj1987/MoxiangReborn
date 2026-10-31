#pragma once

#include <filesystem>
#include <set>
#include <string>

namespace mxh::patch {

struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int build = 0;

    std::string toString() const;
    bool operator<(const Version& other) const;
    bool operator==(const Version& other) const;
    bool operator!=(const Version& other) const;
    static Version fromString(const std::string& str);
};

// Writes `destination` from an https URL. The CLI supplies the Windows
// URLDownloadToFileW implementation. A null downloader keeps the historical
// "HTTPS ... requires Windows" failure on hosts that cannot fetch.
using HttpsDownloader = bool (*)(const std::string& url,
                                 const std::filesystem::path& destination,
                                 std::string& error);

// Optional. Invoked after a file is applied and journaled. A null
// observer leaves update() unchanged. The rehearsal test uses it to
// show a later failure happens after an earlier file was written;
// update() then rolls that write back itself.
using ApplyObserver = void (*)(const std::string& relative_path, void* user);

// Update transaction: manifest parse, backup, apply, journal resume,
// post-apply re-verify, and rollback. file:// sources are local files.
// https:// sources go through HttpsDownloader.
class PatchTransaction {
public:
    void setServer(const std::string& url);
    void setGameDir(const std::string& dir);
    void setManifest(const std::string& path);
    void setManifestSha256(const std::string& value);
    void setHttpsDownloader(HttpsDownloader downloader) noexcept;
    void setApplyObserver(ApplyObserver observer, void* user) noexcept;

    bool checkForUpdates();
    bool update();
    bool rollback();
    void printVersion() const;

private:
    struct PatchFile;
    struct PatchManifest;

    std::filesystem::path journalPath() const;
    bool loadJournal(const Version& version, std::set<std::string>& completed) const;
    bool beginJournal(const Version& version) const;
    bool appendJournal(const std::string& path) const;
    Version getCurrentVersion() const;
    PatchManifest getManifest() const;
    bool createBackup(const PatchManifest& manifest) const;
    bool downloadAndApplyPatch(const PatchFile& file);
    bool updateVersionFile(const Version& version) const;

    std::string serverUrl_;
    std::string gameDir_ = ".";
    std::string manifestPath_;
    std::string manifestSha256_;
    HttpsDownloader httpsDownloader_ = nullptr;
    ApplyObserver applyObserver_ = nullptr;
    void* applyObserverUser_ = nullptr;
};

}  // namespace mxh::patch
