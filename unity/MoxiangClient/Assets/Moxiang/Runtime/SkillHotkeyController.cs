using System;
using UnityEngine;
using UnityEngine.EventSystems;
using TMPro;
using UnityEngine.UI;

namespace Moxiang
{
    /// <summary>
    /// Resolves gameplay skill shortcuts and submits server-authoritative skill requests.
    /// </summary>
    public sealed class SkillHotkeyController : MonoBehaviour
    {
        public ConnectionPanel connection;
        public TargetSelectionController targetSelection;
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

        public static bool TextInputOwnsKeyboard(GameObject selectedObject)
        {
            if (selectedObject == null) return false;
            var tmp = selectedObject.GetComponentInParent<TMP_InputField>();
            if (tmp != null && tmp.isActiveAndEnabled && tmp.interactable) return true;
            var legacy = selectedObject.GetComponentInParent<InputField>();
            return legacy != null && legacy.isActiveAndEnabled && legacy.interactable;
        }

        private void Update()
        {
            if (connection == null || connection.Observed.state != CoreState.InGame) return;
            if (hotkeys == null || TextInputOwnsKeyboard(EventSystem.current == null
                ? null : EventSystem.current.currentSelectedGameObject)) return;
            if (EventSystem.current != null && EventSystem.current.IsPointerOverGameObject()) return;
            for (int i = 0; i < hotkeys.Length; ++i)
            {
                if (!Input.GetKeyDown(hotkeys[i])) continue;
                if (TryResolveSkill(hotkeys[i], hotkeys, skillIds, out uint skillId))
                {
                    if (targetSelection == null || !targetSelection.TryGetTarget(out var id, out var position))
                    {
                        selectedTargetObjectId = 0;
                        targetGamePosition = default;
                        return;
                    }
                    selectedTargetObjectId = id; targetGamePosition = position;
                    SkillRequested?.Invoke(skillId);
                    if (selectedTargetObjectId != 0)
                        connection.UseSkill(skillId, selectedTargetObjectId, targetGamePosition.x, targetGamePosition.y);
                }
                break;
            }
        }
    }
}
