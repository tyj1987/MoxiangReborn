using System;
using UnityEngine;

namespace Moxiang
{
    /// <summary>Map presentation follows the admitted server map and generation.</summary>
    public sealed class MapVisualController : MonoBehaviour
    {
        [Serializable] public sealed class Entry
        {
            public ushort mapNumber;
            public GameObject prefab;
            public ImportedHeightField heightField;
            public string scenePath;
        }
        public Entry[] maps = Array.Empty<Entry>();
        public Map10InputController movement;
        public ServerEntityRegistry entities;
        public bool IsReady { get; private set; }
        public string Failure { get; private set; }
        public GameObject ActiveRoot { get; private set; }
        private WuguanMap44SceneLoader sceneLoader;
        private bool loadingScene;
        private ushort map;
        private ulong session, generation;
        private ImportedHeightField displayHeight;
        private Collider sceneGround;

        public bool Matches(ulong sessionId,ulong mapId) => IsReady && session==sessionId && generation==mapId;

        public bool TryGetDisplayPosition(ulong sessionId,ulong mapId,float x,float z,out Vector3 position)
        {
            position=default;
            if(!Matches(sessionId,mapId) || ActiveRoot==null)return false;
            // Canonical centered coordinates are also used by input conversion.
            // Reject accidental transforms instead of silently changing XZ.
            var t=ActiveRoot.transform;
            if(t.position!=Vector3.zero || t.rotation!=Quaternion.identity || t.lossyScale!=Vector3.one)return false;
            if(displayHeight!=null)
            {
                if(!MapDisplayHeight.TrySample(displayHeight,x,z,out var height))return false;
                position=new MapCoordinates(displayHeight.descriptor.width,displayHeight.descriptor.depth)
                    .ToScene(new Vector3(x,height,z));
                return true;
            }
            // Map44 owns a reviewed scene collider, not a HFL. Never raycast
            // arbitrary entities or substitute a plane when it is unavailable.
            if(sceneGround==null || !sceneGround.enabled || !sceneGround.gameObject.activeInHierarchy ||
                !float.IsFinite(x)||!float.IsFinite(z)||x<0||z<0||
                x>WuguanMap44Content.WorldWidth||z>WuguanMap44Content.WorldDepth)return false;
            var point=new MapCoordinates(WuguanMap44Content.WorldWidth,WuguanMap44Content.WorldDepth)
                .ToScene(new Vector3(x,0,z));
            var bounds=sceneGround.bounds;
            if(!sceneGround.Raycast(new Ray(new Vector3(point.x,bounds.max.y+1,point.z),Vector3.down),
                out var hit,bounds.size.y+2))return false;
            position=new Vector3(point.x,hit.point.y,point.z);
            return true;
        }

        public void Observe(CoreSnapshot snapshot)
        {
            if (snapshot.state != CoreState.InGame) { Clear(); return; }
            if ((IsReady || loadingScene) && map == snapshot.game.mapNumber && session == snapshot.sessionGeneration &&
                generation == snapshot.mapGeneration) return;
            Clear();
            Entry selected = null;
            foreach (var candidate in maps ?? Array.Empty<Entry>())
            {
                if (candidate == null || candidate.mapNumber != snapshot.game.mapNumber) continue;
                if (selected != null) { Failure = "地图视觉配置重复"; return; }
                selected = candidate;
            }
            if(selected!=null&&!string.IsNullOrEmpty(selected.scenePath))
            {
                if(selected.mapNumber!=WuguanMap44Content.MapNumber||selected.scenePath!=WuguanMap44SceneLoader.ScenePath)
                {Failure="Unreviewed map scene binding.";return;}
                map=snapshot.game.mapNumber;session=snapshot.sessionGeneration;generation=snapshot.mapGeneration;
                if(!sceneLoader)sceneLoader=gameObject.AddComponent<WuguanMap44SceneLoader>();
                loadingScene=true;
                sceneLoader.Begin(content=>{
                    if(!this)return;
                    ActiveRoot=content.gameObject;
                    sceneGround=content.navigation;
                    if(movement){movement.mapCollider=content.navigation;movement.mapWidth=WuguanMap44Content.WorldWidth;movement.mapDepth=WuguanMap44Content.WorldDepth;}
                    if(entities){entities.mapWidth=WuguanMap44Content.WorldWidth;entities.mapDepth=WuguanMap44Content.WorldDepth;entities.displaySurface=this;}
                    loadingScene=false;IsReady=true;Failure=null;
                },error=>{if(this){loadingScene=false;Failure=error;}});
                return;
            }
            if (selected == null || selected.prefab == null || selected.heightField == null ||
                selected.heightField.inspectionMesh == null)
            { Failure = "地图 " + snapshot.game.mapNumber + " 的视觉资源尚未就绪"; return; }
            var field = selected.heightField.descriptor;
            if(!MapDisplayHeight.TrySample(selected.heightField,0,0,out _))
            { Failure="Map display height data is not ready or has an invalid coordinate basis."; return; }
            displayHeight=selected.heightField;
            // Validate the coordinate basis before exposing any collider/input.
            _ = new MapCoordinates(field.width, field.depth);
            ActiveRoot = Instantiate(selected.prefab, transform);
            ActiveRoot.SetActive(true);
            var collider = ActiveRoot.AddComponent<MeshCollider>();
            collider.sharedMesh = selected.heightField.inspectionMesh;
            if (movement != null)
            {
                movement.mapCollider = collider;
                movement.mapWidth = field.width; movement.mapDepth = field.depth;
            }
            if (entities != null) { entities.mapWidth = field.width; entities.mapDepth = field.depth; entities.displaySurface=this; }
            map = snapshot.game.mapNumber; session = snapshot.sessionGeneration; generation = snapshot.mapGeneration;
            IsReady = true; Failure = null;
        }

        public void Clear()
        {
            IsReady = false;loadingScene=false;
            displayHeight=null;sceneGround=null;
            if(entities!=null && entities.displaySurface==this)entities.displaySurface=null;
            if(sceneLoader){bool streamed=sceneLoader.HasOwnedScene;sceneLoader.Clear();if(streamed)ActiveRoot=null;}
            if (entities != null) entities.ClearPresentation();
            if (movement != null) movement.mapCollider = null;
            if (ActiveRoot != null)
            {
                // Disable immediately; Destroy is deferred in a running Player.
                ActiveRoot.SetActive(false);
                if (Application.isPlaying) Destroy(ActiveRoot); else DestroyImmediate(ActiveRoot);
                ActiveRoot = null;
            }
        }
        private void OnDisable() { Clear(); }
    }
}
