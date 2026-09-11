using UnityEngine;

namespace Moxiang
{
    /// <summary>Server-authoritative ground item state for an audited drop prefab.</summary>
    public sealed class ServerGroundDrop : MonoBehaviour
    {
        public uint ObjectId { get; private set; }
        public uint ItemId { get; private set; }
        public ushort Count { get; private set; }

        public void Initialize(uint objectId, uint itemId, ushort count)
        {
            ObjectId = objectId;
            ItemId = itemId;
            Count = count;
        }
    }
}
