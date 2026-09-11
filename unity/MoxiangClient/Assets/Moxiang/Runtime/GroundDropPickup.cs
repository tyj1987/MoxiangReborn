using UnityEngine;

namespace Moxiang
{
    /// <summary>Forwards a click to the server; inventory changes arrive from PickupAck.</summary>
    public sealed class GroundDropPickup : MonoBehaviour
    {
        public ConnectionPanel connection;
        public ServerGroundDrop drop;
        private bool pending;

        public void Confirmed() { pending = false; }

        private void OnMouseDown()
        {
            if (pending || connection == null || drop == null || drop.ObjectId == 0) return;
            var result = connection.Pickup(drop.ObjectId);
            pending = result == CoreResult.Ok;
        }
    }
}
