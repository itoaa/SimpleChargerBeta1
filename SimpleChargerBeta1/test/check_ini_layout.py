#!/usr/bin/env python3
"""Jamfor offset i shared/tsLayout.hpp och shared/eepromLayout.hpp mot
tunerStudio_v2.2.ini (OutputChannels och Constants per sida).
Kor: python3 -B check_ini_layout.py [ini]. Exit 1 vid avvikelse."""
import os, re, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, 'ref'))
import tsparse  # noqa: E402

ROOT = os.path.join(HERE, '..')
INI = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'tunerStudio_v2.2.ini')

def consts(path):
    out = {}
    for m in re.finditer(r'const\s+uint(?:8|16)_t\s+(\w+)\s*=\s*(\d+)U', open(path).read()):
        out[m.group(1)] = int(m.group(2))
    return out

ts = consts(os.path.join(ROOT, 'shared', 'tsLayout.hpp'))
ee = consts(os.path.join(ROOT, 'shared', 'eepromLayout.hpp'))
secs, _ = tsparse.parse(INI)
och, cfg = tsparse.outputs(secs)
och = dict((d['name'], d) for d in och if d['offset'] is not None)
cons, pcfg = tsparse.constants(secs)
errors = []
checked = 0

def expect(what, have, want):
    global checked
    checked += 1
    if have != want:
        errors.append('%s: firmware %s, ini %s' % (what, have, want))

# Realtid: RtXxx -> xxx (ADC-namn med versalt ADC i ini)
special = {'RtEffBase': 'eff_cellCount', 'RtMosfetTempAdc': 'mosfetTempADC', 'RtBatTempAdc': 'batTempADC',
           'RtOutVoltAdc': 'outVoltADC', 'RtOutAmpAdc': 'outAmpADC', 'RtInVoltAdc': 'inVoltADC',
           'RtInAmpAdc': 'inAmpADC', 'RtDriverVoltAdc': 'driverVoltADC'}
for k, v in sorted(ts.items()):
    if not k.startswith('Rt') or k == 'RtBlockSize':
        continue
    name = special.get(k, k[2].lower() + k[3:])
    if name not in och:
        errors.append('%s: kanal %s saknas i ini' % (k, name))
        continue
    expect(k, v, och[name]['offset'])
expect('ochBlockSize', ts['RtBlockSize'], int(cfg.get('ochBlockSize', '0')))

# Sida 1: P1Xxx -> Xxx (tabeller med eget namn)
p1 = dict((d['name'], d) for d in cons if d['page'] == 1)
p1map = {'P1MosfetTempADCTable': 'mosfetTempADCTable', 'P1MosfetTempTable': 'mosfetTempTable',
         'P1AccelerationDelay': 'accelerationDelay'}
for k, v in sorted(ee.items()):
    if not k.startswith('P1') or k == 'P1TableLength':
        continue
    name = p1map.get(k, k[2:])
    if name not in p1:
        errors.append('%s: %s saknas pa sida 1 i ini' % (k, name))
        continue
    expect(k, v, p1[name]['offset'])

# Sida 3/5 och sida 2 (bitfalt -> forsta faltet som borjar pa offset)
def first_at(page, off):
    return [d['name'] for d in cons if d['page'] == page and d['offset'] == off]
for k, page, name in (('P3SensorBits', 3, None), ('P3DiagMode', 3, 'diagMode'), ('P3DiagDuty', 3, 'diagDuty'),
                      ('P5ModeBits', 5, 'chargeMode'), ('P5TypeBits', 5, 'batteryType'),
                      ('P5CapacityAh', 5, 'capacityAh'), ('P2StartBits', 2, 'startMode'),
                      ('P2StartDelayMin', 2, 'startDelayMin')):
    names = first_at(page, ts[k])
    checked += 1
    if not names or (name is not None and name not in names):
        errors.append('%s (offset %d sida %d): ini har %s' % (k, ts[k], page, names))

# Sidstorlekar
sizes = [int(x) for x in re.findall(r'\d+', pcfg.get('pageSize', ''))]
for i, k in enumerate(('Page1Size', 'Page2Size', 'Page3Size', 'Page4Size', 'Page5Size')):
    if i < len(sizes):
        expect(k, ee[k], sizes[i])
    else:
        errors.append('pageSize saknas i ini')

# Bitar i errorBits/restrictionBits (hex-konstanter Err*/Restr*) mot ini-indikatorerna
# { errorBits & N } / { restrictionBits & N }. Fangar b12 bulkAhPct (BESLUT nr 36)
# och b8 burn avvisad (BESLUT nr 35). Bade indicator och indicatorTemplate raknas
# (BESLUT nr 44: de flesta bitindikatorer ligger i hogerklicksmenyn).
hexc = dict((m.group(1), int(m.group(2), 16)) for m in re.finditer(
    r'const\s+uint16_t\s+(\w+)\s*=\s*0x([0-9A-Fa-f]+)U', open(os.path.join(ROOT, 'shared', 'tsLayout.hpp')).read()))
initext = open(INI, encoding='latin-1').read()
for prefix, chan in (('Err', 'errorBits'), ('Restr', 'restrictionBits')):
    fw = sorted(v for k, v in hexc.items() if k.startswith(prefix))
    ini = sorted(int(x) for x in re.findall(r'indicator(?:Template)? = \{ %s & (\d+) \}' % chan, initext))
    expect('%s-bitar (indikatorer)' % chan, fw, ini)
    expect('%s-bitar unika' % chan, len(set(fw)), len(fw))

for e in errors:
    print('FEL:', e)
print('check_ini_layout: %d kontroller, %d fel (%s)' % (checked, len(errors), os.path.basename(INI)))
sys.exit(1 if errors else 0)
