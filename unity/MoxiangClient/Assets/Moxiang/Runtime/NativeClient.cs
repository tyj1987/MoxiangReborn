using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Moxiang
{
    public enum CoreResult : uint { Ok, InvalidArgument, InvalidHandle, WrongState, NetworkError, ProtocolError, BufferTooSmall, Unsupported, NotReady, InternalError, Rejected }
    public enum CoreState : uint { Idle, LoginConnecting, AwaitLoginAck, AgentConnecting, AwaitCharacterList, CharacterListReady, AwaitCharacterSelect, AwaitGameIn, InGame, Failed, ShuttingDown, AwaitCharacterCreate }
    public enum LegacyTextEncoding : uint { Utf8 = 0, Korean949 = 2, Chinese936 = 4 }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct CoreCharacter
    {
        public uint characterId, valid;
        public ushort level, mapNumber;
        public byte gender, faceType, hairType, reserved0;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 10)] public ushort[] wornItems;
        public uint nameLength;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 65)] public byte[] name;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)] public byte[] reserved1;
        public string DisplayName => NativeClient.Decode(name, nameLength);
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct CoreGame
    {
        public uint playerId, userId, nameLength;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 65)] public byte[] name;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)] public byte[] reserved0;
        public ushort level, mapNumber;
        public uint life, maxLife, mp, maxMp;
        public ulong experience;
        public uint money;
        public ushort genGol, minChub, cheRyuk, simMek, positionX, positionZ;
        public ushort serverYear, serverMonth, serverDay, serverHour;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct CoreSnapshot
    {
        public uint structSize, apiVersion;
        public CoreState state;
        public CoreResult lastResult;
        public ulong revision, sessionGeneration, mapGeneration;
        public uint userId, selectedCharacterId;
        public ushort selectedMapNumber, characterCount;
        public ulong droppedEventCount;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)] public CoreCharacter[] characters;
        public CoreGame game;
        public uint errorLength;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)] public byte[] error;
        public string Error => NativeClient.Decode(error, errorLength);
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct CoreEvent
    {
        public uint structSize, type;
        public CoreResult result;
        public CoreState state;
        public ulong sequence, requestId, sessionGeneration, mapGeneration;
        public uint argument0, argument1, textLength, reserved0;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)] public byte[] text;
        public string Text => NativeClient.Decode(text, textLength);
    }

    /// <summary>Owns one native session. Call on the Unity main thread and Dispose before domain reload.</summary>
    public sealed class NativeClient : IDisposable
    {
        public const uint ApiVersion = 0x00010002;
        private const string Library = "mxh_unity_core";
        private static readonly UTF8Encoding Utf8 = new UTF8Encoding(false, true);
        private readonly object gate = new object();
        private ulong handle;
        private ulong nextRequestId;

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        private struct ConnectArgs
        {
            public uint structSize, flags, timeoutMs;
            public ushort loginPort, reserved0;
            public uint hostLength, userLength, passwordLength;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)] public byte[] host;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)] public byte[] user;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)] public byte[] password;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        private struct Command
        {
            public uint structSize, type, payloadSize, reserved0, argument0, argument1;
            public ulong requestId, expectedSessionGeneration, expectedMapGeneration;
            public uint nameLength;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 65)] public byte[] name;
            public byte sex, hair, face, clothOption, bootOption, weaponOption;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)] public byte[] reserved1;
        }

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] private static extern uint mxh_unity_get_api_version();
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] private static extern CoreResult mxh_unity_create(out ulong session);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] private static extern CoreResult mxh_unity_destroy(ulong session);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] private static extern CoreResult mxh_unity_connect(ulong session, ref ConnectArgs args);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] private static extern CoreResult mxh_unity_disconnect(ulong session);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] private static extern CoreResult mxh_unity_tick(ulong session);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] private static extern CoreResult mxh_unity_submit_command(ulong session, ref Command command);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] private static extern CoreResult mxh_unity_poll_event(ulong session, IntPtr buffer, uint size, out uint required);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] private static extern CoreResult mxh_unity_copy_snapshot(ulong session, IntPtr buffer, uint size, out uint required);

        public NativeClient()
        {
            if (Marshal.SizeOf<Command>() != 128 || Marshal.OffsetOf<Command>(nameof(Command.name)).ToInt32() != 52)
                throw new InvalidOperationException("Managed command ABI layout mismatch.");
            if (IntPtr.Size != 8) throw new PlatformNotSupportedException("Moxiang core requires a 64-bit process.");
            if (mxh_unity_get_api_version() != ApiVersion) throw new InvalidOperationException("Native core API version mismatch.");
            Check(mxh_unity_create(out handle));
            if (handle == 0) throw new InvalidOperationException("Native core returned an invalid handle.");
        }

        public CoreResult Connect(string host, ushort port, string user, string password, bool useHsel = true, LegacyTextEncoding textEncoding = LegacyTextEncoding.Utf8)
        {
            if (port == 0) throw new ArgumentOutOfRangeException(nameof(port));
            if (!Enum.IsDefined(typeof(LegacyTextEncoding), textEncoding)) throw new ArgumentOutOfRangeException(nameof(textEncoding));
            var args = new ConnectArgs { structSize = (uint)Marshal.SizeOf<ConnectArgs>(), flags = (useHsel ? 1u : 0u) | (uint)textEncoding,
                timeoutMs = 15000, loginPort = port };
            args.host = Encode(host, 256, out args.hostLength);
            args.user = Encode(user, 64, out args.userLength, 17);
            args.password = Encode(password, 64, out args.passwordLength, 17);
            try { lock (gate) { EnsureAlive(); return mxh_unity_connect(handle, ref args); } }
            finally { Array.Clear(args.password, 0, args.password.Length); }
        }

        public CoreResult Tick() { lock (gate) { EnsureAlive(); return mxh_unity_tick(handle); } }
        public CoreResult Disconnect() { lock (gate) { EnsureAlive(); return mxh_unity_disconnect(handle); } }

        public CoreResult SelectCharacter(uint characterId, uint channel, CoreSnapshot observed)
        {
            lock (gate)
            {
                EnsureAlive();
                var command = new Command { structSize = (uint)Marshal.SizeOf<Command>(), type = 1,
                    argument0 = characterId, argument1 = channel, requestId = ++nextRequestId,
                    expectedSessionGeneration = observed.sessionGeneration, expectedMapGeneration = observed.mapGeneration,
                    name = new byte[65], reserved1 = new byte[5] };
                return mxh_unity_submit_command(handle, ref command);
            }
        }

        public CoreResult CreateCharacter(string name, byte sex, byte hair, byte face, byte cloth, byte boots, byte weapon, CoreSnapshot observed)
        {
            byte[] encoded = Encode(name, 65, out uint length, 64);
            lock (gate)
            {
                EnsureAlive();
                var command = new Command { structSize = (uint)Marshal.SizeOf<Command>(), type = 2, payloadSize = 80,
                    requestId = ++nextRequestId, expectedSessionGeneration = observed.sessionGeneration,
                    expectedMapGeneration = observed.mapGeneration, nameLength = length, name = encoded,
                    sex = sex, hair = hair, face = face, clothOption = cloth, bootOption = boots, weaponOption = weapon,
                    reserved1 = new byte[5] };
                return mxh_unity_submit_command(handle, ref command);
            }
        }

        // Sends a predicted current position. The legacy server sends only
        // corrections to us; successful sends do not confirm acceptance.
        public CoreResult Move(ushort x, ushort z, bool stop, CoreSnapshot observed)
        {
            lock (gate)
            {
                EnsureAlive();
                var command = new Command { structSize = (uint)Marshal.SizeOf<Command>(), type = stop ? 4u : 3u,
                    argument0 = x, argument1 = z, requestId = ++nextRequestId,
                    expectedSessionGeneration = observed.sessionGeneration, expectedMapGeneration = observed.mapGeneration,
                    name = new byte[65], reserved1 = new byte[5] };
                return mxh_unity_submit_command(handle, ref command);
            }
        }

        public CoreSnapshot Snapshot()
        {
            lock (gate)
            {
                EnsureAlive();
                int size = Marshal.SizeOf<CoreSnapshot>();
                var buffer = Marshal.AllocHGlobal(size);
                try
                {
                    Check(mxh_unity_copy_snapshot(handle, buffer, (uint)size, out uint required));
                    if (required != size) throw new InvalidOperationException("Native snapshot size mismatch.");
                    var value = Marshal.PtrToStructure<CoreSnapshot>(buffer);
                    if (value.structSize != size || value.apiVersion != ApiVersion || value.characterCount > 5)
                        throw new InvalidOperationException("Invalid native snapshot header.");
                    return value;
                }
                finally { Marshal.FreeHGlobal(buffer); }
            }
        }

        public bool PollEvent(out CoreEvent value)
        {
            lock (gate)
            {
                EnsureAlive();
                int size = Marshal.SizeOf<CoreEvent>();
                var buffer = Marshal.AllocHGlobal(size);
                try
                {
                    var result = mxh_unity_poll_event(handle, buffer, (uint)size, out uint required);
                    if (result == CoreResult.NotReady) { value = default; return false; }
                    Check(result);
                    if (required != size) throw new InvalidOperationException("Native event size mismatch.");
                    value = Marshal.PtrToStructure<CoreEvent>(buffer);
                    if (value.structSize != size) throw new InvalidOperationException("Invalid native event header.");
                    return true;
                }
                finally { Marshal.FreeHGlobal(buffer); }
            }
        }

        public void Dispose()
        {
            lock (gate)
            {
                if (handle == 0) return;
                var owned = handle;
                handle = 0;
                Check(mxh_unity_destroy(owned));
            }
            GC.SuppressFinalize(this);
        }

        ~NativeClient()
        {
            // Last-resort owner cleanup; Unity components still dispose deterministically.
            try { if (handle != 0) mxh_unity_destroy(handle); } catch { }
        }

        private void EnsureAlive() { if (handle == 0) throw new ObjectDisposedException(nameof(NativeClient)); }
        private static void Check(CoreResult result) { if (result != CoreResult.Ok) throw new InvalidOperationException("Native core: " + result); }
        public static byte[] Encode(string value, int capacity, out uint length, uint maximumBytes = uint.MaxValue)
        {
            if (string.IsNullOrEmpty(value) || value.IndexOf('\0') >= 0) throw new ArgumentException("Value must be nonempty and contain no NUL.");
            int count = Utf8.GetByteCount(value);
            if (count >= capacity || count > maximumBytes) throw new ArgumentException("Value exceeds the native UTF-8 byte limit.");
            var bytes = new byte[capacity];
            Utf8.GetBytes(value, 0, value.Length, bytes, 0);
            length = (uint)count;
            return bytes;
        }
        public static string Decode(byte[] bytes, uint length)
        {
            if (bytes == null || length >= bytes.Length) throw new InvalidOperationException("Invalid native UTF-8 string length.");
            return Utf8.GetString(bytes, 0, checked((int)length));
        }
    }
}
