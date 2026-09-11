using System;
using UnityEngine;
using UnityEngine.EventSystems;

namespace Moxiang
{
    /// <summary>
    /// Resolves gameplay skill shortcuts and submits server-authoritative skill requests.
    /// </summary>
    public sealed class SkillHotkeyController : MonoBehaviour
    {
        public ConnectionPanel connection;
        public KeyCode[] hotkeys = { KeyCode.Alpha1, KeyCode.Alpha2, KeyCode.Alpha3, KeyCode.Alpha4,
            KeyCode.Alpha5, KeyCode.Alpha6, KeyCode.Alpha7, KeyCode.Alpha8 };
        public uint[] skillIds = new uint[8];
        public event Action<uint> SkillRequested;
        public uint selectedTargetObjectId;
        public Vector2 targetGamePosition;

        public static bool TryResolveSkill(KeyCode pressed, KeyCode[] keys, uint[] ids, out uint skillId)
        {
            skillId = 0;
            if (keys == null || ids == null) return false;
            int count = Mathf.Min(keys.Length, ids.Length);
            for (int i = 0; i < count; ++i)
            {
                if (keys[i] != pressed || ids[i] == 0) continue;
                skillId = ids[i];
                return true;
            }
            return false;
        }

        private void Update()
        {
            if (connection == null || connection.Observed.state != CoreState.InGame) return;
            if (EventSystem.current != null && EventSystem.current.IsPointerOverGameObject()) return;
            for (int i = 0; i < hotkeys.Length; ++i)
            {
                if (!Input.GetKeyDown(hotkeys[i])) continue;
                if (TryResolveSkill(hotkeys[i], hotkeys, skillIds, out uint skillId))
                    SkillRequested?.Invoke(skillId);
                    if (selectedTargetObjectId != 0)
                        connection.UseSkill(skillId, selectedTargetObjectId, targetGamePosition.x, targetGamePosition.y);
                break;
            }
        }
    }
}
