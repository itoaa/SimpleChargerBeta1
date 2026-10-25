#!/usr/bin/env python3
"""Standardvarden (godkanda, BESLUT.md nr 22) for tabeller som saknar defaultValue i
SmedbyCharger_v2.2.ini (sida 1 MOSFET-kurva och alla sex tabeller pa sida 4).

Inget ar verifierat mot hardvara [U]; NTC-kurvorna ar PLATSHALLARE. Kortet
har i dag ingen NTC (TemperaturePin PD7 ar digital, DS18B20 ej lankad), sa
NTC-kurvorna ar platshallare som gor tabellerna monotona och anvandbara om
en NTC monteras. Ini-filen andras inte; gen_defaults_golden.py lagger in
dessa varden for falt utan defaultValue.

NTC-modell [U]: 10 kOhm @ 25 C, B = 3950 K, NTC mot GND, 10 kOhm pull-up mot
AVcc (= Vref), ADC 10 bit: adc10 = round(1024 * R / (R + Rp)), max 1023.
MOSFET-tabellen lagras som ADC/4 (BESLUT.md nr 16): adc4 = adc10 >> 2.

Kor fristaende for att skriva ut tabellen:  python3 table_proposals.py
"""
import math

NTC_R25 = 10000.0
NTC_B = 3950.0
PULLUP = 10000.0


def ntc_adc10(temp_c):
    t = temp_c + 273.15
    r = NTC_R25 * math.exp(NTC_B * (1.0 / t - 1.0 / 298.15))
    return min(1023, int(round(1024.0 * r / (r + PULLUP))))


# Temperaturpunkter i stigande ADC-ordning (= fallande temperatur), 12 st.
MOSFET_TEMPS = [125, 110, 100, 90, 80, 70, 60, 50, 40, 25, 0, -20]
BAT_TEMPS = [60, 50, 45, 40, 35, 30, 25, 20, 10, 0, -10, -20]


def proposals():
    p = {}
    p["mosfetTempADCTable"] = [ntc_adc10(t) >> 2 for t in MOSFET_TEMPS]
    p["mosfetTempTable"] = list(MOSFET_TEMPS)
    p["batNtcADC"] = [ntc_adc10(t) for t in BAT_TEMPS]
    p["batNtcTemp"] = list(BAT_TEMPS)
    # Deratering fran derateStartTemp (sida 3, standard 70 C) ned till 0 % vid 120 C.
    p["derateTempBins"] = [70, 80, 90, 100, 110, 120]
    p["deratePct"] = [100, 80, 60, 40, 20, 0]
    # Tid-strom-skydd: platt 15,00 A = hardvarutaket (HW_AMP_MAX_CA 1500 i
    # auto_params_v2.2.py, MaxChargeAmp max 15 A). Begransar alltsa inte mer
    # an i dag; tiderna ar bara en stigande axel (0 s .. 1 h).
    p["ampTimeBins"] = [0, 60, 300, 900, 3600]
    p["ampTimeLimit"] = [1500, 1500, 1500, 1500, 1500]  # 0,01 A
    for k in ("mosfetTempADCTable", "batNtcADC", "derateTempBins", "ampTimeBins"):
        v = p[k]
        assert all(a < b for a, b in zip(v, v[1:])), k + " maste vara strikt stigande"
    return p


if __name__ == "__main__":
    for k, v in proposals().items():
        print("%-20s %s" % (k, v))
