using System;
using System.IO;
using UnityEditor.AssetImporters;
using UnityEngine;

namespace Moxiang.Editor
{
    [ScriptedImporter(1, "mxhdds")]
    public sealed class MxhDdsImporter : ScriptedImporter
    {
        public static Texture2D Read(byte[] bytes)
        {
            uint U32(int offset) => BitConverter.ToUInt32(bytes, offset);
            if (!BitConverter.IsLittleEndian || bytes.Length < 128 || U32(0) != 0x20534444 || U32(4) != 124 || U32(76) != 32)
                throw new InvalidDataException("Invalid legacy DDS header.");
            int height = checked((int)U32(12)), width = checked((int)U32(16)), mips = checked((int)U32(28));
            if (mips == 0) mips = 1;
            if (width < 1 || height < 1 || width > 8192 || height > 8192 || mips > 14 ||
                (U32(80) & 4) == 0 || U32(112) != 0 || U32(24) > 1)
                throw new InvalidDataException("Unsupported DDS dimensions, pixel format or cube/volume layout.");
            TextureFormat format; int blockSize;
            if (U32(84) == 0x31545844) { format = TextureFormat.DXT1; blockSize = 8; }
            else if (U32(84) == 0x35545844) { format = TextureFormat.DXT5; blockSize = 16; }
            else throw new InvalidDataException("Only explicitly supported DXT1/DXT5 sources can be imported.");
            long expected = 128; int w = width, h = height;
            for (int i = 0; i < mips; ++i)
            {
                expected += (long)((w + 3) / 4) * ((h + 3) / 4) * blockSize;
                if (i + 1 < mips && w == 1 && h == 1) throw new InvalidDataException("Excess DDS mip levels.");
                w = Math.Max(1, w / 2); h = Math.Max(1, h / 2);
            }
            if (expected != bytes.LongLength) throw new InvalidDataException("DDS compressed payload length mismatch.");
            var texture = new Texture2D(width, height, format, mips, false);
            var payload = new byte[bytes.Length - 128]; Array.Copy(bytes, 128, payload, 0, payload.Length);
            texture.LoadRawTextureData(payload); texture.Apply(false, true);
            texture.wrapMode = TextureWrapMode.Repeat; texture.filterMode = FilterMode.Trilinear; texture.anisoLevel = 4;
            return texture;
        }
        public override void OnImportAsset(AssetImportContext context)
        {
            var texture = Read(File.ReadAllBytes(context.assetPath));
            texture.name = Path.GetFileNameWithoutExtension(context.assetPath);
            context.AddObjectToAsset("texture", texture); context.SetMainObject(texture);
        }
    }
}
