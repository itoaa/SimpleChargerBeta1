#!/usr/bin/env python3
# Genererar tunerStudio_v2.2.ini fran SmedbyCharger_v2.2.ini (Drive, 2026-10-24,
# md5 55ba5c2a1b30bba0bd108f1253a7d464) med firmwareandringarna for v2.2.
# Anvandning: python3 -B tools/make_ini22.py SmedbyCharger_v2.2.ini tunerStudio_v2.2.ini
# Varje ersattning kontrolleras (assert) sa att kallfilen matchar exakt.
# Validering: make -C test test (check_ini_layout.py + ref/validate_ini_v2.2.py).
import sys
src, dst = sys.argv[1], sys.argv[2]
t = open(src, encoding='latin-1').read()
def rep(old, new, count=1):
    global t
    n = t.count(old)
    assert n == count, (old, n)
    t = t.replace(old, new)
hdr_add = (
"; -------------------------------------------------------------------------------------\n"
"; tunerStudio_v2.2.ini i repot SimpleChargerBeta1 (LTT-Sweden AB), 2026-10-25.\n"
"; Utgangspunkt: SmedbyCharger_v2.2.ini (Drive, 2026-10-24). Andringar for firmware v2.2:\n"
";   * ochBlockSize 114 -> 115, ny kanal eepromWarnBits pa offset 114 (BESLUT nr 21).\n"
";   * Indikatorer for eepromWarnBits b0..b7.\n"
";   * voltLimit-visning (BESLUT nr 24): MaxChargeVolt = 0 betyder UTGANG AV. Matarna\n"
";     'Hogsta spanning' visar da 0 i stallet for 15,00 V (nya kanaler voltLimitShown/\n"
";     pcVoltLimitShown, ny matare voltLimitNowG) och en rod indikator visas. eff_voltLimit och pa_voltLimit ar\n"
";     oforandrade, sa pa_diffCount jamfor som forut.\n"
"; Firmware-tolkning av %o/%c: 1 byte vardera (sidor <= 255 B). Inte testad i riktig TunerStudio.\n"
"; Compat-filerna (tunerStudio.ini, SmedbyCharger_compat.ini) ar OFORANDRADE.\n"
"; -------------------------------------------------------------------------------------\n")
rep("[MegaTune]\n", hdr_add + "[MegaTune]\n")
rep("   ochBlockSize  = 114\n", "   ochBlockSize  = 115\n")
rep("   pageRefreshReq       = scalar, U08, 113, \"\", 1, 0\n",
    "   pageRefreshReq       = scalar, U08, 113, \"\", 1, 0\n"
    "   ; FIRMWARE: EEPROM-varningar (BESLUT nr 21): b0 huvud ogiltigt (alla sidor standard), b1..b5 CRC-fel sida 1..5 (sidan standard), b6 burn misslyckades, b7 ingen EEPROM-backend. Lyckad burn nollar.\n"
    "   eepromWarnBits       = scalar, U08, 114, \"\", 1, 0\n")
# voltLimit-visning: efter pa_voltLimit-formeln (formelordning)
import re
m = re.search(r"^   pa_voltLimit         = \{.*\n", t, re.M)
assert m
add = ("   ; FIRMWARE (BESLUT nr 24): MaxChargeVolt = 0 = utgang av. Visa 0 i matarna i stallet for typens voltLimit.\n"
       "   mcvOutputOff         = { (MaxChargeVolt == 0) ? 1 : 0 }\n"
       "   voltLimitShown       = { (MaxChargeVolt == 0) ? 0 : eff_voltLimit }, \"V\"\n"
       "   pcVoltLimitShown     = { (MaxChargeVolt == 0) ? 0 : pa_voltLimit }, \"V\"\n")
t = t[:m.end()] + add + t[m.end():]
rep('   eff_voltLimitG         = eff_voltLimit, "Hogsta spanning", "V", 0, 60, 0, 0, 60, 60, 2, 2\n',
    '   eff_voltLimitG         = eff_voltLimit, "Hogsta spanning (autoberakning)", "V", 0, 60, 0, 0, 60, 60, 2, 2\n'
    '   voltLimitNowG          = voltLimitShown, "Hogsta spanning nu (0 = utgang av)", "V", 0, 60, 0, 0, 60, 60, 2, 2\n')
rep('   gauge = eff_voltLimitG\n', '   gauge = eff_voltLimitG\n   gauge = voltLimitNowG\n')
rep('   pa_voltLimitG          = pa_voltLimit, "Hogsta spanning (PC)", ',
    '   pa_voltLimitG          = pcVoltLimitShown, "Hogsta spanning (PC, 0 = utgang av)", ')
anchor = re.search(r"^   indicator = \{ \(autoWarnBits & 1024\) > 0 \}.*\n", t, re.M)
assert anchor
ind = ("   indicator = { mcvOutputOff }, \"MaxChargeVolt satt\", \"UTGANG AV: MaxChargeVolt = 0 (sida 1)\", white, black, red, black\n"
       "   indicator = { eepromWarnBits & 1 }, \"EEPROM huvud ok\", \"EEPROM: huvud ogiltigt - standardvarden\", white, black, red, black\n"
       "   indicator = { eepromWarnBits & 2 }, \"EEPROM sida 1 ok\", \"EEPROM: CRC-fel sida 1 - standardvarden\", white, black, red, black\n"
       "   indicator = { eepromWarnBits & 4 }, \"EEPROM sida 2 ok\", \"EEPROM: CRC-fel sida 2 - standardvarden\", white, black, red, black\n"
       "   indicator = { eepromWarnBits & 8 }, \"EEPROM sida 3 ok\", \"EEPROM: CRC-fel sida 3 - standardvarden\", white, black, red, black\n"
       "   indicator = { eepromWarnBits & 16 }, \"EEPROM sida 4 ok\", \"EEPROM: CRC-fel sida 4 - standardvarden\", white, black, red, black\n"
       "   indicator = { eepromWarnBits & 32 }, \"EEPROM sida 5 ok\", \"EEPROM: CRC-fel sida 5 - standardvarden\", white, black, red, black\n"
       "   indicator = { eepromWarnBits & 64 }, \"Burn ok\", \"EEPROM: burn misslyckades\", white, black, red, black\n"
       "   indicator = { eepromWarnBits & 128 }, \"EEPROM-backend ok\", \"EEPROM: ingen backend\", white, black, red, black\n")
t = t[:anchor.end()] + ind + t[anchor.end():]
open(dst, 'w', encoding='latin-1', newline='').write(t)
