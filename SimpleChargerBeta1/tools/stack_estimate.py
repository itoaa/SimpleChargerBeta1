#!/usr/bin/env python3
"""Statisk uppskattning av stackdjup fran avr-gcc -fcallgraph-info=su.

Kor (fran SimpleChargerBeta1/):
  make clean && make all CXXFLAGS_EXTRA=-fcallgraph-info=su
  python3 -B tools/stack_estimate.py [katalog]

Metod: varje funktions egen ram (bytes enligt .ci) + 2 B returadress per
anrop, langsta vag fran main. Indirekta anrop (funktionspekare: TS-krokar,
BurnGuard, EEPROM-backend, send) finns inte i grafen och laggs till for
hand nedan. Biblioteksrutiner (libgcc/avr-libc, t.ex. flyttal och
division) saknar .ci och far ett schablontillagg LIB_ALLOWANCE [U].
Avbrottsrutiner laggs till separat (varsta ISR + 2 B returadress).
Resultatet ar en UPPSKATTNING, inte en matning."""
import os, re, sys

ROOT = sys.argv[1] if len(sys.argv) > 1 else '.'
LIB_ALLOWANCE = 32   # [U] libgcc/avr-libc-rutiner utan .ci (flyttal, division)
RET = 2              # ATmega328P: 16-bitars PC -> 2 B per anrop

INDIRECT = {
    # TunerStudio -> krokar i main.cpp
    'TunerStudio::handleCommand': ['onBurn', 'onCommand', 'onPageWritten'],
    'TunerStudio::onByte': ['onBurn', 'onCommand', 'onPageWritten', 'uartSend'],
    'TunerStudio::send': ['uartSend'],
    # EepromStore -> BurnGuard och backend
    'EepromStore::burn': ['guardCharging', 'guardOff', 'guardRestore'],
    'EepromStore::writeAndVerify': ['avrRead', 'avrUpdate'],
    'EepromStore::loadPage': ['avrRead'],
    'EepromStore::load': ['avrRead'],
    'EepromStore::storedCrc': ['avrRead'],
}

nodes, edges = {}, {}
label_of = {}
for dp, _, fs in os.walk(ROOT):
    for f in fs:
        if not f.endswith('.ci'):
            continue
        txt = open(os.path.join(dp, f)).read()
        for m in re.finditer(r'node: \{ title: "([^"]+)" label: "([^"]*)"', txt):
            title, label = m.group(1), m.group(2)
            short = title.split(':')[-1] if ':' in title and not title.startswith('_Z') else title
            sig = label.split('\\n')[0]
            b = re.search(r'(\d+) bytes', label)
            name = re.sub(r'\(.*', '', sig).split()[-1] if sig else short
            key = short
            label_of[key] = name
            if b:
                nodes[key] = int(b.group(1))
        for m in re.finditer(r'edge: \{ sourcename: "([^"]+)" targetname: "([^"]+)"', txt):
            s = m.group(1); t = m.group(2)
            s = s.split(':')[-1] if not s.startswith('_Z') else s
            t = t.split(':')[-1] if not t.startswith('_Z') else t
            edges.setdefault(s, set()).add(t)

by_name = {}
for k, n in label_of.items():
    by_name.setdefault(n, []).append(k)
for src, dsts in INDIRECT.items():
    for sk in by_name.get(src, []):
        for d in dsts:
            for dk in by_name.get(d, []) + [k for k, n in label_of.items() if n.endswith(d)]:
                edges.setdefault(sk, set()).add(dk)

memo = {}
def depth(k, stack=()):
    if k in stack:
        return 0, ['(rekursion %s)' % label_of.get(k, k)]
    if k in memo:
        return memo[k]
    own = nodes.get(k, 0)
    best, path = 0, []
    for t in edges.get(k, ()):
        d, p = depth(t, stack + (k,))
        if d + RET > best:
            best, path = d + RET, p
    memo[k] = (own + best, ['%s (%d)' % (label_of.get(k, k), own)] + path)
    return memo[k]

mains = by_name.get('main', [])
if not mains:
    sys.exit('hittar ingen main i .ci-filerna')
d, p = depth(mains[0])
isr = [k for k, n in label_of.items() if n.startswith('__vector')]
isr_d = max([depth(k)[0] for k in isr] or [0])
print('Langsta vag fran main: %d B' % d)
for x in p:
    print('   ', x)
print('Varsta ISR: %d B + %d B returadress' % (isr_d, RET))
total = d + isr_d + RET + LIB_ALLOWANCE
print('Uppskattat stackdjup [U]: %d B (inkl. %d B schablon for bibliotek)' % (total, LIB_ALLOWANCE))
