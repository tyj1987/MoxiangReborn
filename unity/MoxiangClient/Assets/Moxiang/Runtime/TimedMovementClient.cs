using System;

namespace Moxiang
{
    /// <summary>
    /// Minimal managed wrapper around the bounded versioned timed-movement wire.
    /// Owns one (epoch, sequence) pair per Unity session and forwards MXMH /
    /// MXMC frames to the native core via NativeClient.SubmitExtended. The
    /// native core binds the wire epoch to the server-allocated epoch received
    /// in the first state broadcast; this class mirrors that contract by
    /// remembering the last accepted epoch and refusing to send under any
    /// other one.
    /// </summary>
    public sealed class TimedMovementClient
    {
        public const uint CommandHelloTimed = 5;
        public const uint CommandTimedRoute = 6;
        public const uint CommandTimedStop = 7;

        private readonly NativeClient client;
        private ulong epoch;
        private ulong sequence;

        public TimedMovementClient(NativeClient client)
        {
            this.client = client ?? throw new ArgumentNullException(nameof(client));
        }

        public bool HasEpoch => epoch != 0;

        /// <summary>Current server-allocated epoch, or 0 before any hello round-trip.</summary>
        public ulong Epoch => epoch;

        public ulong NextSequence
        {
            get { return sequence == 0 ? 1 : sequence + 1; }
        }

        public CoreResult SubmitHello()
        {
            var payload = NativeClient.EncodeTimedHello();
            var snapshot = client.Snapshot();
            var result = client.SubmitExtended(CommandHelloTimed, payload, 0, snapshot);
            if (result == CoreResult.Ok) sequence = 0; // hello does not consume a sequence
            return result;
        }

        public CoreResult SubmitRoute((ushort x, ushort z)[] points)
        {
            if (points == null || points.Length == 0 || points.Length > 15)
                throw new ArgumentOutOfRangeException(nameof(points));
            if (epoch == 0) throw new InvalidOperationException("Submit hello before route");
            var next = NextSequence;
            var payload = NativeClient.EncodeTimedRoute(epoch, next, points);
            var snapshot = client.Snapshot();
            var result = client.SubmitExtended(CommandTimedRoute, payload, next, snapshot);
            if (result == CoreResult.Ok) sequence = next;
            return result;
        }

        public CoreResult SubmitStop(ushort x, ushort z)
        {
            if (epoch == 0) throw new InvalidOperationException("Submit hello before stop");
            var next = NextSequence;
            var payload = NativeClient.EncodeTimedStop(epoch, next, x, z);
            var snapshot = client.Snapshot();
            var result = client.SubmitExtended(CommandTimedStop, payload, next, snapshot);
            if (result == CoreResult.Ok) sequence = next;
            return result;
        }

        /// <summary>
        /// Inspect a movement state event (event types 10/11). Captures the
        /// server-allocated epoch on first observation, validates monotonic
        /// state sequence, and returns true if the event was accepted. Returns
        /// false for stale or out-of-order states, which the caller should drop
        /// without surfacing to gameplay.
        /// </summary>
        public bool OnMovementState(CoreEvent evt)
        {
            if (evt.type != 10 && evt.type != 11) return false;
            if (evt.text == null || evt.text.Length < NativeClient.TimedCommandHeaderSize)
                return false;
            // Minimum: MXMS|version|kind|count|0 + epoch u64 + cmdseq u64 +
            // stateseq u64 + time u64 + 3 floats = 52 bytes header. The core
            // also rejects short payloads at decode time, but we keep this
            // guard locally for clarity.
            if (evt.text.Length < NativeClient.TimedCommandHeaderSize + 28)
                return false;
            // Decode the 8-byte epoch at offset 8 (little endian).
            ulong observedEpoch = 0;
            for (int i = 0; i < 8; ++i)
                observedEpoch |= ((ulong)evt.text[8 + i]) << (8 * i);
            ulong observedStateSeq = 0;
            for (int i = 0; i < 8; ++i)
                observedStateSeq |= ((ulong)evt.text[24 + i]) << (8 * i);
            if (observedEpoch == 0) return false;
            if (epoch != 0 && observedEpoch != epoch) return false;
            if (observedStateSeq <= sequence) return false;
            epoch = observedEpoch;
            return true;
        }

        public void Reset()
        {
            epoch = 0;
            sequence = 0;
        }
    }
}