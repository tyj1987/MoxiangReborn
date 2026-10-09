#!/usr/bin/env python3
"""Run only source-reviewed r18 pure/MockDb tests on the PC.

The manifest binds each executable and its test source. It is produced only
after classification/review; this runner has no caller-supplied test filter.
Database-backed tests, including SQLite :memory:, are deliberately absent.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "modern" / "build-vitals-r18"
CASES = {
    "mxh_original_vitals_store_tests": {
        "source": "modern/tests/unit/server/original_vitals_store_test.cpp",
        "cases": [
            "OriginalVitalsWireDelta.RejectsUnrepresentableChangesBeforeCommit",
            "OriginalVitalsStore.MissingPendingAndMalformedRowsFailClosed",
            "OriginalVitalsStore.PendingInitializationHappensExactlyOnce",
            "OriginalVitalsStore.VitalsAndItemWriteCommitOrRollbackTogether",
            "OriginalVitalsStore.StaleRevisionAndFailedRollbackDoNotClaimSuccess",
            "OriginalVitalsStore.CasConflictQuarantinesOldSessionWithoutFurtherDbCalls",
            "OriginalVitalsStore.CompanionNoOpMustFailInsideSameTransaction",
            "OriginalVitalsStore.FailedBeginNeverRollsBackAndRetiresAdapter",
            "OriginalVitalsStore.FailedCommitRetiresSharedAdapterEvenAfterRollback",
            "OriginalVitalsStore.LostCommitReplyRemainsUnknownEvenIfReadbackSeesIt",
            "OriginalVitalsStore.RevisionBoundAndInconsistentDeathNeverOpenTransaction",
        ],
    },
    "mxh_player_tests": {
        "source": "modern/tests/unit/server/player_test.cpp",
        "cases": [
            "PlayerExperience.CarriesRemainderAcrossLevels",
            "PlayerExperience.OneGrantRaisesAtMostOneLevelLikeLegacySetPlayerExpPoint",
            "OriginalExperience.NormalSegmentsAwardOnceAndPersistRecoveryFlag",
            "OriginalExperience.LargeOneGrantRaisesOnlyOneLevelAndCarriesSegments",
            "OriginalExperience.OverSeventyLevelAndPointOverflowAreBounded",
            "OriginalExperience.RejectsNonFiniteLiveEventAwardBeforePublishingKill",
        ],
    },
    "mxh_server_handler_tests": {
        "source": "modern/tests/unit/server/server_handler_test.cpp",
        "cases": [
            "AgentCharacterCreateTest.PendingVitalsFailureRollsBackCharacterAndClothes",
            "AgentCharacterCreateTest.ProgressFailureRollsBackCharacterBeforeClothes",
            "AgentCharacterCreateTest.UnknownCommitRetiresAdapterWithoutFalseNack",
            "AgentCharacterCreateTest.FailedRollbackRetiresAdapterWithoutFalseNack",
            "MapHandlerTest.RevisionedKillNeverPublishesIncompatibleGroundObtain",
            "MapHandlerTest.MonsterDeathAwardsExperienceAndSendsLegacyNotification",
            "MapHandlerTest.ProgressionGameInSnapshotRestoresNonzeroExperienceAndPoints",
            "MapHandlerTest.NormalExperienceSegmentRefillsAndPersistsOnce",
            "MapHandlerTest.FailedMonsterExperienceTransactionKeepsMonsterAliveAndNoRewardWire",
            "MapHandlerTest.RevisionedSkillSpendsMpBeforeMonsterAttackAndRejectsUnsafeTargets",
            "MapHandlerTest.PreloadedQuestRejectsRevisionedSkillBeforeMpAndMonsterChange",
            "MapHandlerTest.LethalRewardFailureNacksWithoutSkillAckOrSecondMpSpend",
            "MapHandlerTest.RevisionedVitalsRejectsUncoupledItemAndQuestWritesBeforeMutation",
            "MapHandlerTest.MissingVitalsRowRejectsGameInBeforeRuntimeRegistration",
            "MapHandlerTest.MissingProgressRowRejectsGameInBeforeRuntimeRegistration",
            "MapHandlerTest.PartialNullProgressRejectsGameInWithoutBackfill",
            "MapHandlerTest.PersistedZeroHpStaysDeadAcrossFreshGameIn",
            "MapHandlerTest.MonsterAiAttackUsesSkillDelayAndPublishesAuthoritativeDeath",
            "MapHandlerTest.LaterUnknownMonsterHitSuppressesEarlierQueuedVictimEvents",
        ],
    },
    "mxh_client_tests": {
        "source": "modern/tests/unit/client/cingame_input_test.cpp",
        "cases": [
            "InGamePlayable.OriginalProgressionNotificationsUseAbsolutePackedValues",
            "InGamePlayable.MaximaThenDeltaDoNotDoubleClampCurrentVitals",
            "InGamePlayable.OriginalPresentReviveRequiresReadyAndAuthoritativeMove",
        ],
    },
}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    if set(manifest) != set(CASES):
        parser.error("manifest target set differs from reviewed allowlist")
    args.output_dir.mkdir(parents=True, exist_ok=False)
    results = []
    for target, spec in CASES.items():
        row = manifest[target]
        if row["source"] != spec["source"] or row["cases"] != spec["cases"]:
            parser.error(f"{target}: manifest classification differs from reviewed allowlist")
        kind = "client" if target == "mxh_client_tests" else "server"
        expected_exe = BUILD / "tests" / "unit" / kind / (target + ".exe")
        exe = Path(row["exe"])
        source = ROOT / spec["source"]
        if exe.resolve() != expected_exe.resolve() or not exe.is_file() or digest(exe) != row["exeSha256"]:
            parser.error(f"{target}: executable SHA mismatch")
        if not source.is_file() or digest(source) != row["sourceSha256"]:
            parser.error(f"{target}: test source SHA mismatch")
        if any("*" in case or "?" in case or ":" in case or "-" in case
               for case in spec["cases"]):
            parser.error(f"{target}: wildcard/negative filter forbidden")
        xml_path = args.output_dir / f"{target}.xml"
        cmd = [str(exe), "--gtest_filter=" + ":".join(spec["cases"]),
               "--gtest_repeat=1", "--gtest_output=xml:" + str(xml_path)]
        try:
            result = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True,
                                    encoding="utf-8", errors="replace", check=False,
                                    timeout=120,
                                    env={k: v for k, v in os.environ.items()
                                         if not k.upper().startswith("GTEST_")})
            output = result.stdout + result.stderr
            exit_code = result.returncode
        except subprocess.TimeoutExpired as exc:
            output = str(exc) + "\n"
            exit_code = 124
        (args.output_dir / f"{target}.log").write_text(
            output, encoding="utf-8")
        observed = set()
        if xml_path.is_file():
            try:
                root = ET.parse(xml_path).getroot()
                for case in root.iter("testcase"):
                    if case.attrib.get("status") != "run" or \
                       case.attrib.get("result") != "completed" or \
                       case.find("failure") is not None or case.find("skipped") is not None:
                        continue
                    observed.add(case.attrib["classname"] + "." + case.attrib["name"])
            except (ET.ParseError, KeyError):
                pass
        if observed != set(spec["cases"]):
            exit_code = 1
        results.append({"target": target, "cases": len(spec["cases"]),
                        "observedPassed": len(observed), "exitCode": exit_code,
                        "exeSha256": row["exeSha256"],
                        "sourceSha256": row["sourceSha256"]})
        if exit_code:
            break
    (args.output_dir / "results.json").write_text(
        json.dumps(results, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(results, separators=(",", ":")))
    return 0 if len(results) == len(CASES) and all(r["exitCode"] == 0 for r in results) else 1


if __name__ == "__main__":
    sys.exit(main())
