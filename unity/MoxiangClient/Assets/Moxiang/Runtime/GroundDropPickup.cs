using UnityEngine;

namespace Moxiang
{
    /// <summary>Forwards a click to the server; inventory changes arrive from PickupAck.</summary>
    public sealed class GroundDropPickup : MonoBehaviour
    {
        public ConnectionPanel connection;
        public ServerGroundDrop drop;

        private void OnMouseDown()
        {
            if (connection == null || drop == null || drop.ObjectId == 0) return;
            connection.Pickup(drop.ObjectId);
        }
    }
}
