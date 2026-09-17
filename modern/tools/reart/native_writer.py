"""Deterministic export to CURRENT MOD/STM/ANM/CHX parser contracts.

No original artwork is consumed. Native coordinates are centimetres, Y-up;
meters are used in the editable source/interchange. Live catalog scaling,
handedness/culling and gameplay placement still require DX11 calibration.
"""
from __future__ import annotations
import json, math, struct
from pathlib import Path

IDENTITY = [1.,0,0,0, 0,1.,0,0, 0,0,1.,0, 0,0,0,1.]

def put(buf, offset, fmt, *values):
    struct.pack_into('<'+fmt, buf, offset, *values)

def name(buf, offset, value):
    raw = value.encode('ascii')
    if len(raw) > 127: raise ValueError('Native name exceeds 127 bytes')
    buf[offset:offset+len(raw)] = raw

def mesh_body(mesh, index, native_scale=100.):
    p,n,uv,idx = mesh['positions'],mesh['normals'],mesh['uvs'],mesh['indices']
    if not 0<len(p)<=65535 or len(p)!=len(n) or len(p)!=len(uv):
        raise ValueError('Native mesh vertex/normal/UV count')
    if len(idx)%3 or any(i<0 or i>=len(p) for i in idx): raise ValueError('Bad indices')
    header=bytearray(324); put(header,0,'I',index); put(header,32,'3f',1,1,1)
    put(header,60,'16f',*IDENTITY); put(header,192,'I',0xffffffff); name(header,196,mesh['name'])
    h=bytearray(48); put(h,4,'I',len(p)); put(h,16,'I',len(uv)); put(h,24,'II',1,2)
    data=header+h
    data += struct.pack('<'+'f'*(len(p)*3),*(v*native_scale for row in p for v in row))
    data += struct.pack('<'+'f'*(len(uv)*2),*(v for row in uv for v in row))
    face=bytearray(28); put(face,0,'I',mesh['material']); put(face,8,'I',len(idx)//3)
    data += face+struct.pack('<'+'H'*len(idx),*idx)
    skin=mesh.get('skin')
    if skin:
        records=[]; heads=bytearray()
        for weights in skin:
            heads+=struct.pack('<BI',len(weights),len(records))
            for bone,w,offset,normal in weights:
                records.append(struct.pack('<If6f',bone,w,*(v*native_scale for v in offset),*normal))
        data+=struct.pack('<III',len(skin),len(records),0)+heads+b''.join(records)
    else: data+=struct.pack('<III',0,0,0)
    data+=struct.pack('<'+'f'*(len(n)*3),*(v for row in n for v in row))
    return data

def export_native(root:Path, asset_name:str, meshes:list, materials:list, bones=None):
    bones=bones or []
    # FILE_SCENE_HEADER followed by typed material/object chunks.
    header=bytearray(28); put(header,0,'4I',1,len(meshes)+len(bones),len(materials),len(meshes)+len(bones))
    mod=header
    stm=bytearray(struct.pack('<II',1,len(materials)))
    for i,mat in enumerate(materials):
        rgb=mat['rgb']; diffuse=0xff000000 | (rgb[0]<<16) | (rgb[1]<<8) | rgb[2]
        tex='runtime/textures/'+mat['name']+'.dds'
        m=bytearray(1444); put(m,4,'I',diffuse); name(m,156,tex);put(m,1436,'II',i,0)
        mod+=struct.pack('<II',0xf1000000,len(m))+m
        s=bytearray(168);put(s,4,'I',i);put(s,12,'I',diffuse);name(s,36,tex);stm+=s
    stm+=struct.pack('<I',len(meshes))
    for i,mesh in enumerate(meshes):
        body=mesh_body(mesh,100+i);mod+=struct.pack('<II',0xf4000000,len(body))+body;stm+=body
    for bone in bones:
        b=bytearray(324);put(b,0,'I',bone['index']);put(b,8,'3f',*(v*100 for v in bone['local']))
        put(b,32,'3f',1,1,1);t=IDENTITY[:];t[12:15]=[v*100 for v in bone['world']];put(b,60,'16f',*t)
        put(b,192,'I',bone['parent']);name(b,196,bone['name']);mod+=struct.pack('<II',0xf5000000,len(b))+b
    (root/'runtime/models'/f'{asset_name}.mod').write_bytes(mod)
    if not bones:
        # Deliberately no collision objects: samples do not alter gameplay paths.
        stm+=struct.pack('<I',0);(root/'runtime/models'/f'{asset_name}.stm').write_bytes(stm)

def export_motion(root:Path,asset_name:str,bones:list,amplitude=.045):
    header=bytearray(160);put(header,0,'7I',1,160,0,120,30,len(bones),1)
    data=header
    for bone in bones:
        frames=list(range(0,121,5));h=bytearray(152);put(h,0,'5I',bone['index'],len(frames),1,0,0);name(h,20,bone['name'])
        payload=h+struct.pack('<II3f',0,0,*(v*100 for v in bone['local']))
        for frame in frames:
            angle=amplitude*math.sin(2*math.pi*frame/120)*min(bone['index'],3)
            payload+=struct.pack('<II4f',frame*160,frame,0,0,math.sin(angle/2),math.cos(angle/2))
        data+=struct.pack('<II',0xf5000000,len(payload))+payload
    path='runtime/animations/'+asset_name+'_idle.anm';(root/path).write_bytes(data)
    chx=f'*MOD_FILE_NUM\t1\n*MOD_FILE_NAME\truntime/models/{asset_name}.mod\n*MOTION_NUM\t1\n{path}\n'
    (root/'runtime/models'/f'{asset_name}.chx').write_text(chx,encoding='ascii')

def export_glb(root:Path,asset_name:str,meshes:list,materials:list):
    """Unskinned rest-pose exchange mesh; native skin/motion is separate."""
    binary=bytearray();views=[];access=[];out=[]
    def add(values,fmt,kind,component,count,bounds=False):
        while len(binary)%4:binary.append(0)
        offset=len(binary);binary.extend(struct.pack('<'+fmt*len(values),*values))
        vi=len(views);views.append({'buffer':0,'byteOffset':offset,'byteLength':len(binary)-offset})
        a={'bufferView':vi,'componentType':component,'count':count,'type':kind}
        if bounds:a.update(min=[min(values[i::3]) for i in range(3)],max=[max(values[i::3]) for i in range(3)])
        access.append(a);return len(access)-1
    for mesh in meshes:
        p=add([v for x in mesh['positions'] for v in x],'f','VEC3',5126,len(mesh['positions']),True)
        n=add([v for x in mesh['normals'] for v in x],'f','VEC3',5126,len(mesh['normals']))
        uv=add([v for x in mesh['uvs'] for v in x],'f','VEC2',5126,len(mesh['uvs']))
        idx=add(mesh['indices'],'H','SCALAR',5123,len(mesh['indices']))
        out.append({'name':mesh['name'],'primitives':[{'attributes':{'POSITION':p,'NORMAL':n,'TEXCOORD_0':uv},'indices':idx,'material':mesh['material']}]})
    doc={'asset':{'version':'2.0','generator':'MoxiangReborn original authoring v0.1 (rest pose)'},'scene':0,'scenes':[{'nodes':list(range(len(out)))}],
         'nodes':[{'mesh':i,'name':m['name']} for i,m in enumerate(out)],'meshes':out,'buffers':[{'byteLength':len(binary)}],'bufferViews':views,'accessors':access,
         'materials':[{'name':m['name'],'pbrMetallicRoughness':{'baseColorFactor':[v/255 for v in m['rgb']]+[1],'metallicFactor':m.get('metallic',0),'roughnessFactor':m.get('roughness',.7)}} for m in materials]}
    text=json.dumps(doc,separators=(',',':')).encode();text+=b' '*((-len(text))%4);binary+=b'\0'*((-len(binary))%4)
    glb=struct.pack('<III',0x46546c67,2,12+8+len(text)+8+len(binary))+struct.pack('<II',len(text),0x4e4f534a)+text+struct.pack('<II',len(binary),0x004e4942)+binary
    (root/'interchange'/f'{asset_name}.glb').write_bytes(glb)

def dds_rgba(width,height,pixels:bytes):
    if len(pixels)!=width*height*4:raise ValueError('DDS pixel count')
    h=bytearray(124);put(h,0,'7I',124,0x100f,height,width,width*4,0,0)
    put(h,72,'8I',32,0x41,0,32,0xff,0xff00,0xff0000,0xff000000);put(h,104,'I',0x1000)
    return b'DDS '+h+pixels
