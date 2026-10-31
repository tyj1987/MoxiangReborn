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
//   MoxianAutoPatcher --rehearsal
//
// Update flow:
//   1. Check server for latest version
//   2. Download patch manifest (JSON)
//   3. Download patch files
//   4. Verify checksums
//   5. Apply patches (binary diff or full replacement)
//   6. Update version file

#include "patch_rehearsal.hpp"
#include "patch_transaction.hpp"

#include <filesystem>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

bool download_https(const std::string& url,
                    const std::filesystem::path& destination,
                    std::string& error) {
    // Widen bytes the same way the previous in-process call did. This is
    // not a UTF-8 conversion; the historical URLDownloadToFileW argument
    // was a wchar buffer filled from the char URL.
    const std::wstring wide(url.begin(), url.end());
    if (URLDownloadToFileW(nullptr, wide.c_str(), destination.c_str(), 0, nullptr) != S_OK) {
        error = "URLDownloadToFileW failed";
        return false;
    }
    return true;
}
#endif

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
    std::cout << "  --rehearsal       Update then roll back a synthetic temp tree" << std::endl;
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
    std::cout << "  MoxianAutoPatcher --rehearsal" << std::endl;
}

int main(int argc, char* argv[]) {
    mxh::patch::PatchTransaction patcher;
#ifdef _WIN32
    patcher.setHttpsDownloader(&download_https);
#endif

    bool checkOnly = false;
    bool update = false;
    bool rollback = false;
    bool rehearsal = false;
    bool showVersion = false;
    bool sawDir = false;
    bool sawServer = false;
    bool sawManifest = false;

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
        } else if (arg == "--rehearsal") {
            rehearsal = true;
        } else if (arg == "--version") {
            showVersion = true;
        } else if (arg == "--server" && i + 1 < argc) {
            sawServer = true;
            patcher.setServer(argv[++i]);
        } else if (arg == "--dir" && i + 1 < argc) {
            sawDir = true;
            patcher.setGameDir(argv[++i]);
        } else if (arg == "--manifest" && i + 1 < argc) {
            sawManifest = true;
            patcher.setManifest(argv[++i]);
        } else if (arg == "--manifest-sha256" && i + 1 < argc) {
            sawManifest = true;
            patcher.setManifestSha256(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }

    try {
    if (rehearsal) {
        mxh::patch::RehearsalCliArgs gate;
        gate.rehearsal = true;
        gate.has_directory = sawDir;
        gate.has_update = update;
        gate.has_rollback = rollback;
        gate.has_check = checkOnly;
        gate.has_server = sawServer;
        gate.has_manifest = sawManifest;
        gate.has_version = showVersion;
        if (const auto conflict = mxh::patch::rehearsal_cli_conflict(gate)) {
            std::cerr << "PATCH_REHEARSAL REJECT " << *conflict << "\n";
            return 2;
        }
        // The temp path is created inside the CLI and passed explicitly.
        // --dir is rejected above so this never writes at an operator path.
        return mxh::patch::run_patch_rehearsal_cli(
            std::cout, std::cerr, std::filesystem::temp_directory_path());
    }

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
