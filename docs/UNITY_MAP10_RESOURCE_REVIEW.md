# Map10 Unity resource review

Profile: `unity-remaster-v1`  
Canonical root: `modern/data/PlayDH`  
Review date: 2026-09-12

`Map/10.hfl` currently has two byte-distinct candidates:

| Source | Bytes | SHA-256 | Classification |
|---|---:|---|---|
| `Resource/Map/10.hfl` | 137,030 | `e24de771dca4a44da03baea1a284c1717ba10d9134db1365e590f39f47aa849f` | generated placeholder |
| `Map.pak!/10.hfl` (entry 265) | 1,188,682 | `a774fc78a439afe0b435b88a562ede471af013d731b7f10c1c1f2a402e2aa454` | unknown provenance |

The loose candidate is rejected for release because it is generated. The PAK
candidate is the only non-placeholder candidate and is the one used by the
existing Map10 runtime evidence, but it cannot be marked `verified-original`
without an external provenance attestation matching its SHA-256. No source is
silently selected and no placeholder is copied into the Unity release profile.

Related evidence: `EVID-20260826-map-dependency-closure` and the current
`unity-remaster-v1` manifest. A release selection may be added only when the
attestation identifies the authoritative source and reproduces this digest.
