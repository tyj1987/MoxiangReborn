"""Read-only Map10 identity and placeholder audit using actual C++ decoders.

Requires a checkout containing the small PlayDH tables. LFS reference is
optional; its absence is reported explicitly rather than substituted.
"""
from __future__ import annotations
import argparse,hashlib,importlib.util,json,subprocess,tempfile
from pathlib import Path
BASE='89a11b3653549bab7471f53f8f0aafb72e98ce2a'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def audit(repo:Path,probe:Path,reference:Path|None=None):
    resources=repo/'modern/data/PlayDH/Resource';evidence={};decoded={}
    with tempfile.TemporaryDirectory() as tmp:
        for name in ['MapKindInfo','MapChange','StaticNpc']:
            src=resources/(name+'.bin');dst=Path(tmp)/(name+'.txt')
            subprocess.run([str(probe),'--decode',str(src),str(dst)],check=True,capture_output=True)
            decoded[name]=dst.read_bytes();evidence[name]={'path':src.relative_to(repo).as_posix(),'sha256':sha(src),'decoded_sha256':sha(dst)}
    names={};name_hex={}
    for row in decoded['MapKindInfo'].splitlines():
        fields=row.split(b'\t')
        if len(fields)>=2 and fields[0].isdigit():
            i=int(fields[0]);name_hex[i]=fields[1].hex()
            try:names[i]=fields[1].decode('cp950',errors='strict')
            except UnicodeDecodeError:names[i]=None
    anchors=[]
    for line,row in enumerate(decoded['StaticNpc'].splitlines(),1):
        f=row.split()
        if len(f)==7 and f[0]==b'10':anchors.append({'decoded_line':line,'kind':int(f[1]),'name':f[2].decode('cp950'), 'id':int(f[3]),'world_x':int(f[4]),'world_z':int(f[5]),'direction':int(f[6])})
    links=[]
    for line,row in enumerate(decoded['MapChange'].splitlines(),1):
        f=row.split()
        if len(f)>=10 and f[3].isdigit() and f[4].isdigit() and (f[3]==b'10' or f[4]==b'10'):
            links.append({'decoded_line':line,'id':int(f[0]),'source_map':int(f[3]),'target_map':int(f[4]),'source_position':[int(f[5]),int(f[6])],'target_position':[int(f[7]),int(f[8])]})
    spec=importlib.util.spec_from_file_location('legacy_placeholder',repo/'modern/tools/gen_hfl_placeholders.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    hfl=[]
    for p in sorted((resources/'Map').glob('*.hfl')):
        if not p.stem.isdigit():continue
        raw=p.read_bytes()
        try:matching=raw==module.synthesize_placeholder(raw,int(p.stem));error=None
        except ValueError as exc:matching=None;error=str(exc)
        hfl.append({'map_id':int(p.stem),'sha256':sha(p),'bytes':len(raw),'exact_generator_match':matching,'error':error})
    original={'available':False,'reason':'LFS archive reference not provided'}
    if reference:
        original={'available':True,'sha256':sha(reference),'bytes':reference.stat().st_size,'dimensions':json.loads(subprocess.check_output([str(probe),'--hfl',str(reference)],text=True))}
    return {'schema':1,'project':'tyj1987/MoxiangReborn','baseline':BASE,'scope':'only current decoded table records, loose HFL files and supplied hash-verifiable reference; not full active-gameplay coverage',
        'sources':evidence,'map10':{'name':names.get(10),'raw_name_hex':name_hex.get(10),'name_codec':'cp950, strictly decoded for this record only','anchors':anchors,'transitions':links},'map17':{'name':names.get(17),'raw_name_hex':name_hex.get(17)},'loose_hfl_count':len(hfl),'exact_generator_matches':sum(x['exact_generator_match'] is True for x in hfl),'loose_hfl':hfl,'map10_archive_reference':original,'mutation':'none'}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,required=True);p.add_argument('--probe',type=Path,required=True);p.add_argument('--reference',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if a.output.exists():p.error('Refusing to overwrite report')
    result=audit(a.repo,a.probe,a.reference);a.output.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps({k:v for k,v in result.items() if k not in ['loose_hfl','sources']},ensure_ascii=False,indent=2))
