using UnityEngine;

namespace Moxiang.Tests
{
    // Explicit synthetic surface for entity protocol tests; never a game asset fallback.
    internal sealed class DisplaySurfaceFixture : System.IDisposable
    {
        readonly GameObject owner,prefab;
        readonly ImportedHeightField field;
        readonly Mesh mesh;
        bool disposed;
        public readonly MapVisualController Controller;
        public DisplaySurfaceFixture(ServerEntityRegistry registry,ulong session=0,ulong generation=1,
            float height=5000,MapVisualController existing=null)
        {
            owner=existing==null ? new GameObject("SyntheticSurface") : null;
            Controller=existing!=null ? existing : owner.AddComponent<MapVisualController>();
            prefab=new GameObject("SyntheticTerrain");
            field=ScriptableObject.CreateInstance<ImportedHeightField>();
            field.descriptor=new HeightFieldDescriptor {width=51200,depth=51200,faceSize=51200,heightCountX=2,heightCountZ=2};
            field.heights=new[]{height,height,height,height};
            mesh=new Mesh();
            mesh.vertices=new[]{new Vector3(-25.6f,height*.001f,-25.6f),new Vector3(25.6f,height*.001f,-25.6f),
                new Vector3(-25.6f,height*.001f,25.6f),new Vector3(25.6f,height*.001f,25.6f)};
            mesh.triangles=new[]{0,2,1,1,2,3};
            field.inspectionMesh=mesh;
            Controller.maps=new[]{new MapVisualController.Entry {mapNumber=10,prefab=prefab,heightField=field}};
            Controller.Observe(new CoreSnapshot {state=CoreState.InGame,sessionGeneration=session,mapGeneration=generation,
                game=new CoreGame {mapNumber=10}});
            registry.displaySurface=Controller;
            registry.mapWidth=51200;registry.mapDepth=51200;
        }
        public void Dispose()
        {
            if(disposed)return;
            disposed=true;
            Controller.Clear();
            if(owner!=null)Object.DestroyImmediate(owner);
            Object.DestroyImmediate(prefab);Object.DestroyImmediate(field);Object.DestroyImmediate(mesh);
        }
    }
}
