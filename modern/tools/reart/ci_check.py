"""Execute the same narrow acceptance gates on Windows and Linux.

Never starts, deploys or changes the game's runtime. Keeps logs and original
sample artifacts outside the checkout; success is NOT whole-game acceptance.
"""
from __future__ import annotations
import argparse, hashlib, json, os, platform, shutil, subprocess, sys, zipfile
from pathlib import Path

def run(command: list[str], log: Path, cwd: Path, *, expected: int=0):
    result = subprocess.run(command, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.write_bytes(result.stdout)
    print(log.name + ': exit ' + str(result.returncode), flush=True)
    if result.returncode != expected:
        print(result.stdout.decode('utf-8', errors='replace'), flush=True)
        raise RuntimeError(f'{command[0]} exited {result.returncode}, expected {expected}')
    return result

def check(repo: Path, output: Path):
    if output.exists():
        raise ValueError('CI output must be a new directory')
    repo=repo.resolve(); output=output.resolve(); output.mkdir(parents=True)
    logs=output/'evidence'; logs.mkdir()
    tool=repo/'modern/tools/reart'
    environment={'platform':platform.platform(),'python':sys.version,'commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(), 'scope':'isolated current C++ parser build and original-asset tooling; not full client or graphical acceptance'}
    (logs/'environment.json').write_text(json.dumps(environment,indent=2),encoding='utf-8')
    run([sys.executable,'-m','unittest','discover','-s','modern/tests/reart','-v'],logs/'python-tests.log',repo)
    pack=output/'moxiang-reart-v1'
    run([sys.executable,str(tool/'build_samples.py'),'--output',str(pack)],logs/'sample-generation.json',repo)
    build=output/'native-build'
    run(['cmake','-S',str(tool),'-B',str(build)],logs/'cmake-configure.log',repo)
    run(['cmake','--build',str(build),'--config','Release','--parallel','2'],logs/'cmake-build.log',repo)
    candidates=[build/'Release/reart_probe.exe',build/'reart_probe.exe',build/'reart_probe']
    probe=next((p for p in candidates if p.is_file()),None)
    if probe is None:
        raise RuntimeError('No native probe built')
    run([str(probe),'--pack',str(pack)],logs/'native-probe.json',repo)
    run([sys.executable,str(tool/'audit_map10.py'),'--repo',str(repo),'--probe',str(probe),'--output',str(logs/'map10-audit.json')],logs/'map10-audit.log',repo)
    audit=json.loads((logs/'map10-audit.json').read_text(encoding='utf-8'))
    if audit['map10']['name']!='嵩山' or audit['map17']['name']!='蘭州':
        raise RuntimeError('Map identity drift; review the data baseline')
    run([sys.executable,str(tool/'pack.py'),str(pack),str(output/'BLOCKED-release.zip')],logs/'release-blocked.log',repo,expected=2)
    if (output/'BLOCKED-release.zip').exists():
        raise RuntimeError('Release gate produced an unauthorized package')
    run([sys.executable,str(tool/'pack.py'),str(pack),str(output/'original-assets-development.zip'),'--development'],logs/'development-package.json',repo)
    run([sys.executable,str(tool/'make_review.py'),str(pack),str(output/'MoxiangReborn-review.html')],logs/'review-generation.log',repo)
    # Snapshot EXACT new implementation files, not original game assets,
    # credentials, .git, third-party libraries, fonts or generated binaries.
    with zipfile.ZipFile(output/'original-asset-tools.zip','w',zipfile.ZIP_DEFLATED) as archive:
        roots=['modern/tools/reart','modern/tests/reart','modern/docs/art-direction']
        for root in roots:
            for path in sorted((repo/root).rglob('*')):
                if path.is_file() and not path.is_symlink() and '__pycache__' not in path.parts and path.suffix in {'.py','.cpp','.txt','.md','.html','.json'}:
                    archive.write(path,path.relative_to(repo).as_posix())
        workflow=repo/'.github/workflows/reart-validation.yml'
        archive.write(workflow,workflow.relative_to(repo).as_posix())
    status=subprocess.check_output(['git','status','--porcelain=v1'],cwd=repo,text=True)
    (logs/'checkout-after.txt').write_text(status,encoding='utf-8')
    if status.strip():
        raise RuntimeError('Unexpected checkout modifications')
    report={'result':'pass','environment':environment,'assets':7,'manifest_artifacts':35,'native_probe':json.loads((logs/'native-probe.json').read_text(encoding='utf-8')), 'release_gate':'rejected samples as required','gameplay_and_visual_acceptance':'not_run'}
    (logs/'summary.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    checksums={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in output.iterdir() if p.is_file()}
    (output/'SHA256.json').write_text(json.dumps(checksums,indent=2),encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False,indent=2))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path('.'))
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    check(args.repo,args.output)
