#!/usr/bin/env python3
"""Statisk validering av TunerStudio ini-filer for SmedbyCharger.
Kontrollerar: sektioner, signatur, unika namn, offset/storlek inom pageSize och utan overlapp,
typintervall for lo/hi, bitfalt, OutputChannels-tackning == ochBlockSize, och att varje
referens (dialog-falt, kurvor, tabeller, matare, FrontPage, Datalog, formler, kommandon,
defaultValue) pekar pa nagot som finns.
v2.1: EEPROM-summa <= --eeprom (1024), sidor <= 256 B vid %o, matare i dialoger, unika kommandon,
formelordning, och AUTOLAGE: varje ovr_X har man_X + eff_X + pa_X med samma typ/enhet, man_X ar
villkorat av ovr_X i dialog och eff_X visas som matare i en dialog.
v2.2: [EventTriggers] (triggeredPageRefresh/timedPageRefresh): sida finns, uttryck anvander bara
OutputChannels, iniSpecVersion >= 3.36 (3.29 for timed); kedjade kommandon i [ControllerCommands] finns.  Kor: python3 validate_ini.py fil.ini [--sig "..."] [--och N] [--page N,..]
Detta ar en skriptkontroll - INTE ett test i riktiga TunerStudio."""
import re,sys,argparse
from tsparse import *
BUILTIN={'time','secl','deadValue','CELSIUS','true','false','dataLogTime','timeNow','ochTime'}
FUNCS={'abs','min','max','sqrt','log','exp','pow','round','floor','ceil','isNaN','timeNow','lastValue','smoothBasic','tableLookup','arrayValue','sin','cos','tan','if','accumulate','isOnline','isAdvancedMathAvailable','bitStringValue','stringValue'}
def ids(expr):
    e=re.sub(r'"[^"]*"','',expr)
    toks=set(re.findall(r'\b[A-Za-z_][A-Za-z0-9_]*\b',e))
    return {t for t in toks if not re.match(r'^0x',t)}
def bitrange(b):
    m=re.match(r'\[(\d+):(\d+)\]',b.replace(' ',''))
    return (int(m.group(1)),int(m.group(2))) if m else None
def validate(path,exp_sig=None,exp_och=None,exp_pages=None,eeprom=1024):
    E=[];W=[]
    secs,order=parse(path); warn=[]
    cons,pcfg=constants(secs,warn); outs,ocfg=outputs(secs,warn); pcv=pcvars(secs); hdr=header(secs)
    W+= [f'rad {i}: {m}' for i,m in warn]
    for s in ('Constants','OutputChannels'):
        if s not in secs: E.append(f'sektion [{s}] saknas')
    for s in set(order):
        if order.count(s)>1: E.append(f'sektion [{s}] finns {order.count(s)} ganger')
    sig=hdr.get('signature','').strip('"') if hdr.get('signature','').startswith('"') else hdr.get('signature')
    if sig is None: E.append('signature saknas')
    elif exp_sig is not None and sig!=exp_sig: E.append(f'signature "{sig}" != forvantad "{exp_sig}"')
    for k in ('queryCommand','versionInfo'):
        if k not in hdr: W.append(f'{k} saknas i [MegaTune]/[TunerStudio]')
    # page config
    nP=int(pcfg.get('nPages','1').split(',')[0])
    sizes=[int(x) for x in re.findall(r'\d+',pcfg.get('pageSize','0'))]
    if len(sizes)!=nP: E.append(f'pageSize har {len(sizes)} varden men nPages={nP}')
    for key in ('pageReadCommand','pageValueWrite','pageChunkWrite','burnCommand','pageActivate','pageIdentifier'):
        if key in pcfg and nP>1:
            n=len(split_args(pcfg[key]))
            if n!=nP: E.append(f'{key} har {n} varden men nPages={nP}')
    if exp_pages and sizes!=exp_pages: E.append(f'pageSize {sizes} != forvantat {exp_pages}')
    # v2.1: EEPROM-budget och 1-byte-offset (%o)
    if eeprom and sum(sizes)>eeprom: E.append(f'EEPROM: summa pageSize {sum(sizes)} B > {eeprom} B')
    if '%o' in pcfg.get('pageValueWrite','')+pcfg.get('pageChunkWrite','') and '%2o' not in pcfg.get('pageValueWrite',''):
        for i,sz in enumerate(sizes):
            if sz>256: E.append(f'sida {i+1}: {sz} B men W/X anvander 1-byte offset (%o) - max 256')
    en=pcfg.get('endianness')
    if en=='litle': W.append('endianness="litle" (felstavat; skriv little)')
    elif en not in ('little','big'): E.append(f'endianness="{en}" (maste vara little/big)')
    # names
    names={}
    for d in cons: names.setdefault(d['name'],[]).append(('Constants',d['line']))
    for i,k in pcv: names.setdefault(k,[]).append(('PcVariables',i))
    for d in outs: names.setdefault(d['name'],[]).append(('OutputChannels',d['line']))
    for n,l in names.items():
        if len(l)>1: E.append(f'namn "{n}" definieras {len(l)} ganger: {l}')
    # constants layout
    used={}
    for d in cons:
        if d['page'] is None: E.append(f'{d["name"]}: ingen page = ... fore variabeln'); continue
        if d['offset'] is None: E.append(f'{d["name"]} (rad {d["line"]}): ogiltig offset'); continue
        if d['type'] not in TYPESIZE: E.append(f'{d["name"]}: okand typ {d["type"]}'); continue
        ps=sizes[d['page']-1] if 0<d['page']<=len(sizes) else None
        if ps is None: E.append(f'{d["name"]}: page {d["page"]} finns inte'); continue
        if d['offset']<0 or d['offset']+d['size']>ps:
            E.append(f'{d["name"]}: offset {d["offset"]}+{d["size"]} utanfor pageSize {ps} (sida {d["page"]})')
        tw=TYPESIZE[d['type']]*8
        br=bitrange(d['bits']) if d['kind']=='bits' else None
        if d['kind']=='bits':
            if not br or br[0]>br[1] or br[1]>=tw: E.append(f'{d["name"]}: bitintervall {d["bits"]} ogiltigt for {d["type"]}')
            else:
                n=2**(br[1]-br[0]+1); no=len(d['options'])
                if no>n: E.append(f'{d["name"]}: {no} alternativ men bara {n} mojliga')
                elif no<n: W.append(f'{d["name"]}: {no} alternativ for {n} lagen (TS fyller ut, men "INVALID" rekommenderas)')
        for b in range(d['offset'],d['offset']+d['size']):
            key=(d['page'],b)
            mask=set(range(br[0]+8*(b-d['offset']) if False else br[0],br[1]+1)) if br else None
            for other,omask in used.get(key,[]):
                if mask is None or omask is None or (mask & omask):
                    E.append(f'overlapp sida {d["page"]} byte {b}: {other} och {d["name"]}'); break
            used.setdefault(key,[]).append((d['name'],mask))
        if d['kind'] in('scalar','array'):
            sc=num(d['scale']); tr=num(d['translate']) or 0.0; lo=num(d['lo']); hi=num(d['hi'])
            if sc in (None,0): E.append(f'{d["name"]}: ogiltig skala "{d["scale"]}"')
            elif lo is not None and hi is not None:
                if lo>hi: E.append(f'{d["name"]}: lo {lo} > hi {hi}')
                rmin,rmax=TYPERANGE[d['type']]
                for v,lab in ((lo,'lo'),(hi,'hi')):
                    raw=v/sc-tr
                    if raw<rmin-1e-6 or raw>rmax+1e-6: E.append(f'{d["name"]}: {lab}={v} ger raavarde {raw:g} utanfor {d["type"]} [{rmin},{rmax}]')
            if not d['units'].startswith('"'): W.append(f'{d["name"]}: enhet saknar citattecken')
    # output channels
    och=int(re.findall(r'\d+',ocfg.get('ochBlockSize','0'))[0]) if ocfg.get('ochBlockSize') else None
    if och is None: E.append('ochBlockSize saknas')
    if exp_och is not None and och!=exp_och: E.append(f'ochBlockSize {och} != forvantat {exp_och}')
    if 'ochGetCommand' not in ocfg: E.append('ochGetCommand saknas')
    cov={}
    for d in outs:
        if d['kind']=='formula': continue
        if d['offset'] is None or d['type'] not in TYPESIZE: E.append(f'{d["name"]} (rad {d["line"]}): ogiltig offset/typ'); continue
        br=bitrange(d.get('bits','')) if d['kind']=='bits' else None
        if och is not None and d['offset']+d['size']>och: E.append(f'{d["name"]}: offset {d["offset"]}+{d["size"]} > ochBlockSize {och}')
        for b in range(d['offset'],d['offset']+d['size']):
            mask=set(range(br[0],br[1]+1)) if br else None
            for other,omask in cov.get(b,[]):
                if (mask is None) != (omask is None): continue  # bitfalt som alias over en skalar ar tillatet (lasning)
                if mask is None or (mask & omask):
                    E.append(f'OutputChannels overlapp byte {b}: {other} och {d["name"]}'); break
            cov.setdefault(b,[]).append((d['name'],mask))
    if och is not None:
        missing=[b for b in range(och) if b not in cov]
        if missing: E.append(f'OutputChannels tacker inte byte {missing} av ochBlockSize {och}')
        rawsum=sum(d['size'] for d in outs if d['kind'] in('scalar','array') )
        bitsbytes=len({b for d in outs if d['kind']=='bits' for b in range(d['offset'] or 0,(d['offset'] or 0)+d['size'])} - {b for d in outs if d['kind']=='scalar' for b in range(d['offset'] or 0,(d['offset'] or 0)+d['size'])})
        if rawsum+bitsbytes!=och: W.append(f'summa skalar-storlekar {rawsum}+bitbytes {bitsbytes} != ochBlockSize {och}')
    chan={d['name'] for d in outs}; const={d['name'] for d in cons}|{k for i,k in pcv}
    allv=chan|const|BUILTIN
    def chk(expr,where):
        for t in ids(expr)-allv-FUNCS:
            E.append(f'{where}: okand variabel "{t}" i {expr.strip()}')
    seen=set(); fnames={d['name'] for d in outs if d['kind']=='formula'}
    for d in outs:
        if d['kind']=='formula':
            chk(d['expr'],f'formel {d["name"]}')
            later=(ids(d['expr'])&fnames)-seen-{d['name']}
            if later: W.append(f'formel {d["name"]} anvander formler som definieras senare: {sorted(later)}')
            if d['name'] in ids(d['expr']): E.append(f'formel {d["name"]} refererar sig sjalv')
            seen.add(d['name'])
    # UserDefined / dialogs
    dialogs=set();curves=set();tables=set();gauges={};cmds=set();helps=set()
    for i,k,v in secs.get('UserDefined',[]):
        if k=='dialog': dialogs.add(split_args(v)[0])
        if k=='help': helps.add(split_args(v)[0])
    for i,k,v in secs.get('CurveEditor',[]):
        if k=='curve': curves.add(split_args(v)[0])
    for i,k,v in secs.get('TableEditor',[]):
        if k=='table': a=split_args(v); tables.add(a[0]); tables.add(a[1]) if len(a)>1 else None
    for i,k,v in secs.get('ControllerCommands',[]):
        if k!='#': cmds.add(k)
    for i,k,v in secs.get('GaugeConfigurations',[]):
        if k in('#','gaugeCategory'): continue
        if k in gauges: E.append(f'matare "{k}" definieras flera ganger')
        a=split_args(v); gauges[k]=a
        if len(a)<11: E.append(f'matare {k}: {len(a)} falt (11 kravs)')
        if a[0] not in chan|BUILTIN: E.append(f'matare {k}: kanal "{a[0]}" finns inte i OutputChannels')
        nums=[num(x) for x in a[3:9]] if len(a)>=9 else []
        if nums and None not in nums:
            lo,hi,lod,low,hiw,hid=nums
            if lo>=hi: E.append(f'matare {k}: min {lo} >= max {hi}')
    targets=dialogs|curves|tables|{'std_realtime','std_separator','std_ms2gentherm','std_constants','std_injection','std_accel','std_warmup','std_trigwiz','std_ms3sdConsole','std_port_edit','std_hiddenFile'}
    cur_dialog=None; dlg_gauges=set()
    for i,k,v in secs.get('UserDefined',[]):
        a=split_args(v) if v else []
        if k=='dialog': cur_dialog=a[0]
        elif k in('field','displayOnlyField') and len(a)>=2 and a[1] and not a[1].startswith('{'):
            if a[1] not in const: E.append(f'rad {i} dialog {cur_dialog}: falt "{a[1]}" finns inte i Constants/PcVariables')
            for x in a[2:]:
                if x.startswith('{'): chk(x,f'rad {i} villkor')
        elif k=='panel':
            if a[0] not in targets: E.append(f'rad {i} dialog {cur_dialog}: panel "{a[0]}" finns inte')
            for x in a[1:]:
                if x.startswith('{'): chk(x,f'rad {i} villkor')
        elif k=='commandButton':
            if len(a)<2 or a[1] not in cmds: E.append(f'rad {i}: commandButton -> "{a[1] if len(a)>1 else ""}" saknas i [ControllerCommands]')
            for x in a[2:]:
                if x.startswith('{'): chk(x,f'rad {i} villkor')
        elif k=='topicHelp' and not a[0].startswith('"') and a[0] not in helps:
            E.append(f'rad {i}: topicHelp "{a[0]}" finns inte')
        elif k=='gauge' and cur_dialog:
            if a[0] not in gauges: E.append(f'rad {i} dialog {cur_dialog}: matare "{a[0]}" finns inte')
            else: dlg_gauges.add(a[0])
        elif k=='help' and cur_dialog and False: pass
    for i,k,v in secs.get('Menu',[]):
        a=split_args(v) if v else []
        if k=='subMenu' and a[0] not in targets: E.append(f'rad {i}: subMenu "{a[0]}" pekar inte pa dialog/kurva/tabell')
        for x in a[1:]:
            if x.startswith('{'): chk(x,f'rad {i} menyvillkor')
    for sec,arrk in (('CurveEditor',('xBins','yBins')),('TableEditor',('xBins','yBins','zBins'))):
        cur=None
        for i,k,v in secs.get(sec,[]):
            a=split_args(v) if v else []
            if k in('curve','table'): cur=a[0]
            elif k in arrk:
                if a[0] not in const: E.append(f'{sec} {cur}: {k} "{a[0]}" finns inte i Constants')
                if len(a)>1 and a[1] and a[1] not in chan: E.append(f'{sec} {cur}: {k} kanal "{a[1]}" finns inte i OutputChannels')
            elif k.startswith('gauge') and a[0] not in gauges: E.append(f'{sec} {cur}: matare "{a[0]}" finns inte')
    # curve x/y array sizes
    csize={d['name']:d['count'] for d in cons}
    for sec in ('CurveEditor',):
        cur=None;xb=None
        for i,k,v in secs.get(sec,[]):
            a=split_args(v) if v else []
            if k=='curve': cur=a[0]; xb=None
            elif k=='xBins': xb=a[0]
            elif k=='yBins' and xb in csize and a[0] in csize and csize[xb]!=csize[a[0]]:
                E.append(f'CurveEditor {cur}: xBins {xb}[{csize[xb]}] och yBins {a[0]}[{csize[a[0]]}] har olika langd')
    for i,k,v in secs.get('FrontPage',[]):
        a=split_args(v) if v else []
        if re.match(r'gauge\d+$',k) and a and a[0] and a[0] not in gauges: E.append(f'FrontPage {k}: matare "{a[0]}" finns inte')
        if k=='indicator' and a:
            chk(a[0],f'FrontPage indikator rad {i}')
            m=re.search(r'&\s*(\d+)\s*\}',a[0])
            if m and int(m.group(1)) not in (1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768) and not a[0].strip().startswith('{ (') :
                W.append(f'FrontPage indikator rad {i}: "& {m.group(1)}" ar inte en enskild bit (&0 ar alltid falskt)')
    for i,k,v in secs.get('Datalog',[]):
        a=split_args(v) if v else []
        if k=='entry' and a[0] not in chan|BUILTIN: E.append(f'Datalog: "{a[0]}" finns inte i OutputChannels')
    for i,k,v in secs.get('ConstantsExtensions',[]):
        a=split_args(v) if v else []
        if k in('defaultValue','requiresPowerCycle','readOnly') and a[0] not in const: E.append(f'ConstantsExtensions {k}: "{a[0]}" finns inte')
        elif k=='defaultValue' and len(a)>1:
            dd=[d for d in cons if d['name']==a[0]]
            if dd and dd[0]['kind']=='bits' and a[1] not in dd[0]['options']: E.append(f'defaultValue {a[0]}: {a[1]} ar inte ett av bit-alternativen')
            if dd and dd[0]['kind']=='scalar' and num(a[1]) is not None and num(dd[0]['lo']) is not None and not (num(dd[0]['lo'])<=num(a[1])<=num(dd[0]['hi'])):
                E.append(f'defaultValue {a[0]}: {a[1]} utanfor {dd[0]["lo"]}..{dd[0]["hi"]}')
    # kommandon: unika bytestrangar
    cv={}
    for i,k,v in secs.get('ControllerCommands',[]):
        if k!='#': cv.setdefault(v.strip(),[]).append(k)
    for v,ks in cv.items():
        if len(ks)>1: E.append(f'kommandona {ks} skickar samma bytes {v}')
    # ---- AUTOLAGE (v2.1): ovr_X -> man_X (sida) + eff_X (realtime) + pa_X (formel), visning i dialog
    cmap={d['name']:d for d in cons}; omap={d['name']:d for d in outs}
    dfield={}; cur=None
    for i,k,v in secs.get('UserDefined',[]):
        a=split_args(v) if v else []
        if k=='dialog': cur=a[0]
        elif k=='field' and len(a)>=2: dfield.setdefault(a[1],[]).append(' '.join(a[2:]))
    ovr=[n[4:] for n in cmap if n.startswith('ovr_')]
    for X in ovr:
        o_=cmap['ovr_'+X]
        if o_['kind']!='bits' or [x.strip('"') for x in o_['options']]!=['Auto','Manuell']:
            E.append(f'ovr_{X}: ska vara bits med alternativen "Auto","Manuell"')
        m=cmap.get('man_'+X); e=omap.get('eff_'+X); pa=omap.get('pa_'+X)
        if m is None: E.append(f'ovr_{X}: man_{X} saknas i Constants')
        if e is None or e['kind']=='formula': E.append(f'ovr_{X}: effektivt varde eff_{X} saknas i OutputChannels (firmware-kanal)')
        if pa is None or pa['kind']!='formula': E.append(f'ovr_{X}: PC-forhandsvisning pa_{X} saknas (formel)')
        if m is not None and e is not None and e['kind']!='formula':
            if m['units']!=e['units']: E.append(f'{X}: enhet man_ {m["units"]} != eff_ {e["units"]}')
            if num(m['scale'])!=num(e['scale']) or m['type']!=e['type']: E.append(f'{X}: typ/skala man_ ({m["type"]},{m["scale"]}) != eff_ ({e["type"]},{e["scale"]})')
        if m is not None:
            conds=dfield.get('man_'+X)
            if not conds: E.append(f'man_{X} visas inte i nagon dialog')
            elif not any(('ovr_'+X) in c for c in conds): E.append(f'man_{X}: dialogfaltet saknar villkor med ovr_{X} (ska vara inaktivt i Auto)')
        if 'ovr_'+X not in dfield: E.append(f'ovr_{X} visas inte i nagon dialog')
        if not any(gauges[g][0]=='eff_'+X for g in dlg_gauges): E.append(f'eff_{X}: inget matare med effektivt varde i nagon dialog')
    for n in cmap:
        if n.startswith('man_') and n[4:] not in ovr: E.append(f'{n} saknar ovr_{n[4:]}')
    for n in omap:
        if n.startswith('eff_') and n[4:] not in ovr: E.append(f'{n} saknar ovr_{n[4:]}')
    if ovr:
        if len(ovr)>32: E.append(f'{len(ovr)} override-bitar ryms inte i effManualMask (U32)')
        if 'effManualMask' not in omap: E.append('effManualMask saknas i OutputChannels')
        if 'autoWarnBits' not in omap: E.append('autoWarnBits saknas i OutputChannels')
        for need in ('chargeMode','batteryType','capacityAh'):
            if need not in cmap: E.append(f'autolage: {need} saknas i Constants')
    # ---- v2.2: [EventTriggers] (automatisk omlasning av sidor) + kedjade kommandon
    try: spec=float(hdr.get('iniSpecVersion','0'))
    except ValueError: spec=0.0
    if 'iniSpecVersion' not in hdr:
        for i,k,v in secs.get('TunerStudio',[]):
            if k=='iniSpecVersion': spec=num(v) or 0.0
    ntrig=0
    for i,k,v in secs.get('EventTriggers',[]):
        if k=='#': continue
        a=split_args(v) if v else []
        if k in('triggeredPageRefresh','timedPageRefresh'):
            ntrig+=1
            need=3.36 if k=='triggeredPageRefresh' else 3.29
            if spec<need: E.append(f'rad {i}: {k} kraver iniSpecVersion >= {need} (har {spec})')
            pg=num(a[0]) if a else None
            if pg is None or not (1<=pg<=nP): E.append(f'rad {i}: {k} sida "{a[0] if a else ""}" finns inte (1..{nP})')
            if len(a)<2 or not a[1].startswith('{'): E.append(f'rad {i}: {k} saknar uttryck {{ ... }}')
            else:
                for t in ids(a[1])-chan-BUILTIN-FUNCS:
                    E.append(f'rad {i}: {k}-uttryck anvander "{t}" som inte ar en OutputChannel')
                for t in ids(a[1])&chan:
                    if omap[t]['kind']=='formula': W.append(f'rad {i}: {k} anvander formeln {t} - bor vara en firmware-kanal')
        else: W.append(f'rad {i}: okant nyckelord i [EventTriggers]: {k}')
    for i,k,v in secs.get('ControllerCommands',[]):
        if k=='#': continue
        for part in split_args(v):
            if part and not part.startswith('"') and part not in cmds: E.append(f'rad {i}: kommando {k} kedjar till okant kommando "{part}"')
    for i,k,v in secs.get('SettingContextHelp',[]):
        if k!='#' and k not in const: W.append(f'SettingContextHelp: "{k}" finns inte i Constants')
    stats=dict(signature=sig,pages=sizes,och=och,constants=len(cons),channels=len([d for d in outs if d['kind']!='formula']),
               formulas=len([d for d in outs if d['kind']=='formula']),dialogs=len(dialogs),curves=len(curves),tables=len(tables),
               gauges=len(gauges),commands=len(cmds),datalog=len([1 for i,k,v in secs.get('Datalog',[]) if k=='entry']),eeprom=sum(sizes),overrides=len(ovr),pageRefreshTriggers=ntrig)
    return E,W,stats
if __name__=='__main__':
    ap=argparse.ArgumentParser(); ap.add_argument('files',nargs='+'); ap.add_argument('--sig'); ap.add_argument('--och',type=int)
    ap.add_argument('--pages'); ap.add_argument('--eeprom',type=int,default=1024); ap.add_argument('-q',action='store_true',help='visa inte varningar')
    a=ap.parse_args(); bad=0
    for f in a.files:
        E,W,s=validate(f,a.sig,a.och,[int(x) for x in a.pages.split(',')] if a.pages else None,a.eeprom)
        print(f'== {f}\n   {s}')
        for e in E: print('   FEL:',e)
        if not a.q:
            for w in W: print('   varning:',w)
        print(f'   => {"OK" if not E else "FEL"} ({len(E)} fel, {len(W)} varningar)')
        bad+=bool(E)
    sys.exit(1 if bad else 0)
