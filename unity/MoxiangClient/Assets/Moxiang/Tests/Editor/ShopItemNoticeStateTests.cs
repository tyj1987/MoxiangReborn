using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class ShopItemNoticeStateTests
    {
        private static CoreSnapshot Snapshot(ulong session = 3, ulong map = 7)
        {
            return new CoreSnapshot {
                state = CoreState.InGame,
                sessionGeneration = session,
                mapGeneration = map,
                game = new CoreGame { playerId = 7001 }
            };
        }

        private static CoreEvent Notice(uint type, uint protocol, ulong sequence, ulong session = 3, ulong map = 7)
        {
            return new CoreEvent {
                type = type,
                result = CoreResult.Ok,
                state = CoreState.InGame,
                sequence = sequence,
                sessionGeneration = session,
                mapGeneration = map,
                argument0 = 55001,
                reserved0 = protocol,
                textLength = 0
            };
        }

        [Test]
        public void ProtectionNoticeAllowsZeroCountAndRejectsStaleOrWrongIdentity()
        {
            var state = new ShopItemNoticeState();
            state.Observe(Snapshot());
            var e = Notice(NativeClient.EventShopProtection, 150, 1);
            e.argument0 = 0;
            Assert.That(state.Accept(e), Is.True);
            Assert.That(state.Message, Is.EqualTo("综合保护剩余 0 次。"));
            Assert.That(state.Accept(e), Is.False);
            e.sequence = 2; e.reserved0 = 109; e.argument0 = 55312;
            Assert.That(state.Accept(e), Is.False);
            e.argument0 = 55311;
            Assert.That(state.Accept(e), Is.True);
            Assert.That(state.Message, Is.EqualTo("金钱保护已使用。"));
            e.sequence = 3; e.mapGeneration = 6;
            Assert.That(state.Accept(e), Is.False);
        }

        [Test]
        public void AcceptsOriginalOneMinuteAndUseEndNotifications()
        {
            var state = new ShopItemNoticeState();
            state.Observe(Snapshot());
            Assert.That(state.Accept(Notice(NativeClient.EventShopItemOneMinute,
                NativeClient.ProtocolShopItemOneMinute, 10)), Is.True);
            Assert.That(state.Kind, Is.EqualTo(ShopItemNoticeKind.OneMinute));
            Assert.That(state.ItemId, Is.EqualTo(55001));
            StringAssert.Contains("1 分钟", state.Message);

            Assert.That(state.Accept(Notice(NativeClient.EventShopItemUseEnd,
                NativeClient.ProtocolShopItemUseEnd, 11)), Is.True);
            Assert.That(state.Kind, Is.EqualTo(ShopItemNoticeKind.UseEnd));
            StringAssert.Contains("已到期", state.Message);
        }

        [Test]
        public void RejectsStaleGenerationReplayAndMalformedIdentity()
        {
            var state = new ShopItemNoticeState();
            state.Observe(Snapshot());
            Assert.That(state.Accept(Notice(NativeClient.EventShopItemOneMinute,
                NativeClient.ProtocolShopItemOneMinute, 10, map: 6)), Is.False);
            Assert.That(state.Accept(Notice(NativeClient.EventShopItemOneMinute,
                NativeClient.ProtocolShopItemOneMinute, 10)), Is.True);
            Assert.That(state.Accept(Notice(NativeClient.EventShopItemUseEnd,
                NativeClient.ProtocolShopItemUseEnd, 10)), Is.False);
            var malformed = Notice(NativeClient.EventShopItemUseEnd,
                NativeClient.ProtocolShopItemUseEnd, 11);
            malformed.argument0 = 0x10000;
            Assert.That(state.Accept(malformed), Is.False);

            state.Observe(Snapshot(session: 4));
            Assert.That(state.Kind, Is.EqualTo(ShopItemNoticeKind.None));
            Assert.That(state.Message, Is.Null);
        }

        [Test]
        public void RejectsWrongStateSessionProtocolAndUnexpectedPayload()
        {
            var state = new ShopItemNoticeState();
            state.Observe(Snapshot());

            Assert.That(state.Accept(Notice(NativeClient.EventShopItemOneMinute,
                NativeClient.ProtocolShopItemOneMinute, 10, session: 2)), Is.False);

            var wrongProtocol = Notice(NativeClient.EventShopItemUseEnd,
                NativeClient.ProtocolShopItemOneMinute, 11);
            Assert.That(state.Accept(wrongProtocol), Is.False);

            var unexpectedArgument = Notice(NativeClient.EventShopItemUseEnd,
                NativeClient.ProtocolShopItemUseEnd, 12);
            unexpectedArgument.argument1 = 1;
            Assert.That(state.Accept(unexpectedArgument), Is.False);

            var unexpectedText = Notice(NativeClient.EventShopItemUseEnd,
                NativeClient.ProtocolShopItemUseEnd, 13);
            unexpectedText.textLength = 1;
            Assert.That(state.Accept(unexpectedText), Is.False);

            var notInGame = Snapshot();
            notInGame.state = CoreState.Failed;
            state.Observe(notInGame);
            Assert.That(state.Accept(Notice(NativeClient.EventShopItemUseEnd,
                NativeClient.ProtocolShopItemUseEnd, 14)), Is.False);
        }

        [Test]
        public void ClearDropsAcceptedNotice()
        {
            var state = new ShopItemNoticeState();
            state.Observe(Snapshot());
            Assert.That(state.Accept(Notice(NativeClient.EventShopItemOneMinute,
                NativeClient.ProtocolShopItemOneMinute, 10)), Is.True);

            state.Clear();

            Assert.That(state.Kind, Is.EqualTo(ShopItemNoticeKind.None));
            Assert.That(state.Message, Is.Null);
            Assert.That(state.ItemId, Is.Zero);
        }
    }
}
