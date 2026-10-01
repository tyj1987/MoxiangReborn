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
                    if(movement){movement.mapCollider=content.navigation;movement.mapWidth=WuguanMap44Content.WorldWidth;movement.mapDepth=WuguanMap44Content.WorldDepth;}
                    if(entities){entities.mapWidth=WuguanMap44Content.WorldWidth;entities.mapDepth=WuguanMap44Content.WorldDepth;}
                    loadingScene=false;IsReady=true;Failure=null;
                },error=>{if(this){loadingScene=false;Failure=error;}});
                return;
            }
            if (selected == null || selected.prefab == null || selected.heightField == null ||
                selected.heightField.inspectionMesh == null)
            { Failure = "地图 " + snapshot.game.mapNumber + " 的视觉资源尚未就绪"; return; }
            var field = selected.heightField.descriptor;
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
            if (entities != null) { entities.mapWidth = field.width; entities.mapDepth = field.depth; }
            map = snapshot.game.mapNumber; session = snapshot.sessionGeneration; generation = snapshot.mapGeneration;
            IsReady = true; Failure = null;
        }

        public void Clear()
        {
            IsReady = false;loadingScene=false;
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
