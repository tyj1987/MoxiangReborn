using System;
using System.Collections;
using System.IO;
using UnityEngine;

namespace Moxiang
{
    /// <summary>Explicit opt-in standalone smoke evidence. Never runs in a normal player session.</summary>
    public sealed class DevelopmentRunProbe : MonoBehaviour
    {
        [Serializable] private sealed class Report
        {
            public string runId, utc, unity, graphics, scene, status, error;
            public bool nativeLifecyclePassed;
            public bool networkRequested, gameInReached;
            public bool shopAppearanceReceived;
            public ushort[] shopAppearanceAvatar, shopAppearanceSkin;
            public bool characterCreated;
            public bool movementRequested, movementPassed;
            public bool collisionPassed;
            public bool questNpcRequested, questDialogueOpened, questNpcPassed;
            public bool combatTimelineRequested, skillSubmitted, skillReleaseObserved, skillHitObserved;
            public bool playerLifeObserved, hudLifeObserved, playerHitObserved;
            public uint playerLifeBefore, playerLifeAfter, snapshotLifeAtCapture;
            public int playerLifeDelta;
            public bool deathRequested, deathObserved, deadMoveRejected, deadSkillRejected;
            public bool skillReleaseBeforeHit, hitAnimationObserved;
            public uint skillReleaseCaster, skillReleaseId;
            public ulong skillObjectId;
            public uint skillHitTarget, skillHitDamage, skillHitResult;
            public uint questNpcVisualKind;
            public int materializedMonsterCount;
            public int animatedMonsterCount;
            public uint[] materializedMonsterKinds;
            public string questHeading, questBody, questAction, questFeedback;
            public int movementProbeVersion = 2;
            public uint playerId;
            public ushort mapNumber;
            public int nativeCycles, frame, width, height, terrainVertices;
            public bool gameplayAccepted = false;
        }
        private string output;

        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void StartIfRequested()
        {
            if (!Debug.isDebugBuild || Application.isEditor) return;
            var args = Environment.GetCommandLineArgs();
            if (Array.IndexOf(args, "--mxh-pickup-loop") >= 0) return;
            int index = Array.IndexOf(args, "--mxh-smoke-output");
            if (index < 0 || index + 1 >= args.Length) return;
            var probe = new GameObject("ExplicitDevelopmentProbe").AddComponent<DevelopmentRunProbe>();
            probe.output = Path.GetFullPath(args[index + 1]);
            probe.StartCoroutine(probe.Run());
        }

        private IEnumerator Run()
        {
            var report = new Report { runId = Guid.NewGuid().ToString("N"), utc = DateTime.UtcNow.ToString("O"),
                unity = Application.unityVersion, graphics = SystemInfo.graphicsDeviceType.ToString(),
                scene = UnityEngine.SceneManagement.SceneManager.GetActiveScene().path };
            try
            {
                Directory.CreateDirectory(output);
                for (int i = 0; i < 100; ++i)
                {
                    using (var client = new NativeClient())
                    {
                        var snapshot = client.Snapshot();
                        if (snapshot.state != CoreState.Idle || snapshot.apiVersion != NativeClient.ApiVersion)
                            throw new InvalidOperationException("Native lifecycle snapshot mismatch.");
                    }
                    report.nativeCycles++;
                }
                report.nativeLifecyclePassed = true;
            }
            catch (Exception exception) { report.error = exception.GetType().Name + ": " + exception.Message; }
            string loginPort = Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT");
            report.networkRequested = !string.IsNullOrEmpty(loginPort);
            report.questNpcRequested = Environment.GetEnvironmentVariable("MXH_SMOKE_QUEST_NPC") == "1";
            report.combatTimelineRequested = Environment.GetEnvironmentVariable("MXH_SMOKE_COMBAT_TIMELINE") == "1";
            report.deathRequested = Environment.GetEnvironmentVariable("MXH_SMOKE_PLAYER_DEATH") == "1";
            if (report.networkRequested)
            {
                var panel = FindFirstObjectByType<ConnectionPanel>();
                System.Action<CoreEvent> observeQuestNpc = item => {
                    if (item.type == NativeClient.EventNpcAdded && item.argument0 == 572)
                        report.questNpcVisualKind = item.reserved0;
                };
                if (panel != null && report.questNpcRequested) panel.CoreEventReceived += observeQuestNpc;
                System.Action<CoreEvent> observeCombat = item => {
                    if (item.type == NativeClient.EventSkillRelease)
                    {
                        report.skillReleaseObserved = true;
                        report.skillReleaseCaster = item.argument0;
                        report.skillReleaseId = item.argument1;
                        report.skillObjectId = item.requestId;
                    }
                    else if (item.type == NativeClient.EventSkillHit && item.argument0 == panel.Observed.game.playerId &&
                             item.argument1 > 0 && item.reserved0 != 0)
                    {
                        report.playerHitObserved = true;
                    }
                    else if (item.type == NativeClient.EventSkillHit && item.argument0 == 50023 &&
                             item.argument1 > 0 && item.reserved0 != 0)
                    {
                        report.skillHitObserved = true;
                        report.skillReleaseBeforeHit = report.skillReleaseObserved;
                        report.skillHitTarget = item.argument0;
                        report.skillHitDamage = item.argument1;
                        report.skillHitResult = item.reserved0;
                    }
                    else if (item.type == NativeClient.EventPlayerDeath && item.argument0 == panel.Observed.game.playerId)
                    {
                        report.deathObserved = item.argument1 != 0;
                    }
                    else if (item.type == NativeClient.EventPlayerLife)
                    {
                        report.playerLifeObserved = true;
                        report.playerLifeAfter = item.argument1;
                        report.playerLifeDelta = unchecked((int)item.reserved0);
                    }
                };
                if (panel != null && report.combatTimelineRequested) panel.CoreEventReceived += observeCombat;
                string observerAccount = Environment.GetEnvironmentVariable("MXH_SMOKE_OBSERVER_USER");
                string observerSecret = string.IsNullOrEmpty(observerAccount) ? null : Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD");
                report.movementRequested = !string.IsNullOrEmpty(observerAccount);
                try
                {
                    panel.BeginDevelopmentProbe("127.0.0.1", loginPort,
                        Environment.GetEnvironmentVariable("MXH_SMOKE_USER"), Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD"));
                    Environment.SetEnvironmentVariable("MXH_SMOKE_PASSWORD", null);
                }
                catch (Exception exception) { report.error = exception.GetType().Name + ": " + exception.Message; }
                float deadline = Time.realtimeSinceStartup + 30;
                string createName = Environment.GetEnvironmentVariable("MXH_SMOKE_CREATE_NAME");
                bool createSubmitted = false;
                bool questSubmitted = false;
                float lastSkillSubmit = -10f;
                while (panel != null && Time.realtimeSinceStartup < deadline)
                {
                    if (panel.Observed.state == CoreState.CharacterListReady)
                    {
                        if (!string.IsNullOrEmpty(createName) && panel.Observed.characterCount == 0 && !createSubmitted)
                        {
                            var creation = FindFirstObjectByType<CharacterCreatePanel>();
                            if (creation == null) { report.error = "Character creation UI missing."; break; }
                            creation.characterName.text = createName; creation.create.onClick.Invoke(); createSubmitted = true;
                        }
                        else if (panel.Observed.characterCount > 0)
                        {
                            report.characterCreated = createSubmitted && panel.Observed.characters[0].DisplayName == createName;
                            panel.SelectFirstForDevelopmentProbe();
                        }
                    }
                    if (panel.Observed.state == CoreState.InGame)
                    {
                        report.gameInReached = true; report.playerId = panel.Observed.game.playerId; report.mapNumber = panel.Observed.game.mapNumber;
                        if (report.playerLifeBefore == 0 && !report.playerLifeObserved)
                            report.playerLifeBefore = panel.Observed.game.life;
                        report.shopAppearanceReceived = panel.ShopAppearance.Received;
                        if (report.shopAppearanceReceived)
                        {
                            report.shopAppearanceAvatar = new ushort[panel.ShopAppearance.Avatar.Count];
                            report.shopAppearanceSkin = new ushort[panel.ShopAppearance.Skin.Count];
                            for (int i = 0; i < report.shopAppearanceAvatar.Length; ++i)
                                report.shopAppearanceAvatar[i] = panel.ShopAppearance.Avatar[i];
                            for (int i = 0; i < report.shopAppearanceSkin.Length; ++i)
                                report.shopAppearanceSkin[i] = panel.ShopAppearance.Skin[i];
                            if (!report.questNpcRequested && !report.combatTimelineRequested) break;
                        }
                    }
                    if (report.gameInReached && report.combatTimelineRequested)
                    {
                        if (!report.skillHitObserved && Time.realtimeSinceStartup - lastSkillSubmit >= 0.75f)
                        {
                            report.skillSubmitted |= panel.UseSkill(1, 50023, 44178.5f, 13253.4f) == CoreResult.Ok;
                            lastSkillSubmit = Time.realtimeSinceStartup;
                        }
                        if (report.skillHitObserved)
                        {
                            foreach (var target in FindObjectsByType<TargetSelectable>(FindObjectsSortMode.None))
                            {
                                if (target.objectId != report.skillHitTarget || target.GetComponentInParent<ServerEntityRegistry>() == null) continue;
                                var animation = target.GetComponent<ServerMonsterAnimation>();
                                report.hitAnimationObserved |= animation != null && animation.CurrentMotion == ServerMonsterAnimation.HitMotion;
                                break;
                            }
                            if (report.hitAnimationObserved && report.playerLifeObserved && report.playerHitObserved && report.shopAppearanceReceived &&
                                (!report.deathRequested || report.deathObserved))
                            {
                                if (report.deathRequested)
                                {
                                    report.deadMoveRejected = panel.Move(44178, 13253, false) == CoreResult.WrongState;
                                    report.deadSkillRejected = panel.UseSkill(1, 50023, 44178.5f, 13253.4f) == CoreResult.WrongState;
                                }
                                break;
                            }
                        }
                    }
                    if (report.gameInReached && report.questNpcRequested)
                    {
                        var dialogue = FindFirstObjectByType<QuestDialogueController>(FindObjectsInactive.Include);
                        var questState = FindFirstObjectByType<QuestStateController>(FindObjectsInactive.Include);
                        var targetSelection = FindFirstObjectByType<TargetSelectionController>();
                        if (!questSubmitted && dialogue != null && questState != null && questState.ActiveQuests.ContainsKey(180))
                        {
                            TargetSelectable identity = null;
                            if (targetSelection != null)
                                foreach (var candidate in FindObjectsByType<TargetSelectable>(FindObjectsSortMode.None))
                                    if (candidate.isNpc && candidate.objectId == 572 && candidate.GetComponentInParent<ServerEntityRegistry>() != null)
                                    { identity = candidate; break; }
                            if (identity == null) { yield return null; continue; }
                            report.questNpcVisualKind = identity.visualKind;
                            report.questDialogueOpened = targetSelection.Select(identity) && dialogue.IsVisible;
                            if (!report.questDialogueOpened) { report.error = "Quest 180 dialogue did not open for NPC 572."; break; }
                            report.questHeading = dialogue.heading == null ? "" : dialogue.heading.text;
                            report.questBody = dialogue.body == null ? "" : dialogue.body.text;
                            report.questAction = dialogue.continueLabel == null ? "" : dialogue.continueLabel.text;
                            dialogue.Submit();
                            questSubmitted = true;
                        }
                        if (questSubmitted && dialogue != null && dialogue.feedback != null && dialogue.feedback.text == "任务步骤已更新")
                        {
                            report.questFeedback = dialogue.feedback.text;
                            report.questNpcPassed = true;
                            if (report.shopAppearanceReceived) break;
                        }
                    }
                    if (panel.Observed.state == CoreState.Failed) { report.error = panel.Observed.Error; break; }
                    yield return null;
                }
                if (!report.gameInReached && string.IsNullOrEmpty(report.error)) report.error = "GameIn acknowledgement not reached within 30 seconds.";
                if (report.gameInReached && !report.shopAppearanceReceived && string.IsNullOrEmpty(report.error))
                    report.error = "GameIn shop appearance block did not reach managed state.";
                if (report.questNpcRequested && !report.questNpcPassed && string.IsNullOrEmpty(report.error))
                    report.error = "Quest NPC acknowledgement did not reach the visible dialogue within 30 seconds.";
                if (report.combatTimelineRequested &&
                    (!report.skillSubmitted || !report.skillReleaseObserved || !report.skillHitObserved ||
                     !report.skillReleaseBeforeHit || !report.hitAnimationObserved || !report.playerLifeObserved ||
                     report.playerLifeDelta >= 0 || report.playerLifeAfter >= report.playerLifeBefore) && string.IsNullOrEmpty(report.error))
                    report.error = "Authoritative player and monster damage timelines were not observed within 30 seconds.";
                if (report.gameInReached && report.movementRequested)
                    yield return DevelopmentMovementProbe.Run(panel, ushort.Parse(loginPort), observerAccount, observerSecret,
                        (passed, error) => { report.movementPassed = passed; report.collisionPassed = passed; if (!passed) report.error = error; });
                observerSecret = null;
                if (panel != null && report.questNpcRequested) panel.CoreEventReceived -= observeQuestNpc;
                if (panel != null && report.combatTimelineRequested) panel.CoreEventReceived -= observeCombat;
            }
            for (int i = 0; i < 120; ++i) yield return null;
            yield return new WaitForEndOfFrame();
            try
            {
                report.frame = Time.frameCount; report.width = Screen.width; report.height = Screen.height;
                var panel = FindFirstObjectByType<ConnectionPanel>();
                report.status = panel == null ? "missing panel" : panel.status.text;
                report.snapshotLifeAtCapture = panel == null ? 0 : panel.Observed.game.life;
                report.hudLifeObserved = panel != null && report.playerLifeObserved &&
                    report.snapshotLifeAtCapture < report.playerLifeBefore &&
                    report.status.Contains("HP " + report.snapshotLifeAtCapture + "/");
                var monsterKinds = new System.Collections.Generic.HashSet<uint>();
                foreach (var target in FindObjectsByType<TargetSelectable>(FindObjectsSortMode.None))
                {
                    if (!target.isNpc && target.objectId >= 50000 && target.GetComponentInParent<ServerEntityRegistry>() != null)
                    {
                        report.materializedMonsterCount++;
                        monsterKinds.Add(target.visualKind);
                        if (target.GetComponent<ServerMonsterAnimation>() != null) report.animatedMonsterCount++;
                    }
                }
                report.materializedMonsterKinds = new uint[monsterKinds.Count];
                monsterKinds.CopyTo(report.materializedMonsterKinds);
                Array.Sort(report.materializedMonsterKinds);
                var terrain = GameObject.Find("Map10GeometryInspection");
                report.terrainVertices = terrain == null ? 0 : terrain.GetComponent<MeshFilter>().sharedMesh.vertexCount;
                var image = ScreenCapture.CaptureScreenshotAsTexture();
                File.WriteAllBytes(Path.Combine(output, "player.png"), image.EncodeToPNG());
                Destroy(image);
                File.WriteAllText(Path.Combine(output, "report.json"), JsonUtility.ToJson(report, true));
                Debug.Log("MXH_PLAYER_SMOKE_COMPLETE run=" + report.runId + " native=" + report.nativeLifecyclePassed);
            }
            catch (Exception exception) { Debug.LogError("MXH_PLAYER_SMOKE_FAILED " + exception.GetType().Name); Application.Quit(1); yield break; }
            Application.Quit(report.nativeLifecyclePassed && (!report.networkRequested || report.gameInReached) &&
                (!report.movementRequested || report.movementPassed) && string.IsNullOrEmpty(report.error) ? 0 : 1);
        }
    }
}
