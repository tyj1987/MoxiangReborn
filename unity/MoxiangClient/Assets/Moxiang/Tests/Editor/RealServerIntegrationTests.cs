using System;
using System.Diagnostics;
using System.Threading;
using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class RealServerIntegrationTests
    {
        [Test]
        public void RealThreeServerGameInAndReconnect()
        {
            string portText = Environment.GetEnvironmentVariable("MXH_SMOKE_LOGIN_PORT");
            if (string.IsNullOrEmpty(portText)) Assert.Ignore("Requires unity_three_server_smoke.py --editor-test isolated real servers.");
            ushort port = ushort.Parse(portText);
            string user = Environment.GetEnvironmentVariable("MXH_SMOKE_USER");
            string password = Environment.GetEnvironmentVariable("MXH_SMOKE_PASSWORD");
            string createName = Environment.GetEnvironmentVariable("MXH_SMOKE_CREATE_NAME");
            uint characterId = string.IsNullOrEmpty(createName) ? 111u : 0u;
            bool createSubmitted = false, createConfirmed = false;
            using (var client = new NativeClient())
            {
                ulong lastGeneration = 0;
                for (int attempt = 0; attempt < 100; ++attempt)
                {
                    Assert.That(client.Connect("127.0.0.1", port, user, password), Is.EqualTo(CoreResult.Ok));
                    var timer = Stopwatch.StartNew();
                    CoreSnapshot state = default;
                    while (timer.Elapsed < TimeSpan.FromSeconds(30))
                    {
                        client.Tick(); state = client.Snapshot();
                        while (client.PollEvent(out CoreEvent item))
                            if (item.type == 6) { Assert.That(item.result, Is.EqualTo(CoreResult.Ok), item.Text); createConfirmed = true; }
                        if (state.state == CoreState.CharacterListReady)
                        {
                            if (!string.IsNullOrEmpty(createName) && state.characterCount == 0 && !createSubmitted)
                            {
                                Assert.That(client.CreateCharacter(createName, 0, 0, 0, 0, 0, 0, state), Is.EqualTo(CoreResult.Ok));
                                createSubmitted = true; continue;
                            }
                            Assert.That(state.characterCount, Is.EqualTo(1));
                            if (characterId == 0)
                            {
                                Assert.That(state.characters[0].DisplayName, Is.EqualTo(createName));
                                characterId = state.characters[0].characterId;
                                Assert.That(characterId, Is.GreaterThan(0));
                            }
                            Assert.That(client.SelectCharacter(characterId, 0, state), Is.EqualTo(CoreResult.Ok));
                        }
                        if (state.state == CoreState.InGame || state.state == CoreState.Failed) break;
                        Thread.Sleep(10);
                    }
                    Assert.That(state.state, Is.EqualTo(CoreState.InGame), state.Error);
                    Assert.That(state.game.playerId, Is.EqualTo(characterId));
                    if (!string.IsNullOrEmpty(createName)) Assert.That(createConfirmed, Is.True);
                    Assert.That(state.game.mapNumber, Is.EqualTo(10));
                    Assert.That(state.sessionGeneration, Is.GreaterThan(lastGeneration));
                    lastGeneration = state.sessionGeneration;
                    Assert.That(client.Disconnect(), Is.EqualTo(CoreResult.Ok));
                    Assert.That(client.Snapshot().state, Is.EqualTo(CoreState.Idle));
                    Thread.Sleep(100);
                }
            }
        }
    }
}
