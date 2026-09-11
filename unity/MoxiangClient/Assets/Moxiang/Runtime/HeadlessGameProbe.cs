using System.Collections;
using UnityEngine;

namespace Moxiang
{
    /// <summary>
    /// Headless smoke for production / CI: connects to a real
    /// DistributeServer using credentials from environment variables, drives
    /// the first character into MapServer, and prints periodic status
    /// updates so an operator can confirm the live wiring.
    ///
    /// Activated by MXH_HEADLESS_PROBE=1. Required env:
    ///   MXH_LOGIN_HOST  (default 127.0.0.1)
    ///   MXH_LOGIN_PORT  (default 6001)
    ///   MXH_LOGIN_USER  (required)
    ///   MXH_LOGIN_PASS  (required)
    ///   MXH_HEADLESS_QUIT_SECONDS  (default 0 = run until terminated)
    /// </summary>
    public sealed class HeadlessGameProbe : MonoBehaviour
    {
        private NativeClient client;
        private string failure;
        private float lastPrint;

        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void AutoSpawn()
        {
            if (System.Environment.GetEnvironmentVariable("MXH_HEADLESS_PROBE") != "1") return;
            var go = new GameObject("Moxiang.HeadlessGameProbe");
            DontDestroyOnLoad(go);
            go.AddComponent<HeadlessGameProbe>();
        }

        private void Start()
        {
            if (System.Environment.GetEnvironmentVariable("MXH_HEADLESS_PROBE") != "1") return;
            try
            {
                var host = System.Environment.GetEnvironmentVariable("MXH_LOGIN_HOST") ?? "127.0.0.1";
                var portStr = System.Environment.GetEnvironmentVariable("MXH_LOGIN_PORT") ?? "6001";
                var user = System.Environment.GetEnvironmentVariable("MXH_LOGIN_USER") ?? "";
                var pass = System.Environment.GetEnvironmentVariable("MXH_LOGIN_PASS") ?? "";
                if (user.Length == 0 || pass.Length == 0)
                {
                    Debug.LogError("MXH_HEADLESS_PROBE: MXH_LOGIN_USER and MXH_LOGIN_PASS must be set");
                    Application.Quit(2);
                    return;
                }
                if (!ushort.TryParse(portStr, out var port))
                {
                    Debug.LogError("MXH_HEADLESS_PROBE: invalid MXH_LOGIN_PORT=" + portStr);
                    Application.Quit(2);
                    return;
                }
                client = new NativeClient();
                var result = client.Connect(host, port, user, pass, useHsel: true,
                                              textEncoding: LegacyTextEncoding.Utf8);
                if (result != CoreResult.Ok)
                {
                    Debug.LogError("MXH_HEADLESS_PROBE: Connect failed: " + result);
                    Application.Quit(2);
                    return;
                }
                Debug.Log("MXH_HEADLESS_PROBE: connected to " + host + ":" + port +
                          " as user " + user);
                lastPrint = Time.realtimeSinceStartup;
                StartCoroutine(TickAndMaybeQuit());
            }
            catch (System.Exception e)
            {
                Debug.LogError("MXH_HEADLESS_PROBE: " + e.GetType().Name + ": " + e.Message);
                Application.Quit(3);
            }
        }

        private IEnumerator TickAndMaybeQuit()
        {
            var quitSeconds = 0f;
            float.TryParse(System.Environment.GetEnvironmentVariable("MXH_HEADLESS_QUIT_SECONDS") ?? "0",
                           out quitSeconds);
            var deadline = quitSeconds > 0 ? Time.realtimeSinceStartup + quitSeconds : -1f;
            bool selected = false;
            uint pickedCharacter = 0;
            while (true)
            {
                if (client == null) yield break;
                try { client.Tick(); } catch (System.Exception e)
                {
                    Debug.LogError("MXH_HEADLESS_PROBE: tick failed: " + e.Message);
                    Application.Quit(3);
                    yield break;
                }
                var snap = client.Snapshot();
                // Drive the next state once the modern core has caught up.
                if (!selected && snap.state == CoreState.CharacterListReady &&
                    snap.characterCount > 0)
                {
                    pickedCharacter = snap.characters[0].characterId;
                    Debug.Log("MXH_HEADLESS_PROBE: selecting characterId=" + pickedCharacter);
                    var r = client.SelectCharacter(pickedCharacter, 0, snap);
                    if (r != CoreResult.Ok)
                        Debug.LogError("MXH_HEADLESS_PROBE: SelectCharacter failed: " + r);
                    selected = true;
                }
                if (Time.realtimeSinceStartup - lastPrint >= 1f)
                {
                    lastPrint = Time.realtimeSinceStartup;
                    Debug.Log(string.Format(
                        "MXH_HEADLESS_PROBE: state={0} result={1} chars={2} map={3} x={4} z={5} selected={6}",
                        snap.state, snap.lastResult, snap.characterCount,
                        snap.game.mapNumber, snap.game.positionX, snap.game.positionZ,
                        pickedCharacter));
                }
                if (snap.state == CoreState.InGame)
                {
                    Debug.Log("MXH_HEADLESS_PROBE: in-game reached map=" + snap.game.mapNumber +
                              " x=" + snap.game.positionX + " z=" + snap.game.positionZ);
                }
                if (deadline > 0 && Time.realtimeSinceStartup >= deadline)
                {
                    Debug.Log("MXH_HEADLESS_PROBE: deadline reached, quitting");
                    client.Disconnect();
                    Application.Quit(0);
                    yield break;
                }
                yield return null;
            }
        }

        private void OnDestroy()
        {
            try { client?.Disconnect(); } catch { }
        }
    }
}