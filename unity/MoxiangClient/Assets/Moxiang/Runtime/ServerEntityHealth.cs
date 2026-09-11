using UnityEngine;

namespace Moxiang
{
    /// <summary>Optional server-authoritative life sink for audited entity prefabs.</summary>
    public sealed class ServerEntityHealth : MonoBehaviour
    {
        public uint CurrentLife { get; private set; }

        public void SetCurrentLife(uint value)
        {
            CurrentLife = value;
        }
    }
}
