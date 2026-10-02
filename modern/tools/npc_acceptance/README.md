# NPC ground-measurement candidate (not Unity-validated)

This directory is an independent acceptance tool, outside Unity `Assets`. It
does not patch game logic, vendor files, layers, transforms or physics settings.
The ROG executor can obtain it from this source branch and integrate it into the
existing acceptance project. No new project or second ROG task is required.

The reported old `WriteSample` uses a global Default-layer ray from 2.5 units
above the NPC. Its first hit can be the NPC's CharacterController. The reported
negative gap (-1.5..-1.73) therefore does not prove floor penetration. The
reported 198-second visible NPC/room Play observation remains separate evidence.

## C# API and dependencies

`NpcGroundMeasurementCandidate.Measure(npcRoot, roomRoot, approvedFloors,
excludedSurfaces, bodyParts, footMarker, maxAbove, maxBelow, minUpDot)` returns a
`Sample`; all bindings are explicit. It requires UnityEngine (Transform,
Collider.Raycast, CharacterController, Rigidbody, SkinnedMeshRenderer, Bounds)
and System/Collections.Generic, including `float.IsFinite`. It targets the
existing Unity 6000.6 project. It has no Effekseer, Animator, input, database or
network dependency. The accompanying C# tests need NUnit and Unity physics in
the existing EditMode test assembly.

- `npcRoot` and `roomRoot`: actual live hierarchy roots, not guessed paths.
- `approvedFloors`: source-reviewed floor Collider references only. Do not
  automatically pass all 160 room meshes. `excludedSurfaces` explicitly holds
  ceilings/roofs/walls and wins even if a collider is accidentally in both lists.
  No algorithm can identify a semantic floor from an arbitrary triangle/name;
  this small binding must be reviewed in the imported room hierarchy. An empty
  exclusion list is allowed only when the floor allowlist is already reviewed.
- Queries go directly to each allowed static collider. Disabled/inactive,
  trigger, CharacterController, NPC-descendant, non-room and Rigidbody-attached
  colliders (including the moving test ball) are rejected. A positive `minUpDot`
  rejects downward/vertical faces. The nearest allowed hit inside the explicit
  local height window is selected; an invalid or missing hit returns false/NaN.
  Call after the normal physics transform update; tests use SyncTransforms.
- `bodyParts`: all reviewed body SkinnedMeshRenderers, excluding accessories;
  active/enabled NPC descendants only, deduplicated. Their aggregate world AABB
  produces **renderBoundsGap**, never an anatomical foot measurement. Animation,
  clothing and renderer bounds policy can change this metric.
- `footMarker`: optional reviewed anatomical/contact marker below `npcRoot`.
  Only its actual world Y produces **footMarkerGap**. No marker means false/NaN,
  not an AABB substitution. Sample left and right separately if needed. All
  reported gaps refer to the sampled XZ column, not a complete foot-support test.
- `maxAbove/maxBelow/minUpDot`: finite, explicit measurement parameters. Record
  their values; they do not modify movement/physics. A nearby sampling window
  reduces overhead geometry ambiguity but does not replace the floor allowlist.

Minimal integration in the **existing** `WriteSample` is to call Measure and
write its fields. This package does not assume the full local file path or
replace that file without seeing it. Add the helper to the existing acceptance
assembly, the test file to its existing EditMode test assembly, and bind the
room/floor/body references in the existing SceneBuilder. Do not copy the test
class into a production runtime assembly.

## CSV and runnable summary validation

Keep elapsed/frame-time/contact/ball columns independently. Add these required
columns, writing booleans as true/false, invariant-culture floats and literal
NaN for invalid gaps; use proper CSV escaping:

```text
schema,groundValid,groundCollider,bodyMeshCount,boundsValid,renderBoundsGap,footMarkerValid,footMarkerGap,reason
npc-ground-v2,true,floor#1,2,true,-0.2,true,0.01,foot-marker-and-render-bounds
npc-ground-v2,false,,0,false,NaN,false,NaN,no-reviewed-floor-hit
```

Run from the repository root:

```sh
python3 -m unittest discover -s modern/tools/npc_acceptance -p 'test_*.py'
python3 modern/tools/npc_acceptance/summarize_ground_measurements.py <new-measurement.csv>
```

The summary reports valid/invalid floor counts, separate valid sample counts,
signed means and nearest-rank p95 for the two distinct gap metrics, and reason
counts. Empty metrics are JSON null; invalid samples never become zeros. It
rejects contradictory validity flags/counts, missing hit identity, nonfinite
valid gaps and the old `feet_gap` schema. `acceptancePassed` remains null:
statistics alone do not approve a character or floor.

## Validation status and the existing ROG verification round

Cloud: five Python tests cover real summary behavior (NaN exclusion, AABB vs
marker separation, p95, six inconsistent-row cases and old-schema rejection).
Unity/C# compilation and the two physics/EditMode cases **have not run**.
Passing Python validates the CSV analyzer, not Unity raycast correctness.

In the already scheduled ROG round, run the two C# cases: Default-layer self
Controller + dynamic body + explicitly excluded ceiling must not win; disabled
floor/no hit stays invalid; multiple body meshes and an anatomical marker retain
different values; duplicate renderer bindings do not inflate counts; leaving the
floor becomes invalid. Then sample the actual room with the reviewed floor
allowlist, save collider identities, CSV, parameters and screenshots. Do not move
the NPC or disable its Controller to make a gap approach zero. AABB and marker
measurements must remain separately labelled, even when both are near zero.
