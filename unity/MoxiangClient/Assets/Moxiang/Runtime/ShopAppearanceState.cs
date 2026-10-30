using System;
using System.Collections.Generic;

namespace Moxiang
{
    // Receipt of the wire block is not proof of server persistence initialization.
    // Do not use Received as the final character composition readiness gate.
    public sealed class ShopAppearanceState
    {
        public bool Received { get; private set; }
        public IReadOnlyList<ushort> Avatar { get; private set; } = Array.Empty<ushort>();
        public IReadOnlyList<ushort> Skin { get; private set; } = Array.Empty<ushort>();
        public IReadOnlyList<byte> Raw { get; private set; } = Array.Empty<byte>();
        public uint StreetStallDecoration { get; private set; }
        private CoreSnapshot observed;
        private ulong sequence;

        public void Clear()
        {
            Received = false; Avatar = Array.Empty<ushort>(); Skin = Array.Empty<ushort>();
            Raw = Array.Empty<byte>(); StreetStallDecoration = 0; sequence = 0;
        }

        public void Observe(CoreSnapshot snapshot)
        {
            if (snapshot.state != CoreState.InGame || snapshot.sessionGeneration != observed.sessionGeneration ||
                snapshot.mapGeneration != observed.mapGeneration || snapshot.game.playerId != observed.game.playerId)
                Clear();
            observed = snapshot;
        }

        public bool Accept(CoreEvent e)
        {
            if (e.type != NativeClient.EventShopAppearance || observed.state != CoreState.InGame ||
                e.state != CoreState.InGame || e.sessionGeneration != observed.sessionGeneration ||
                e.mapGeneration != observed.mapGeneration || e.argument0 != observed.game.playerId ||
                e.sequence <= sequence) return false;
            if (e.result != CoreResult.Ok || e.argument1 != 1 || e.text == null ||
                e.textLength != 120 || e.text.Length < 120)
            {
                Clear(); sequence = e.sequence; return false;
            }
            var bytes = new byte[120]; Array.Copy(e.text, bytes, 120);
            var avatar = new ushort[23]; var skin = new ushort[5];
            for (int i = 0; i < avatar.Length; ++i) avatar[i] = U16(bytes, i * 2);
            for (int i = 0; i < skin.Length; ++i) skin[i] = U16(bytes, 106 + i * 2);
            Avatar = Array.AsReadOnly(avatar); Skin = Array.AsReadOnly(skin); Raw = Array.AsReadOnly(bytes);
            StreetStallDecoration = (uint)bytes[116] | (uint)bytes[117] << 8 |
                (uint)bytes[118] << 16 | (uint)bytes[119] << 24;
            sequence = e.sequence; Received = true; return true;
        }

        private static ushort U16(byte[] bytes, int offset) => (ushort)(bytes[offset] | bytes[offset + 1] << 8);
    }
}
