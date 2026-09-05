// mxh/client/crash_dump.hpp
//
// Phase 0 §6.3 controlled unhandled-exception handler.  When the
// client crashes (or an unhandled C++ exception escapes Process()),
// the handler walks the Win32 SEH chain, dumps a minidump into
// the run's `dumps/` directory named with the run id + pid +
// timestamp, and exits with the documented exception code.  The
// capture tool (`scripts/capture-gamein.ps1`) pre-creates the
// `dumps/` directory; the handler refuses to run when the env
// var MXH_DUMP_DIR is unset, so release builds without the
// capture harness are unaffected.
//
// Files are written even when the dump itself fails (the failure
// is recorded as `dumps/<run>-<pid>-<ts>.fail.log` with the Win32
// error code, per plan §6.3 step 5).
#pragma once

#include <cstdint>
#include <string>

namespace mxh::client {

// Install the unhandled-exception filter.  Reads the dump directory
// from the MXH_DUMP_DIR env var, the run id from MXH_RUN_ID, and
// the process name from MXH_PROCESS.  The first install wins; a
// second call is a no-op.  Safe to call from DllMain-style entry
// points.
void install_crash_dump_handler() noexcept;

// The current run id (empty string when not set).  Exposed for
// diagnostics; do not cache the returned pointer.
const char* crash_dump_run_id() noexcept;

} // namespace mxh::client
