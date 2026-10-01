# Legacy quaternion oracle

Fixture: legacy-motion-math.json, 36 pairs at interpolation alpha 0.37.
Source: reference/legacy-source/4dddd9a6/SWorking/SS3DGFunc.dll
SHA-256: F87933EEEFFDF820D35A1D7E37745E901DB1D78E7B0D9FEE3D80B5916CD796DA

SS3DGeometryForMuk.dll imports this DLL. The x86 oracle calls exported
_QuaternionSlerp@16 followed by _MatrixFromQuaternion@8. It does not implement
the math under test. Generator: modern/tools/MoxianUnityAssetExport/legacy_motion_oracle.cpp.
Build using MSVC x86 cl /EHsc /std:c++17; run with absolute DLL path and output
JSON path as arguments. Original binaries are unchanged. The probe is a local
verification tool and is not bundled in the Unity Player.

Disassembly identifies Sin at RVA 0x1d90, ACos at 0x1f20, QuaternionSlerp at
0x2ae0, and its epsilon float (0.05) at RVA 0x15144. They use polynomial
approximations without output normalization; Unity Slerp differs. Fixture pairs
cover identity, cardinal/arbitrary rotations, equivalent opposite signs and the
near-parallel linear branch. This does not prove complete animation, action
timing, or compatibility with every historical DLL variant.
