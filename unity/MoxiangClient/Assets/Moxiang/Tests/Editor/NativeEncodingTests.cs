using System;
using System.Runtime.InteropServices;
using System.Text;
using NUnit.Framework;

namespace Moxiang.Tests
{
    public sealed class NativeEncodingTests
    {
        [Test]
        public void CredentialLimitIsUtf8BytesAndRejectsNul()
        {
            byte[] value = NativeClient.Encode(new string('墨', 21), 64, out uint count);
            Assert.That(count, Is.EqualTo(63));
            Assert.That(value[63], Is.Zero);
            Assert.Throws<ArgumentException>(() => NativeClient.Encode(new string('墨', 22), 64, out _));
            Assert.Throws<ArgumentException>(() => NativeClient.Encode("a\0b", 64, out _));
            Assert.Throws<ArgumentException>(() => NativeClient.Encode(new string('墨', 6), 64, out _, 17));
            Assert.That(NativeClient.Encode(new string('a', 17), 64, out _, 17).Length, Is.EqualTo(64));
        }

        [Test]
        public void InvalidUtf8IsNotSilentlyReplaced()
        {
            Assert.Throws<DecoderFallbackException>(() => NativeClient.Decode(new byte[] { 0xff, 0 }, 1));
            Assert.Throws<InvalidOperationException>(() => NativeClient.Decode(new byte[] { 65 }, 1));
            Assert.That(NativeClient.Decode(new byte[] { 0xe5, 0xa2, 0xa8, 0 }, 3), Is.EqualTo("墨"));
        }

        [Test]
        public void PackedLayoutMatchesNativeV1Contract()
        {
            Assert.That(Marshal.SizeOf<CoreCharacter>(), Is.EqualTo(108));
            Assert.That(Marshal.SizeOf<CoreGame>(), Is.EqualTo(140));
            Assert.That(Marshal.SizeOf<CoreEvent>(), Is.EqualTo(320));
            Assert.That(Marshal.SizeOf<CoreSnapshot>(), Is.EqualTo(1000));
            Assert.That(Marshal.OffsetOf<CoreSnapshot>(nameof(CoreSnapshot.sessionGeneration)).ToInt32(), Is.EqualTo(24));
            Assert.That(Marshal.OffsetOf<CoreSnapshot>(nameof(CoreSnapshot.characters)).ToInt32(), Is.EqualTo(60));
        }
    }
}
