using System;
using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class NativeClientTests
    {
        [Test]
        public void RealNativePluginCreatesSnapshotsAndDisposesRepeatedly()
        {
            for (int i = 0; i < 100; ++i)
            {
                var client = new NativeClient();
                var state = client.Snapshot();
                Assert.That(state.apiVersion, Is.EqualTo(NativeClient.ApiVersion));
                Assert.That(state.state, Is.EqualTo(CoreState.Idle));
                Assert.That(state.characterCount, Is.Zero);
                Assert.That(client.SelectCharacter(1, 0, state), Is.EqualTo(CoreResult.WrongState));
                client.Dispose();
                client.Dispose();
                Assert.Throws<ObjectDisposedException>(() => client.Snapshot());
            }
        }
    }
}
