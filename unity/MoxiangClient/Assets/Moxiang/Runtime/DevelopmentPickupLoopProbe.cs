using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using UnityEngine;

namespace Moxiang
{
    // Explicit Development Player only; automatic component/protocol calls, not human clicks.
    public sealed class DevelopmentPickupLoopProbe : MonoBehaviour
    {
        [Serializable] public sealed class Item
        {
            public uint databaseId, itemId, position, durability, rareId, quickPosition, itemParameter;
        }
        [Serializable] public sealed class Report
        {
            public int version = 1;
            public string phase, stage, error, runId;
            public bool passed, gameIn, presentationReady, hit, zeroLife, dropObserved, pickupSubmitted, pickupAck, inventoryFresh, disconnected;
            public bool humanAcceptance = false, mouseInteraction = false;
            public string interaction = "UseSkill/Move commands; GroundDropPickup.RequestPickup; disconnect.onClick.Invoke";
            public uint playerId, mapNumber, dropId, itemId, count, acquiredDatabaseId;
            public int skillAttempts;
            public string lastSkillSubmission;
            public Item[] before = Array.Empty<Item>(), after = Array.Empty<Item>();
        }
        private readonly Report report = new Report();
        private ConnectionPanel panel;
        private string output;
        private StreamWriter events;
        private float deadline, overallDeadline, nextCommand;
        private bool selected, finished;
        private ulong inventorySequence, ackSequence;
        private const uint Target = 50023;

        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void StartIfRequested()
        {
            if (!Debug.isDebugBuild || Application.isEditor) return;
            var args = Environment.GetCommandLineArgs();
            if (Array.IndexOf(args, "--mxh-pickup-loop") < 0) return;
            int index = Array.IndexOf(args, "--mxh-smoke-output");
            if (index < 0 || index + 1 >= args.Length) { Application.Quit(1); return; }
            var probe = new GameObject("ExplicitPickupLoopProbe").AddComponent<DevelopmentPickupLoopProbe>();
            probe.output = Path.GetFullPath(args[index + 1]);
            try { probe.Begin(); }
            catch (Exception e) { probe.Fail(e); }
        }

        private void Begin()
        {
            Directory.CreateDirectory(output);
            events = new StreamWriter(Path.Combine(output, "pickup-events.jsonl")) { AutoFlush = true };
            report.phase = Environment.GetEnvironmentVariable("MXH_SMOKE_PICKUP_PHASE");
            report.runId = Environment.GetEnvironmentVariable("MXH_RUN_ID");
            if (report.phase != "first" && report.phase != "second" && report.phase != "verify")
                throw new InvalidOperationException("Unknown pickup phase.");
            if (Environment.GetEnvironmentVariable("MXH_SMOKE_PICKUP_LOOP") != "1")
                throw new InvalidOperationException("Isolated harness opt-in required.");
            string port = Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT");
            if (!ushort.TryParse(port, out ushort number) || number == 0) throw new InvalidOperationException("Invalid loopback port.");
            panel = FindFirstObjectByType<ConnectionPanel>();
            if (panel == null) throw new InvalidOperationException("Connection panel missing.");
            if (panel.mapVisuals == null) throw new InvalidOperationException("Map presentation controller missing.");
            panel.CoreEventReceived += Observe;
            overallDeadline = Time.realtimeSinceStartup + 210;
            Stage("login", 30);
            // Deliberately no host override; the isolated harness owns all three loopback servers.
            panel.BeginDevelopmentProbe("127.0.0.1", port,
                Environment.GetEnvironmentVariable("MXH_SMOKE_USER"), Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD"));
            Environment.SetEnvironmentVariable("MXH_SMOKE_PASSWORD", null);
        }

        private void Stage(string stage, float seconds)
        {
            report.stage = stage;
            deadline = Time.realtimeSinceStartup + seconds;
        }

        private void Observe(CoreEvent e)
        {
            if (finished) return;
            // No credentials or chat text; native sequence/generations retain protocol evidence.
            if (e.type == NativeClient.EventSkillHit || e.type == NativeClient.EventEntityLife ||
                e.type == NativeClient.EventGroundDrop || e.type == NativeClient.EventPickupConfirmed ||
                e.type == NativeClient.EventInventory || e.type == NativeClient.EventDisconnected)
                events.WriteLine(JsonUtility.ToJson(new EventEvidence(e)));
            if (e.type == NativeClient.EventDisconnected) report.disconnected = true;
            if (e.state != CoreState.InGame || e.sessionGeneration != panel.Observed.sessionGeneration ||
                e.mapGeneration != panel.Observed.mapGeneration) return;
            if (e.type == NativeClient.EventSkillHit && e.argument0 == Target && e.argument1 > 0 && e.reserved0 != 0)
                report.hit = true;
            if (e.type == NativeClient.EventEntityLife && e.argument0 == Target && e.argument1 == 0)
                report.zeroLife = true;
            if (e.type == NativeClient.EventGroundDrop && report.zeroLife && report.dropId == 0)
            {
                string[] fields = e.Text.Split(',');
                if (fields.Length != 3 || !uint.TryParse(fields[0], out uint count) || count == 0) return;
                report.dropId = e.argument0; report.itemId = e.argument1; report.count = count;
                report.dropObserved = report.dropId != 0 && report.itemId != 0;
            }
            if (e.type == NativeClient.EventPickupConfirmed && e.argument0 == report.dropId && report.pickupSubmitted)
            {
                if (e.result != CoreResult.Ok) { Fail(new InvalidOperationException("Authoritative pickup rejection.")); return; }
                if (e.argument1 != report.itemId || e.Text != report.count.ToString(CultureInfo.InvariantCulture))
                { Fail(new InvalidOperationException("Pickup acknowledgement identity/count mismatch.")); return; }
                report.pickupAck = true; ackSequence = e.sequence;
            }
            if (e.type == NativeClient.EventInventory && panel.Inventory.Matches(panel.Observed)) inventorySequence = e.sequence;
        }

        [Serializable] private sealed class EventEvidence
        {
            public ulong sequence, session, map;
            public uint type, argument0, argument1, reserved0;
            public string result;
            public EventEvidence(CoreEvent e) { sequence = e.sequence; session = e.sessionGeneration; map = e.mapGeneration;
                type = e.type; argument0 = e.argument0; argument1 = e.argument1; reserved0 = e.reserved0; result = e.result.ToString(); }
        }

        private Item[] Inventory() => panel.Inventory.Slots.Where(i => i.DatabaseId != 0).Select(i => new Item {
            databaseId = i.DatabaseId, itemId = i.ItemId, position = i.Position, durability = i.Durability,
            rareId = i.RareId, quickPosition = i.QuickPosition, itemParameter = i.ItemParameter }).ToArray();

        private void Update()
        {
            if (finished) return;
            try { Step(); } catch (Exception e) { Fail(e); }
        }

        private void Step()
        {
            if (Time.realtimeSinceStartup > deadline || Time.realtimeSinceStartup > overallDeadline)
                throw new TimeoutException("Evidence deadline exceeded in " + report.stage);
            if (panel.Observed.state == CoreState.Failed) throw new InvalidOperationException(panel.Observed.Error);
            if (report.stage == "login")
            {
                if (!selected && panel.Observed.state == CoreState.CharacterListReady && panel.Observed.characterCount > 0)
                { panel.SelectFirstForDevelopmentProbe(); selected = true; }
                if (panel.Observed.state != CoreState.InGame || !panel.Inventory.Matches(panel.Observed) || !panel.MapPresentationReady) return;
                report.playerId = panel.Observed.game.playerId; report.mapNumber = panel.Observed.game.mapNumber;
                if (report.playerId != 111 || report.mapNumber != 10) throw new InvalidOperationException("Unexpected isolated fixture identity/map.");
                report.gameIn = report.presentationReady = true; report.before = Inventory();
                if (report.phase == "verify") { report.after = Inventory(); Disconnect(); }
                else Stage("combat", 120);
                return;
            }
            if (report.stage == "disconnect")
            {
                if (report.disconnected && panel.Observed.state == CoreState.Idle) Finish(true);
                return;
            }
            if (panel.Observed.state != CoreState.InGame || panel.Observed.game.life == 0)
                throw new InvalidOperationException("Lost live InGame state; no revive or success substitution.");
            if (report.stage == "combat")
            {
                if (report.zeroLife && report.hit) { Stage("pickup", 30); return; }
                if (Time.realtimeSinceStartup < nextCommand) return;
                nextCommand = Time.realtimeSinceStartup + 0.8f;
                var target = FindObjectsByType<TargetSelectable>(FindObjectsSortMode.None).FirstOrDefault(t =>
                    t.objectId == Target && t.GetComponentInParent<ServerEntityRegistry>() != null);
                if (target == null) return;
                var position = new MapCoordinates(51200, 51200).ToGame(target.transform.position);
                report.skillAttempts++;
                report.lastSkillSubmission = panel.UseSkill(1, Target, position.x, position.z).ToString(); // Submission alone is never evidence of a hit.
            }
            else if (report.stage == "pickup")
            {
                if (!report.dropObserved || report.pickupSubmitted) return;
                var pickup = FindObjectsByType<GroundDropPickup>(FindObjectsSortMode.None).FirstOrDefault(p =>
                    p.drop != null && p.drop.ObjectId == report.dropId && p.GetComponentInParent<ServerEntityRegistry>() != null);
                if (pickup == null) return;
                var position = new MapCoordinates(51200, 51200).ToGame(pickup.transform.position);
                float dx = position.x - panel.Observed.game.positionX, dz = position.z - panel.Observed.game.positionZ;
                if (dx * dx + dz * dz > 400 * 400)
                {
                    if (Time.realtimeSinceStartup >= nextCommand) { panel.Move(checked((ushort)position.x), checked((ushort)position.z), false); nextCommand = Time.realtimeSinceStartup + 1; }
                    return;
                }
                if (pickup.RequestPickup() != CoreResult.Ok) throw new InvalidOperationException("Pickup component did not submit.");
                report.pickupSubmitted = true; Stage("inventory", 15);
            }
            else if (report.stage == "inventory" && report.pickupAck && inventorySequence > ackSequence && panel.Inventory.Matches(panel.Observed))
            {
                report.after = Inventory();
                var oldIds = new HashSet<uint>(report.before.Select(i => i.databaseId));
                var added = report.after.Where(i => !oldIds.Contains(i.databaseId)).ToArray();
                if (added.Length != 1 || added[0].itemId != report.itemId || added[0].itemParameter != report.count)
                    throw new InvalidOperationException("Fresh authoritative inventory lacks exactly one matching new item.");
                report.acquiredDatabaseId = added[0].databaseId; report.inventoryFresh = true;
                Disconnect();
            }
        }

        private void Disconnect()
        {
            Stage("disconnect", 10);
            panel.disconnect.onClick.Invoke();
        }
        private void Fail(Exception e) { report.error = e.GetType().Name + ": " + e.Message; Finish(false); }
        private void Finish(bool passed)
        {
            if (finished) return;
            finished = true; report.passed = passed;
            if (panel != null) panel.CoreEventReceived -= Observe;
            events?.Dispose();
            try { File.WriteAllText(Path.Combine(output, "pickup-report.json"), JsonUtility.ToJson(report, true)); }
            catch (Exception e) { Debug.LogError("Pickup evidence write failed: " + e.GetType().Name); passed = false; }
            Application.Quit(passed ? 0 : 1);
        }
    }
}
