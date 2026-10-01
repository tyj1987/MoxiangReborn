using UnityEngine;

namespace Moxiang
{
    // Presentation only: TerrainTileMesh uses the 00->11 diagonal. The
    // inspection collider uses the other diagonal; neither replaces server TTB.
    public static class MapDisplayHeight
    {
        public static bool TrySample(ImportedHeightField field,float x,float z,out float height)
        {
            height=0;
            if(field==null || field.descriptor==null || field.heights==null)return false;
            var d=field.descriptor;
            if(d.heightCountX<2 || d.heightCountZ<2 || !float.IsFinite(d.faceSize) || d.faceSize<=0 ||
                !float.IsFinite(d.width) || !float.IsFinite(d.depth) ||
                (long)d.heightCountX*d.heightCountZ!=field.heights.Length ||
                d.width!=(d.heightCountX-1)*d.faceSize || d.depth!=(d.heightCountZ-1)*d.faceSize ||
                !float.IsFinite(x) || !float.IsFinite(z) || x<0 || z<0 || x>d.width || z>d.depth)return false;
            int ix=Mathf.Min((int)(x/d.faceSize),d.heightCountX-2);
            int iz=Mathf.Min((int)(z/d.faceSize),d.heightCountZ-2);
            float u=x/d.faceSize-ix,v=z/d.faceSize-iz;
            int a=iz*d.heightCountX+ix;
            float h00=field.heights[a],h10=field.heights[a+1];
            float h01=field.heights[a+d.heightCountX],h11=field.heights[a+d.heightCountX+1];
            if(!float.IsFinite(h00)||!float.IsFinite(h10)||!float.IsFinite(h01)||!float.IsFinite(h11))return false;
            height=u>=v ? h00*(1-u)+h10*(u-v)+h11*v : h00*(1-v)+h11*u+h01*(v-u);
            return float.IsFinite(height);
        }
    }
}
