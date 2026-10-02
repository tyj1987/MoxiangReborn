#!/usr/bin/env python3
"""Run original red reproductions and patched C# regressions using .NET8 SDK.
Unity/native types are test doubles; actual vendor source is compiled unchanged.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import subprocess

HERE=Path(__file__).resolve().parent

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dotnet',default='dotnet')
    parser.add_argument('--artifacts',required=True,help='New directory outside the game project')
    args=parser.parse_args()
    out=Path(args.artifacts).absolute();out.mkdir(parents=True,exist_ok=False)
    spec=importlib.util.spec_from_file_location('patcher',HERE/'apply_patch.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    manifest=json.loads((HERE/'manifest.json').read_text())
    original={relative:(HERE/'tests/upstream'/Path(relative).name).read_bytes() for relative in manifest['files']}
    for path,data in original.items():
        if module.sha(data)!=manifest['files'][path]['original_sha256']:raise ValueError('Fixture hash mismatch')
    patched=module.patched_files(original,(HERE/'lifecycle.patch').read_text(encoding='utf-8'))
    for path,data in patched.items():
        if module.sha(data)!=manifest['files'][path]['patched_sha256']:raise ValueError('Output hash mismatch')
    (out/'NuGet.Config').write_text('<configuration><packageSources><clear /></packageSources></configuration>')
    env=dict(os.environ,DOTNET_CLI_HOME=str(out/'dotnet-home'),DOTNET_CLI_TELEMETRY_OPTOUT='1',DOTNET_GENERATE_ASPNET_CERTIFICATE='false')
    for name,sources,test in [('original',original,'BaselineRepro.cs'),('patched',patched,'LifecycleTests.cs')]:
        stage=out/name;(stage/'source').mkdir(parents=True)
        for path,data in sources.items():(stage/'source'/Path(path).name).write_bytes(data)
        command=[args.dotnet,'build',str(HERE/'tests/LifecycleTests.csproj'),'-p:VendorSources='+str(stage/'source'),'-p:TestMain='+test,'-p:BaseIntermediateOutputPath='+str(stage/'obj')+os.sep,'-p:BaseOutputPath='+str(stage/'bin')+os.sep,'-p:RestoreConfigFile='+str(out/'NuGet.Config')]
        with (stage/'build.log').open('w') as log:subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
        result=subprocess.run([args.dotnet,str(stage/'bin/Debug/net8.0/LifecycleTests.dll')],env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        (stage/'test.log').write_text(result.stdout)
        print(result.stdout,end='')
        if name=='original':
            if result.returncode!=1 or '3 baseline failures' not in result.stdout:raise RuntimeError('Expected original three-defect reproduction')
        elif result.returncode!=0:raise RuntimeError('Patched lifecycle tests failed')
    print('Managed red/green passed; Unity/native runtime acceptance remains pending.')

if __name__=='__main__':main()
