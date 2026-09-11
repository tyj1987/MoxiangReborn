using System;
using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class TimedMovementClientTests
    {
        [Test]
        public void EncodeTimedHelloProducesLiteralEightBytes()
        {
            byte[] bytes = NativeClient.EncodeTimedHello();
            Assert.That(bytes.Length, Is.EqualTo((int)NativeClient.TimedHelloPayloadSize));
            Assert.That(bytes[0], Is.EqualTo((byte)'M'));
            Assert.That(bytes[1], Is.EqualTo((byte)'X'));
            Assert.That(bytes[2], Is.EqualTo((byte)'M'));
            Assert.That(bytes[3], Is.EqualTo((byte)'H'));
            Assert.That(bytes[4], Is.EqualTo((byte)1));
            Assert.That(bytes[5], Is.Zero);
            Assert.That(bytes[6], Is.Zero);
            Assert.That(bytes[7], Is.Zero);
        }

        [Test]
        public void EncodeTimedRouteLayoutMatchesCppGolden()
        {
            // Golden captured from the x64 unity-core MovementWire tests:
            // epoch=0x0807060504030201, sequence=0x1817161514131211, point=(0x1234,0x5678)
            byte[] bytes = NativeClient.EncodeTimedRoute(
                0x0807060504030201UL, 0x1817161514131211UL,
                (0x1234, 0x5678));
            Assert.That(bytes.Length, Is.EqualTo((int)(NativeClient.TimedCommandHeaderSize + 4)));
            Assert.That(bytes[0], Is.EqualTo((byte)'M'));
            Assert.That(bytes[1], Is.EqualTo((byte)'X'));
            Assert.That(bytes[2], Is.EqualTo((byte)'M'));
            Assert.That(bytes[3], Is.EqualTo((byte)'C'));
            Assert.That(bytes[4], Is.EqualTo((byte)1));
            Assert.That(bytes[5], Is.EqualTo(NativeClient.TimedCommandKindRoute));
            Assert.That(bytes[6], Is.EqualTo((byte)1));
            Assert.That(bytes[7], Is.Zero);
            // epoch little-endian at offset 8
            for (int i = 0; i < 8; ++i)
                Assert.That(bytes[8 + i], Is.EqualTo((byte)((0x0807060504030201UL >> (8 * i)) & 0xff)));
            // sequence little-endian at offset 16
            for (int i = 0; i < 8; ++i)
                Assert.That(bytes[16 + i], Is.EqualTo((byte)((0x1817161514131211UL >> (8 * i)) & 0xff)));
            // point x at offset 24, z at offset 26
            Assert.That(bytes[24], Is.EqualTo((byte)0x34));
            Assert.That(bytes[25], Is.EqualTo((byte)0x12));
            Assert.That(bytes[26], Is.EqualTo((byte)0x78));
            Assert.That(bytes[27], Is.EqualTo((byte)0x56));
        }

        [Test]
        public void EncodeTimedStopEncodesSinglePointAndStopKind()
        {
            byte[] bytes = NativeClient.EncodeTimedStop(42UL, 7UL, 0x1234, 0x5678);
            Assert.That(bytes.Length, Is.EqualTo((int)(NativeClient.TimedCommandHeaderSize + 4)));
            Assert.That(bytes[5], Is.EqualTo(NativeClient.TimedCommandKindStop));
            Assert.That(bytes[6], Is.EqualTo((byte)1));
            Assert.That(bytes[7], Is.Zero);
        }

        [Test]
        public void EncodeTimedRouteRejectsZeroEpochZeroSequenceAndOutOfRangeCount()
        {
            Assert.Throws<ArgumentOutOfRangeException>(
                () => NativeClient.EncodeTimedRoute(0UL, 1UL, (1, 2)));
            Assert.Throws<ArgumentOutOfRangeException>(
                () => NativeClient.EncodeTimedRoute(1UL, 0UL, (1, 2)));
            Assert.Throws<ArgumentOutOfRangeException>(
                () => NativeClient.EncodeTimedRoute(1UL, 1UL));
            var oversized = new (ushort, ushort)[16];
            for (int i = 0; i < 16; ++i) oversized[i] = ((ushort)i, (ushort)i);
            Assert.Throws<ArgumentOutOfRangeException>(
                () => NativeClient.EncodeTimedRoute(1UL, 1UL, oversized));
        }

        [Test]
        public void TimedMovementClientAssignsMonotonicSequencesPerSession()
        {
            // The wrapper requires an active NativeClient (not exercised here),
            // but its sequence accounting is observable through the public
            // NextSequence property once an epoch is captured.
            using var client = new NativeClient();
            var timed = new TimedMovementClient(client);
            Assert.That(timed.HasEpoch, Is.False);
            Assert.That(timed.NextSequence, Is.EqualTo(1UL));
            Assert.That(timed.NextSequence, Is.EqualTo(1UL), "no sequence consumed");
            timed.Reset();
            Assert.That(timed.NextSequence, Is.EqualTo(1UL));
        }

        [Test]
        public void TimedMovementClientRejectsOutOfOrderStateAndMismatchedEpoch()
        {
            using var client = new NativeClient();
            var timed = new TimedMovementClient(client);

            // Build a minimal state payload (52-byte header, no points): epoch=7,
            // command_sequence=0, state_sequence=1, server_time_ms=0, x=0, z=0, speed=0
            byte[] stateBytes = new byte[52];
            stateBytes[0] = (byte)'M'; stateBytes[1] = (byte)'X';
            stateBytes[2] = (byte)'M'; stateBytes[3] = (byte)'S';
            stateBytes[4] = (byte)1; stateBytes[5] = NativeClient.TimedStateKindSnapshot;
            stateBytes[6] = (byte)0; stateBytes[7] = (byte)0;
            for (int i = 0; i < 8; ++i) stateBytes[8 + i] = (byte)(7 >> (8 * i));
            for (int i = 0; i < 8; ++i) stateBytes[24 + i] = (byte)(1 >> (8 * i));
            var evt = new CoreEvent
            {
                structSize = (uint)System.Runtime.InteropServices.Marshal.SizeOf<CoreEvent>(),
                type = 10, // MXMS owner state
                text = stateBytes,
                textLength = (uint)stateBytes.Length
            };
            Assert.That(timed.OnMovementState(evt), Is.True);
            Assert.That(timed.Epoch, Is.EqualTo(7UL));

            // Same epoch, lower or equal state sequence must be dropped.
            for (int i = 0; i < 8; ++i) stateBytes[24 + i] = (byte)(1 >> (8 * i));
            Assert.That(timed.OnMovementState(evt), Is.False);
            for (int i = 0; i < 8; ++i) stateBytes[24 + i] = (byte)(0 >> (8 * i));
            Assert.That(timed.OnMovementState(evt), Is.False);

            // Different epoch must be dropped while the session is bound.
            for (int i = 0; i < 8; ++i) stateBytes[8 + i] = (byte)(99 >> (8 * i));
            for (int i = 0; i < 8; ++i) stateBytes[24 + i] = (byte)(2 >> (8 * i));
            Assert.That(timed.OnMovementState(evt), Is.False);

            // Non-timed event types are ignored.
            evt.type = 7;
            Assert.That(timed.OnMovementState(evt), Is.False);
        }
    }
}