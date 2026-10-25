"""Minimal TunerStudio ini parser (analysis + validation). Read-only on input files."""
import re
TYPESIZE={'U08':1,'S08':1,'U16':2,'S16':2,'U32':4,'S32':4,'F32':4,'U8':1,'S8':1}
TYPERANGE={'U08':(0,255),'U8':(0,255),'S08':(-128,127),'S8':(-128,127),'U16':(0,65535),'S16':(-32768,32767),
           'U32':(0,2**32-1),'S32':(-2**31,2**31-1),'F32':(-3.4e38,3.4e38)}
def read(path):
    raw=open(path,'rb').read()
    try: return raw.decode('utf-8')
    except UnicodeDecodeError: return raw.decode('latin-1')
def strip_comment(line):
    out=[];q=False
    for ch in line:
        if ch=='"': q=not q
        if ch==';' and not q: break
        out.append(ch)
    return ''.join(out).rstrip()
def split_args(s):
    parts=[];cur='';q=False;depth=0
    for ch in s:
        if ch=='"': q=not q
        if not q and ch in '{[': depth+=1
        if not q and ch in '}]': depth-=1
        if ch==',' and not q and depth==0:
            parts.append(cur.strip());cur=''
        else: cur+=ch
    parts.append(cur.strip()); return parts
def parse(path):
    secs={};order=[];cur=None
    for i,line in enumerate(read(path).splitlines(),1):
        l=strip_comment(line).strip()
        if not l: continue
        m=re.match(r'^\[([^\]]+)\]$',l)
        if m:
            cur=m.group(1).strip(); order.append(cur); secs.setdefault(cur,[]); continue
        if cur is None: continue
        if l.startswith('#'):
            secs[cur].append((i,'#',l)); continue
        if '=' in l:
            k,v=l.split('=',1); secs[cur].append((i,k.strip(),v.strip()))
        else: secs[cur].append((i,l,''))
    return secs,order
def num(s):
    try: return float(s)
    except: return None
def _fix_offset(a,warn,i,name):
    # tolerate a missing comma after the offset ("16 "C"") but report it
    m=re.match(r'^(-?\d+)\s+(".*")$',a[2]) if len(a)>2 else None
    if m:
        warn.append((i,f'{name}: saknat kommatecken efter offset'))
        a=a[:2]+[m.group(1),m.group(2)]+a[3:]
    return a
def constants(secs,warn=None):
    warn=warn if warn is not None else []
    res=[];page=None;pcfg={}
    for i,k,v in secs.get('Constants',[]):
        if k=='#': continue
        if k=='page': page=int(v); continue
        a=split_args(v) if v else []
        if a and a[0] in ('scalar','array','bits','string'):
            a=_fix_offset(a,warn,i,k)
            d={'line':i,'name':k,'kind':a[0],'page':page,'type':a[1] if len(a)>1 else ''}
            d['offset']=int(a[2]) if len(a)>2 and re.match(r'^-?\d+$',a[2]) else None
            if a[0]=='array':
                dims=re.findall(r'\d+',a[3]); n=1
                for x in dims: n*=int(x)
                d['count']=n; d['shape']=a[3]; rest=a[4:]
            elif a[0]=='bits':
                d['count']=1; d['bits']=a[3]; rest=[]; d['options']=a[4:]
            elif a[0]=='string':
                d['count']=int(a[3]) if len(a)>3 and a[3].isdigit() else 1; rest=[]
            else:
                d['count']=1; rest=a[3:]
            d['size']=(TYPESIZE.get(d['type'],0) if a[0]!='string' else 1)*d['count']
            for key,idx in (('units',0),('scale',1),('translate',2),('lo',3),('hi',4),('digits',5)):
                d[key]=rest[idx] if len(rest)>idx else ''
            res.append(d)
        else: pcfg.setdefault(k,v)
    return res,pcfg
def outputs(secs,warn=None):
    warn=warn if warn is not None else []
    res=[];cfg={}
    for i,k,v in secs.get('OutputChannels',[]):
        if k=='#': continue
        a=split_args(v) if v else []
        if a and a[0] in ('scalar','bits','array'):
            a=_fix_offset(a,warn,i,k)
            d={'line':i,'name':k,'kind':a[0],'type':a[1],'offset':int(a[2]) if len(a)>2 and re.match(r'^-?\d+$',a[2]) else None}
            d['size']=TYPESIZE.get(d['type'],0)
            if a[0]=='scalar':
                d['units']=a[3] if len(a)>3 else ''; d['scale']=a[4] if len(a)>4 else ''; d['translate']=a[5] if len(a)>5 else ''
            else:
                d['bits']=a[3] if len(a)>3 else ''; d['units']='';d['scale']='';d['translate']=''
            res.append(d)
        elif v.startswith('{'):
            expr=v[:v.rfind('}')+1] if '}' in v else v
            res.append({'line':i,'name':k,'kind':'formula','expr':expr,'size':0,'offset':None,'units':''})
        else: cfg[k]=v
    return res,cfg
def pcvars(secs):
    out=[]
    for i,k,v in secs.get('PcVariables',[]):
        if k!='#' and v: out.append((i,k))
    return out
def header(secs):
    h={}
    for s in ('MegaTune','TunerStudio'):
        for i,k,v in secs.get(s,[]):
            if k in('signature','queryCommand','versionInfo','iniSpecVersion','MTversion') and k not in h: h[k]=v
    return h
