"""Reproducible Blender PBR bake runner; use from the existing repository."""
from pathlib import Path
import argparse, datetime, json, os, subprocess, sys

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--blender",default=str(Path(os.environ.get("ProgramFiles",r"C:\Program Files"))/"Blender Foundation/Blender 5.2/blender.exe"))
    args=parser.parse_args()
    exe=Path(args.blender)
    if not exe.is_file():
        raise FileNotFoundError("Pinned Blender executable missing: "+str(exe))
    root=Path(__file__).resolve().parent.parent
    output=root/"modern/out/unity-remaster/wuguan-training-hall"
    output.mkdir(parents=True,exist_ok=True)
    started=datetime.datetime.now(datetime.timezone.utc)
    log=output/("pbr-bake-"+started.strftime("%Y%m%d-%H%M%S")+".log")
    command=[str(exe),"--background","--threads","16","--python-exit-code","2","--python",
             str(root/"unity/source-overlays/wuguan-training-hall/bake_pbr.py")]
    print("BAKE_LOG",log,flush=True)
    with log.open("w",encoding="utf-8") as f:
        result=subprocess.run(command,cwd=root,stdout=f,stderr=subprocess.STDOUT,timeout=540)
    print(log.read_text(encoding="utf-8",errors="replace")[-5000:])
    if result.returncode:
        return result.returncode
    manifest=root/"unity/MoxiangClient/Assets/Moxiang/Art/WuguanTrainingHall/pbr-manifest.json"
    if not manifest.exists() or manifest.stat().st_mtime<started.timestamp():
        raise RuntimeError("Fresh PBR publication manifest missing")
    data=json.loads(manifest.read_text(encoding="utf-8"))
    print("PBR_RUNNER_OK",data["atlasSize"],len(data["textures"]),"exit=0")
    return 0
if __name__=="__main__":
    sys.exit(main())
