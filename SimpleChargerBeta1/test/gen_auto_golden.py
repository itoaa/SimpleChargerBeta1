#!/usr/bin/env python3
"""Golden-vektorer for AutoParams (firmware) fran auto_params_v2.2.py.

Referensen ar test/ref/auto_params_v2.2.py (md5 36477a1af53ac70082480ed36e2e9eb2).
Drive-originalet (md5 ef56d1755987754a3a591b8dd32f4c74) har 48 V kvar; repots kopia
har 48 V borttagen enligt BESLUT nr 45 och ska laddas upp till Drive av Ola.
Fallen (rng-sekvensen) ar oforandrade, sa bara rader med nominell spanning
index 4 (48 V) skiljer sig mot tidigare golden. Skriptet kontrollerar md5, kor
referensens selftest och skriver en C++-include till stdout. Inget har
ar slumpmassigt mellan korningar (fast fro), sa utdata ar deterministisk.

Kor: python3 gen_auto_golden.py > build/golden_auto.inc
"""
import hashlib, importlib.util, io, os, random, sys, contextlib

HERE = os.path.dirname(os.path.abspath(__file__))
REF = os.path.join(HERE, 'ref', 'auto_params_v2.2.py')
REF_MD5 = '36477a1af53ac70082480ed36e2e9eb2'

if hashlib.md5(open(REF, 'rb').read()).hexdigest() != REF_MD5:
    sys.exit('auto_params_v2.2.py har andrats (md5) - uppdatera REF_MD5 medvetet')
spec = importlib.util.spec_from_file_location('ap22', REF)
ap = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ap)
with contextlib.redirect_stdout(io.StringIO()):
    ap.selftest()

P = ap.PNAMES
CT = {n: t for n, t, *_ in ap.PARAMS}
assert len(P) == 21
rng = random.Random(20261025)

def rnd_val(n):
    t = CT[n]
    if t == 'U08': return rng.randint(0, 255)
    if t == 'S08': return rng.randint(-128, 127)
    return rng.choice([rng.randint(0, 65535), rng.randint(0, 3000), rng.randint(0, 2000)])

def lim(n, hi):
    t = CT[n]
    if t == 'U08': return 255 if hi else 0
    if t == 'S08': return 127 if hi else -128
    return 65535 if hi else 0

# Manuella vektorer (man_*): noll, max, min, realistiska och slumpade.
MANS = [{n: 0 for n in P}, {n: lim(n, True) for n in P}, {n: lim(n, False) for n in P}]
for bt, ah in ((1, 400), (0, 600), (5, 1000), (2, 70)):
    e, _, _ = ap.effective(bt, ah)
    MANS.append(dict(e))
MANS.append(dict(MANS[3], bulkAmp=500, absorbVolt=1600, bulkMaxTime=300, floatMaxTime=0, tempCompMv=-40))
MANS.append(dict(MANS[3], tailAmp=2000, floatVolt=1500, chargeTempMin=50, chargeTempMax=10, restartVolt=1500))
MANS.append(dict(MANS[3], tempCompMv=-128, cellCount=255, voltLimit=65535))
MANS.append(dict(MANS[3], tempCompMv=-10, cellCount=6, voltLimit=1500, absorbVolt=1440))
while len(MANS) < 48:
    MANS.append({n: rnd_val(n) for n in P})

AHS = [0, 1, 7, 10, 25, 49, 50, 70, 100, 250, 400, 600, 1000, 1499, 2000, 5000, 10000, 30000, 65535]
cases = []
def add(bt, nomv, ah, ovr=0, man=0, tce=1, bats=0, cap=1500, floor=30, mca=0, mcv=0, sens=1):
    cases.append((bt, nomv, ah, ovr, man, tce, bats, cap, floor, mca, mcv, sens))

# A: typer x spanningar x Ah (standardvarden)
for bt in range(8):
    for nomv in range(8):
        for ah in AHS:
            add(bt, nomv, ah)
# B: gransvarden sida 1/3: MaxChargeAmp, MaxChargeVolt, givare
for bt in range(8):
    for ah in (0, 70, 400, 1000, 65535):
        for mca in (0, 1, 550, 1500, 1501, 65535):
            for mcv in (0, 1, 1400, 2900, 65535):
                for sens in range(4):
                    add(bt, 1, ah, mca=mca, mcv=mcv, sens=sens)
# C: autoAmpCap x tailAmpFloor
for bt in range(6):
    for ah in (0, 7, 70, 400, 2000):
        for cap in (0, 1, 500, 1500, 3000, 65535):
            for floor in (0, 1, 30, 500, 65535):
                add(bt, rng.randint(0, 4), ah, cap=cap, floor=floor)
# D: en override-bit i taget x manuella vektorer
for bit in range(21):
    for m in range(len(MANS)):
        for bt in (0, 1, 5, 6):
            add(bt, 1, rng.choice((0, 400, 1000)), ovr=1 << bit, man=m, bats=rng.randint(0, 1))
# E: slumpade kombinationer av allt
for _ in range(6000):
    add(rng.randint(0, 7), rng.choice((0, 1, 1, 2, 3, 4, 5, 7)), rng.choice(AHS + [rng.randint(0, 65535)]),
        ovr=rng.choice((0, (1 << 21) - 1, rng.getrandbits(21), 1 << rng.randint(0, 20))),
        man=rng.randrange(len(MANS)), tce=rng.randint(0, 1), bats=rng.randint(0, 1),
        cap=rng.choice((1500, 0, 800, rng.randint(0, 65535))), floor=rng.choice((30, 0, rng.randint(0, 2000))),
        mca=rng.choice((0, 0, 550, 1500, rng.randint(0, 65535))), mcv=rng.choice((0, 0, 1400, rng.randint(0, 65535))),
        sens=rng.randint(0, 3))

out = []
w = out.append
w('/* GENERERAD av gen_auto_golden.py fran auto_params_v2.2.py (md5 %s). Andra inte for hand. */' % REF_MD5)
w('static const int32_t kGoldenMan[%d][21] = {' % len(MANS))
for m in MANS:
    w('  {' + ','.join(str(m[n]) for n in P) + '},')
w('};')
w('/* btype,nomv,ahD,ovr,man,tce,batSensor,ampCap,floor,mca,mcv,sensor | eff[21] | mask, warn */')
w('static const int32_t kGoldenAuto[][35] = {')
effs = []
for c in cases:
    bt, nomv, ah, ovr, mi, tce, bats, cap, floor, mca, mcv, sens = c
    e, wb, mask = ap.effective(bt, ah, ovr_mask=ovr, man=MANS[mi], temp_comp_enable=bool(tce), bat_sensor=bool(bats),
                               nomv_idx=nomv, amp_cap_ca=cap, tail_floor_ca=floor,
                               max_charge_amp=mca, max_charge_volt=mcv, sensor_idx=sens)
    effs.append(e)
    w('  {' + ','.join(str(x) for x in list(c) + [e[n] for n in P] + [mask, wb]) + '},')
w('};')
w('static const unsigned kGoldenAutoCount = %d;' % len(cases))

# comp_target: utvalda resultat x temperaturer x borvarden x pa/av
TS = [-32768, -1000, -101, -100, -99, -50, -1, 0, 1, 100, 249, 250, 251, 350, 449, 450, 451, 1000, 32767]
w('/* caseIdx, v, tDc, enabled, expected */')
w('static const int32_t kGoldenComp[][5] = {')
nc = 0
for i in range(0, len(cases), 97):
    e = effs[i]
    for t in TS:
        for v in (e['absorbVolt'], e['floatVolt'], 0, 65535):
            for en in (0, 1):
                w('  {%d,%d,%d,%d,%d},' % (i, v, t, en, ap.comp_target(v, e, t, bool(en))))
                nc += 1
w('};')
w('static const unsigned kGoldenCompCount = %d;' % nc)
# Negativa tal: S08-extremer i tempCompMv x celler (voltLimit 65535)
w('/* tempCompMv, cellCount, v, tDc, expected */')
w('static const int32_t kGoldenCompSyn[][5] = {')
ns = 0
for tc in (-128, -40, -1, 1, 127):
    for cells in (1, 6, 255):
        e = {n: 0 for n in P}; e.update(tempCompMv=tc, cellCount=cells, voltLimit=65535)
        for t in TS:
            for v in (0, 30000):
                w('  {%d,%d,%d,%d,%d},' % (tc, cells, v, t, ap.comp_target(v, e, t, True)))
                ns += 1
w('};')
w('static const unsigned kGoldenCompSynCount = %d;' % ns)
print('\n'.join(out))
print('golden: %d effective-fall, %d comp-fall' % (len(cases), nc), file=sys.stderr)
