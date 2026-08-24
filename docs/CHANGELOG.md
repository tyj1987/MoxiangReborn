# Changelog

## Unreleased

### Governance and provenance

- Protected 12 unreachable Git commits with backup refs and a verified bundle.
- Promoted the 8,642-file legacy recovery snapshot to a read-only reference area.
- Added SHA-256 manifests for the current PlayDH tree and recovered SWorking profile.
- Removed historical phase/session documents that were no longer authoritative.
- Replaced the corrupt mixed-encoding ignore file and expanded binary attributes.
- Removed exact untracked duplicate UI header copies and the generated restoration baseline.

### Runtime foundation

- Added explicit `playdh-current` and `sworking-2008-reference` resource profiles.
- Removed the server launcher’s implicit source-recovery scratch fallback.
- Changed the local acceptance server default map to Map10.
- Added atomic client settings persistence for display, audio and last-account preferences.
- Added typed GameLoadingCoordinator transfer handling and progress/error state.
- Character-list parsing now retains appearance, equipment, level and map fields.
- Client startup now rejects a profile missing required Map10, entity, UI or audio resources.
- Removed the synthetic placeholder dialog from the product path.

### Still in progress

- Launcher/update UI, display transition, complete character presentation, loading, map fidelity, live UI binding, collision, effects, SFX and full legacy comparison remain open.
