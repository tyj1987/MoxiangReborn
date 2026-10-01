"""Build true high/low geometry pairs and selected-to-active normal/AO maps."""
from pathlib import Path
import argparse,datetime,os,subprocess,sys,json

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--blender",default=str(Path(os.environ.get("ProgramFiles",r"C:\Program Files"))/"Blender Foundation/Blender 5.2/blender.exe"))
    args=parser.parse_args();exe=Path(args.blender)
    if not exe.is_file():raise FileNotFoundError(exe)
    root=Path(__file__).resolve().parents[1];out=root/"modern/out/unity-remaster/wuguan-training-hall"
    out.mkdir(parents=True,exist_ok=True);started=datetime.datetime.now(datetime.timezone.utc)
    log=out/("highlow-"+started.strftime("%Y%m%d-%H%M%S")+".log")
    command=[str(exe),"--background","--threads","16","--python-exit-code","2","--python",str(root/"unity/source-overlays/wuguan-training-hall/refine_high_low.py")]
    print("HIGHLOW_LOG",log,flush=True)
    with log.open("w",encoding="utf-8") as f:result=subprocess.run(command,cwd=root,stdout=f,stderr=subprocess.STDOUT,timeout=540)
    print(log.read_text(encoding="utf-8",errors="replace")[-5000:])
    if result.returncode:return result.returncode
    report=root/"unity/MoxiangClient/Assets/Moxiang/Art/WuguanTrainingHall/highlow-manifest.json"
    if not report.exists() or report.stat().st_mtime<started.timestamp():raise RuntimeError("Fresh high/low publication missing")
    data=json.loads(report.read_text());print("HIGHLOW_RUNNER_OK",data["refinedPairs"],data["highTriangles"])
    return 0
if __name__=="__main__":sys.exit(main())
