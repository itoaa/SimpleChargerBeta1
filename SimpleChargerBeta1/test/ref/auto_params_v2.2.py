#!/usr/bin/env python3
"""auto_params.py - referensimplementation av AUTOLAGET for SmedbyCharger v2.2.

v2.2 (Olas beslut 2026-10-24): tempkomp -4,0 mV/°C/cell for ALLA blytyper, ref 25 °C,
sida 1 i 0,01 A / 0,01 V (max_charge_amp/max_charge_volt anges nu i cA/cV), bulkMaxTime raknas
om fran MANUELL bulkstrom, float raknas in i totalMaxTime, 15 A hardvarutak galler aven manuella
strommar, kalcium/EFB 14,7 V, LiFePO4 = OBEKRAFTAT forslag (Victron).

Indata : batterityp, kapacitet (Ah), nominell spanning, tak/golv, hardvarugranser, overrides.
Utdata : effektiva laddparametrar (samma enheter/upplosning som i ini-filen: 0,01 A / 0,01 V / min / s).

Skriven med HELTALSARITMETIK (int32) sa att den kan portas rakt av till C++ pa ATmega328P:
  cA  = 0,01 A      cV  = 0,01 V      dAh = 0,1 Ah
  u01mV = 0,1 mV (per cell)          dmV = 0,1 mV/°C/cell      dC = 0,1 °C
Kalla for blybatterivarden: Blybatteri-laddning.md (avsnitt 9). Referenstemperatur 25 °C.
LiFePO4-raden kommer INTE fran rapporten utan fran Victron Lithium Smart-manualen (se SPEC_v2.2.md). Raden ar OBEKRAFTAD..

Kor:  python3 auto_params.py              -> de tre exemplen + alla typer for 12 V
      python3 auto_params.py AGM 40       -> ett eget fall   (typ-nyckel eller index, Ah)
"""
import sys

# ---------------------------------------------------------------- typtabell
# Kolumner (heltal):
#  crate   : laddstrom i promille av C (100 = 0,10C)
#  absV    : absorptionsspanning, 0,1 mV per cell vid 25 °C (24500 = 2,450 V/cell = 14,70 V for 6 celler)
#  tail    : slutstrom i 0,1 % av C (30 = 3,0 %)  0 = av (absorption avslutas bara pa tid)
#  floatV  : float, 0,1 mV/cell
#  floatA  : max floatstrom, promille av C
#  absMax  : max absorptionstid, min
#  floatMax: max floattid, min (0 = obegransad)
#  tc      : temperaturkompensering, 0,1 mV/°C/cell (-40 = -4,0)
#  restV   : omstart (underhall) under denna, 0,1 mV/cell
#  prechV  : forladdning under denna, 0,1 mV/cell
#  prechA  : forladdningsstrom, promille av C
#  vMax    : hogsta tillatna (kompenserade) spanning, 0,1 mV/cell
#  presV   : "batteri anslutet" over denna, 0,1 mV/cell
#  tMin/tMax: laddning tillaten mellan, °C
#  chem    : 'lead' eller 'lfp' (styr cellantal)
TYPES = [
 # key       namn i TS                 crate absV  tail floatV floatA absMax floatMax tc  restV prechV prechA vMax  presV tMin tMax chem
 ('WET',   'Bly vatt start (SLI)',     100, 24500, 30, 22500, 50, 240, 2880, -40, 20833, 17500, 30, 25000, 5000, 0, 45, 'lead'),
 ('AGM',   'Bly AGM',                  200, 24000, 20, 22667, 50, 240, 1440, -40, 20833, 17500, 30, 25000, 5000, 0, 45, 'lead'),
 ('GEL',   'Bly GEL',                  100, 23750, 15, 22833, 50, 240, 1440, -40, 20833, 17500, 30, 25000, 5000, 0, 45, 'lead'),
 ('DEEP',  'Bly vatt djupcykel',       100, 24500, 20, 22500, 50, 240, 1440, -40, 20833, 17500, 30, 25000, 5000, 0, 45, 'lead'),
 ('CAEFB', 'Bly kalcium/EFB',          100, 24500, 30, 22500, 50, 240, 1440, -40, 20833, 17500, 30, 25000, 5000, 0, 45, 'lead'),
 ('LFP',   'LiFePO4 (obekraftat)',     500, 35500,  0, 33750, 50, 120, 1440,   0, 33000, 25000, 30, 36500, 5000, 5, 50, 'lfp'),
]
TYPE_NAMES = [t[1] for t in TYPES] + ['Anpassad (manuellt)', 'INVALID']
T_CUSTOM = 6
COLS = ('key','name','crate','absV','tail','floatV','floatA','absMax','floatMax','tc','restV','prechV','prechA','vMax','presV','tMin','tMax','chem')
def trow(i): return dict(zip(COLS, TYPES[i]))

NOMV = [6, 12, 24, 36, 48]                          # batteryNomVolt-alternativ (index 0..4)
CELLS = {'lead': [3, 6, 12, 18, 24], 'lfp': [2, 4, 8, 12, 16]}
SENSOR_MAX_CA = [500, 2000, 3000, 0]               # ACS712-05B / -20A / -30A / shunt(0 = ingen grans)
HW_AMP_MAX_CA = 1500                               # 15 A hardvarutak (Ola) - galler auto OCH manuellt
TOTAL_TIME_MAX = 10080                             # 7 dygn, ovre grans for totalMaxTime

# Gemensamma konstanter (rapporten avsnitt 9 "Gemensamt")
BULK_AH_PCT      = 120    # s1 avbryts (FEL) efter 120 % av C
BULK_TIME_FACTOR = 720    # 1,2 x 60 min  -> bulkMaxTime = 720 * dAh / cA   [min]
BULK_TIME_MIN, BULK_TIME_MAX = 60, 1200            # 1 h .. 20 h (CTEK max 20 h)
ABS_MIN_TIME_S   = 60     # slutstrom maste hallas 60 s (filtrerar ACS712-brus)
PRECH_MAX_MIN    = 120    # 10,5 V maste passeras inom ca 2 h, annars FEL
COMP_T_MIN_DC, COMP_T_MAX_DC = -100, 450           # tempkomp anvander T inom -10..+45 °C
T_REF_DC         = 250    # 25,0 °C

# ---------------------------------------------------------------- parameterlista
# Ordningen = bitnummer i override-masken (ovr-byte 0..2, bit 0..7) och ordning i man_*/eff_*.
#  name, ctype, unit, scale, lo, hi, digits, beskrivning
PARAMS = [
 ('cellCount',         'U08', 'celler', 1,    1,    32,    0, 'Antal celler i serie'),
 ('bulkAmp',           'U16', 'A',      0.01, 0,    30,    2, 'Laddstrom bulk (CC) = min(C-rate x Ah, tak)'),
 ('absorbVolt',        'U16', 'V',      0.01, 0,    60,    2, 'Absorptionsspanning vid 25 °C'),
 ('floatVolt',         'U16', 'V',      0.01, 0,    60,    2, 'Floatspanning vid 25 °C'),
 ('tailAmp',           'U16', 'A',      0.01, 0,    30,    2, 'Slutstrom absorption = max(% x Ah, golv), 0 = bara tid'),
 ('floatAmp',          'U16', 'A',      0.01, 0,    30,    2, 'Max strom i float = 0,05C (<= bulk)'),
 ('bulkMaxTime',       'U16', 'min',    1,    0,    6000,  0, 'Max tid bulk (1,2 x C / I), overskridning = FEL'),
 ('bulkAhPct',         'U08', '%',      1,    0,    250,   0, 'Max laddad Ah i bulk, % av C (0 = av)'),
 ('absorbMaxTime',     'U16', 'min',    1,    0,    6000,  0, 'Max tid absorption'),
 ('absorbMinTime',     'U16', 's',      1,    0,    3600,  0, 'Slutstrom maste hallas sa lange'),
 ('floatMaxTime',      'U16', 'min',    1,    0,    10080, 0, 'Max tid float (0 = obegransad), sedan overvakning'),
 ('restartVolt',       'U16', 'V',      0.01, 0,    60,    2, 'Omstart fran overvakning under denna'),
 ('tempCompMv',        'S08', 'mV/C/cell', 0.1, -10, 0,    1, 'Temperaturkompensering per cell'),
 ('chargeTempMin',     'S08', 'C',      1,    -40,  60,    0, 'Laddning tillaten fran'),
 ('chargeTempMax',     'S08', 'C',      1,    0,    80,    0, 'Laddning tillaten till'),
 ('prechargeVolt',     'U16', 'V',      0.01, 0,    60,    2, 'Forladdning (lag strom) under denna'),
 ('prechargeAmp',      'U16', 'A',      0.01, 0,    30,    2, 'Forladdningsstrom = max(0,03C, golv) <= bulk'),
 ('prechargeMaxTime',  'U16', 'min',    1,    0,    1440,  0, 'Forladdningsgransen maste passeras inom, annars FEL'),
 ('voltLimit',         'U16', 'V',      0.01, 0,    60,    2, 'Hogsta tillatna utspanning efter kompensering'),
 ('totalMaxTime',      'U16', 'min',    1,    0,    10080, 0, 'Total max-tid forladdning+bulk+absorption+float (v2.2: float raknas in, max 10080)'),
 ('batteryPresentVolt','U16', 'V',      0.01, 0,    60,    2, 'Batteri anslutet over denna'),
]
PNAMES = [p[0] for p in PARAMS]
PIDX = {n: i for i, n in enumerate(PNAMES)}
UNIT_RAW = {'A': 'cA', 'V': 'cV'}   # internt heltal

# Varningsbitar i OutputChannels autoWarnBits (U16)
WARN = [
 (0, 'AH_MISSING',   'Kapacitet 0 Ah i Auto - laddning startar inte'),
 (1, 'CUSTOM_TYPE',  'Batterityp Anpassad/ogiltig - alla varden tas fran man_*'),
 (2, 'AMP_CAPPED',   'C-rate x Ah over taket - bulkstrommen begransad'),
 (3, 'TAIL_FLOOR',   'Slutstrom/forladdning/float hojd till golvet (ACS712-upplosning)'),
 (4, 'VOLT_CLAMPED', 'Spanning begransad av voltLimit / MaxChargeVolt'),
 (5, 'NOMV_INVALID', 'Ogiltig nominell spanning'),
 (6, 'TC_NO_SENSOR', 'Tempkomp pa men batterigivare saknas - ingen kompensering'),
 (7, 'INCONSISTENT', 'Inkonsekvent: float>=absorption, slut>=bulk, tMin>=tMax eller restart>=float'),
 (8, 'OVERRIDE',     'Minst en parameter ar manuellt overstyrd'),
 (9, 'TIME_CLAMPED', 'bulkMaxTime begransad till 60..1200 min'),
 (10,'FLOAT_UNLIMITED','floatMaxTime = 0 (obegransad float) - totaltiden skyddar inte float'),
]
W = {k: 1 << b for b, k, _ in WARN}

def _div_round(a, b):            # avrundning till narmaste, a>=0, b>0 (som (a + b/2) / b i C)
    return (a + b // 2) // b
def _vpc(u01mV, cells):          # 0,1 mV/cell * celler -> cV
    return _div_round(u01mV * cells, 100)

def hw_amp_cap(amp_cap_ca=HW_AMP_MAX_CA, max_charge_amp=0, sensor_idx=1):
    """Strom-tak (cA) = min(autoAmpCap, 15 A hardvarutak, MaxChargeAmp (sida 1, cA, 0 = av), givarens matomrade)."""
    cap = min(amp_cap_ca, HW_AMP_MAX_CA)
    if max_charge_amp > 0: cap = min(cap, max_charge_amp)
    smax = SENSOR_MAX_CA[sensor_idx] if sensor_idx < len(SENSOR_MAX_CA) else 0
    if smax > 0: cap = min(cap, smax)
    return cap

def bulk_time(ah_d, bulk_ca):
    """bulkMaxTime (min) = ceil(1,2 x C / I) = ceil(720 * dAh / cA), klampad 60..1200. Returnerar (min, warnbits)."""
    bt = (BULK_TIME_FACTOR * ah_d + bulk_ca - 1) // bulk_ca if bulk_ca > 0 else BULK_TIME_MAX
    w = W['TIME_CLAMPED'] if (bt < BULK_TIME_MIN or bt > BULK_TIME_MAX) else 0
    return max(BULK_TIME_MIN, min(BULK_TIME_MAX, bt)), w

def total_time(prech, bulk, absorb, flt):
    """v2.2: totalMaxTime = forladdning + bulk + absorption + float (min). float 0 (obegransad) bidrar med 0."""
    return min(TOTAL_TIME_MAX, prech + bulk + absorb + flt)

def auto_values(btype, ah_d, nomv_idx=1, amp_cap_ca=1500, tail_floor_ca=30,
                max_charge_amp=0, max_charge_volt=0, sensor_idx=1):
    """Ren autoberakning (inga overrides). ah_d = Ah*10. max_charge_amp/_volt = sida 1 i cA/cV (v2.2: 0,01 A/V), 0 = av.
    Returnerar (dict param->heltal i ini-raavarde, warnbits)."""
    w = 0
    if btype >= len(TYPES):
        return None, W['CUSTOM_TYPE']
    t = trow(btype)
    if nomv_idx >= len(NOMV):
        return None, W['NOMV_INVALID']
    if ah_d == 0:
        w |= W['AH_MISSING']
    cells = CELLS[t['chem']][nomv_idx]
    # strom-tak = min(autoAmpCap, MaxChargeAmp (sida 1, om >0), givarens matomrade)
    cap = hw_amp_cap(amp_cap_ca, max_charge_amp, sensor_idx)
    bulk = _div_round(t['crate'] * ah_d, 100)            # promille * dAh / 100 = cA
    if bulk > cap: bulk = cap; w |= W['AMP_CAPPED']
    def floor_cap(x):                                     # golv, aldrig over bulk
        nonlocal w
        if x < tail_floor_ca: x = tail_floor_ca; w |= W['TAIL_FLOOR']
        return min(x, bulk)
    tail = 0 if t['tail'] == 0 else floor_cap(_div_round(t['tail'] * ah_d, 100))    # 0,1 % = promille: promille * dAh / 100 = cA
    flo  = floor_cap(_div_round(t['floatA'] * ah_d, 100))
    pre  = floor_cap(_div_round(t['prechA'] * ah_d, 100))
    # spanningar
    vlim = _vpc(t['vMax'], cells)
    if max_charge_volt > 0: vlim = min(vlim, max_charge_volt)
    def vclamp(x):
        nonlocal w
        if x > vlim: w |= W['VOLT_CLAMPED']; return vlim
        return x
    absV = vclamp(_vpc(t['absV'], cells)); fltV = vclamp(_vpc(t['floatV'], cells))
    # tider
    bt, tw = bulk_time(ah_d, bulk); w |= tw
    v = dict(cellCount=cells, bulkAmp=bulk, absorbVolt=absV, floatVolt=fltV, tailAmp=tail, floatAmp=flo,
             bulkMaxTime=bt, bulkAhPct=BULK_AH_PCT, absorbMaxTime=t['absMax'], absorbMinTime=ABS_MIN_TIME_S,
             floatMaxTime=t['floatMax'], restartVolt=_vpc(t['restV'], cells), tempCompMv=t['tc'],
             chargeTempMin=t['tMin'], chargeTempMax=t['tMax'], prechargeVolt=_vpc(t['prechV'], cells),
             prechargeAmp=pre, prechargeMaxTime=PRECH_MAX_MIN, voltLimit=vlim,
             totalMaxTime=total_time(PRECH_MAX_MIN, bt, t['absMax'], t['floatMax']), batteryPresentVolt=_vpc(t['presV'], cells))
    return v, w

def effective(btype, ah_d, ovr_mask=0, man=None, temp_comp_enable=True, bat_sensor=True, **kw):
    """Det firmware ska gora: auto -> overrides -> sakerhetsgranser -> konsistens.
    ovr_mask: bit i (PARAMS-ordning) = 1 -> anvand man[namn]. Typ Anpassad => alla fran man."""
    man = man or {}
    auto, w = auto_values(btype, ah_d, **kw)
    mask = ovr_mask
    if auto is None:                     # Anpassad/ogiltig -> allt manuellt
        mask = (1 << len(PARAMS)) - 1
        auto = {n: 0 for n in PNAMES}
    eff = {}
    for i, n in enumerate(PNAMES):
        eff[n] = man.get(n, 0) if (mask >> i) & 1 else auto[n]
    if ovr_mask: w |= W['OVERRIDE']
    # Hardvarugranser galler ALLTID, aven for manuella varden
    mcv = kw.get('max_charge_volt', 0); mca = kw.get('max_charge_amp', 0)
    if mcv > 0 and eff['voltLimit'] > mcv: eff['voltLimit'] = mcv; w |= W['VOLT_CLAMPED']
    for n in ('absorbVolt', 'floatVolt'):
        if eff[n] > eff['voltLimit']: eff[n] = eff['voltLimit']; w |= W['VOLT_CLAMPED']
    hcap = hw_amp_cap(HW_AMP_MAX_CA, mca, kw.get('sensor_idx', 1))   # 15 A / MaxChargeAmp / givare (inte autoAmpCap)
    for n in ('bulkAmp', 'tailAmp', 'floatAmp', 'prechargeAmp'):
        if eff[n] > hcap: eff[n] = hcap; w |= W['AMP_CAPPED']
    # v2.2: harledda tider foljer de EFFEKTIVA vardena (om de sjalva inte ar manuella)
    if not (mask >> PIDX['bulkMaxTime']) & 1 and (mask >> PIDX['bulkAmp']) & 1:
        eff['bulkMaxTime'], tw = bulk_time(ah_d, eff['bulkAmp']); w |= tw     # manuell bulkstrom -> ny bulktid
    if not (mask >> PIDX['totalMaxTime']) & 1:
        eff['totalMaxTime'] = total_time(eff['prechargeMaxTime'], eff['bulkMaxTime'], eff['absorbMaxTime'], eff['floatMaxTime'])
    if eff['floatMaxTime'] == 0: w |= W['FLOAT_UNLIMITED']
    if temp_comp_enable and eff['tempCompMv'] != 0 and not bat_sensor: w |= W['TC_NO_SENSOR']
    if (eff['floatVolt'] >= eff['absorbVolt'] or (eff['tailAmp'] and eff['tailAmp'] >= eff['bulkAmp'])
            or eff['chargeTempMin'] >= eff['chargeTempMax'] or eff['restartVolt'] >= eff['floatVolt']):
        w |= W['INCONSISTENT']
    return eff, w, mask

def comp_target(v_cv, eff, t_dc, enabled=True):
    """Kompenserad borspanning (cV) vid batteritemperatur t_dc (0,1 °C). Anropas varje regler-varv."""
    if not enabled or eff['tempCompMv'] == 0: return min(v_cv, eff['voltLimit'])
    t = max(COMP_T_MIN_DC, min(COMP_T_MAX_DC, t_dc))
    d = eff['tempCompMv'] * eff['cellCount'] * (t - T_REF_DC)       # 0,01 mV
    d = -_div_round(-d, 1000) if d < 0 else _div_round(d, 1000)      # -> cV, symmetrisk avrundning
    return max(0, min(v_cv + d, eff['voltLimit']))

def fmt(n, raw):
    p = PARAMS[PIDX[n]]
    return f'{raw * p[3]:.{p[6]}f} {p[2]}'

def warn_text(w):
    return ', '.join(k for b, k, _ in WARN if (w >> b) & 1) or '-'

def report(btype, ah, nomv_idx=1, title=None, **kw):
    eff, w, mask = effective(btype, int(round(ah * 10)), nomv_idx=nomv_idx, **kw)
    print(f'\n=== {title or ""} {TYPE_NAMES[btype]}, {ah:g} Ah, {NOMV[nomv_idx] if nomv_idx < len(NOMV) else "?"} V ===')
    for n in PNAMES:
        print(f'  {n:<19} {fmt(n, eff[n]):>16}   (raa {eff[n]})')
    for tc in (0, 100, 350):
        print(f'  komp. absorption vid {tc/10:>4.1f} °C: {comp_target(eff["absorbVolt"], eff, tc)/100:.2f} V'
              f'   float: {comp_target(eff["floatVolt"], eff, tc)/100:.2f} V')
    print(f'  autoWarnBits = 0x{w:04X} ({warn_text(w)})   manuell-mask = 0x{mask:06X}')
    return eff, w

def _key(s):
    for i, t in enumerate(TYPES):
        if s.upper() == t[0] or s == str(i): return i
    raise SystemExit(f'okand typ {s}; valj {[t[0] for t in TYPES]}')

def selftest():
    # rimlighetskontroller som aven ska finnas som enhetstester i firmware
    e, w, _ = effective(1, 400)                      # 40 Ah AGM
    assert (e['bulkAmp'], e['absorbVolt'], e['tailAmp'], e['floatVolt']) == (800, 1440, 80, 1360), e
    e, w, _ = effective(0, 600)                      # 60 Ah vatt
    assert (e['bulkAmp'], e['absorbVolt'], e['tailAmp'], e['floatVolt']) == (600, 1470, 180, 1350), e
    e, w, _ = effective(2, 1000)                     # 100 Ah GEL
    assert (e['bulkAmp'], e['absorbVolt'], e['tailAmp'], e['floatVolt']) == (1000, 1425, 150, 1370), e
    e, w, _ = effective(1, 1000)                     # 100 Ah AGM -> 20 A begransas till 15 A
    assert e['bulkAmp'] == 1500 and w & W['AMP_CAPPED']
    e, w, _ = effective(2, 70)                       # 7 Ah GEL -> slut 0,105 A -> golv 0,30 A
    assert e['tailAmp'] == 30 and w & W['TAIL_FLOOR']
    e, w, _ = effective(1, 400, ovr_mask=1 << PIDX['bulkAmp'], man={'bulkAmp': 500})
    assert e['bulkAmp'] == 500 and e['absorbVolt'] == 1440 and w & W['OVERRIDE']
    e, w, m = effective(T_CUSTOM, 400, man={n: 0 for n in PNAMES})
    assert w & W['CUSTOM_TYPE'] and m == (1 << len(PARAMS)) - 1
    e, w, _ = effective(0, 600, max_charge_volt=1400)  # sida-1-grans 14,00 V (cV)
    assert e['absorbVolt'] == 1400 and w & W['VOLT_CLAMPED']
    e, w, _ = effective(1, 400, ovr_mask=1 << PIDX['absorbVolt'], man={'absorbVolt': 1600})
    assert e['absorbVolt'] == 1500 and w & W['VOLT_CLAMPED']  # manuellt varde kan inte ga over voltLimit
    e, w, _ = effective(0, 0)
    assert w & W['AH_MISSING']
    e, _, _ = effective(1, 400)
    assert comp_target(1440, e, 0) == 1500 and comp_target(1440, e, 350) == 1416
    assert len(PARAMS) <= 24
    # v2.2-beslut
    assert all(trow(i)['tc'] == -40 for i in range(5)), 'tempkomp -4 for alla blytyper'
    assert trow(4)['absV'] * 6 // 100 == 1470, 'kalcium/EFB 14,7 V'
    e, w, _ = effective(1, 400, ovr_mask=1 << PIDX['bulkAmp'], man={'bulkAmp': 500})
    assert e['bulkMaxTime'] == 576 and e['totalMaxTime'] == 120 + 576 + 240 + 1440, e   # 720*400/500 = 576
    e, w, _ = effective(1, 400, ovr_mask=(1 << PIDX['bulkAmp']) | (1 << PIDX['bulkMaxTime']), man={'bulkAmp': 500, 'bulkMaxTime': 300})
    assert e['bulkMaxTime'] == 300 and e['totalMaxTime'] == 120 + 300 + 240 + 1440
    e, w, _ = effective(0, 600)
    assert e['totalMaxTime'] == 120 + 720 + 240 + 2880                  # float inraknad
    e, w, _ = effective(1, 400, ovr_mask=1 << PIDX['bulkAmp'], man={'bulkAmp': 2500})
    assert e['bulkAmp'] == 1500 and w & W['AMP_CAPPED']                 # 15 A tak aven manuellt
    e, w, _ = effective(1, 400, max_charge_amp=550)                      # MaxChargeAmp 5,50 A (cA)
    assert e['bulkAmp'] == 550
    e, w, _ = effective(1, 400, ovr_mask=1 << PIDX['floatMaxTime'], man={'floatMaxTime': 0})
    assert w & W['FLOAT_UNLIMITED'] and e['totalMaxTime'] == 120 + 360 + 240
    print('selftest OK (18 kontroller)')

if __name__ == '__main__':
    if len(sys.argv) >= 3:
        report(_key(sys.argv[1]), float(sys.argv[2]), int(sys.argv[3]) if len(sys.argv) > 3 else 1)
    else:
        selftest()
        report(1, 40, title='Exempel 1:')
        report(0, 60, title='Exempel 2:')
        report(2, 100, title='Exempel 3:')
        print('\n--- Oversikt alla typer, 12 V (A / V / A / V / min) ---')
        print(f'  {"typ":<24}{"Ah":>5}{"bulk":>8}{"abs":>8}{"slut":>7}{"float":>8}{"bulkMax":>8}{"varn":>8}')
        for i in range(len(TYPES)):
            for ah in (7, 40, 60, 100, 200):
                e, w, _ = effective(i, ah * 10)
                print(f'  {TYPE_NAMES[i]:<24}{ah:>5}{e["bulkAmp"]/100:>8.2f}{e["absorbVolt"]/100:>8.2f}'
                      f'{e["tailAmp"]/100:>7.2f}{e["floatVolt"]/100:>8.2f}{e["bulkMaxTime"]:>8}{w:>#8x}')
