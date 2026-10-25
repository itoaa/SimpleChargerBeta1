# -*- coding: ascii -*-
# Presentationssteg for tunerStudio_v2.2.ini (anropas av tools/make_ini22.py).
# BESLUT nr 42 (strom 0..15 A), nr 43 (spanning 0..55 V), nr 46 (driverVolt 0..16 V) och
# nr 44 (presentation: engelska texter, en teckenkodning, framsida, hjalp, menyer).
# Andrar bara presentation och min/max (hi). Namn, typer, offset, skala och
# standardvarden (utom oversatta alternativtexter) ar oforandrade; det kontrolleras
# med assert nedan och med test/check_ini_layout.py + ref/validate_ini_v2.2.py.
# Filen skrivs latin-1: enda icke-ASCII-tecknet ar grad-tecknet (0xB0).
import os
import re

# Hardvarutak fran firmware (shared/hwLimits.hpp), BESLUT nr 45: en enda kalla.
HW_HPP = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'shared', 'hwLimits.hpp')
def hw_limits(path=HW_HPP):
    txt = open(path).read()
    return dict((m.group(1), int(m.group(2))) for m in re.finditer(
        r'const\s+uint(?:8|16)_t\s+(Hw\w+)\s*=\s*(\d+)U', txt))
HW = hw_limits()
def num(x):
    return ('%.2f' % x).rstrip('0').rstrip('.')
HW_VOLT = num(HW['HwVoltMaxCv'] / 100.0)          # '55'
HW_DRV = num(HW['HwDriverVoltMaxDv'] / 10.0)      # '16' (BESLUT nr 46)

D = '\xb0'          # grad-tecknet (latin-1 0xB0)
DC = D + 'C'

# ---------------------------------------------------------------- hjalpmedel
def split_raw(s):
    """Dela pa komma utanfor citat/klamrar. Behaller blanktecken i varje del."""
    parts, cur, q, depth = [], '', False, 0
    for ch in s:
        if ch == '"':
            q = not q
        if not q and ch in '{[':
            depth += 1
        if not q and ch in '}]':
            depth -= 1
        if ch == ',' and not q and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += ch
    parts.append(cur)
    return parts

def set_tok(toks, i, new):
    lead = re.match(r'\s*', toks[i]).group(0)
    toks[i] = lead + new

def section_span(t, name):
    m = re.search(r'^\[%s\]\n' % re.escape(name), t, re.M)
    assert m, name
    n = re.search(r'^\[', t[m.end():], re.M)
    end = m.end() + n.start() if n else len(t)
    return m.end(), end

def replace_section(t, name, body):
    a, b = section_span(t, name)
    return t[:a] + body + t[b:]

def keys_in(t, name, key):
    a, b = section_span(t, name)
    out = []
    for line in t[a:b].splitlines():
        m = re.match(r'^\s*%s\s*=\s*([A-Za-z_]\w*)' % key, line)
        if m:
            out.append(m.group(1))
    return out

def lhs_in(t, name):
    a, b = section_span(t, name)
    out = []
    for line in t[a:b].splitlines():
        m = re.match(r'^\s*([A-Za-z_]\w*)\s*=', line)
        if m:
            out.append(m.group(1))
    return out

# ---------------------------------------------------------------- [Constants]
UNIT_MAP = {'"C"': '"' + DC + '"', '"celler"': '"cells"', '"steg"': '"steps"',
            '"mV/C/cell"': '"mV/' + DC + '/cell"', '"% kap"': '"%"',
            '"ggr"': '""', '"x"': '""', '"st"': '""'}
UNIT_FIELD = {'accelerationDelay': ('"x10 ms"', '"ms"'),          # BESLUT nr 15
              'mosfetTempADCTable': ('"ADC"', '"ADC/4"')}         # BESLUT nr 16

STEP = ['s%d' % i for i in range(1, 5)]
HI_AMP = ([s + x for s in STEP for x in ('_current', '_endAmpMax', '_endAmpMin')] +
          ['man_bulkAmp', 'man_tailAmp', 'man_floatAmp', 'man_prechargeAmp',
           'ampTimeLimit', 'MaxInputAmp'])                         # BESLUT nr 42
HI_VOLT = ([s + x for s in STEP for x in ('_volt', '_endVoltMax', '_endVoltMin')] +
           ['man_absorbVolt', 'man_floatVolt', 'man_restartVolt', 'man_prechargeVolt',
            'man_voltLimit', 'man_batteryPresentVolt'])            # BESLUT nr 43
HI_DRV = ['driverVoltMin', 'driverVoltMax']                        # BESLUT nr 43, 46
HI = {}
HI.update({n: ('30', '15') for n in HI_AMP})
HI.update({n: ('60', HW_VOLT) for n in HI_VOLT})   # BESLUT nr 45: HwVoltMaxCv
# Falt som redan har ratt hi i kallfilen men ska folja firmware (kontrolleras, andras inte)
HI_CHECK = {'MaxChargeVolt': HW_VOLT}                            # BESLUT nr 45
HI.update({n: ('25', HW_DRV) for n in HI_DRV})     # BESLUT nr 46: HwDriverVoltMaxDv

METHOD = ['Constant current (CC)', 'Constant voltage (CV)', 'Monitor', 'Off']
ONEND = ['Next stage', 'Go to stage 1', 'Stop (done)', 'Stop with error']
OPTS = {
    'startMode': ['Manual (Start button)', 'Auto when battery connected',
                  'Timer (delayed start)', 'INVALID'],
    'restartMode': ['No restart', 'Restart on voltage drop'],
    'mosfetTempSensor': ['None', 'DS18B20 (not active)', 'NTC table (not active)', 'INVALID'],
    'batTempSensor': ['None', 'DS18B20 (not active)', 'NTC table (not active)', 'INVALID'],
    'currentSensor': ['ACS712-05B (5 A)', 'ACS712-20A (20 A)', 'ACS712-30A (30 A)',
                      'Shunt + amplifier (not supported)'],
    'ampZeroAtBoot': ['No', 'Yes (output off while zeroing)'],
    'inAmpSensor': ['None (efficiency not shown)', 'Fitted (input current measured)'],
    'diagMode': ['Normal', 'Diagnostics: fixed duty (not active, output off)',
                 'Diagnostics: measure only (output off)', 'INVALID'],
    'chargeMode': ['Auto (type + Ah)', 'Manual stage table s1-s4 (not active)',
                   'Simple CC/CV (charger max values)', 'INVALID'],
    'tempCompEnable': ['Off', 'On - NOT ACTIVE, NO EFFECT (no battery temp sensor)'],   # BESLUT nr 48
    # BESLUT nr 45: index 4..7 ogiltiga
    'batteryNomVolt': ['6 V', '12 V', '24 V', '36 V', 'INVALID', 'INVALID', 'INVALID', 'INVALID'],
    'batteryType': ['Lead flooded starter (SLI)', 'Lead AGM', 'Lead GEL',
                    'Lead flooded deep cycle', 'Lead calcium/EFB', 'LiFePO4 (unconfirmed)',
                    'Custom (manual)', 'INVALID'],
}
for s in STEP:
    OPTS[s + '_method'] = METHOD
    OPTS[s + '_onEnd'] = ONEND

def q(s):
    return '"' + s + '"'

def do_constants(t):
    a, b = section_span(t, 'Constants')
    lines = t[a:b].split('\n')
    done_hi, done_opt, done_unit = set(), {}, set()
    for i, line in enumerate(lines):
        m = re.match(r'^(\s*)([A-Za-z_]\w*)(\s*=\s*)(scalar|array|bits)(,.*)$', line)
        if not m:
            continue
        name, kind = m.group(2), m.group(4)
        toks = split_raw(m.group(4) + m.group(5))
        if kind in ('scalar', 'array'):
            ui = 3 if kind == 'scalar' else 4
            hi = 7 if kind == 'scalar' else 8
            u = toks[ui].strip()
            if name in UNIT_FIELD:
                assert u == UNIT_FIELD[name][0], (name, u)
                set_tok(toks, ui, UNIT_FIELD[name][1])
                done_unit.add(name)
            elif u in UNIT_MAP:
                set_tok(toks, ui, UNIT_MAP[u])
            if name in HI:
                assert toks[hi].strip() == HI[name][0], (name, toks[hi])
                set_tok(toks, hi, HI[name][1])
                done_hi.add(name)
            if name in HI_CHECK:
                assert toks[hi].strip() == HI_CHECK[name], (name, toks[hi])
                done_hi.add(name)
        else:
            if name in OPTS:
                old = [x.strip().strip('"') for x in toks[4:]]
                new = OPTS[name]
                assert len(old) == len(new), (name, old)
                for k, o in enumerate(new):
                    set_tok(toks, 4 + k, q(o))
                done_opt[name] = dict(zip(old, new))
        lines[i] = m.group(1) + name + m.group(3) + ','.join(toks)
    assert done_hi == set(HI) | set(HI_CHECK), (set(HI) | set(HI_CHECK)) - done_hi
    assert set(done_opt) == set(OPTS), set(OPTS) - set(done_opt)
    assert done_unit == set(UNIT_FIELD)
    t = t[:a] + '\n'.join(lines) + t[b:]
    # standardvarden for oversatta alternativ
    a, b = section_span(t, 'ConstantsExtensions')
    lines = t[a:b].split('\n')
    for i, line in enumerate(lines):
        m = re.match(r'^(\s*defaultValue\s*=\s*)(\w+)(,\s*)"(.*)"\s*$', line)
        if m and m.group(2) in done_opt:
            mp = done_opt[m.group(2)]
            assert m.group(4) in mp, line
            lines[i] = m.group(1) + m.group(2) + m.group(3) + q(mp[m.group(4)])
    t = t[:a] + '\n'.join(lines) + t[b:]
    return t

def do_outputs(t):
    a, b = section_span(t, 'OutputChannels')
    lines = t[a:b].split('\n')
    och_map = {k: UNIT_MAP[k] for k in ('"C"', '"celler"', '"steg"', '"mV/C/cell"')}
    for i, line in enumerate(lines):
        m = re.match(r'^(\s*[A-Za-z_]\w*\s*=\s*)(.*)$', line)
        if not m or line.strip().startswith(';'):
            continue
        toks = split_raw(m.group(2))
        if toks[0].strip() == 'scalar' and len(toks) > 3 and toks[3].strip() in och_map:
            set_tok(toks, 3, och_map[toks[3].strip()])
        elif toks[0].strip().startswith('{') and len(toks) > 1 and toks[-1].strip() in och_map:
            set_tok(toks, len(toks) - 1, och_map[toks[-1].strip()])
        else:
            continue
        lines[i] = m.group(1) + ','.join(toks)
    return t[:a] + '\n'.join(lines) + t[b:]

# ---------------------------------------------------------------- etiketter for autolaget
AUTO = {
    'cur': [('bulkAmp', 'Bulk current'), ('tailAmp', 'Tail current (absorption end)'),
            ('floatAmp', 'Float current max'), ('prechargeAmp', 'Precharge current')],
    'volt': [('cellCount', 'Cell count'), ('absorbVolt', 'Absorption voltage'),
             ('floatVolt', 'Float voltage'), ('restartVolt', 'Restart below'),
             ('prechargeVolt', 'Precharge below'), ('voltLimit', 'Voltage limit'),
             ('batteryPresentVolt', 'Battery detected above')],
    'time': [('bulkMaxTime', 'Bulk max time (error)'),
             ('bulkAhPct', 'Bulk charge limit (error)'),
             ('absorbMaxTime', 'Absorption max time'),
             ('absorbMinTime', 'Time below tail current'),
             ('floatMaxTime', 'Float max time (0 = unlimited)'),
             ('prechargeMaxTime', 'Precharge max time (error)'),
             ('totalMaxTime', 'Total max time')],
    'temp': [('tempCompMv', 'Temp compensation per cell (not active)'),
             ('chargeTempMin', 'Charge temp min (not active)'),
             ('chargeTempMax', 'Charge temp max (not active)')],
}
LABEL = {x: l for g in AUTO.values() for x, l in g}

# ---------------------------------------------------------------- [GaugeConfigurations]
GTITLE = {
    'outAmpGauge': 'Output current', 'outVoltGauge': 'Output voltage',
    'outPowerGauge': 'Output power', 'ampHoursGauge': 'Charged capacity',
    'wattHoursGauge': 'Charged energy', 'socGauge': 'State of charge (estimate)',
    'cellVoltGauge': 'Cell voltage', 'stageGauge': 'Stage', 'stateGauge': 'Charger state',
    'stageTimeGauge': 'Time in stage', 'chargeTimeGauge': 'Charge time',
    'voltSetGauge': 'Voltage setpoint', 'ampSetGauge': 'Current setpoint',
    'inVoltGauge': 'Input voltage', 'inAmpGauge': 'Input current',
    'inPowerGauge': 'Input power', 'efficiencyGauge': 'Efficiency',
    'powerLossGauge': 'Power loss', 'dutyGauge': 'Duty', 'pulseWidthGauge': 'PWM step',
    'pwmFreqGauge': 'PWM frequency', 'driverVoltGauge': 'Driver voltage',
    'mosfetTempGauge': 'MOSFET temp (not active)', 'batTempGauge': 'Battery temp (not active)',
    'extraTempGauge': 'Extra temp (not active)', 'derateGauge': 'Derating (not active)',
    'overAmpTimeGauge': 'Time over current limit (not active)',
    'mosfetTempADCGauge': 'MOSFET temp ADC', 'batTempADCGauge': 'Battery NTC ADC',
    'outVoltADCGauge': 'Output voltage ADC', 'outAmpADCGauge': 'Output current ADC',
    'inVoltADCGauge': 'Input voltage ADC', 'inAmpADCGauge': 'Input current ADC',
    'driverVoltADCGauge': 'Driver voltage ADC', 'loopTimeGauge': 'Loop time (not active)',
    'errorBitsGauge': 'Error bits', 'secondsGauge': 'Uptime',
    'voltLimitNowG': 'Voltage limit now (0 = output off)', 'autoWarnG': 'Auto warnings',
    'tempCompDeltaG': 'Temp compensation now (not active)',
    'effAbsorbNowG': 'Absorption now (compensated)', 'effFloatNowG': 'Float now (compensated)',
    'pa_diffCountG': 'PC preview differs from firmware',
}
for x, l in LABEL.items():
    GTITLE['eff_%sG' % x] = l
    GTITLE['pa_%sG' % x] = l + ' (PC)'
GTITLE['eff_voltLimitG'] = 'Voltage limit (auto calc.)'
GTITLE['pa_voltLimitG'] = 'Voltage limit (PC, 0 = output off)'
GCAT = {'Laddning': 'Charging', 'Natsida': 'Input side',
        'Drivsteg och temp': 'Driver and temperature', 'Diagnos': 'Diagnostics',
        'Autolage - effektivt (firmware)': 'Auto mode - effective (firmware)',
        'Autolage - PC-forhandsvisning': 'Auto mode - preview (PC)'}
GLIM = {}
for g in ('outAmpGauge', 'ampSetGauge', 'inAmpGauge'):
    GLIM[g] = ('0', '20', '0', '0', '14', '15')                  # BESLUT nr 42
for x in ('bulkAmp', 'tailAmp', 'floatAmp', 'prechargeAmp'):
    for p in ('eff_', 'pa_'):
        GLIM[p + x + 'G'] = ('0', '15', '0', '0', '15', '15')    # BESLUT nr 42
# BESLUT nr 47: framsidans spanningsmatare, rod zon fran taket (hiWarning = hiDanger)
GLIM['outVoltGauge'] = ('0', '60', '0', '0', HW_VOLT, HW_VOLT)  # 0..60 V, rod fran 55 V (HwVoltMaxCv)
GLIM['inVoltGauge'] = ('0', '70', '9', '10', '65', '65')        # 0..70 V, rod fran 65 V (MaxInputVolt max)

def do_gauges(t):
    a, b = section_span(t, 'GaugeConfigurations')
    lines = t[a:b].split('\n')
    seen = set()
    for i, line in enumerate(lines):
        m = re.match(r'^(\s*gaugeCategory\s*=\s*)"(.*)"\s*$', line)
        if m:
            lines[i] = m.group(1) + q(GCAT[m.group(2)])
            continue
        m = re.match(r'^(\s*)([A-Za-z_]\w*)(\s*=\s*)(.*)$', line)
        if not m or line.strip().startswith(';'):
            continue
        name = m.group(2)
        toks = split_raw(m.group(4))
        assert len(toks) == 11, line
        set_tok(toks, 1, q(GTITLE[name]))
        u = toks[2].strip()
        if u in UNIT_MAP:
            set_tok(toks, 2, UNIT_MAP[u])
        if name in GLIM:
            for k, v in enumerate(GLIM[name]):
                set_tok(toks, 3 + k, v)
        seen.add(name)
        lines[i] = m.group(1) + name + m.group(3) + ','.join(toks)
    assert seen == set(GTITLE), (set(GTITLE) - seen, seen - set(GTITLE))
    return t[:a] + '\n'.join(lines) + t[b:]

# ---------------------------------------------------------------- [FrontPage]
W, B = 'white', 'black'
def ind(expr, off, on, onbg, tmpl=False):
    k = 'indicatorTemplate' if tmpl else 'indicator'
    return '   %s = { %s }, %s, %s, %s, %s, %s, %s' % (k, expr, q(off), q(on), W, B, onbg, B)

NA = ' (not active)'
STATUS_T = [(8, 'Derating' + NA, 'DERATING', 'yellow'), (32, 'Diagnostics mode', 'DIAGNOSTICS MODE', 'red'),
            (64, 'Timer waiting', 'Timer waiting', 'green')]
RESTR = [(1, 'Max output current', 'LIMIT: MAX OUTPUT CURRENT'),
         (2, 'Max input current' + NA, 'LIMIT: MAX INPUT CURRENT'),
         (4, 'High MOSFET temp' + NA, 'LIMIT: HIGH MOSFET TEMP'),
         (8, 'Battery temp' + NA, 'LIMIT: BATTERY TEMP'),
         (16, 'Low input voltage' + NA, 'LIMIT: LOW INPUT VOLTAGE'),
         (32, 'Max output voltage', 'LIMIT: MAX OUTPUT VOLTAGE'),
         (64, 'PWM limit', 'LIMIT: PWM LIMIT'),
         (128, 'Time-current' + NA, 'LIMIT: TIME-CURRENT'),
         (256, 'No rejected burn', 'BURN REJECTED: CHARGING - CHANGES NOT SAVED'),
         (512, 'Current sensor in range', 'CURRENT SENSOR SATURATED: VALUE CLAMPED'),
         (1024, 'Current sensor type as at boot', 'CURRENT SENSOR TYPE CHANGED: RESTART NEEDED')]
ERR = [(1, 'Output over-voltage', 'ERROR: OUTPUT OVER-VOLTAGE'),
       (2, 'Output over-current', 'ERROR: OUTPUT OVER-CURRENT'),
       (4, 'High input voltage', 'ERROR: HIGH INPUT VOLTAGE'),
       (8, 'Low input voltage' + NA, 'ERROR: LOW INPUT VOLTAGE'),
       (16, 'MOSFET over-temp' + NA, 'ERROR: MOSFET OVER-TEMP'),
       (32, 'Low driver voltage' + NA, 'ERROR: LOW DRIVER VOLTAGE'),
       (64, 'High driver voltage' + NA, 'ERROR: HIGH DRIVER VOLTAGE'),
       (128, 'Temp sensor fault' + NA, 'ERROR: TEMP SENSOR FAULT'),
       (256, 'Battery temp out of range' + NA, 'ERROR: BATTERY TEMP OUT OF RANGE'),
       (512, 'Max charge time', 'ERROR: MAX CHARGE TIME'),
       (1024, 'Battery missing/reversed' + NA, 'ERROR: BATTERY MISSING/REVERSED'),
       (2048, 'Current sensor zero fault' + NA, 'ERROR: CURRENT SENSOR ZERO FAULT'),
       (4096, 'Bulk charge limit', 'ERROR: BULK CHARGE LIMIT (bulkAhPct)')]
AUTOW = [(1, 'Capacity set', 'AUTO: CAPACITY 0 AH - CHARGING WILL NOT START', 'red'),
         (2, 'Battery type with auto values', 'AUTO: CUSTOM/INVALID TYPE - ALL MANUAL VALUES USED', 'yellow'),
         (4, 'Bulk current not capped', 'AUTO: C-RATE X AH ABOVE CAP - BULK CURRENT LIMITED', 'yellow'),
         (8, 'No current raised to floor', 'AUTO: TAIL/FLOAT/PRECHARGE CURRENT RAISED TO FLOOR', 'yellow'),
         (16, 'Voltage not clamped', 'WARNING: CHARGE VOLTAGE CLAMPED (VOLTAGE LIMIT / MAX CHARGE VOLTAGE)', 'red'),
         (32, 'Nominal voltage valid', 'AUTO: INVALID NOMINAL VOLTAGE', 'red'),
         (64, 'Temp compensation ok', 'AUTO: TEMP COMPENSATION ON BUT NO SENSOR - NOT COMPENSATED', 'yellow'),
         (128, 'Values consistent', 'AUTO: INCONSISTENT VALUES - CHECK FLOAT/ABSORPTION/TAIL/TEMP/RESTART', 'red'),
         (256, 'No manual override', 'AUTO: AT LEAST ONE VALUE IS MANUAL', 'yellow'),
         (512, 'Bulk max time not clamped', 'AUTO: BULK MAX TIME CLAMPED TO 60..1200 MIN', 'yellow'),
         (1024, 'Float time limited', 'AUTO: FLOAT MAX TIME 0 (UNLIMITED) - TOTAL TIME DOES NOT COVER FLOAT', 'yellow')]
AUTOW_VCLAMP = 16   # autoParams.hpp WarnVoltClamped (kontrolleras i check_ini_layout)
EEP = [(1, 'EEPROM header ok', 'EEPROM: HEADER INVALID - DEFAULTS LOADED'),
       (2, 'EEPROM page 1 ok', 'EEPROM: CRC ERROR PAGE 1 - DEFAULTS LOADED'),
       (4, 'EEPROM page 2 ok', 'EEPROM: CRC ERROR PAGE 2 - DEFAULTS LOADED'),
       (8, 'EEPROM page 3 ok', 'EEPROM: CRC ERROR PAGE 3 - DEFAULTS LOADED'),
       (16, 'EEPROM page 4 ok', 'EEPROM: CRC ERROR PAGE 4 - DEFAULTS LOADED'),
       (32, 'EEPROM page 5 ok', 'EEPROM: CRC ERROR PAGE 5 - DEFAULTS LOADED'),
       (64, 'Burn ok', 'EEPROM: BURN FAILED'),
       (128, 'EEPROM backend ok', 'EEPROM: NO BACKEND')]

def na_on(off, on):
    # BESLUT nr 48: en bit som firmware aldrig satter ska inte se aktiv ut, inte ens i pa-lage
    return on + ' (NOT ACTIVE)' if off.endswith(NA) else on

def frontpage():
    L = ['   ; BESLUT nr 44, 48: 8 matare och 13 indikatorer pa standardpanelen. Ovriga indikatorer',
         '   ; ar indicatorTemplate (hogerklicksmenyn, spec kap. 16). Rod bakgrund = fel/varning.',
         '   gauge1 = outVoltGauge', '   gauge2 = outAmpGauge', '   gauge3 = outPowerGauge',
         '   gauge4 = voltSetGauge', '   gauge5 = inVoltGauge', '   gauge6 = driverVoltGauge',
         '   gauge7 = dutyGauge', '   gauge8 = stageGauge', '',
         ind('statusBits & 1', 'Output off', 'Output on', 'green'),
         ind('statusBits & 2', 'CC mode', 'CC mode', 'green'),
         ind('statusBits & 4', 'CV mode', 'CV mode', 'green'),
         ind('statusBits & 16', 'No battery', 'Battery connected', 'green'),
         '   ; Stegindikatorer 0..4 (BESLUT nr 31): 0 forladdning (statusBits b7), 1 bulk,',
         '   ; 2 absorption, 3 float (tillstand 3 Laddar), 4 overvaka (tillstand 4 Klar).',
         ind('(stage == 0) && ((statusBits & 128) > 0)', 'Stage 0 Precharge', 'Stage 0 PRECHARGE', 'green'),
         ind('(chargerState == 3) && (stage == 1)', 'Stage 1 Bulk', 'Stage 1 BULK', 'green'),
         ind('(chargerState == 3) && (stage == 2)', 'Stage 2 Absorption', 'Stage 2 ABSORPTION', 'green'),
         ind('(chargerState == 3) && (stage == 3)', 'Stage 3 Float', 'Stage 3 FLOAT', 'green'),
         ind('(chargerState == 4) && (stage == 4)', 'Stage 4 Monitor', 'Stage 4 MONITOR', 'green'),
         ind('anyError', 'No error', 'ERROR - SEE INDICATORS', 'red'),
         ind('mcvOutputOff', 'Max charge voltage set', 'OUTPUT OFF: MAX CHARGE VOLTAGE = 0', 'red'),
         ind('eepromWarnBits > 0', 'EEPROM ok', 'EEPROM WARNING - SEE INDICATORS', 'red'),
         '   ; BESLUT nr 48: WarnVoltClamped (autoWarnBits b4) syns pa framsidan, rod.',
         ind('(autoWarnBits & %d) > 0' % AUTOW_VCLAMP, 'Voltage not clamped', 'WARNING: CHARGE VOLTAGE CLAMPED (VOLTAGE LIMIT / MAX CHARGE VOLTAGE)', 'red'),
         '',
         '   ; --- indicatorTemplate: valbara via hogerklick ---']
    for bit, off, on, bg in STATUS_T:
        L.append(ind('statusBits & %d' % bit, off, na_on(off, on), bg, True))
    L.append(ind('(chargerState == 3) && (chargeMode == 2)', 'Simple CC/CV', 'Simple CC/CV charging', 'green', True))
    for bit, off, on in RESTR:
        L.append(ind('restrictionBits & %d' % bit, off, na_on(off, on), 'yellow', True))
    for bit, off, on in ERR:
        L.append(ind('errorBits & %d' % bit, off, na_on(off, on), 'red', True))
    L.append(ind('anyLatched', 'No stored errors', 'STORED ERRORS - SEE DIAGNOSTICS', 'red', True))
    for bit, off, on, bg in AUTOW:
        if bit == AUTOW_VCLAMP:
            continue   # pa framsidan (BESLUT nr 48)
        L.append(ind('(autoWarnBits & %d) > 0' % bit, off, on, bg, True))
    for bit, off, on in EEP:
        L.append(ind('eepromWarnBits & %d' % bit, off, on, 'red', True))
    L.append(ind('pa_diffCount > 0', 'PC preview = firmware', 'PC PREVIEW DIFFERS FROM FIRMWARE', 'yellow', True))
    L.append(ind('chargeMode == 0', 'Auto mode off', 'Auto mode', 'green', True))
    return '\n'.join(L) + '\n\n'

# ---------------------------------------------------------------- hjalptexter
HELP = [
 ('safetyHelp', 'Safety', [
  'The hardware limit is 15 A. All current fields are limited to 0-15 A and the firmware limits every effective current to 15 A.',
  'Max charge voltage = 0 means that the output is always off. This is the factory default. Set Max charge voltage before charging.',
  'There are no temperature sensors in this firmware. All temperature protection is marked (not active) and does not protect the battery or the MOSFETs. 125 ' + DC + ' (Max MOSFET temp) is the MOSFET junction limit Tj max, not an active protection.',
  'Until temperature sensors exist, cooling and the total max charge time (totalMaxTime) are the only protection. Do not leave the charger unattended.',
  'Check all voltages, currents and times against the battery data sheet before charging.',
  'Burn is rejected while charging. Stop charging first, then burn.']),
 ('accelerationDelayHelp', 'Soft start (acceleration delay)', [
  'Not active: the value is stored but the firmware does not use it yet.',
  'Planned function: the PWM duty changes one step at a time so that start and ramp-down are smooth. Acceleration delay is the waiting time before each new PWM step up. 0 = no extra waiting.',
  'A higher value gives a softer start but slower regulation.']),
 ('chargingStatesHelp', 'Charging stages 1-4 (manual stage table)', [
  'Note: the stage table is only used when Charge mode = Manual stage table. That mode is not implemented in the firmware yet and the charger does not start in that mode.',
  'The charger runs stage 1, 2, 3 and 4. On end decides if it goes to the next stage, starts again at stage 1 or stops.',
  'Methods: Constant current (CC), Constant voltage (CV), Monitor (output off, the voltage is watched) and Off (the stage is skipped).',
  'A stage ends when any active end condition is met (0 = condition off), but not before Min time.',
  'Example: stage 1 CC 9.00 A, end when U >= 14.40 V. The charger charges at 9 A until the voltage reaches 14.40 V and then goes to stage 2.',
  'The charger max values (Max charge current, Max charge voltage) and the 15 A hardware limit always apply on top of the stages.',
  'Copy effective values to stage table (Auto mode - battery) fills the table from the effective auto values.']),
 ('autoHelp', 'Auto mode', [
  'Set battery type, nominal voltage and capacity. The firmware calculates all charge parameters: current = C-rate x capacity limited by the current cap, voltages per cell x cell count, tail current = share of capacity with a floor, and the times.',
  'To change one parameter, set its source to Manuell (manual) and enter the value. The other parameters stay Auto.',
  'The gauges on the right show the effective value that the firmware uses. They need a connection.',
  'Max charge voltage, Max charge current and the voltage limit always apply, also to manual values.',
  'Absorption and float voltages are given at 25 ' + DC + '. Temperature compensation is not active because there is no battery temperature sensor.',
  'The copy buttons change the pages in the controller. TunerStudio should then reload the page. If the fields do not update, close and open the dialog again. Burn afterwards as usual.',
  'Preview (PC) is calculated by TunerStudio. The effective values from the firmware are the reference.']),
 ('protectionHelp', 'Protection', [
  'Active in this firmware: output over-voltage (Max charge voltage), output over-current (Max charge current and the 15 A hardware limit), high input voltage (Max input voltage) and, in Auto mode, the max times and the bulk charge limit.',
  'Not active (stored only): MOSFET temperature stop and derating, temperature hysteresis, min input voltage, current restore, automatic retries after a fault, time-current protection and min/max driver voltage.']),
 ('calibrationHelp', 'Sensors and calibration', [
  'Current sensor: select the ACS712 type that is fitted. A change takes effect after a controller restart. Until then the charger does not start.',
  'Shunt + amplifier is not supported. The firmware then uses its built-in sensor type.',
  'Zero current sensor at boot: the output is off while the sensor is zeroed. The Zero current sensor button does the same with the output off.',
  'Not active (stored only): scale and offset calibration and the temperature sensors.']),
 ('diagHelp', 'Diagnostics', [
  'Both diagnostics modes keep the output off in this firmware. Fixed duty is not active.',
  'Diagnostics mode is not saved. After a restart the mode is always Normal.',
  'Clear errors acknowledges the errors (Error state goes back to Waiting). Reset counters clears the shown Ah, Wh and charge time, not the protection time. Restart controller and Zero current sensor are refused while charging.']),
]
HELPNAMES = [h[0] for h in HELP]

def wrap(s, n=72):
    out, cur = [], ''
    for w in s.split(' '):
        if cur and len(cur) + 1 + len(w) > n:
            out.append(cur)
            cur = w
        else:
            cur = (cur + ' ' + w) if cur else w
    if cur:
        out.append(cur)
    return out

def help_blocks():
    L = []
    for name, title, paras in HELP:
        L.append('help = %s, %s' % (name, q(title)))
        for k, p in enumerate(paras):
            L.append('   text = %s' % q(p + ('<br>' if k < len(paras) - 1 else '')))
    return L

def help_dialogs():
    # Help-menyn pekar pa dialoger (ref/validate_ini_v2.2.py godtar inte subMenu -> help-namn).
    L = []
    for name, title, paras in HELP:
        L.append('dialog = dlgHelp_%s, %s' % (name, q(title)))
        L.append('   topicHelp = %s' % name)
        for k, p in enumerate(paras):
            if k:
                L.append('   field = ""')
            for line in wrap(p):
                L.append('   field = %s' % q(line))
        L.append('')
    return L

# ---------------------------------------------------------------- [UserDefined]
COND_ON = '{ (chargeMode == 0) && (batteryType < 6) }'
def cond_man(x):
    return '{ (chargeMode == 0) && ((ovr_%s == 1) || (batteryType >= 6)) }' % x

def auto_pair(x):
    return ['   field = %s, ovr_%s, %s' % (q(LABEL[x]), x, COND_ON),
            '   field = "   Manual value", man_%s, %s' % (x, cond_man(x))]

def stage_dialog(n):
    s = 's%d' % n
    c = '{ (chargeMode == 1) && (%s_method != 3) }' % s
    L = ['dialog = dlgStage%d, "Stage %d"' % (n, n), '   topicHelp = chargingStatesHelp',
         '   field = "Method", %s_method, { chargeMode == 1 }' % s,
         '   field = "On end", %s_onEnd, %s' % (s, c),
         '   field = "Current (CC setpoint / CV limit)", %s_current, { (chargeMode == 1) && (%s_method <= 1) }' % (s, s),
         '   field = "Voltage (CV setpoint / CC limit)", %s_volt, { (chargeMode == 1) && (%s_method <= 1) }' % (s, s),
         '   field = "#End when (0 = off):"']
    for lab, f in (('U >=', 'endVoltMax'), ('U <=', 'endVoltMin'), ('I >=', 'endAmpMax'),
                   ('I <=', 'endAmpMin'), ('Charged, share of capacity', 'endAhPct'),
                   ('Max time', 'maxTime'), ('Min time', 'minTime'),
                   ('Battery temp above (not active)', 'tempMax'),
                   ('Battery temp below (not active)', 'tempMin')):
        L.append('   field = %s, %s_%s, %s' % (q(lab), s, f, c))
    L.append('')
    return L

def userdefined():
    L = help_blocks() + ['']
    L += ['dialog = dlgBasic, "Charger max values (hardware)"',
          '   topicHelp = accelerationDelayHelp',
          '   field = "Charge current, Simple mode (0 = max)", ChargeAmp',
          '   field = "Max charge current (0 = 15 A limit)", MaxChargeAmp',
          '   field = "Max charge voltage (0 = output off)", MaxChargeVolt',
          '   field = "Max input current (not active)", MaxInputAmp',
          '   field = "Max input voltage (0 = off)", MaxInputVolt',
          '   field = "Min input voltage (not active)", minInputVolt',
          '   field = "Max PWM", MaxPWM',
          '   field = "Min PWM", MinPWM',
          '   field = "MOSFET Tj max (reference only, not a protection)", MaxTemp',
          '   field = "Soft start per PWM step (not active)", accelerationDelay',
          '   field = "!Max charge voltage = 0 keeps the output off."',
          '']
    # Battery limits (befintliga falt, ingen ny konstant)
    L += ['dialog = dlgBatteryLimitsSet, "Battery"',
          '   topicHelp = autoHelp',
          '   field = "Battery type", batteryType',
          '   field = "Nominal voltage", batteryNomVolt',
          '   field = "Capacity (C20)", capacityAh',
          '   field = "Current cap in Auto", autoAmpCap, { chargeMode == 0 }']
    for x in ('voltLimit', 'chargeTempMin', 'chargeTempMax', 'totalMaxTime'):
        L += auto_pair(x)
    L += ['   field = "#Outer limits (always apply):"',
          '   field = "Max charge voltage (0 = output off)", MaxChargeVolt',
          '   field = "Max charge current (0 = 15 A limit)", MaxChargeAmp',
          '   field = "!No temperature sensor: temperature limits are not active."',
          '',
          'dialog = dlgBatteryLimitsEff, "Effective (firmware)"',
          '   gauge = voltLimitNowG', '   gauge = eff_chargeTempMinG',
          '   gauge = eff_chargeTempMaxG', '   gauge = eff_totalMaxTimeG', '',
          'dialog = dlgBatteryLimits, "Battery limits", xAxis',
          '   panel = dlgBatteryLimitsSet', '   panel = dlgBatteryLimitsEff', '']
    L += ['dialog = dlgAutoMainSet, "Battery"',
          '   topicHelp = autoHelp',
          '   field = "Charge mode", chargeMode',
          '   field = "Battery type", batteryType',
          '   field = "Nominal voltage", batteryNomVolt',
          '   field = "Capacity (C20)", capacityAh',
          '   field = "Temperature compensation (not active, read-only)", tempCompEnable, { 0 }',   # BESLUT nr 48: skrivskyddat
          '   field = "#Temperature compensation has no effect: there is no battery temp sensor."',
          '   field = "Current cap in Auto", autoAmpCap, { chargeMode == 0 }',
          '   field = "Floor for tail/float/precharge current", tailAmpFloor, { chargeMode == 0 }',
          '   field = "#Custom = no auto values, all manual values are used."',
          '   field = "!Manual stage table is not active: the charger does not start in that mode."',
          '   commandButton = "Copy auto values to manual values", cmdCopyAutoToMan, { (chargeMode == 0) && (batteryType < 6) }',
          '   commandButton = "Set all parameters to Auto", cmdAllAuto, { chargeMode == 0 }',
          '   commandButton = "Copy effective values to stage table s1-s4", cmdCopyEffToStages, { (statusBits & 1) == 0 }',
          '',
          'dialog = dlgAutoMainEff, "Status (firmware)"',
          '   gauge = autoWarnG', '   gauge = pa_diffCountG', '   gauge = effAbsorbNowG',
          '   gauge = effFloatNowG', '',
          'dialog = dlgAutoMain, "Auto mode - battery (type + capacity)", xAxis',
          '   panel = dlgAutoMainSet', '   panel = dlgAutoMainEff', '']
    for key, dname, title, extra in (('cur', 'Cur', 'current', []), ('volt', 'Volt', 'voltage', ['voltLimitNowG']),
                                     ('time', 'Time', 'times', []), ('temp', 'Temp', 'temperature (not active)', [])):
        L += ['dialog = dlgAuto%sSet, "Source and manual value"' % dname, '   topicHelp = autoHelp']
        for x, _ in AUTO[key]:
            L += auto_pair(x)
        if key == 'temp':
            L.append('   field = "!No battery temperature sensor: these values are not used."')
        L += ['', 'dialog = dlgAuto%sEff, "Effective (firmware)"' % dname]
        for x, _ in AUTO[key]:
            L.append('   gauge = eff_%sG' % x)
            if x == 'voltLimit':
                L += ['   gauge = %s' % g for g in extra]
        L += ['', 'dialog = dlgAuto%s, "Auto mode - %s", xAxis' % (dname, title),
              '   panel = dlgAuto%sSet' % dname, '   panel = dlgAuto%sEff' % dname, '']
    L += ['dialog = dlgAutoPreview1, ""']
    L += ['   gauge = pa_%sG' % x for x in ('cellCount', 'bulkAmp', 'absorbVolt', 'floatVolt', 'tailAmp',
          'floatAmp', 'bulkMaxTime', 'bulkAhPct', 'absorbMaxTime', 'absorbMinTime', 'floatMaxTime')]
    L += ['', 'dialog = dlgAutoPreview2, ""']
    L += ['   gauge = pa_%sG' % x for x in ('restartVolt', 'tempCompMv', 'chargeTempMin', 'chargeTempMax',
          'prechargeVolt', 'prechargeAmp', 'prechargeMaxTime', 'voltLimit', 'totalMaxTime',
          'batteryPresentVolt', 'diffCount')]
    L += ['', 'dialog = dlgAutoPreview, "Auto mode - preview (PC, formulas, not firmware)", xAxis',
          '   topicHelp = autoHelp', '   panel = dlgAutoPreview1', '   panel = dlgAutoPreview2', '']
    L += ['dialog = dlgStart, "Start and timer"',
          '   topicHelp = safetyHelp',
          '   field = "Start mode", startMode',
          '   field = "Start delay", startDelayMin, { startMode == 2 }',
          '   field = "Restart (below Restart voltage, see Auto mode)", restartMode',
          '   commandButton = "Start charging", cmdStart, { (errorBits == 0) && ((statusBits & 1) == 0) }',
          '   commandButton = "Stop charging", cmdStop', '']
    for n in range(1, 5):
        L += stage_dialog(n)
    L += ['dialog = dlgStages12, "", xAxis', '   panel = dlgStage1', '   panel = dlgStage2', '',
          'dialog = dlgStages34, "", xAxis', '   panel = dlgStage3', '   panel = dlgStage4', '',
          'dialog = dlgProfile, "Charging stages 1-4 (not active)", yAxis',
          '   topicHelp = chargingStatesHelp', '   panel = dlgStages12', '   panel = dlgStages34', '']
    L += ['dialog = dlgProtection, "Protection"',
          '   topicHelp = protectionHelp',
          '   field = "Max input voltage (0 = off)", MaxInputVolt',
          '   field = "!Not active in this firmware (stored only):"',
          '   field = "MOSFET Tj max (reference only)", MaxTemp',
          '   field = "Derating starts at", derateStartTemp',
          '   field = "Temperature hysteresis", tempHyst',
          '   field = "Min input voltage", minInputVolt',
          '   field = "Current restore delay", currentRestoreMs',
          '   field = "Current restore step", currentRestoreStep',
          '   field = "Automatic retries after fault", faultRetryCount',
          '   field = "Delay before retry", faultRetryDelay, { faultRetryCount > 0 }',
          '   field = "#Precharge, battery limits and times: see Auto mode."', '',
          'dialog = dlgDrive, "Driver / PWM"',
          '   topicHelp = protectionHelp',
          '   field = "Max PWM", MaxPWM',
          '   field = "Min PWM", MinPWM',
          '   field = "!Not active in this firmware (stored only):"',
          '   field = "PWM frequency (fixed 31 kHz)", pwmFreqKHz',
          '   field = "Soft start 0 to setpoint", softStartMs',
          '   field = "Max PWM change per loop", dutyStepMax',
          '   field = "Min driver voltage", driverVoltMin',
          '   field = "Max driver voltage", driverVoltMax', '',
          'dialog = dlgSensors, "Sensors"',
          '   topicHelp = calibrationHelp',
          '   field = "Current sensor (restart needed)", currentSensor',
          '   field = "Zero current sensor at boot", ampZeroAtBoot',
          '   field = "Input current sensor", inAmpSensor',
          '   field = "MOSFET temp sensor (not active)", mosfetTempSensor',
          '   field = "Battery temp sensor (not active)", batTempSensor', '',
          'dialog = dlgCalibration, "Calibration (not active)"',
          '   topicHelp = calibrationHelp',
          '   field = "!Scale and offset are stored but not used by the firmware."']
    for lab, f in (('Output voltage', 'OutVolt'), ('Input voltage', 'InVolt'), ('Output current', 'OutAmp'),
                   ('Input current', 'InAmp'), ('Driver voltage', 'Driver')):
        L += ['   field = "%s scale", cal%sScale' % (lab, f), '   field = "%s offset", cal%sOffset' % (lab, f)]
    L += ['   commandButton = "Zero current sensor (output off)", cmdZeroAmp, { (statusBits & 1) == 0 }', '',
          'dialog = dlgDiag, "Diagnostics"',
          '   topicHelp = diagHelp',
          '   field = "!Diagnostics mode turns the output off. Fixed duty is not active."',
          '   field = "Diagnostics mode", diagMode',
          '   field = "Fixed duty (not active)", diagDuty, { diagMode == 1 }',
          '   commandButton = "Clear errors", cmdClearErrors',
          '   commandButton = "Reset Ah/Wh/time", cmdResetCounters',
          '   commandButton = "Restart controller", cmdReboot, { (statusBits & 1) == 0 }', '',
          'dialog = dlgCommands, "Control"',
          '   topicHelp = safetyHelp',
          '   commandButton = "Start", cmdStart, { (errorBits == 0) && ((statusBits & 1) == 0) }',
          '   commandButton = "Stop", cmdStop',
          '   commandButton = "Clear errors", cmdClearErrors',
          '   commandButton = "Reset counters", cmdResetCounters',
          '   field = "#Save (Burn): use the normal TunerStudio Burn button."',
          '   field = "!Burn is rejected while charging."', '',
          'dialog = dlgMosfetCurve, "MOSFET temp curve (not active)"',
          '   topicHelp = calibrationHelp', '   panel = mosfetTempCurve', '',
          'dialog = dlgBatNtcCurve, "Battery NTC curve (not active)"',
          '   topicHelp = calibrationHelp', '   panel = batNtcCurve', '',
          'dialog = dlgDerateCurve, "Derating (not active)"',
          '   topicHelp = protectionHelp', '   panel = derateCurve', '',
          'dialog = dlgAmpTimeCurve, "Time-current protection (not active)"',
          '   topicHelp = protectionHelp', '   panel = ampTimeCurve', '']
    L += help_dialogs()
    return '\n'.join(L) + '\n'

CURVES = '''   curve = mosfetTempCurve, "MOSFET temp calibration (not active)"
      columnLabel = "ADC/4", "Temp"
      xAxis = 0, 255, 6
      yAxis = -40, 130, 6
      xBins = mosfetTempADCTable, mosfetTempADC
      yBins = mosfetTempTable
      gauge = mosfetTempGauge

   curve = batNtcCurve, "Battery NTC (not active)"
      columnLabel = "ADC", "Temp"
      xAxis = 0, 1023, 6
      yAxis = -40, 130, 6
      xBins = batNtcADC, batTempADC
      yBins = batNtcTemp
      gauge = batTempGauge

   curve = derateCurve, "Derating vs MOSFET temp (not active)"
      columnLabel = "Temp", "Current"
      xAxis = 0, 130, 6
      yAxis = 0, 100, 5
      xBins = derateTempBins, mosfetTemp
      yBins = deratePct
      gauge = mosfetTempGauge

   curve = ampTimeCurve, "Allowed current vs time (not active)"
      columnLabel = "Time", "Current"
      xAxis = 0, 3600, 6
      yAxis = 0, 15, 6
      xBins = ampTimeBins, overAmpTime
      yBins = ampTimeLimit
      gauge = outAmpGauge

'''

def menu():
    L = ['   menuDialog = main',
         '   menu = "&Charger"',
         '      subMenu = dlgBasic, "Charger max values"',
         '      subMenu = dlgBatteryLimits, "Battery limits"',
         '      subMenu = std_separator',
         '      subMenu = dlgAutoMain, "Auto mode - battery (type + capacity)"',
         '      subMenu = dlgAutoCur, "Auto mode - current", { chargeMode == 0 }',
         '      subMenu = dlgAutoVolt, "Auto mode - voltage", { chargeMode == 0 }',
         '      subMenu = dlgAutoTime, "Auto mode - times", { chargeMode == 0 }',
         '      subMenu = dlgAutoTemp, "Auto mode - temperature (not active)", { chargeMode == 0 }',
         '      subMenu = dlgAutoPreview, "Auto mode - preview (PC)"',
         '      subMenu = std_separator',
         '      subMenu = dlgStart, "Start / timer"',
         '      subMenu = dlgProfile, "Charging stages 1-4 (not active)", { chargeMode == 1 }',
         '      subMenu = dlgCommands, "Start / stop"',
         '   menu = "&Protection"',
         '      subMenu = dlgProtection, "Protection"',
         '      subMenu = dlgDerateCurve, "Derating (not active)"',
         '      subMenu = dlgAmpTimeCurve, "Time-current protection (not active)"',
         '   menu = "&Driver"',
         '      subMenu = dlgDrive, "PWM / driver"',
         '      subMenu = dlgSensors, "Sensors"',
         '   menu = "&Calibration"',
         '      subMenu = dlgCalibration, "Scale / offset (not active)"',
         '      subMenu = dlgMosfetCurve, "MOSFET temp curve (not active)"',
         '      subMenu = dlgBatNtcCurve, "Battery NTC curve (not active)"',
         '   menu = "D&iagnostics"',
         '      subMenu = dlgDiag, "Diagnostics / errors"',
         '   menu = "Tools"',
         '      subMenu = std_realtime, "Realtime display"',
         '   menu = "Help"']
    for name, title, _ in HELP:
        L.append('      subMenu = dlgHelp_%s, %s' % (name, q(title)))
    return '\n'.join(L) + '\n\n'

# ---------------------------------------------------------------- [SettingContextHelp]
def ctx():
    H = [
     ('ChargeAmp', 'Charge current in Simple CC/CV mode. 0 = use Max charge current.'),
     ('MaxChargeAmp', 'Max output current. 0 or values above 15 A give the 15 A hardware limit. Also applies to Auto and manual values.'),
     ('MaxInputVolt', 'Input voltage at or above this value stops the charger (error). 0 = off.'),
     ('MaxChargeVolt', 'Highest allowed output voltage. 0 = output always off (factory default).'),
     ('MaxInputAmp', 'Max input current. Not active: needs an input current sensor and is not used by the firmware yet.'),
     ('MaxPWM', 'Highest PWM duty step (0-25).'),
     ('MinPWM', 'Lowest PWM duty step when the output is on.'),
     ('MaxTemp', 'MOSFET junction temperature limit Tj max (125 ' + DC + ' = MOSFET limit). Reference value only, NOT an active protection: there is no temperature sensor. Cooling and the total max charge time are the only protection; do not leave the charger unattended.'),
     ('accelerationDelay', 'Soft start delay per PWM step. Not active: stored but not used yet.'),
     ('mosfetTempADCTable', 'MOSFET temp curve, x axis: ADC value divided by 4. Not active.'),
     ('mosfetTempTable', 'MOSFET temp curve, y axis: temperature for each ADC point. Not active.'),
     ('startMode', 'How charging starts.'),
     ('restartMode', 'In Monitor: restart from bulk when the voltage falls below the restart voltage (Auto mode).'),
     ('startDelayMin', 'Delayed start in minutes (Start mode = Timer).'),
    ]
    for n in range(1, 5):
        s = 's%d' % n
        p = 'Stage %d: ' % n
        H += [(s + '_method', p + 'method.'),
              (s + '_onEnd', p + 'what happens when an end condition is met.'),
              (s + '_tempMax', p + 'end if battery temp is above. Not active.'),
              (s + '_current', p + 'current (CC setpoint / CV current limit).'),
              (s + '_volt', p + 'voltage (CV setpoint / CC voltage limit).'),
              (s + '_endVoltMax', p + 'end when U >= this value (0 = off).'),
              (s + '_endVoltMin', p + 'end when U <= this value (0 = off).'),
              (s + '_endAmpMax', p + 'end when I >= this value (0 = off).'),
              (s + '_endAmpMin', p + 'end when I <= this value (CV end, 0 = off).'),
              (s + '_maxTime', p + 'max time (0 = unlimited).'),
              (s + '_minTime', p + 'min time before the stage may end.'),
              (s + '_tempMin', p + 'end or wait if battery temp is below. Not active.'),
              (s + '_endAhPct', p + 'end after this share of the capacity has been charged (0 = off).')]
    H += [
     ('pwmFreqKHz', 'PWM frequency. Not active: the firmware uses a fixed 31 kHz.'),
     ('softStartMs', 'Soft start time from 0 to setpoint. Not active.'),
     ('dutyStepMax', 'Max PWM change per control loop. Not active.'),
     ('driverVoltMin', 'Gate driver voltage below this value = error. Not active. Range 0-16 V.'),
     ('driverVoltMax', 'Gate driver voltage above this value = error. Not active. Range 0-16 V; the firmware limits stored values to 16.0 V.'),
     ('mosfetTempSensor', 'MOSFET temperature sensor. Not active.'),
     ('batTempSensor', 'Battery temperature sensor. Not active.'),
     ('currentSensor', 'Fitted current sensor type (gives the scale). Takes effect after a controller restart. Shunt + amplifier is not supported.'),
     ('ampZeroAtBoot', 'Zero the current sensor at boot with the output off.'),
     ('inAmpSensor', 'Input current sensor fitted? Controls if input power, loss and efficiency are calculated (PC).'),
     ('derateStartTemp', 'MOSFET temperature where derating starts. Not active.'),
     ('tempHyst', 'Hysteresis before restart after over-temperature. Not active.'),
     ('minInputVolt', 'Input voltage below this value = error (0 = off). Not active.'),
     ('currentRestoreMs', 'Current restore: delay after a limit before the current ramps up again. Not active.'),
     ('currentRestoreStep', 'Current restore: PWM steps per ramp step. Not active.'),
     ('faultRetryCount', 'Number of automatic retries after a fault (0 = always manual). Not active.'),
     ('faultRetryDelay', 'Delay before an automatic retry. Not active.'),
     ('diagMode', 'Diagnostics mode. Both diagnostics modes keep the output off. Not saved: always Normal after restart.'),
     ('diagDuty', 'Fixed duty in diagnostics mode 1. Not active.'),
    ]
    for lab, f in (('output voltage', 'OutVolt'), ('input voltage', 'InVolt'), ('output current', 'OutAmp'),
                   ('input current', 'InAmp'), ('driver voltage', 'Driver')):
        H += [('cal%sScale' % f, 'Calibration of %s: scale factor (measured = raw x scale + offset). Not active.' % lab),
              ('cal%sOffset' % f, 'Calibration of %s: offset. Not active.' % lab)]
    H += [
     ('batNtcADC', 'Battery NTC curve, x axis: ADC value. Not active.'),
     ('batNtcTemp', 'Battery NTC curve, y axis: temperature for each ADC point. Not active.'),
     ('derateTempBins', 'Derating curve, x axis: MOSFET temperature. Not active.'),
     ('deratePct', 'Derating curve, y axis: allowed current in % of setpoint. Not active.'),
     ('ampTimeBins', 'Time-current curve, x axis: time over the current limit. Not active.'),
     ('ampTimeLimit', 'Time-current curve, y axis: allowed current (0-15 A). Not active.'),
     ('chargeMode', 'Charge mode. Auto = the firmware calculates all parameters from battery type and capacity (each parameter can be set manually). Manual stage table is not active.'),
     ('tempCompEnable', 'Temperature compensation of absorption and float (ref. 25 ' + DC + '). Not active: no battery temperature sensor.'),
     ('batteryType', 'Battery type, selects the row in the type table. Custom = no auto values, all manual values are used.'),
     ('batteryNomVolt', 'Nominal battery voltage, gives the cell count (lead 2 V/cell, LiFePO4 3.2 V/cell). 6, 12, 24 or 36 V; the output is limited to ' + HW_VOLT + ' V.'),
     ('capacityAh', 'Battery capacity C (20 h rating). The only value that must be set in Auto mode.'),
     ('autoAmpCap', 'Current cap in Auto. Effective cap = lowest of this value, the 15 A hardware limit, Max charge current and the sensor range.'),
     ('tailAmpFloor', 'Floor for tail, float and precharge current (ACS712 is uncertain below about 0.5 A).'),
    ]
    PDESC = {
     'cellCount': 'Number of cells in series.',
     'bulkAmp': 'Bulk current (CC) = C-rate x capacity, limited by the cap.',
     'absorbVolt': 'Absorption voltage at 25 ' + DC + '.',
     'floatVolt': 'Float voltage at 25 ' + DC + '.',
     'tailAmp': 'Tail current that ends absorption = share of capacity with a floor. 0 = time only.',
     'floatAmp': 'Max current in float (0.05 C, not above bulk).',
     'bulkMaxTime': 'Max bulk time (1.2 x C / I). Exceeded = error.',
     'bulkAhPct': 'Max charge in bulk, % of capacity (0 = off). Exceeded = error.',
     'absorbMaxTime': 'Max absorption time.',
     'absorbMinTime': 'The tail current must be held this long.',
     'floatMaxTime': 'Max float time (0 = unlimited), then Monitor.',
     'restartVolt': 'Restart from Monitor below this voltage.',
     'tempCompMv': 'Temperature compensation per cell. Not active: no battery temperature sensor.',
     'chargeTempMin': 'Charging allowed from this battery temperature. Not active: no sensor.',
     'chargeTempMax': 'Charging allowed up to this battery temperature. Not active: no sensor.',
     'prechargeVolt': 'Precharge (low current) below this voltage.',
     'prechargeAmp': 'Precharge current (0.03 C with floor, not above bulk).',
     'prechargeMaxTime': 'The precharge voltage must be passed within this time, else error.',
     'voltLimit': 'Highest allowed output voltage after compensation.',
     'totalMaxTime': 'Total max time for precharge, bulk, absorption and float (max 10080 min).',
     'batteryPresentVolt': 'Battery detected above this voltage.',
    }
    assert set(PDESC) == set(LABEL)
    order = ['cellCount', 'bulkAmp', 'absorbVolt', 'floatVolt', 'tailAmp', 'floatAmp', 'bulkMaxTime',
             'bulkAhPct', 'absorbMaxTime', 'absorbMinTime', 'floatMaxTime', 'restartVolt', 'tempCompMv',
             'chargeTempMin', 'chargeTempMax', 'prechargeVolt', 'prechargeAmp', 'prechargeMaxTime',
             'voltLimit', 'totalMaxTime', 'batteryPresentVolt']
    for x in order:
        H.append(('ovr_' + x, 'Source for %s: Auto = calculated, Manuell = manual value.' % LABEL[x].split(' (')[0]))
    for x in order:
        H.append(('man_' + x, 'Manual value: %s Used when the source is Manuell or the type is Custom.' % PDESC[x]))
    return '\n'.join('   %s = %s' % (k, q(v)) for k, v in H) + '\n\n', [k for k, _ in H]

DATALOG = [('seconds', 'Time', 'int', '%d'), ('chargerState', 'State', 'int', '%d'),
           ('stage', 'Stage', 'int', '%d'), ('statusBits', 'Status', 'int', '%d'),
           ('restrictionBits', 'Limits', 'int', '%d'), ('errorBits', 'Errors', 'int', '%d'),
           ('pulseWidth', 'PWM', 'int', '%d'), ('dutyPct', 'Duty', 'float', '%.1f'),
           ('outVolt', 'Uout', 'float', '%.2f'), ('outAmp', 'Iout', 'float', '%.2f'),
           ('inVolt', 'Uin', 'float', '%.2f'), ('inAmp', 'Iin', 'float', '%.2f'),
           ('outPower', 'Pout', 'float', '%.1f'), ('inPower', 'Pin', 'float', '%.1f'),
           ('efficiency', 'Eff', 'float', '%.1f'), ('ampHours', 'Ah', 'float', '%.2f'),
           ('wattHours', 'Wh', 'float', '%.1f'), ('mosfetTemp', 'Tmos', 'float', '%.1f'),
           ('batTemp', 'Tbat', 'float', '%.1f'), ('driverVolt', 'Udrv', 'float', '%.2f'),
           ('voltSetpoint', 'Uset', 'float', '%.2f'), ('ampSetpoint', 'Iset', 'float', '%.2f'),
           ('derateNow', 'Derate', 'int', '%d'), ('stageTime', 'Tstage', 'int', '%d'),
           ('loopTimeUs', 'Loop', 'int', '%d'), ('eff_bulkAmp', 'IbulkEff', 'float', '%.2f'),
           ('eff_absorbVolt', 'UabsEff', 'float', '%.2f'), ('eff_floatVolt', 'UfloatEff', 'float', '%.2f'),
           ('eff_tailAmp', 'ItailEff', 'float', '%.2f'), ('tempCompDelta', 'Ucomp', 'float', '%.2f'),
           ('autoWarnBits', 'AutoWarn', 'int', '%d'), ('effManualMask', 'ManMask', 'int', '%d')]

def datalog():
    L = ['   ; entry = kanal, "rubrik", typ, "format"']
    L += ['   entry = %s, %s, %s, %s' % (c, q(h), ty, q(f)) for c, h, ty, f in DATALOG]
    return '\n'.join(L) + '\n\n'

# ---------------------------------------------------------------- huvudfunktion
def apply(t):
    # 1. en teckenkodning: kallfilen ar UTF-8 i nagra tecken men lases latin-1
    t = t.replace('\xc2\xb0', D).replace('\xe2\x80\x93', '-')
    old_dialogs = set(keys_in(t, 'UserDefined', 'dialog'))
    old_cmdbtn = set(re.findall(r'commandButton = "[^"]*", (\w+)', t))
    old_ctx = set(lhs_in(t, 'SettingContextHelp'))
    old_log = [l.split(',')[0].split('=')[1].strip() for l in t[slice(*section_span(t, 'Datalog'))].splitlines()
               if l.strip().startswith('entry')]
    old_curves = keys_in(t, 'CurveEditor', 'curve')
    old_gauges = set(lhs_in(t, 'GaugeConfigurations')) - {'gaugeCategory'}
    t = do_constants(t)
    t = do_outputs(t)
    t = do_gauges(t)
    t = replace_section(t, 'FrontPage', frontpage())
    t = replace_section(t, 'UserDefined', userdefined())
    t = replace_section(t, 'CurveEditor', CURVES)
    t = replace_section(t, 'Menu', menu())
    body, keys = ctx()
    t = replace_section(t, 'SettingContextHelp', body)
    t = replace_section(t, 'Datalog', datalog())
    # 2. kontroller: inga namn har forsvunnit
    new_dialogs = set(keys_in(t, 'UserDefined', 'dialog'))
    assert old_dialogs <= new_dialogs, old_dialogs - new_dialogs
    assert old_cmdbtn <= set(re.findall(r'commandButton = "[^"]*", (\w+)', t))
    assert old_ctx <= set(keys), old_ctx - set(keys)
    assert len(keys) == len(set(keys))
    assert old_log == [c for c, _, _, _ in DATALOG]
    assert old_curves == keys_in(t, 'CurveEditor', 'curve')
    assert old_gauges == set(lhs_in(t, 'GaugeConfigurations')) - {'gaugeCategory'}
    # 3. en teckenkodning: bara ASCII + grad-tecknet
    bad = sorted({c for c in t if ord(c) > 126 and c != D} | {c for c in t if ord(c) < 32 and c not in '\r\n\t'})
    assert not bad, [hex(ord(c)) for c in bad]
    return t
