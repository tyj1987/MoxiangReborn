"""Manifest allowlist packaging. Release is fail-closed by default."""
from __future__ import annotations
import argparse, json, os, tempfile, zipfile
from pathlib import Path
from manifest import AssetError, checked_path, read_json, validate

def package(root:Path, destination:Path, *, development:bool=False):
    if destination.exists():raise AssetError('Destination already exists')
    report=validate(root,release=not development)
    document=read_json(checked_path(root,'manifest.json'));names={'manifest.json'}
    for asset in document['assets']:
        names.update(x['path'] for x in asset['files'])
        names.update(x['path'] for x in asset.get('acceptance_evidence',[]) if x.get('result')=='pass')
    destination.parent.mkdir(parents=True,exist_ok=True)
    temp=None
    try:
        handle,temp=tempfile.mkstemp(prefix='.reart-',suffix='.zip',dir=destination.parent);os.close(handle)
        with zipfile.ZipFile(temp,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
            for name in sorted(names):
                path=checked_path(root,name);info=zipfile.ZipInfo(name,date_time=(2026,9,17,0,0,0));info.external_attr=0o100644<<16
                archive.writestr(info,path.read_bytes(),compress_type=zipfile.ZIP_DEFLATED)
        # Requires an exclusively owned staging directory, not an adversarial
        # concurrently modified filesystem. Recheck the collected snapshot.
        validate(root,release=not development)
        with zipfile.ZipFile(temp) as archive:
            for asset in document['assets']:
                for f in asset['files']:
                    import hashlib
                    if hashlib.sha256(archive.read(f['path'])).hexdigest()!=f['sha256']:raise AssetError('Pack snapshot changed')
        if destination.exists():raise AssetError('Destination appeared during packaging')
        os.replace(temp,destination);temp=None
    finally:
        if temp and os.path.exists(temp):os.unlink(temp)
    return report
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('root',type=Path);p.add_argument('output',type=Path);p.add_argument('--development',action='store_true');a=p.parse_args()
    try:print(json.dumps(package(a.root,a.output,development=a.development),indent=2))
    except AssetError as e:p.exit(2,str(e)+'\n')
