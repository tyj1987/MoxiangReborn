#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "patch_rehearsal.hpp"

#include "patch_transaction.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace mxh::patch {
namespace {

std::string lower_ascii(std::string value) {
    for (char& ch : value) {
        const auto uc = static_cast<unsigned char>(ch);
        if (uc >= 'A' && uc <= 'Z') ch = static_cast<char>(uc - 'A' + 'a');
    }
    return value;
}

bool component_is_playdh(const fs::path& part) {
    const std::string value = part.generic_string();
    if (value.empty() || value == "." || value == ".." || value == "/" || value == "\\") return false;
    return lower_ascii(value) == "playdh";
}

bool path_contains_playdh(const fs::path& path) {
    for (const auto& part : path) {
        if (component_is_playdh(part)) return true;
    }
    return false;
}

fs::path repository_from(const fs::path& start) {
    std::error_code ec;
    fs::path seed = fs::absolute(start, ec);
    if (ec || seed.empty()) return {};
    for (int i = 0; i < 8 && !seed.empty(); ++i) {
        if (fs::exists(seed / ".git", ec)) return fs::weakly_canonical(seed, ec);
        const fs::path parent = seed.parent_path();
        if (parent == seed) break;
        seed = parent;
    }
    return {};
}

// The player executable walks only from the current directory. The test
// binary may also search MXH_REPO_ROOT, which is not compiled into
// MoxianAutoPatcher.
fs::path repository_root() {
#ifdef MXH_REPO_ROOT
    if (const fs::path from_tests = repository_from(MXH_REPO_ROOT); !from_tests.empty()) return from_tests;
#endif
    std::error_code ec;
    const fs::path cwd = fs::current_path(ec);
    if (ec || cwd.empty()) return {};
    return repository_from(cwd);
}

fs::path resolve_existing_prefix(const fs::path& absolute_normal) {
    std::error_code ec;
    fs::path existing = absolute_normal;
    while (!existing.empty() && !fs::exists(existing, ec)) {
        const fs::path parent = existing.parent_path();
        if (parent == existing) break;
        existing = parent;
    }
    fs::path canon = fs::weakly_canonical(existing, ec);
    if (ec) canon = existing;
    const fs::path tail = absolute_normal.lexically_relative(existing.lexically_normal());
    if (!tail.empty() && tail != ".") canon /= tail;
    return canon.lexically_normal();
}

bool is_inside_directory(const fs::path& child, const fs::path& parent) {
    const fs::path rel = child.lexically_normal().lexically_relative(parent.lexically_normal());
    if (rel.empty() || rel == ".") return true;
    for (const auto& part : rel) {
        if (part == "..") return false;
    }
    return true;
}

bool is_strict_descendant(const fs::path& parent, const fs::path& child) {
    if (!is_inside_directory(child, parent)) return false;
    const fs::path rel = child.lexically_normal().lexically_relative(parent.lexically_normal());
    return !rel.empty() && rel != ".";
}

bool path_is_inside(const fs::path& parent, const fs::path& child) {
    std::error_code ec;
    const fs::path parent_abs = fs::weakly_canonical(fs::absolute(parent, ec), ec);
    if (ec) return false;
    const fs::path child_abs = fs::absolute(child, ec).lexically_normal();
    if (ec) return false;
    return is_strict_descendant(parent_abs, child_abs);
}

std::string version_file_bytes() {
    return std::string("VERSION=") + std::string(kRehearsalFromVersion) + "\nCHANNEL=rehearsal\n";
}

bool parent_allowed(const fs::path& root, const fs::path& parent) {
    const fs::path root_abs = fs::absolute(root).lexically_normal();
    const fs::path parent_abs = fs::absolute(parent).lexically_normal();
    return parent_abs == root_abs || is_strict_descendant(root_abs, parent_abs);
}

bool write_inside(const fs::path& root, const fs::path& path, std::string_view bytes, std::string& error) {
    if (!path_is_inside(root, path) || !parent_allowed(root, path.parent_path())) {
        error = "rehearsal write escapes the target directory";
        return false;
    }
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    if (ec) {
        error = "cannot create rehearsal directory";
        return false;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "cannot write rehearsal file";
        return false;
    }
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.flush();
    if (!output) {
        error = "cannot write rehearsal file";
        return false;
    }
    return true;
}

std::vector<std::string> split_tabs(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    std::istringstream row(line);
    while (std::getline(row, field, '\t')) fields.push_back(field);
    return fields;
}

// Every FILE row must be present with the manifest digest, and that digest
// must differ from the pre-update manifest (a missing path counts as different).
bool payload_was_applied(const fs::path& manifest_path, const TreeManifest& before,
                         const TreeManifest& applied) {
    std::ifstream input(manifest_path);
    if (!input) return false;
    int files = 0;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("VERSION=", 0) == 0) continue;
        const auto fields = split_tabs(line);
        if (fields.size() != 5 || fields[0] != "FILE") continue;
        ++files;
        const std::string expected = lower_hex(fields[3]);
        const auto found = applied.files.find(fields[1]);
        if (found == applied.files.end() || lower_hex(found->second) != expected) return false;
        const auto prior = before.files.find(fields[1]);
        if (prior != before.files.end() && lower_hex(prior->second) == lower_hex(found->second)) return false;
    }
    if (!input.eof()) return false;
    return files > 0;
}

bool manifest_hashes_complete(const TreeManifest& manifest) {
    if (manifest.files.empty()) return false;
    for (const auto& entry : manifest.files) {
        if (entry.second.size() != 64) return false;
    }
    return true;
}

std::string unique_rehearsal_id() {
    unsigned long pid = 0;
#ifdef _WIN32
    pid = static_cast<unsigned long>(GetCurrentProcessId());
#else
    pid = static_cast<unsigned long>(::getpid());
#endif
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::to_string(pid) + "-" + std::to_string(now);
}

struct CoutSilencer {
    std::ostringstream sink;
    std::streambuf* old = nullptr;
    CoutSilencer() : old(std::cout.rdbuf(sink.rdbuf())) {}
    ~CoutSilencer() { std::cout.rdbuf(old); }
    CoutSilencer(const CoutSilencer&) = delete;
    CoutSilencer& operator=(const CoutSilencer&) = delete;
};

}  // namespace

const std::array<NamedBytes, 4>& standard_installed_files() {
    static const std::string version = version_file_bytes();
    static const std::array<NamedBytes, 4> files{{
        {"MHVerInfo.ver", version},
        {"bin/client.dat", "rehearsal-old-client"},
        {"bin/note.txt", "rehearsal-old-note"},
        {"logs/client.log", "keep-this-log"},
    }};
    return files;
}

const std::array<NamedBytes, 3>& standard_update_files() {
    static const std::array<NamedBytes, 3> files{{
        {"bin/client.dat", "rehearsal-new-client"},
        {"bin/note.txt", "rehearsal-new-note"},
        {"bin/extra.dat", "rehearsal-added"},
    }};
    return files;
}

std::string TreeManifest::canonical_text() const {
    std::string text;
    for (const auto& entry : files) {
        text += entry.first;
        text += '\t';
        text += entry.second;
        text += '\n';
    }
    return text;
}

std::string TreeManifest::digest() const {
    const std::string text = canonical_text();
    return sha256_bytes(text.data(), text.size());
}

TreeManifest capture_tree_manifest(const fs::path& root) {
    TreeManifest manifest;
    std::error_code ec;
    if (!fs::is_directory(root, ec) || ec) return manifest;
    for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
         it != end && !ec; it.increment(ec)) {
        std::error_code file_error;
        if (!it->is_regular_file(file_error) || file_error) continue;
        const std::string relative = it->path().lexically_relative(root).generic_string();
        if (relative.empty() || relative.find("..") != std::string::npos) continue;
        manifest.files.emplace(relative, sha256_file(it->path()));
    }
    return manifest;
}

int restored_file_count(const TreeManifest& before, const TreeManifest& after) {
    int count = 0;
    for (const auto& entry : before.files) {
        const auto found = after.files.find(entry.first);
        if (found != after.files.end() && found->second == entry.second) ++count;
    }
    return count;
}

bool is_updater_scratch_name(std::string_view name) {
    return name == "_backup" || name == ".update-journal" || name == ".created" ||
           name == ".manifest-download.tmp" ||
           name.find(".mxh-new") != std::string_view::npos ||
           name.find(".mxh-download") != std::string_view::npos ||
           name.find(".tmp") != std::string_view::npos;
}

int count_updater_leftovers(const fs::path& root) {
    std::error_code ec;
    if (!fs::is_directory(root, ec) || ec) return 0;
    int count = 0;
    for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end;
         it != end && !ec; it.increment(ec)) {
        if (is_updater_scratch_name(it->path().filename().generic_string())) ++count;
    }
    return count;
}

std::optional<RehearsalRejection> reject_rehearsal_target(const fs::path& target,
                                                         const fs::path& repository,
                                                         bool cli_owned_temp) {
    auto reject = [](RehearsalRejectReason reason, const char* message) {
        return std::optional<RehearsalRejection>(RehearsalRejection{reason, message});
    };
    if (target.empty()) {
        return reject(RehearsalRejectReason::Empty,
                      "rehearsal target must be an explicit absolute path");
    }
    if (path_contains_playdh(target)) {
        return reject(RehearsalRejectReason::PlayDh,
                      "rehearsal target contains a PlayDH path component");
    }
    if (!target.is_absolute()) {
        return reject(RehearsalRejectReason::NotAbsolute,
                      "rehearsal target must be an explicit absolute path");
    }
    const fs::path normal = fs::absolute(target).lexically_normal();
    if (path_contains_playdh(normal)) {
        return reject(RehearsalRejectReason::PlayDh,
                      "rehearsal target contains a PlayDH path component");
    }
    const fs::path resolved = resolve_existing_prefix(normal);
    if (path_contains_playdh(resolved)) {
        return reject(RehearsalRejectReason::PlayDh,
                      "rehearsal target contains a PlayDH path component");
    }
    if (repository.empty()) {
        if (cli_owned_temp) return std::nullopt;
        return reject(RehearsalRejectReason::UnknownRepository,
                      "rehearsal repository root is unknown");
    }
    const fs::path repo_normal = repository.lexically_normal();
    if (is_inside_directory(normal, repo_normal) || is_inside_directory(resolved, repo_normal)) {
        return reject(RehearsalRejectReason::InsideRepository,
                      "rehearsal target is inside the repository working tree");
    }
    return std::nullopt;
}

std::optional<RehearsalRejection> reject_rehearsal_target(const fs::path& target) {
    return reject_rehearsal_target(target, repository_root(), false);
}

bool prepare_standard_rehearsal(const fs::path& target, RehearsalLayout& layout, std::string& error,
                                bool cli_owned_temp) {
    if (const auto rejected = reject_rehearsal_target(target, repository_root(), cli_owned_temp)) {
        error = rejected->message;
        return false;
    }
    std::error_code ec;
    if (!fs::is_directory(target, ec) || ec) {
        error = "rehearsal target directory does not exist";
        return false;
    }
    layout = RehearsalLayout{};
    layout.root = target;
    layout.game = target / "game";
    layout.payload = target / "payload";
    layout.manifest = target / "manifest.txt";
    layout.update_files = static_cast<int>(standard_update_files().size());

    for (const auto& file : standard_installed_files()) {
        if (!write_inside(target, layout.game / fs::path(file.path), file.bytes, error)) return false;
    }
    for (const auto& file : standard_update_files()) {
        if (!write_inside(target, layout.payload / fs::path(file.path), file.bytes, error)) return false;
    }

    std::string manifest_text = std::string("VERSION=") + std::string(kRehearsalToVersion) + "\n";
    for (const auto& file : standard_update_files()) {
        const fs::path source = layout.payload / fs::path(file.path);
        const std::string hash = sha256_bytes(file.bytes.data(), file.bytes.size());
        if (hash.size() != 64) {
            error = "rehearsal payload digest failed";
            return false;
        }
        manifest_text += "FILE\t";
        manifest_text += file.path;
        manifest_text += "\t";
        manifest_text += rehearsal_file_url(source);
        manifest_text += "\t";
        manifest_text += hash;
        manifest_text += "\t";
        manifest_text += std::to_string(file.bytes.size());
        manifest_text += "\n";
    }
    if (!write_inside(target, layout.manifest, manifest_text, error)) return false;
    layout.manifest_sha256 = sha256_file(layout.manifest);
    if (layout.manifest_sha256.size() != 64) {
        error = "rehearsal manifest digest failed";
        return false;
    }
    return true;
}

bool rehearsal_should_pass(const RehearsalLogInput& input) noexcept {
    (void)input.rollback_exit_code;
    return input.update_ok && input.applied_ok && input.manifest_exact && input.leftovers == 0 &&
           input.sha256.size() == 64;
}

std::string format_rehearsal_log(const RehearsalLogInput& input) {
    std::ostringstream out;
    if (input.update_ok) {
        out << "PATCH_REHEARSAL UPDATE OK files=" << input.files << " sha256=" << input.sha256 << "\n";
    } else {
        out << "PATCH_REHEARSAL UPDATE FAIL files=" << input.files << "\n";
    }
    if (input.applied_ok) {
        out << "PATCH_REHEARSAL APPLIED OK\n";
    } else {
        out << "PATCH_REHEARSAL APPLIED MISMATCH\n";
    }
    if (input.manifest_exact) {
        out << "PATCH_REHEARSAL ROLLBACK OK restored=" << input.restored << "\n";
    } else {
        out << "PATCH_REHEARSAL ROLLBACK MISMATCH restored=" << input.restored << "\n";
    }
    out << "PATCH_REHEARSAL LEFTOVERS " << input.leftovers << "\n";
    if (rehearsal_should_pass(input)) {
        out << "PATCH_REHEARSAL PASS\n";
    } else {
        out << "PATCH_REHEARSAL FAIL\n";
    }
    return out.str();
}

RehearsalRunResult execute_rehearsal(const RehearsalLayout& layout, bool cli_owned_temp) {
    RehearsalRunResult result;
    auto fail = [&](const std::string& message) {
        result.text = "PATCH_REHEARSAL REJECT " + message + "\nPATCH_REHEARSAL FAIL\n";
        return result;
    };
    const fs::path repo = repository_root();
    if (const auto rejected = reject_rehearsal_target(layout.root, repo, cli_owned_temp)) {
        return fail(rejected->message);
    }
    if (const auto rejected = reject_rehearsal_target(layout.game, repo, cli_owned_temp)) {
        return fail(rejected->message);
    }
    if (!path_is_inside(layout.root, layout.game) ||
        !path_is_inside(layout.root, layout.payload) ||
        !path_is_inside(layout.root, layout.manifest)) {
        return fail("rehearsal paths escape the target directory");
    }

    const TreeManifest before = capture_tree_manifest(layout.game);
    const std::string digest = before.digest();
    bool update_ok = false;
    bool rollback_exit = false;
    bool applied_ok = false;
    {
        CoutSilencer silence;
        PatchTransaction transaction;
        transaction.setGameDir(layout.game.string());
        transaction.setManifest(rehearsal_file_url(layout.manifest));
        transaction.setManifestSha256(layout.manifest_sha256);
        try {
            update_ok = transaction.update();
        } catch (const std::exception&) {
            update_ok = false;
        }
        // Snapshot the tree after update() returns and before the rehearsal's
        // own rollback(). A failed update() has already rolled itself back,
        // so this snapshot is the restored tree and the payload check fails.
        const TreeManifest applied = capture_tree_manifest(layout.game);
        applied_ok = update_ok && payload_was_applied(layout.manifest, before, applied);
        try {
            rollback_exit = transaction.rollback();
        } catch (const std::exception&) {
            rollback_exit = false;
        }
    }
    const TreeManifest after = capture_tree_manifest(layout.game);
    const bool exact = manifest_hashes_complete(before) && before == after;
    const int restored = restored_file_count(before, after);
    const int leftovers = count_updater_leftovers(layout.root);

    RehearsalLogInput input;
    input.update_ok = update_ok;
    input.rollback_exit_code = rollback_exit;
    input.applied_ok = applied_ok;
    input.manifest_exact = exact;
    input.files = layout.update_files;
    input.restored = restored;
    input.leftovers = leftovers;
    input.sha256 = digest;
    result.pass = rehearsal_should_pass(input);
    result.update_ok = update_ok;
    result.rollback_exit_code = rollback_exit;
    result.applied_ok = applied_ok;
    result.manifest_exact = exact;
    result.files = layout.update_files;
    result.restored = restored;
    result.leftovers = leftovers;
    result.sha256 = digest;
    result.text = format_rehearsal_log(input);
    return result;
}

std::optional<std::string> rehearsal_cli_conflict(const RehearsalCliArgs& args) {
    if (!args.rehearsal) return std::nullopt;
    if (args.has_directory || args.has_update || args.has_rollback || args.has_check ||
        args.has_server || args.has_manifest || args.has_version) {
        return "rehearsal does not accept a target path or another mode; it only writes a temp directory it creates";
    }
    return std::nullopt;
}

int run_patch_rehearsal_cli(std::ostream& out, std::ostream& err, const fs::path& temp_parent) {
    std::error_code ec;
    const fs::path parent = temp_parent.empty() ? fs::temp_directory_path(ec) : temp_parent;
    if (ec || parent.empty()) {
        err << "PATCH_REHEARSAL REJECT system temp directory is unavailable\n";
        return 2;
    }
    const fs::path target = (parent / ("mxh-patch-rehearsal-" + unique_rehearsal_id())).lexically_normal();
    if (!is_strict_descendant(fs::absolute(parent).lexically_normal(), fs::absolute(target).lexically_normal())) {
        err << "PATCH_REHEARSAL REJECT rehearsal temp path escapes its parent\n";
        return 2;
    }
    const fs::path repo = repository_root();
    if (const auto rejected = reject_rehearsal_target(target, repo, true)) {
        err << "PATCH_REHEARSAL REJECT " << rejected->message << "\n";
        err << "PATCH_REHEARSAL FAIL\n";
        return 2;
    }
    if (!fs::create_directory(target, ec) || ec) {
        err << "PATCH_REHEARSAL REJECT cannot create temp directory\n";
        return 2;
    }
    struct Cleanup {
        fs::path parent;
        fs::path target;
        ~Cleanup() {
            if (!is_strict_descendant(fs::absolute(parent).lexically_normal(),
                                      fs::absolute(target).lexically_normal())) {
                return;
            }
            std::error_code error;
            fs::remove_all(target, error);
        }
    } cleanup{parent, target};

    if (const auto rejected = reject_rehearsal_target(target, repo, true)) {
        err << "PATCH_REHEARSAL REJECT " << rejected->message << "\n";
        err << "PATCH_REHEARSAL FAIL\n";
        return 2;
    }
    RehearsalLayout layout;
    std::string error;
    if (!prepare_standard_rehearsal(target, layout, error, true)) {
        err << "PATCH_REHEARSAL REJECT " << error << "\n";
        err << "PATCH_REHEARSAL FAIL\n";
        return 2;
    }
    const RehearsalRunResult result = execute_rehearsal(layout, true);
    out << result.text;
    return result.pass ? 0 : 1;
}

}  // namespace mxh::patch
