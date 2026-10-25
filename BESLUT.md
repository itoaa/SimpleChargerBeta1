# Beslut – SimpleChargerBeta1

Beslut fattade av Ola Andersson (LTT-Sweden AB). Varje beslut har ett nummer. Ändra inte ett beslut i efterhand; lägg i stället till ett nytt beslut som ersätter det gamla.

## 1. Projektet tillhör LTT-Sweden AB (2026-10-25)
SimpleChargerBeta1 ägs av LTT-Sweden AB och hålls helt skilt från privata system och repon.

## 2. Bootloader: Optiboot 512 B (2026-10-25)
ATmega328P med Optiboot, 512 B boot-sektion. Firmware får använda högst 32 256 B flash.

## 3. Gren- och PR-flöde (2026-10-25)
Allt arbete görs på feature-grenar och går via PR. Ingenting committas direkt mot master.

## 4. CI får committa elf/hex på feature-grenen (2026-10-25)
CI-jobbet får committa tillbaka `charger.elf` och `charger.hex` till feature-grenen. Hämta in de ändringarna (merge/pull, ingen rebase) innan du pushar.

## 5. Compat-ini är referens (2026-10-25)
`SmedbyCharger_compat.ini` är referens. Compat-filerna ändras inte.

## 6. EEPROM kopplas in med v2.2-protokollet (2026-10-25)
EEPROM-modulen kopplas in i `main.cpp` tillsammans med v2.2-protokollet, inte före. Fram till dess ska master-bygget vara oförändrat.

## 7. B bränner alla sidor, avvisas under laddning, utgången är av under burn (2026-10-25)
- B skriver alla sidor och skriver bara på uttryckligt kommando.
- Om laddning pågår avvisas B.
- Annars stängs utgången av före skrivningen och återställs efter.

## 8. EEPROM-varningar i en ny statusbyte i realtidsdata (2026-10-25)
- Varningsbitarna läggs i en ny byte i realtidsdata. Förslaget är offset 114, så att blocket växer från 114 till 115 B (se `docs/eeprom-layout.md`).
- En lyckad burn nollar varningarna.

## 9. Kopieringsknapparna bränner inte EEPROM (2026-10-25)
E\x07–E\x09 ändrar bara RAM. Användaren bränner med B.

## 10. MaxChargeVolt 0 = utgång av (2026-10-25)

## 11. Steg 4 Övervaka med omstart vid 12,5 V (2026-10-25)

## 12. Float max 1440 min för AGM/GEL/LiFePO4 (2026-10-25)

## 13. LiFePO4 tas med som obekräftat Victron-förslag (2026-10-25)

## 14. Ingen migrering av sida 1 (2026-10-25)
- Efter uppgradering laddas standardvärden och en varningsbit sätts.
- Ett PC-verktyg för `.msq` är valfritt.

## 15. accelerationDelay: enhet ms, skala 10 (2026-10-25)

## 16. mosfetTempADCTable: enhet ADC/4 (2026-10-25)

## 17. F-kommandot svarar 002 först med v2.2-protokollet (2026-10-25)

## 18. Sensorer (etapp 6) skjuts upp (2026-10-25)

## 19. Lysdioder direkt från MCU-pinnar – undantag (2026-10-25)
- PD3–PD5 driver lysdioderna direkt från MCU-pinnarna. Det är ett uttryckligt undantag från regeln "inget drivs direkt från en MCU-pinne".
- PB1 (PWM) går via en gate-drivare.

## 20. EEPROM-basadress 16 (2026-10-25)
- Layouten börjar på adress 16 och slutar på 323 (308 B).
- Adress 0–15 är reserverade och skrivs aldrig av modulen.
- Adress 0 undviks eftersom den lätt korrumperas vid brownout [U, erfarenhet från AVR-communityt, inte databladet].
- Se `docs/eeprom-layout.md`.

## 21. Statusbyte `eepromWarnBits` på realtidsoffset 114 (2026-10-25)
Gäller beslut 8. Byten är en U08 på offset 114, och realtidsblocket växer från 114 till 115 B. Ini och protokoll ändras när v2.2 kopplas in.

| Bit | Betydelse |
|---|---|
| b0 | HeaderInvalid: alla sidor har standardvärden |
| b1–b5 | CRC-fel på sida 1–5: sidan har standardvärden |
| b6 | BurnFailed |
| b7 | NoBackend |

## 22. Standardvärden för tabeller utan defaultValue (2026-10-25)
Värdena i `test/table_proposals.py` och `shared/eepromDefaults.cpp` är godkända:

| Tabell | Värden |
|---|---|
| mosfetTempADCTable (ADC/4) | 8, 12, 16, 21, 28, 38, 51, 67, 88, 128, 197, 233 |
| mosfetTempTable (°C) | 125, 110, 100, 90, 80, 70, 60, 50, 40, 25, 0, −20 |
| batNtcADC | 204, 270, 310, 355, 404, 456, 512, 570, 685, 789, 874, 935 |
| batNtcTemp (°C) | 60, 50, 45, 40, 35, 30, 25, 20, 10, 0, −10, −20 |
| derateTempBins (°C) | 70–120 i steg om 10 |
| deratePct (%) | 100, 80, 60, 40, 20, 0 |
| ampTimeBins (s) | 0, 60, 300, 900, 3600 |
| ampTimeLimit (0,01 A) | 1500 × 5 (= hårdvarutaket 15 A) |

**NTC-tabellerna är PLATSHÅLLARE.** Det gäller batNtcADC/batNtcTemp och mosfetTempADCTable/mosfetTempTable. Kortet saknar NTC, så värdena bygger på en antagen NTC på 10 kΩ/B3950 med 10 kΩ pull-up och är inte uppmätta. De ska bytas ut när en givare väljs (etapp 6, beslut 18).

## 23. BOD 4,3 V – efuse 0xFC (2026-10-25)
- **Nivå:** BODLEVEL[2:0] = 100, alltså 4,3 V typiskt och 4,1–4,5 V (databladet DS40002061B, tabell 29-12).
- **Efuse-värdet:** BODLEVEL är bit 2:0 i efuse. Bit 7:3 används inte och är 1 som standard (tabell 28-6). Ett programmerat bit läses som 0 (avsnitt 28.2). Alltså är 0xFC = 1111 1100 = BODLEVEL 100.
- **Varför:** vid 16 MHz garanterar databladet korrekt drift först från ungefär 3,8 V (uträknat ur avsnitt 29.3). Med BOD 4,3 V hålls MCU:n i reset innan den hamnar i det osäkra området.
- **Ändras inte:** lfuse och hfuse. Arduino Uno/Optiboot använder lfuse 0xFF och hfuse 0xDE (512 B boot) enligt `boards.txt`.
- **Vem gör det:** Ola sätter säkringen själv med ISP-programmerare. Optiboot kan inte skriva säkringar.

Läs först av alla tre säkringarna och notera värdena:
```
avrdude -c <PROGRAMMER> -P <PORT> -p m328p -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h
```
Om lfuse = 0xFF och hfuse = 0xDE, sätt bara efuse:
```
avrdude -c <PROGRAMMER> -P <PORT> -p m328p -U efuse:w:0xFC:m
```
Läs sedan av igen och kontrollera:
```
avrdude -c <PROGRAMMER> -P <PORT> -p m328p -U efuse:r:-:h
```
- `<PROGRAMMER>` och `<PORT>` är platshållare, till exempel `usbasp` (där `-P` ofta inte behövs) eller `avrisp -P /dev/ttyACM0 -b 19200`.
- Använd inte `-e` (chip erase). Med hfuse 0xDE är EESAVE av, så en chip erase raderar EEPROM.
- Om lfuse/hfuse avviker från 0xFF/0xDE: ändra ingenting, stanna och utred.
- **Läsvärdet:** avrdude.conf (avrdudes main-gren, part m328) har `bitmask = 0x07` för efuse och skriver bara bit 2:0. Därför kan en avläsning visa 0xFC eller 0x04 beroende på avrdude-version och programmerare [U, inte verifierat]. Det som räknas är bit 2:0 = 100.
- **Inte verifierat:**
  - MCU:ns matningsspänning på kortet. Den finns inte i repot. BOD 4,3 V kräver att VCC hela tiden ligger över 4,5 V (max VBOT) med rippel och lastsprång. Annars går MCU:n i reset.
  - Lock-bitarna. Uno använder 0x0F enligt `boards.txt`. LB1 är då inte programmerad, så säkringarna är inte låsta (avsnitt 28.2). Det är inte avläst på något kort.

**BurnGuard kompletterar BOD men ersätter den inte.**
- BurnGuard (beslut 7) stänger av utgången och nekar B under laddning.
- BOD är det enda som hindrar CPU:n från att köra, och skriva EEPROM, vid för låg spänning.
- CRC och huvudet-sist hindrar bara att trasiga data används.

## 24. MaxChargeVolt standardvärde 0 = utgång av (2026-10-25)
Ola har bekräftat det tidigare förslaget: standardvärdet för MaxChargeVolt är **0 = utgång av** (SPEC_v2.2.md öppen fråga 7).

**Skäl:**
- Standardvärden används när huvudet är ogiltigt, vid CRC-fel på sida 1 och efter uppgradering (beslut 14). Då ska laddaren inte ladda på gissade gränser.
- Det stämmer med beslut 10 och `main.cpp:68` i B-2.
- Autoläget räknar redan voltLimit per batterityp.

**Bortvalda alternativ:**
- **14,40 V** underladdar batterier med absorption 14,70 V.
- **14,70 V** tar bort temperaturkompensationen uppåt i kyla.
- **15,00 V** tillför inget för 12 V bly, gör att laddaren laddar okonfigurerad och fungerar inte för 24 V.
- **Tak = absorptionsspänningen** beror på batterityp och temperatur och kan därför inte vara ett fast värde.

**TS-visningen ses över vid inkopplingen.** I v2.2-ini (rad cirka 662, `pa_voltLimit`) och `auto_params_v2.2.py` (rad cirka 164) betyder MaxChargeVolt 0 "inget extra tak". TS visar då voltLimit 15,00 V fast utgången är av. Det hanteras i v2.2-implementationen.
