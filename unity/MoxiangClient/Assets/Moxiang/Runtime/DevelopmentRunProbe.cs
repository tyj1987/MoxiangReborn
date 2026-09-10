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
            public bool characterCreated;
            public bool movementRequested, movementPassed;
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
            if (report.networkRequested)
            {
                var panel = FindFirstObjectByType<ConnectionPanel>();
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
                        break;
                    }
                    if (panel.Observed.state == CoreState.Failed) { report.error = panel.Observed.Error; break; }
                    yield return null;
                }
                if (!report.gameInReached && string.IsNullOrEmpty(report.error)) report.error = "GameIn acknowledgement not reached within 30 seconds.";
                if (report.gameInReached && report.movementRequested)
                    yield return DevelopmentMovementProbe.Run(panel, ushort.Parse(loginPort), observerAccount, observerSecret,
                        (passed, error) => { report.movementPassed = passed; if (!passed) report.error = error; });
                observerSecret = null;
            }
            for (int i = 0; i < 120; ++i) yield return null;
            yield return new WaitForEndOfFrame();
            try
            {
                report.frame = Time.frameCount; report.width = Screen.width; report.height = Screen.height;
                var panel = FindFirstObjectByType<ConnectionPanel>();
                report.status = panel == null ? "missing panel" : panel.status.text;
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
