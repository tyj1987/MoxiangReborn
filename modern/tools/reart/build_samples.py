"""Reproducible ORIGINAL technical-art samples. No legacy artwork input.

Usage: python build_samples.py --output NEW_DIRECTORY
These are pipeline samples, not final characters/maps or release assets.
"""
from __future__ import annotations
import argparse, json, math, random, wave
from pathlib import Path
from native_writer import export_native, export_motion, export_glb, dds_rgba
from manifest import BASELINE, PROJECT, PROFILE, digest, validate

MATERIALS=[
 {'name':'forged_steel','rgb':[171,187,192],'metallic':.9,'roughness':.3},
 {'name':'aged_bronze','rgb':[133,101,55],'metallic':.8,'roughness':.42},
 {'name':'indigo_weave','rgb':[35,62,72],'roughness':.86},
 {'name':'lacquer_wood','rgb':[66,32,23],'roughness':.34},
 {'name':'mountain_stone','rgb':[116,113,100],'roughness':.94},
 {'name':'roof_slate','rgb':[48,58,57],'roughness':.88},
 {'name':'structural_wood','rgb':[86,55,33],'roughness':.83},
 {'name':'silk_tassel','rgb':[114,39,31],'roughness':.6}]

class Mesh:
    def __init__(self,name,material):
        self.data={'name':name,'material':material,'positions':[],'normals':[],'uvs':[],'indices':[]}
    def quad(self,a,b,c,d):
        u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
        n=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
        length=math.sqrt(sum(x*x for x in n))
        if length<1e-12:raise ValueError('Degenerate authoring face')
        n=[x/length for x in n];base=len(self.data['positions'])
        self.data['positions'] += [list(p) for p in [a,b,c,d]]
        self.data['normals'] += [n[:]]*4;self.data['uvs'] += [[0,0],[1,0],[1,1],[0,1]]
        self.data['indices'] += [base,base+1,base+2,base,base+2,base+3]
    def box(self,center,size):
        x,y,z=center;a,b,c=[v/2 for v in size]
        p=[(x+dx*a,y+dy*b,z+dz*c) for dx,dy,dz in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
        for ids in [(0,3,2,1),(4,5,6,7),(0,4,7,3),(1,2,6,5),(3,7,6,2),(0,1,5,4)]:self.quad(*(p[i] for i in ids))
    def rings(self,rings,center=(0,0,0),sides=16):
        x,y,z=center
        for (h0,rx0,rz0),(h1,rx1,rz1) in zip(rings,rings[1:]):
            for i in range(sides):
                t0=2*math.pi*i/sides;t1=2*math.pi*(i+1)/sides
                self.quad((x+rx0*math.cos(t0),y+h0,z+rz0*math.sin(t0)),(x+rx1*math.cos(t0),y+h1,z+rz1*math.sin(t0)),(x+rx1*math.cos(t1),y+h1,z+rz1*math.sin(t1)),(x+rx0*math.cos(t1),y+h0,z+rz0*math.sin(t1)))
        for ri,reverse in [(0,True),(-1,False)]:
            h,rx,rz=rings[ri]
            for i in range(1,sides-1):
                ids=[0,i,i+1] if reverse else [0,i+1,i]
                pp=[(x+rx*math.cos(2*math.pi*j/sides),y+h,z+rz*math.sin(2*math.pi*j/sides)) for j in ids]
                base=len(self.data['positions']);self.data['positions'] += [list(p) for p in pp]
                self.data['normals'] += [[0,-1 if reverse else 1,0]]*3;self.data['uvs'] += [[0,0],[1,0],[.5,1]];self.data['indices'] += [base,base+1,base+2]

def sword():
    blade=Mesh('ridge_forged_blade',0)
    sections=[(.03,.028,.004),(.10,.030,.005),(.55,.024,.004),(.78,.018,.003),(.875,.0008,.0005)]
    blade.rings(sections,sides=4)
    guard=Mesh('bronze_guard_and_pommel',1)
    guard.rings([(-.025,.066,.019),(-.020,.088,.023),(-.007,.074,.020),(.002,.041,.010),(.018,.035,.009)],sides=16)
    guard.rings([(-.205,.024,.014),(-.198,.030,.018),(-.184,.027,.016),(-.178,.018,.011)],sides=16)
    grip=Mesh('wrapped_grip',2);grip.rings([(-.178,.014,.011),(-.04,.017,.013),(-.03,.021,.014)],sides=12)
    binding=Mesh('grip_wrap_seams',3)
    for i in range(10):
        y=-.168+i*.013
        binding.rings([(y,.0155,.012),(y+.003,.0155,.012)],sides=12)
    bones=[]
    for i in range(5):
        bones.append({'index':i,'name':'tassel_'+str(i),'parent':0xffffffff if i==0 else i-1,'local':[0,-.205 if i==0 else -.042,0], 'world':[0,-.205-.042*i,0]})
    tassel=Mesh('skinned_silk_tassel',7)
    for strand in range(12):
        x=(strand-5.5)*.0021
        tassel.rings([(0,.0007,.0007),(.160,.0007,.0007)],center=(x,-.377,.009*math.sin(strand)),sides=4)
    skin=[]
    for p,n in zip(tassel.data['positions'],tassel.data['normals']):
        t=max(0.,min(3.999,(-p[1]-.205)/.042));lo=int(t);fraction=t-lo
        weights=[]
        for bone,w in [(lo,1-fraction),(lo+1,fraction)]:
            if w>1e-6:weights.append([bone,w,[p[j]-bones[bone]['world'][j] for j in range(3)],n[:]])
        skin.append(weights)
    tassel.data['skin']=skin
    return [m.data for m in [blade,guard,grip,binding,tassel]],bones

def scabbard():
    body=Mesh('lacquer_scabbard',3);body.rings([(-.02,.029,.010),(.68,.025,.010),(.79,.018,.009),(.84,.007,.006)],sides=12)
    mounts=Mesh('scabbard_bronze_fittings',1)
    for y,width in [(0,.029),(.12,.029),(.69,.025),(.78,.020)]:mounts.rings([(y,width+.002,.013),(y+.022,width+.002,.013)],sides=12)
    mounts.box((.037,.12,0),(.016,.035,.011));mounts.box((.034,.69,0),(.016,.035,.011))
    return [body.data,mounts.data]

def mountain_gate():
    stone=Mesh('stone_plinths',4);wood=Mesh('posts_beams_and_brackets',6);roof=Mesh('curved_roof_tiles',5);bronze=Mesh('joint_pins',1)
    for x in [-1.75,1.75]:
        stone.box((x,.13,0),(.68,.26,.72));stone.box((x,.31,0),(.51,.10,.56))
        wood.rings([(.36,.18,.18),(3.15,.145,.145)],center=(x,0,0),sides=16)
        for y in [.47,2.5,2.82]:bronze.rings([(y,.185 if y<1 else .159,.185 if y<1 else .159),(y+.026,.185 if y<1 else .159,.185 if y<1 else .159)],center=(x,0,0),sides=16)
        for level in range(3):
            wood.box((x,2.93+level*.14,0),(.58+level*.25,.13,.30 if level%2==0 else .9))
    wood.box((0,2.64,0),(3.8,.22,.24));wood.box((0,3.27,0),(4.5,.20,.4))
    wood.box((0,3.45,0),(4.72,.12,.28))
    columns=28
    for k in range(columns):
        xc=-2.32+k*(4.64/(columns-1));width=.08
        for side in [-1,1]:
            for depth in range(8):
                z0=depth*.13;z1=(depth+1)*.13
                for curve in range(6):
                    a0=math.pi*curve/6;a1=math.pi*(curve+1)/6
                    def point(a,z):
                        return (xc+width*math.cos(a),3.73-.64*z+.27*z*z+.05*math.sin(a)+.10*(abs(xc)/2.32)**5,side*z)
                    pts=[point(a0,z0),point(a0,z1),point(a1,z1),point(a1,z0)]
                    if side<0:pts=pts[::-1]
                    roof.quad(*pts)
    wood.box((0,3.76,0),(4.9,.13,.15))
    return [m.data for m in [stone,wood,roof,bronze]]

def write_textures(root):
    paths=[];size=128
    for mat in MATERIALS:
        pixels=bytearray()
        for y in range(size):
            for x in range(size):
                t=2*math.pi/size
                if 'wood' in mat['name']:v=8*math.sin(t*y*10+2*math.sin(t*x))+3*math.cos(t*x*9)
                elif 'weave' in mat['name'] or 'silk' in mat['name']:v=5*math.cos(t*x*32)*math.cos(t*y*32)
                elif 'steel' in mat['name']:v=4*math.sin(t*y*21)+2*math.cos(t*x*9)
                else:v=5*math.sin(t*x*7)*math.cos(t*y*9)+2*math.cos(t*(x+y)*17)
                pixels.extend([max(0,min(255,round(c+v))) for c in mat['rgb']]+[255])
        path=root/'runtime/textures'/f"{mat['name']}.dds";path.write_bytes(dds_rgba(size,size,pixels));paths.append(path)
    return paths

def texture_samples(root):
    results={};w,h=256,64
    for state,factor in [('normal',1),('hover',1.3),('pressed',.7),('disabled',.45)]:
        p=bytearray()
        for y in range(h):
            for x in range(w):
                edge=min(x,y,w-1-x,h-1-y)
                color=[131,105,62] if edge in (1,2,5) else ([38,43,40] if edge>0 else [0,0,0])
                p.extend([min(255,int(v*factor)) for v in color]+[255 if edge else 0])
        f=root/'runtime/ui'/f'button_{state}.dds';f.write_bytes(dds_rgba(w,h,p));results[state]=f.relative_to(root).as_posix()
    (root/'source/ui-button.json').write_text(json.dumps({'states':results,'nine_slice_px':[8,8,8,8],'text':'runtime-rendered; no font embedded','binding':None},indent=2),encoding='utf-8')
    w,h=256,64;p=bytearray()
    for y in range(h):
        for x in range(w):
            alpha=round(220*(math.sin(math.pi*y/(h-1))**3)*((x/(w-1))**.7))
            p.extend([184,215,215,alpha])
    (root/'runtime/vfx/weapon_trail.dds').write_bytes(dds_rgba(w,h,p))
    (root/'source/weapon-trail.json').write_text(json.dumps({'kind':'decorative_weapon_trajectory','skill_id':None,'authoritative_damage':False,'sample_actual_weapon_positions':True,'lifetime_seconds':.16,'required_cleanup':['cancel','death','map_unload'],'low_quality_preserves_gameplay_cues':True},indent=2),encoding='utf-8')

def wav_pcm(path,channels,rate=48000,bits=24):
    frames=bytearray()
    for i in range(len(channels[0])):
        for ch in channels:
            v=max(-.95,min(.95,ch[i]));n=int(v*((1<<(bits-1))-1));frames+=n.to_bytes(bits//8,'little',signed=True)
    with wave.open(str(path),'wb') as f:f.setnchannels(len(channels));f.setsampwidth(bits//8);f.setframerate(rate);f.writeframes(frames)

def audio_samples(root):
    rate=48000;duration=16;n=rate*duration
    lead=[[0.]*n,[0.]*n];bed=[[0.]*n,[0.]*n]
    melody=[(0,62,1.3),(.75,69,.7),(1.5,67,1.2),(3,64,1.8),(4.5,74,1.4),(6,69,.9),(7.25,67,1.1),(9,62,1.5),(10.5,64,1.0),(12,57,1.8),(14,62,1.3)]
    for note,(at,midi,length) in enumerate(melody):
        hz=440*2**((midi-69)/12);pan=.35+.3*(note%3)/2
        for i in range(int(length*rate)):
            t=i/rate;env=(1-math.exp(-t*180))*math.exp(-t*3.7)*(min(1,(length-t)*35))
            v=.17*env*(math.sin(2*math.pi*hz*t)+.24*math.sin(2*math.pi*hz*2.01*t)*math.exp(-t*8))
            j=(round(at*rate)+i)%n;lead[0][j]+=v*(1-pan);lead[1][j]+=v*pan
    for i in range(n):
        t=i/rate
        v=.025*(math.sin(2*math.pi*73.4375*t)+.5*math.sin(2*math.pi*110.125*t))
        bed[0][i]=v;bed[1][i]=v
    mix=[[lead[c][i]+bed[c][i] for i in range(n)] for c in range(2)]
    wav_pcm(root/'runtime/audio/mountain_motif.wav',mix)
    wav_pcm(root/'source/mountain_lead.wav',lead);wav_pcm(root/'source/mountain_bed.wav',bed)
    for kind,length in [('sword_whoosh',.4),('ui_confirm',.22),('stone_step',.18)]:
        rng=random.Random(20260917);data=[];filtered=0.
        for i in range(round(length*rate)):
            t=i/rate;env=math.sin(math.pi*t/length)**2
            noise=rng.uniform(-1,1);filtered=.82*filtered+.18*noise
            if kind=='ui_confirm':v=.10*math.sin(2*math.pi*(660+330*t/length)*t)*env
            elif kind=='stone_step':v=(.15*filtered+.06*math.sin(2*math.pi*95*t))*math.exp(-t*30)*min(1,t*800)
            else:v=.25*filtered*env
            data.append(v)
        wav_pcm(root/'runtime/audio'/f'{kind}.wav',[data])
    (root/'source/music-score.json').write_text(json.dumps({'authorship':'original algorithmic composition for this project; aesthetic review pending','seconds':16,'sample_rate':rate,'master_bit_depth':24,'melody':melody,'loop_samples':[0,n],'live_audio_binding':None},indent=2),encoding='utf-8')

def build(root:Path):
    if root.exists():raise ValueError('Output must be a new directory; original resources are never overwritten')
    for d in ['runtime/models','runtime/textures','runtime/animations','runtime/ui','runtime/vfx','runtime/audio','source','interchange','evidence']:(root/d).mkdir(parents=True,exist_ok=True)
    write_textures(root);models=[]
    meshes,bones=sword();models.append(('ridge_sword',meshes,bones))
    models.extend([('ridge_scabbard',scabbard(),[]),('songshan_gate_study',mountain_gate(),[])])
    for asset,meshes,bones in models:
        export_native(root,asset,meshes,MATERIALS,bones);export_glb(root,asset,meshes,MATERIALS)
        if bones:export_motion(root,asset,bones)
        (root/'source'/f'{asset}.mesh.json').write_text(json.dumps({'asset':asset,'unit':'meter','up':'Y','handedness':'right-handed authoring; live native culling calibration pending','technical_sample':True,'materials':MATERIALS,'bones':bones,'meshes':meshes},separators=(',',':')),encoding='utf-8')
    texture_samples(root);audio_samples(root)
    entries=[]
    groups=[('shared/materials','material_library',['runtime/textures/*'],{}),('weapon/ridge-sword','weapon',['runtime/models/ridge_sword.*','runtime/animations/*','source/ridge_sword.*','interchange/ridge_sword.*'],{'item_id':None}),('weapon/ridge-scabbard','weapon_accessory',['runtime/models/ridge_scabbard.*','source/ridge_scabbard.*','interchange/ridge_scabbard.*'],{'item_id':None}),('map/10/gate-study','environment_module',['runtime/models/songshan_gate_study.*','source/songshan_gate_study.*','interchange/songshan_gate_study.*'],{'reference_map_id':10,'placement_id':None}),('ui/button-study','ui_control',['runtime/ui/*','source/ui-button.json'],{'control_id':None}),('vfx/trail-study','decorative_vfx',['runtime/vfx/*','source/weapon-trail.json'],{'skill_id':None}),('audio/mountain-motif-study','audio',['runtime/audio/*','source/mountain_*.wav','source/music-score.json'],{'audio_event_id':None})]
    for aid,kind,patterns,binding in groups:
        files=sorted({p for pat in patterns for p in root.glob(pat)})
        entries.append({'asset_id':aid,'kind':kind,'stage':'authored','technical_sample':True,'gameplay_binding':binding,'provenance':{'method':'original procedural authoring; no legacy visual/audio input','review':'pending'},'dependencies':['shared/materials'] if kind in ('weapon','weapon_accessory','environment_module') else [],'files':[{'path':p.relative_to(root).as_posix(),'sha256':digest(p),'bytes':p.stat().st_size} for p in files],'acceptance_evidence':[]})
    manifest={'schema':1,'project':PROJECT,'profile':PROFILE,'baseline_sha':BASELINE,'legacy_fallback':False,'coverage':{'active_gameplay_inventory_complete':False},'required_assets':[a['asset_id'] for a in entries],'assets':entries}
    (root/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    report=validate(root)
    (root/'evidence/integrity.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    return report

if __name__=='__main__':
    cli=argparse.ArgumentParser(description=__doc__);cli.add_argument('--output',type=Path,required=True);args=cli.parse_args()
    print(json.dumps(build(args.output),ensure_ascii=False,indent=2))
