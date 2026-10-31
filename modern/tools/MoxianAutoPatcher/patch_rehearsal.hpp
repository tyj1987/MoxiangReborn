#pragma once

// Synthetic update/rollback rehearsal on top of PatchTransaction.
//
// The CLI (`MoxianAutoPatcher --rehearsal`) creates a temp directory,
// passes that absolute path into prepare/execute, prints a deterministic
// log, and deletes only that directory. It does not accept --dir.
//
// PASS is not the rollback() exit code. Before update, every regular file
// in the synthetic tree is recorded as `relative-path<TAB>sha256`. After
// update and before rollback, every manifest FILE row must match its
// payload digest and differ from that pre-update manifest. After rollback
// the file set and each hash must match the pre-update manifest, and no
// updater scratch may remain. `sha256` on the UPDATE OK line is the
// SHA-256 of that canonical pre-update manifest.
//
// The player binary discovers a repository only by walking up from the
// current directory for `.git`. It does not embed a build-machine path.
// When that walk finds nothing, the temp directory this CLI just created
// is still allowed.
//
// TODO(TYJ-160): MoxianLauncher must not call this. The launcher is a
// Win32 GUI process; invoking update or rollback from it would target a
// real client tree and is not behavior-safe. The supported entry is
// `MoxianAutoPatcher --rehearsal`.

#include "patch_security.hpp"

#include <array>
#include <filesystem>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace mxh::patch {
namespace fs = std::filesystem;

inline constexpr std::string_view kRehearsalFromVersion = "1.0.0.0";
inline constexpr std::string_view kRehearsalToVersion = "1.0.0.1";

struct NamedBytes {
    std::string_view path;
    std::string_view bytes;
};

const std::array<NamedBytes, 4>& standard_installed_files();
const std::array<NamedBytes, 3>& standard_update_files();

struct TreeManifest {
    std::map<std::string, std::string> files;

    std::string canonical_text() const;
    std::string digest() const;
    bool operator==(const TreeManifest& other) const { return files == other.files; }
    bool operator!=(const TreeManifest& other) const { return !(*this == other); }
};

TreeManifest capture_tree_manifest(const fs::path& root);
int restored_file_count(const TreeManifest& before, const TreeManifest& after);
int count_updater_leftovers(const fs::path& root);
bool is_updater_scratch_name(std::string_view name);

enum class RehearsalRejectReason {
    Empty,
    NotAbsolute,
    PlayDh,
    InsideRepository,
    UnknownRepository,
};

struct RehearsalRejection {
    RehearsalRejectReason reason = RehearsalRejectReason::Empty;
    std::string message;
};

// Reject an empty path, a relative path, any path component named PlayDH
// (case-insensitive, including after symlink resolution), and any path
// inside the repository working tree. Does not create or delete anything.
// An empty `repository` means the walk found no checkout. `cli_owned_temp`
// is set only for the directory the rehearsal CLI just created; that case
// is allowed on a player machine that has no repository.
std::optional<RehearsalRejection> reject_rehearsal_target(const fs::path& target);
std::optional<RehearsalRejection> reject_rehearsal_target(const fs::path& target,
                                                         const fs::path& repository,
                                                         bool cli_owned_temp);

struct RehearsalLayout {
    fs::path root;
    fs::path game;
    fs::path payload;
    fs::path manifest;
    std::string manifest_sha256;
    int update_files = 0;
};

// Writes the fixed synthetic tree under an existing absolute directory.
// Returns false before any write when the target is rejected.
// `cli_owned_temp` is true only for the directory the CLI created.
bool prepare_standard_rehearsal(const fs::path& target, RehearsalLayout& layout, std::string& error,
                                bool cli_owned_temp = false);

struct RehearsalLogInput {
    bool update_ok = false;
    // Recorded for tests. rehearsal_should_pass does not read it.
    bool rollback_exit_code = false;
    // True only when every updated file matched its payload digest and
    // differed from the pre-update manifest. A no-op update stays false.
    bool applied_ok = false;
    bool manifest_exact = false;
    int files = 0;
    int restored = 0;
    int leftovers = 0;
    std::string sha256;
};

// True only when the update reported success, the post-update bytes
// matched the payload and differed from the pre-update manifest, the
// pre-update manifest matches after rollback, and no updater scratch
// remains. rollback_exit_code is ignored on purpose.
bool rehearsal_should_pass(const RehearsalLogInput& input) noexcept;
std::string format_rehearsal_log(const RehearsalLogInput& input);

struct RehearsalRunResult {
    bool pass = false;
    bool update_ok = false;
    bool rollback_exit_code = false;
    bool applied_ok = false;
    bool manifest_exact = false;
    int files = 0;
    int restored = 0;
    int leftovers = 0;
    std::string sha256;
    std::string text;
};

RehearsalRunResult execute_rehearsal(const RehearsalLayout& layout, bool cli_owned_temp = false);

struct RehearsalCliArgs {
    bool rehearsal = false;
    bool has_directory = false;
    bool has_update = false;
    bool has_rollback = false;
    bool has_check = false;
    bool has_server = false;
    bool has_manifest = false;
    bool has_version = false;
};

// Non-empty when --rehearsal was combined with a target or another mode.
std::optional<std::string> rehearsal_cli_conflict(const RehearsalCliArgs& args);

// Creates one child directory under temp_parent (or the system temp
// directory when temp_parent is empty), passes that path explicitly to
// the rehearsal, prints the log to `out`, and deletes only that child.
int run_patch_rehearsal_cli(std::ostream& out, std::ostream& err, const fs::path& temp_parent = {});

inline std::string rehearsal_file_url(const fs::path& path) {
    return "file://" + fs::absolute(path).generic_string();
}

}  // namespace mxh::patch
