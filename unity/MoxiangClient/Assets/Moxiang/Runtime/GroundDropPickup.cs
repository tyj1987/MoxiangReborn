using UnityEngine;

namespace Moxiang
{
    /// <summary>Forwards a click to the server; inventory changes arrive from PickupAck.</summary>
    public sealed class GroundDropPickup : MonoBehaviour
    {
        public ConnectionPanel connection;
        public ServerGroundDrop drop;
        public float confirmationTimeoutSeconds = 5f;
        private bool pending;
        private float pendingSince;
        public bool Pending => pending;

        public void Confirmed() { pending = false; }

        private void Update()
        {
            if (pending && confirmationTimeoutSeconds > 0f &&
                Time.unscaledTime - pendingSince >= confirmationTimeoutSeconds)
                pending = false;
        }

        private void OnMouseDown() { RequestPickup(); }

        public CoreResult RequestPickup()
        {
            if (pending || connection == null || drop == null || drop.ObjectId == 0) return CoreResult.NotReady;
            var result = connection.Pickup(drop.ObjectId);
            pending = result == CoreResult.Ok;
            if (pending) pendingSince = Time.unscaledTime;
            return result;
        }
    }
}
