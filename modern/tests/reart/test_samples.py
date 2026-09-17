"""Geometry/native format/source determinism tests; not visual acceptance."""
import json, pathlib, struct, sys, tempfile, unittest, wave, zipfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]/'tools/reart'))
import build_samples as b
from native_writer import dds_rgba,export_glb,export_native,export_motion
from manifest import AssetError,relative_name,validate
from pack import package

class SampleTests(unittest.TestCase):
    def test_native_dds_header(self):
        d=dds_rgba(2,3,b'\xff'*24)
        self.assertEqual(d[:4],b'DDS ');self.assertEqual(len(d),128+24)
        self.assertEqual(struct.unpack_from('<I',d,4)[0],124)
        self.assertEqual(struct.unpack_from('<I',d,76)[0],32)
    def test_dds_rejects_bad_pixels(self):
        with self.assertRaises(ValueError):dds_rgba(3,4,b'')
    def test_sword_is_original_geometry_with_skin(self):
        meshes,bones=b.sword();self.assertEqual(len(bones),5)
        self.assertGreater(sum(len(x['indices'])//3 for x in meshes),500)
        self.assertTrue(any('skin' in m for m in meshes))
    def test_sword_weights_and_bind_pose(self):
        meshes,bones=b.sword()
        for m in meshes:
            for p,ws in zip(m['positions'],m.get('skin',[])):
                self.assertAlmostEqual(sum(w[1] for w in ws),1,places=6)
                for axis in range(3):self.assertAlmostEqual(sum(w[1]*(w[2][axis]+bones[w[0]]['world'][axis]) for w in ws),p[axis],places=6)
    def test_geometry_determinism(self):
        self.assertEqual(b.mountain_gate(),b.mountain_gate());self.assertEqual(b.sword(),b.sword())
    def test_native_vertex_budget(self):
        for meshes in [b.sword()[0],b.scabbard(),b.mountain_gate()]:
            for m in meshes:self.assertLessEqual(len(m['positions']),65535)
    def test_no_existing_output_overwrite(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(ValueError):b.build(pathlib.Path(tmp))
    def test_reject_windows_device_names(self):
        for value in ['con.dds','x/AUX.txt','com1','asset.','asset ']:
            with self.subTest(value=value), self.assertRaises(AssetError):relative_name(value)
    def test_glb_roundtrip_structure(self):
        with tempfile.TemporaryDirectory() as t:
            r=pathlib.Path(t);(r/'interchange').mkdir();export_glb(r,'sword',b.sword()[0],b.MATERIALS)
            data=(r/'interchange/sword.glb').read_bytes();magic,version,length=struct.unpack_from('<III',data)
            self.assertEqual((magic,version,length),(0x46546c67,2,len(data)))
            size,kind=struct.unpack_from('<II',data,12);self.assertEqual(kind,0x4e4f534a)
            doc=json.loads(data[20:20+size]);self.assertEqual(len(doc['meshes']),5)
            for view in doc['bufferViews']:self.assertEqual(view['byteOffset']%4,0)
    def test_native_sword_roundtrip_bytes_deterministic(self):
        with tempfile.TemporaryDirectory() as t:
            r=pathlib.Path(t)
            for d in ['runtime/models','runtime/animations']:(r/d).mkdir(parents=True)
            meshes,bones=b.sword();export_native(r,'sword',meshes,b.MATERIALS,bones);export_motion(r,'sword',bones)
            before=(r/'runtime/models/sword.mod').read_bytes();export_native(r,'sword',meshes,b.MATERIALS,bones)
            self.assertEqual(before,(r/'runtime/models/sword.mod').read_bytes())
            a=(r/'runtime/animations/sword_idle.anm').read_bytes();self.assertEqual(struct.unpack_from('<I',a,20)[0],5)
    def test_wav_master_format(self):
        with tempfile.TemporaryDirectory() as t:
            p=pathlib.Path(t)/'test.wav';b.wav_pcm(p,[[0,.1,-.1],[0,.1,-.1]])
            with wave.open(str(p)) as w:self.assertEqual((w.getnchannels(),w.getsampwidth(),w.getframerate(),w.getnframes()),(2,3,48000,3))

if __name__=='__main__':unittest.main()
