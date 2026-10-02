import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('vendor_patch',HERE/'apply_patch.py')
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)

class ExactVendorPatch(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)/'isolated'
        (self.root/'ProjectSettings').mkdir(parents=True)
        (self.root/'ProjectSettings/ProjectVersion.txt').write_text('m_EditorVersion: test\n')
        self.manifest=json.loads((HERE/'manifest.json').read_text())
        self.originals={}
        for relative in self.manifest['files']:
            p=self.root/relative;p.parent.mkdir(parents=True,exist_ok=True)
            data=(HERE/'tests/upstream'/p.name).read_bytes();p.write_bytes(data)
            self.originals[relative]=data
        self.backup=Path(self.temp.name)/'backup'
    def test_dry_run_does_not_write(self):
        self.assertIn('dry run',mod.apply(self.root,None))
        self.assertFalse(self.backup.exists())
        for p,b in self.originals.items():self.assertEqual((self.root/p).read_bytes(),b)
    def test_exact_apply_backup_and_repeat(self):
        mod.apply(self.root,self.backup,True)
        for p,b in self.originals.items():
            self.assertEqual((self.backup/Path(p).name).read_bytes(),b)
            self.assertEqual(mod.sha((self.root/p).read_bytes()),self.manifest['files'][p]['patched_sha256'])
        self.assertIn('already patched',mod.apply(self.root,self.backup,True))
    def test_changed_vendor_refused_before_any_write(self):
        p=self.root/next(iter(self.originals));p.write_bytes(p.read_bytes()+b'\n')
        with self.assertRaises(ValueError):mod.apply(self.root,self.backup,True)
        self.assertFalse(self.backup.exists())
    def test_crlf_not_silently_normalized(self):
        p=self.root/next(iter(self.originals));p.write_bytes(p.read_bytes().replace(b'\n',b'\r\n'))
        with self.assertRaises(ValueError):mod.apply(self.root,self.backup,True)
    def test_mixed_patch_state_refused(self):
        output=mod.patched_files(self.originals,(HERE/'lifecycle.patch').read_text())
        p=next(iter(output));(self.root/p).write_bytes(output[p])
        with self.assertRaises(ValueError):mod.apply(self.root,self.backup,True)
    def test_context_tampering_refused(self):
        text=(HERE/'lifecycle.patch').read_text().replace('using System.Threading;','using Wrong.Threading;')
        output=mod.patched_files(self.originals,text)
        p='Assets/Effekseer/Scripts/EffekseerSound.cs'
        self.assertNotEqual(mod.sha(output[p]),self.manifest['files'][p]['patched_sha256'])
        with patch.object(mod,'patched_files',return_value=output):
            with self.assertRaises(ValueError):mod.apply(self.root,self.backup,True)
        self.assertFalse(self.backup.exists())
    def test_backup_must_be_outside_project(self):
        with self.assertRaises(ValueError):mod.apply(self.root,self.root/'backup',True)
    def test_symlink_target_refused(self):
        relative=next(iter(self.originals));p=self.root/relative;other=Path(self.temp.name)/'other.cs'
        p.rename(other);p.symlink_to(other)
        with self.assertRaises(ValueError):mod.apply(self.root,self.backup,True)
    def test_write_failure_rolls_back_pair(self):
        real=mod.atomic_write;count=0
        def fail_second(path,data):
            nonlocal count
            count+=1
            if count==2:raise OSError('injected second write error')
            real(path,data)
        with patch.object(mod,'atomic_write',fail_second):
            with self.assertRaises(OSError):mod.apply(self.root,self.backup,True)
        for p,b in self.originals.items():self.assertEqual((self.root/p).read_bytes(),b)

if __name__=='__main__':unittest.main()
