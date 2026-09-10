using System;
using System.Collections;
using UnityEngine;

namespace Moxiang
{
    // Explicit opt-in development evidence. Two native sessions in one Player,
    // not two human players and not a movement/collision acceptance certificate.
    public static class DevelopmentMovementProbe
    {
        public static IEnumerator Run(ConnectionPanel panel, ushort port, string account, string secret, Action<bool, string> complete)
        {
            NativeClient observer = null;
            string error = null;
            try { observer = new NativeClient(); if (observer.Connect("127.0.0.1", port, account, secret) != CoreResult.Ok) error = "Observer connect failed."; }
            catch (Exception exception) { error = exception.Message; }
            secret = null;
            if (error != null) { observer?.Dispose(); complete(false, error); yield break; }
            bool corrected = false;
            ushort x = checked((ushort)(panel.Observed.game.positionX + 64));
            ushort z = checked((ushort)(panel.Observed.game.positionZ + 32));
            uint owner = panel.Observed.game.playerId, packed = x | ((uint)z << 16);
            Action<CoreEvent> receive = item => {
                if (item.type == 8 && item.argument0 == owner && item.argument1 == packed && item.reserved0 == 2)
                    corrected = true;
            };
            panel.CoreEventReceived += receive;
            try
            {
                int stage = 0;
                bool success = false;
                float deadline = Time.realtimeSinceStartup + 20, quietUntil = 0;
                while (error == null && Time.realtimeSinceStartup < deadline)
                {
                    try
                    {
                        observer.Tick(); var snapshot = observer.Snapshot();
                        if (snapshot.state == CoreState.Failed || panel.Observed.state != CoreState.InGame)
                            throw new InvalidOperationException("Movement session failed: " + snapshot.Error);
                        if (snapshot.state == CoreState.CharacterListReady) {
                            if (snapshot.characterCount != 1 || observer.SelectCharacter(snapshot.characters[0].characterId, 0, snapshot) != CoreResult.Ok)
                                throw new InvalidOperationException("Observer selection failed.");
                        }
                        if (stage == 0 && snapshot.state == CoreState.InGame) {
                            if (snapshot.game.playerId == owner || snapshot.game.mapNumber != panel.Observed.game.mapNumber)
                                throw new InvalidOperationException("Observer identity/map mismatch.");
                            if (panel.Move(x, z, false) != CoreResult.Ok) throw new InvalidOperationException("Move submission failed.");
                            stage = 1;
                        }
                        while (observer.PollEvent(out var item))
                        {
                            if (item.type != 9 || item.argument0 != owner) continue;
                            if (item.argument1 != packed) throw new InvalidOperationException("Unexpected/invalid remote position.");
                            if (stage == 1 && item.reserved0 == 13) {
                                if (panel.Move(x, z, true) != CoreResult.Ok) throw new InvalidOperationException("Stop submission failed.");
                                stage = 2;
                            } else if (stage == 2 && item.reserved0 == 8) {
                                if (panel.Move(100, 100, false) != CoreResult.Ok) throw new InvalidOperationException("Correction probe submission failed.");
                                stage = 3;
                            } else throw new InvalidOperationException("Duplicate or unexpected movement broadcast.");
                        }
                        if (stage == 3 && corrected) {
                            if (panel.Observed.game.positionX != x || panel.Observed.game.positionZ != z)
                                throw new InvalidOperationException("Correction was not applied to the snapshot.");
                            stage = 4; quietUntil = Time.realtimeSinceStartup + 0.4f;
                        }
                        if (stage == 4 && Time.realtimeSinceStartup >= quietUntil) success = true;
                    }
                    catch (Exception exception) { error = exception.GetType().Name + ": " + exception.Message; }
                    if (success) { complete(true, null); yield break; }
                    yield return null;
                }
                complete(false, error ?? "Movement probe timed out.");
            }
            finally { panel.CoreEventReceived -= receive; observer.Dispose(); }
        }
    }
}
