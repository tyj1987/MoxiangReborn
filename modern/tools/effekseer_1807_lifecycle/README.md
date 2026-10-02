# Effekseer 1.80.7 sound lifecycle candidate

This is an opt-in patch kit for an **isolated Windows Unity project copy**.
It does not install or modify anything under the game's Unity Assets tree.
Do not apply while that isolated Editor is running. Preserve the backup pair.
The patch changes only the two files named in `manifest.json`; `.meta`, native
DLLs, effects, models, networking and server collision remain untouched.

## Exact provenance and scope

Official repository: https://github.com/effekseer/EffekseerForUnity

Source commit: `8762e12d7e2b97b13feaca940d7de5bba1a2b3d2`, paths
`Dev/Plugin/Assets/Effekseer/Scripts/EffekseerSound.cs` and
`EffekseerRuntime.cs`. The original raw-byte hashes exactly match the installed
ROG files reported as 1.80.7, including UTF-8 BOM, with no CRLF normalization:

- Sound: `9e598af85826abf02a5d606c99403ab48baed902dd67e35bd9ea06a911d756b8`
- Runtime: `47676f4cc4f09bc925b7dc20a70bfe3a1c602a17dfea65a8384956d1ce58879a`

The small `tests/upstream` copies are immutable test fixtures outside Assets,
not installed vendor replacements. `LICENSE.Effekseer` preserves the upstream
MIT notice and applies to those files and the derived patch. Output hashes
are in the manifest. Any different version, line endings, partial patch or
unrecognized bytes are refused; no fuzzy application or automatic upgrades.

## Behavior

The original locks mutable static `Instance`, then sets it to null on disable.
Repeated disable and callbacks after disposal reach `lock(null)`; disposing an
old player also clears a newer owner. Deferred actions read mutable Instance
again instead of retaining their intended recipient.

The patch uses one stable callback gate and explicit enabled/disposed/registered
state. Ownership remains reserved until native unregistration succeeds. Queue
entries capture the owner and generation and revalidate before execution;
disabling closes admission, invalidates the generation, discards pending audio
work and stops existing sounds. Shutdown does not replay Play requests whose
resource pointers could be invalidated by subsequent native teardown.

Registration/unregistration and queued playback execute outside the callback
gate. Lifecycle and Update must use the constructing Unity thread; misuse
throws rather than racing Unity APIs. A retired instance cannot clear another
owner. Dispose is explicit and repeat-safe; the old finalizer that mutated the
singleton from a GC thread is removed. Native registration errors are surfaced,
partial registration attempts are unregistered, and failed unregistration
reserves the disabled owner for retry instead of enabling a second owner. Owned
audio is stopped even when unregistration throws; a simultaneous audio-stop
exception is aggregated with the original unregistration error.

Runtime tracks completed plugin initialization and system activation. Disabled
or incomplete instances do not update. It disables sound before the system,
disposes sound before native termination, avoids duplicate cleanup, and keeps
activation/cleanup exceptions visible. Completion flags change only after the
corresponding cleanup returns successfully. Sound cleanup that has not finished
blocks re-enable; retrying its callback unregistration can finish that stage.
An exception from system.OnDisable or TermPlugin has an unknown partial outcome:
the original error is retained, updates/re-enable and further cleanup attempts
are blocked, and an isolated Editor restart is required. It is unsafe to blindly
retry renderer/resource cleanup or native termination. A failure before InitPlugin returns is
**not** treated as successful initialization and does not force TermPlugin.

## Apply on the existing ROG isolated copy

Use an explicit project and a **new backup directory outside that project**:

```powershell
python modern/tools/effekseer_1807_lifecycle/apply_patch.py --isolated-project C:/isolated/MoxiangClient
python modern/tools/effekseer_1807_lifecycle/apply_patch.py --isolated-project C:/isolated/MoxiangClient --backup-dir C:/isolated-backups/effekseer-1807-before --apply
```

The first command is a no-write check. Both original hashes and both generated
output hashes must match before any write. The second saves and verifies both
originals before replacements, then verifies the patched pair. Ordinary write
errors roll back completed writes; a process/power interruption between files
is not a filesystem transaction. A mixed state is refused for manual inspection
and restoration from the backup. Linked paths and this repository's canonical
Unity project are refused. Repeating application to an exact patched pair is
a no-op. A pair patched by the earlier `3cd63113` kit is not accepted by this
revision: restore its saved original pair first, then check/apply the new kit
with a new backup directory. The utility does not close Editors or alter process settings.

To restore, with the isolated Editor closed, copy the two original `.cs` files
from the retained backup to the same `Assets/Effekseer/Scripts` paths and run
the dry-run check again. Do not remove or regenerate the existing `.meta` files.

## Executed cloud checks

```text
python -m unittest discover -s modern/tools/effekseer_1807_lifecycle -p test_patch.py -v
python modern/tools/effekseer_1807_lifecycle/run_managed_tests.py --dotnet <dotnet8> --artifacts <new-output-directory>
```

Python: 9/9 passing: no-write inspection; exact output/backup/repeat; changed
source; CRLF mismatch; mixed state; altered patch output; backup location;
symlink refusal; and rollback on the second write's injected failure.

Managed red/green compiles the actual original/patched vendor pair using Unity
and native test doubles. Original: three expected failures (repeat disable,
callback after Dispose, stale-owner Dispose). Patched: 22/22 passing, including
normal queued playback/stop, all five callbacks after disposal, repeat and
pre-enable disable, callbacks during native registration/unregistration,
callback waiting across a generation change, detached old work, new-generation
work, ownership, registration/unregistration failures and retry, off-thread
misuse, incomplete initialization and normal Runtime disable/reenable/destroy.
The six review regressions first failed against the `3cd63113` patch: active
owned audio after failed unregister; System disable failing before release;
System disable failing after release; Runtime retry of sound-unregister only;
TermPlugin failure before native termination; and failure after native termination.
They now pass, asserting retained original errors, unchanged completion flags on
failure, blocked re-enable/updates, no unsafe repeat renderer/native release,
and a successful sound-unregister retry. These are managed fault injections,
not evidence of the corresponding exception occurring in Unity.

Cloud used a temporary .NET SDK8.0.100 from Microsoft's official distribution,
verified against its published SHA512. No SDK or build outputs are committed.
Evidence: `/workspace/moxiang-audit-20261001/effekseer-review-final/`.
This is real managed compilation/execution, **not Unity/native acceptance**.

## Limits and one ROG validation pass

- The inspected official native `EffekseerSetSoundPlayerEvent` directly calls
  SetSoundPlayer; it does not expose a quiescence barrier. Its source is supporting
  evidence, not a verified hash of the installed native DLL. Static native
  callbacks carry no owner/epoch token. Work already captured/enqueued for an
  old generation is rejected once both owner and epoch have been captured.
  A callback can read owner, pause across Disable/Enable on that same owner,
  and then read the new epoch: it is indistinguishable from new work. This
  boundary therefore includes already-started callbacks that have not captured
  epoch, as well as native invocations that first enter after re-enable. The
  waiting-on-gate test captures epoch before waiting; it does not cover this
  earlier read window. This patch does not invent an ABI token or claim complete
  cross-generation protection or native worker quiescence.
- Internal native allocation failure partway through InitPlugin, or a partial
  renderer failure inside system.OnEnable, is not made transactionally recoverable
  by this two-file patch. The original exception remains visible; validate the
  observed failure stage before retrying, and use Editor restart if native state
  is uncertain. No catch-and-continue or speculative TermPlugin is added.
- .NET test doubles do not verify Unity destroyed-object semantics, IL2CPP/AOT,
  native resource lifetime, actual audio or scene teardown ordering.

In the existing isolated ROG project, compile once, then exercise normal sound
play/stop, disable twice, disable/enable and Play-stop/Play again, followed by
scene/runtime destruction while callbacks are active. Confirm no lock-null,
MissingReferenceException, hangs, lost normal sound, duplicate registration or
native teardown failures. Capture Editor.log and applied output hashes. Confirm
NPC/terrain tests separately; this patch does not alter their acceptance status.
Do not apply it to the old locked project or force-close unrelated ROG Editors.
