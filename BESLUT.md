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

---

Beslut 25–34 nedan är **implementationsval i etapp 2–5 och 7 (2026-10-25)**, gjorda inom ramen för beslut 1–24 och Olas uppdrag 2026-10-25 10:44. De är inte separat bekräftade av Ola. Ett val som Ola vill ändra ersätts med ett nytt beslut. Inget av detta är verifierat på hårdvara eller i TunerStudio.

## 25. Definition av chargingActive (2026-10-25)
`chargingActive` = laddartillståndet är något av {Timer, SoftStart, Charging, PauseTemp} **eller** duty > 0 **eller** diagMode == 1 (`Charger::chargingActive()`).
- Används av BurnGuard (beslut 7): B avvisas när den är sann.
- Används av E5 (omstart) och E6 (nollställ strömgivare): de ignoreras när den är sann.
- SoftStart och PauseTemp finns som tillstånd men nås inte i denna version (se nr 34).

## 26. B i main.cpp (2026-10-25)
- B anropar `EepromStore::burn()` via BurnGuard. Utgången stängs av (OCR 0) före skrivningen och sätts tillbaka till laddarens duty efter. Eftersom B bara tillåts när `chargingActive` är falsk är den duty i praktiken 0.
- Resultatet syns i realtime 114 `eepromWarnBits`. En avvisad B (laddning pågår) ändrar inte varningsbitarna. *Besvarad 2026-10-25:* den visas i stället som restrictionBits b8, se nr 35.
- Kopieringsknapparna E7–E9 ändrar bara RAM-kopian och bränner aldrig.

## 27. Startsemantik (2026-10-25)
- Laddaren startar **aldrig automatiskt vid uppstart**. Efter boot är duty 0 och tillståndet Waiting.
- Laddning startar på E1, eller vid startMode Auto/Timer när ett batteri ansluts efter boot (frånvarande → närvarande).
- Start kräver: MaxChargeVolt > 0 (beslut 24), MaxPWM > 0 och MinPWM ≤ MaxPWM, chargeMode Auto eller Enkel, inga fel, inte Error-läge. I Auto krävs dessutom ett batteri och att autoWarnBits saknar AH_MISSING och NOMV_INVALID.
- E1 gäller bara det regulatorvarv där det tas emot (`startReq` nollställs i varje `update()`). Ett E1 som kommer i Error-läge lagras alltså inte och startar inte laddning efter E4.
- diagMode tvingas till Normal vid uppstart (SPEC "sparas inte").

## 28. E-kommandon (2026-10-25)
| Kod | Funktion i firmware |
|---|---|
| E1 | Start (gäller bara det varv det tas emot, se nr 27) |
| E2 | Stopp → Waiting. Ingen autostart förrän batteriet har kopplats ur och in igen. |
| E3 | Nollställ visade räknare (Ah, Wh, laddtid). Skyddstiden totalMaxTime nollställs inte. |
| E4 | Kvittera fel. Error → Waiting. |
| E5 | Omstart via watchdog (ca 15 ms) efter att utgången stängts av. Ignoreras när `chargingActive` är sann. |
| E6 | Nollställ strömgivaren. Ignoreras när `chargingActive` är sann. |
| E7 | Autovärden → `man_*` på sida 5 (`ovr`-bitarna rörs inte), pageRefreshReq-bit 0x10 |
| E8 | **Inte implementerad.** Mappningen effektiva värden → stegtabell s1–s4 på sida 2 är inte specificerad. Knappen i ini gör ingenting. |
| E9 | Alla parametrar → Auto: `ovr`-bitarna (sida 5, byte 8–10) nollas. `man_*` lämnas. pageRefreshReq-bit 0x10. |

- Efter E7–E9 och efter varje sidskrivning räknas autoparametrarna om. pageRefreshReq (realtime 113) hålls i cirka 1 s. Det är otestat i TunerStudio.
- Rättelse: commit 19c8c4b beskriver E9 som "nollställ räknare" i rubriken. Rätt är "alla parametrar → Auto" enligt ovan. Koden och testerna följer tabellen.

## 29. Strömgivare och nollställning (2026-10-25)
- Sida 3 bit 4–5 (currentSensor) läses **bara vid uppstart**. En ändring kräver omstart (E5 eller strömavbrott). Avvikelse: SPEC anger inte när den ska gälla.
- 0 = 5 A, 1 = 20 A och 2 = 30 A motsvarar `Current`-typ 1–3. 3 = shunt stöds inte av `Current`, så då används `Current_Measure_type` i `Hardware.hpp`.
- ampZeroAtBoot (bit 6): nollställning efter cirka 200 ms med utgången av, bara om biten är satt.
- **Öppen fråga:** `Current::readCurrent()` returnerar `int` (16 bit) i mA (`scaleAcs712Ma`: `(int)(amps * 1000.0)`). Över 32,767 A är omvandlingen double → int odefinierad. Typ 3 (66 mV/A) kan vid full ADC-skala ge ungefär ±37,9 A [U] (ref 5 000 mV, offset 2 500 mV antaget), så det kan inträffa vid överström eller fel. main räknar om till cA i 32 bit och klampar till ±32 767 cA, men det hjälper inte mot överslag i drivrutinen.

## 30. Realtidsfält som inte mäts (2026-10-25)
| Fält | Värde | Skäl |
|---|---|---|
| Temperaturer och deras ADC | 0 = ej tillgänglig | Ingen givare utan etapp 6 (pinnar saknas, plan fråga 9) |
| inAmp | 0 | Ingen ingångsströmgivare |
| loopTimeUs | 0 | Inte implementerat |
| pwmFreqNow | 31 (kHz) | **Beräknat** från Timer1-registren (WGM10, CS10: 16 MHz / 510 ≈ 31,4 kHz), inte mätt |
| resetCause | MCUSR & 0x0F | Läses i `.init3` innan något annat körs. [U] Optiboot kan redan ha nollställt MCUSR, och då blir värdet 0. Inte verifierat. |

Temperaturkompensering är inaktiv eftersom det inte finns någon batteritemperaturgivare.

## 31. Stegnumrering i Auto (2026-10-25)
| Steg | Namn | Slut |
|---|---|---|
| 0 | Förladdning (CC prechargeAmp) | V > prechargeVolt; prechargeMaxTime → fel MaxTime |
| 1 | Bulk (CC bulkAmp) | V ≥ absorb; bulkMaxTime → fel MaxTime; bulkAhPct → fel BulkAh (nr 36) |
| 2 | Absorption (CV absorb) | I < tail i absorbMinTime s, eller absorbMaxTime |
| 3 | Float (CV float) | floatMaxTime (0 = obegränsat) |
| 4 | Övervaka (utgång av, tillstånd Done) | Omstart till steg 1 när V < restartVolt (12,5 V-standard) och omstart är på |

- Tidsgränserna är i minuter och totalMaxTime gäller hela laddningen. Timern nollställs vid omstart från Övervaka.
- I Enkel-läge är steget alltid 0.
- *Besvarad 2026-10-25:* när bulkAhPct överskrids sätts en egen felbit, errorBits b12 `ErrBulkAh` (nr 36). MaxTime används bara för tidsgränserna.

## 32. Resursmätning (2026-10-25)
- `tools/check_size.sh` läser sektionerna (`avr-size -A`). `-B`-formatet i nyare binutils redovisar `.data` under text, och då underskattades RAM: master rapporterades som 90 B mot verkliga 182 B.
- avr-gcc 14.2.0 (lokalt): flash 7 238 → 18 296 B (gräns 28 672 B), statiskt RAM 182 → 866 B (gräns 1 229 B). Båda är **mätta** med avr-size.
- CI (Ubuntus gcc-avr): flash 18 440 B, statiskt RAM 864 B.
- Stack: **uppskattad** till 368 B [U] med `tools/stack_estimate.py`. Värsta kedjan är main → pollSerial → TunerStudio → onCommand → PageCommands::apply → AutoParams::compute, plus värsta ISR och 32 B schablon för biblioteket. Det lämnar cirka 800 B marginal av 2 048 − 866 = 1 182 B.
- Efter nr 35/36 (lokalt): flash 18 350 B och statiskt RAM 867 B (+54 B / +1 B), mätt med avr-size. Stacken uppskattas fortfarande till 368 B [U].
- Ingen stack-painting är gjord, eftersom hårdvara och simulator saknas. Det ingår i HIL-protokollet.

## 33. voltLimit-visning i tunerStudio_v2.2.ini (2026-10-25)
- I SmedbyCharger_v2.2.ini betyder MaxChargeVolt 0 "inget extra tak", och TunerStudio visade då voltLimit 15,00 V trots att utgången är av (beslut 24).
- Rättelse i den nya `tunerStudio_v2.2.ini`: kanalerna `voltLimitShown`/`pcVoltLimitShown` och mätaren `voltLimitNowG` visar 0 när MaxChargeVolt = 0, och en röd indikator visar att utgången är av. `eff_voltLimit` och `pa_voltLimit` är oförändrade, så `pa_diffCount` jämför som förut.
- Ini-filen genereras med `tools/make_ini22.py` från SmedbyCharger_v2.2.ini (Drive, md5 55ba5c2a…) och valideras i `make test` (`check_ini_layout.py` och `ref/validate_ini_v2.2.py`).
- Compat-filerna (`tunerStudio.ini`, `SmedbyCharger_compat.ini`) är oförändrade.

## 34. Inte implementerat i v2.2-etapperna (2026-10-25)
- Etapp 6 (givare: DS18B20, NTC, ingångsström). Uppskjuten enligt uppdraget.
- chargeMode 1 (manuella stegtabeller s1–s4) och E8.
- diagMode 1 (utgångsstyrning från TunerStudio). Diag-läget stänger bara av utgången.
- Sida 3: skydd och kalibrering utöver givarval och ampZeroAtBoot.
- Tillstånden SoftStart och PauseTemp, återförsök (retries), overAmpTime, derating och loopTimeUs.
- **Öppen fråga:** om MaxChargeVolt sätts lika med absorptionsspänningen ger hårdgränsen (V ≥ MaxChargeVolt → av) och CV-målet (= MaxChargeVolt) av/på-pendling. MaxChargeVolt bör ligga en bit över absorb, eller så görs gränsen strikt (>).

## 35. Indikator för avvisad B under laddning (2026-10-25)
Uppdrag från Ola 2026-10-25: avvisad B ska synas. Valet av plats gjordes vid implementeringen.

- **Plats:** realtime offset 6, `restrictionBits` (U16), **bit 8** (`TsLayout::RestrBurnRejected` = 0x0100). Enligt SPEC_tabeller används bara b0–b7, så b8–b15 är lediga.
- **Sätts** i `EepromStore::burn()` när B avvisas för att `chargingActive` är sann (`BurnRejectedCharging`). Getter: `EepromStore::burnRejectedCharging()`.
- **Nollas:**
  - när en senare B kommer förbi villkoren, alltså vid `BurnOk` eller `BurnErrorVerify` (då tar `eepromWarnBits` b6 BurnFailed över),
  - och vid omstart (flaggan ligger bara i RAM).
- **Nollas inte** när laddningen tar slut. Ändringarna i RAM är fortfarande osparade, så indikatorn fungerar som en påminnelse om att trycka B igen. Den blockerar inte start.
- `BurnRejectedNoGuard` och `BurnErrorNoBackend` lämnar flaggan orörd. NoBackend har egna varningsbitar (b6/b7), och NoGuard betyder att guard saknas, vilket är ett konfigurationsfel och inte en avvisning på grund av laddning.
- main.cpp lägger in biten efter `AutoParams::writeRealtime()` med `tsRtSetBurnRejected()`, som bara rör b8. Laddaren skriver b0–b7 som förut.
- tunerStudio_v2.2.ini har en ny gul indikator `{ restrictionBits & 256 }`, och `check_ini_layout.py` kontrollerar att masken stämmer med tsLayout.hpp.
- **Oförändrat:** layouten i nr 21 (`eepromWarnBits` offset 114, b0–b7), `ochBlockSize` 115 och alla offset.
- **Bortvalda platser:**

  | Plats | Skäl |
  |---|---|
  | `eepromWarnBits` | Alla 8 bitar är upptagna (nr 21). |
  | `errorBits` | Start kräver errorBits = 0, så biten skulle blockera laddning. Den skulle dessutom tolkas som laddarfel och kvitteras med E4. |
  | `pageRefreshReq` | Bitarna är knutna till TunerStudios triggeredPageRefresh per sida. |
  | `statusBits` | b0–b7 är upptagna. |
- Inte verifierat i TunerStudio (HIL 5.1).

## 36. Egen felbit för bulkAhPct (2026-10-25)
- När bulkAhPct överskrids i steg 1 sätts **errorBits b12** (`TsLayout::ErrBulkAh` = 0x1000) i stället för ErrMaxTime. Enligt SPEC används b0–b11, så b12 är ledig.
- Biten beter sig som övriga fel: den låses, laddaren går till Error-läge och E4 kvitterar.
- `ErrMaxTime` gäller nu bara prechargeMaxTime, bulkMaxTime och totalMaxTime.
- **Bitexakthet:** auto_params_v2.2.py definierar bara autoWarnBits och inga errorBits. Python-förlagan delar alltså ingen bit, och golden-testet påverkas inte (test_autoparams oförändrat).
- tunerStudio_v2.2.ini har en ny röd indikator `{ errorBits & 4096 }`. Mätaren errorBitsGauge har max 8191.
- Inte verifierat i TunerStudio.


## 37. Strömskalning i 32-bitars heltal och mättnadsbit b9 (2026-10-25)
Förslag 3 i docs/forslag-2-3-4-6.md, godkänt av Ola 2026-10-25 11:48 (via Max).

- `scaleAcs712Ma32(adc, refMv, offsetMv, mVperAmp)` i shared/scale.cpp räknar i int32 utan double. mV avrundas till närmaste heltal och mA trunkeras mot 0, som den gamla `(int)`-casten.
  - adc > 1023 behandlas som 1023, och mVperAmp ≤ 0 ger 0.
  - Mellanvärdena ryms i int32.
- `scaleAcs712MaSat(...)` klampar resultatet till ±32 767 mA **före** omvandlingen till int16_t och rapporterar om klampning skedde.
  - `Current::readCurrent()` returnerar fortfarande `int` och sparar flaggan, som läses med `Current::saturated()`.
- Den gamla `scaleAcs712Ma` (double, `(int)`-cast) är borttagen. Den hade odefinierat beteende över ±32 767 mA på AVR, och sCurrent var enda användaren.
- `zeroAmpCalibrate()` är oförändrad (double). Trunkering i stället för avrundning ger högst cirka 1 mV, vilket motsvarar cirka 15 mA för typ 3 [U].
- **restrictionBits b9** (`TsLayout::RestrCurrentSat` = 0x0200) visar att senaste strömmätningen var mättad.
  - Biten är levande, inte låst, och sätts i main.cpp med `tsRtSetRestr()` efter `writeRealtime`. Laddaren rör inte biten.
  - En mättad mätning ger ±3 276 cA, vilket alltid är ≥ MaxChargeAmp (högst 1 500). Därmed sätts ErrOverAmpOut och duty blir 0 samma varv.
- tools/make_ini22.py och tunerStudio_v2.2.ini har en gul indikator `{ restrictionBits & 512 }`. check_ini_layout kontrollerar masken.
- Testbygget kör `-fsanitize=float-cast-overflow` utöver ASan/UBSan.
  - Ny test/test_scale.cpp.
  - Mutationskontroll: den gamla double/int16-varianten fälls av sanitizern ("-41157 is outside the range of 'short int'").
- Inte verifierat på hårdvara (ADC, givare) eller i TunerStudio.

## 38. Regleringsmarginal och spärrtid efter hård brytning (2026-10-25)
Förslag 4 i docs/forslag-2-3-4-6.md, godkänt av Ola 2026-10-25 11:48 (via Max). Besvarar den öppna frågan i nr 34. Ändringen ligger i `Charger::update()` (shared/charger.cpp), och konstanterna ligger i charger.hpp bredvid `AbsMaxChargeAmp`.

- **Hårdgränsen är oförändrad:**
  - V ≥ MaxChargeVolt → ErrOverVoltOut.
  - I ≥ MaxChargeAmp → ErrOverAmpOut.
  - Utgången stängs av samma varv.
- **Regleringsmål:**
  - `vSet_` ≤ MCV − `VoltRegMarginCv` (10 cV).
  - `iSet_` ≤ MCA − `AmpRegMarginCa` (20 cA).
  - Subtraktionen är mättad (`subSat`): MCV < 10 cV eller MCA < 20 cA ger mål 0, alltså utgång av, utan underflöde.
  - Begränsningen görs varje varv under laddning, även under spärrtid, så att realtidens börvärden visar det reglerade målet.
  - restrictionBits b0/b5 (max utström/utspänning) betyder nu "målet är begränsat av MCA − 20 / MCV − 10".
- **Steggränser i Auto:**
  - absCv och floatCv begränsas också till MCV − 10 cV.
  - Bulk → absorption sker därför vid V ≥ min(absorb, MCV − 10). Annars skulle bulk aldrig nå gränsen när absorb = MCV, och laddningen skulle sluta med bulkMaxTime-fel.
  - Det är en följd av det godkända förslaget (testfall 1: "utgången når CV vid 14,60 V") och en precisering av nr 31 ("V ≥ absorb" = det reglerade absorb-målet).
- **Spärrtid och hysteres:**
  - Efter hård brytning hålls duty 0 i minst `HardCutHoldMs` = 1 000 ms. Därefter krävs V < MCV − 20 cV och I < MCA − 40 cA.
  - Ny medlem `holdMs_` (uint16_t), där 1 betyder "tiden ute, väntar på villkoret".
  - Spärren nollas **inte** av stopp eller start, av säkerhetsskäl. Ny start under spärr ger utgång av tills spärren släpper.
  - errorBits visar som förut det aktuella varvets fel, och latched samlar dem.
- **Dokumenterade avvikelser:**
  - Enkel-läget laddar till MCV − 0,10 V.
  - Auto reglerar till MCV − 0,10 V när absorb eller float ligger närmare MCV än så. `eff_*` och golden-testerna är oförändrade.
  - Ett batteri som vilar mellan MCV − 20 cV och MCV efter en hård brytning får ingen ny ström förrän spänningen sjunker.
  - Marginalen förhindrar bara pendling om ett reglersteg (±1 duty per 100 ms) ger mindre ändring än 10 cV respektive 20 cA. Det är inte verifierat på hårdvara [U]. Med värdmodellens 1 A per steg slår strömgränsen fortfarande till när ChargeAmp = 0, och testet använder därför 0,1 A per steg.
- **Tester** (test_charger):
  - Auto med MCV = absorb = 14,70 V i 1 h: 0 ErrOverVoltOut, och CV ligger mellan 14,55 och 14,65 V.
  - Enkel med MCV 14,40 V i 1 h: 0 brytvarv.
  - Enkel med ChargeAmp 0: iSet = MCA − 20.
  - Överspänning och överström: duty 0 samma varv, av mellan återstartsgränsen och målet, och exakt 10 varv av efter en brytning.
  - MCV 5/9 och MCA 10.
- **Mutationer som fälls:** utan marginal, utan spärrtid, utan hysteres, subSat utan klampning, och absCv inte kapad.
- Inte verifierat på hårdvara.

## 39. Strömgivartypen låst till boot-värdet (2026-10-25)
Förslag 6 i docs/forslag-2-3-4-6.md, godkänt av Ola 2026-10-25 11:48 (via Max). Fullföljer nr 29 (typen läses vid uppstart).

- **Låsning i main:**
  - main sparar `gBootSensorSel` (sida 3 byte 6 bit 4..5) direkt efter `gStore.load()`.
  - Samma värde styr `Current` (skalan) och alla omräkningar av AutoParams. Det gäller vid boot, efter sidskrivning och för E7–E9, via `PageCommands::autoInputBoot(page1, bootSensor, in)`. `hwCap` och strömmätningen hör därmed alltid ihop.
  - `autoInput()` (läser från sidan) finns kvar för tester. `compute()` och golden-matrisen är oförändrade: test_autoparams ger fortfarande 44137 kontroller och 0 fel.
- **Synligt:**
  - När sidans värde skiljer sig från boot-värdet sätter `Charger::update()` **restrictionBits b10** `RestrSensorRestart` (0x0400) varje varv.
  - Ini har en gul indikator: "STROMGIVARTYP ANDRAD: omstart (E5) kravs, start nekas".
  - Ingen layoutändring.
- **Start nekas:**
  - E1 och autostart i Väntar/Klar startar inte så länge typerna skiljer sig.
  - Timerstart väntar vid 0 kvar i stället för att starta, och startar först när typen stämmer.
  - En laddning som redan pågår avbryts **inte**. Den använder fortfarande boot-skalan, som stämmer med mätningen. Inte heller omstart från Övervaka till steg 1 påverkas.
  - E5 (omstart) är tillåten i vila som förut.
- **Charger::Pages:** ny medlem `bootSensor` som main fyller i. Värdsimuleringen fyller den vid konstruktion, vilket motsvarar boot.
- **Tester:**
  - test_charger:
    - Byte utan omstart: eff_bulkAmp är oförändrad, b10 är satt, E1 nekas och timern väntar.
    - Återställt värde: b10 nollas och E1 startar.
    - Simulerad omstart: b10 är 0 och bulk följer den nya typen.
  - test_pagecmd: sensorSel/autoInputBoot.
  - Mutationer som fälls: sensorOk alltid sant, ingen b10, start inte spärrad, timer inte spärrad.
- Inte verifierat på hårdvara eller i TunerStudio.

## 40. E8: effektiva värden → stegtabell s1–s4 (2026-10-25)
Förslag 2 i docs/forslag-2-3-4-6.md (alternativ A), godkänt av Ola 2026-10-25 11:48 (via Max). Ersätter "E8 gör ingenting" i nr 28/34.

**Mappning** (`PageCommands::copyEffectiveToSteps`, bara sida 2 byte 4..83):

| Steg | method | onEnd | Innehåll |
|---|---|---|---|
| s1 = bulk | CC | Nästa | current bulkAmp, volt absorb, endVoltMax absorb, maxTime bulkMaxTime, endAhPct bulkAhPct |
| s2 = absorption | CV | Nästa | current bulkAmp, volt absorb, endAmpMin tail, maxTime absorbMaxTime, minTime absorbMinTime |
| s3 = float | CV | Nästa | current floatAmp, volt float, maxTime floatMaxTime |
| s4 = övervaka | Monitor om restartMode (sida 2 byte 0 bit 2) är på, annars Av | Gå till steg 1 om Monitor, annars Stopp klar | endVoltMin restartVolt om Monitor |

- Alla övriga fält blir 0.
- tempMax/tempMin = chargeTempMax/Min i alla steg.
- **Förladdningen får ingen plats.** eff_totalMaxTime saknar fält.
- **Klampning** till ini-intervallen:
  - ström 0..3000 cA
  - spänning 0..6000 cV
  - maxTime 0..6000 min
  - minTime 0..65535 s
  - endAhPct 0..200
  - tempMax −40..80
  - tempMin −40..60
  - negativa värden → 0
- **Effektiva värden med override:**
  - `compute(page5, in, r)` utan ignoreOverrides, alltså `man_X` där `ovr_X` = Manuell, begränsat av hårdvarutaken.
  - Detta avvek från den ursprungliga kodskissen i förslaget, som anropade `compute(..., true)`. Skissen motsade förslagets text ("E8 kopierar **effektiva** värden ... override respekteras") och testfall 2. Text och test har följts, och Ola har bekräftat det (nr 41).
- **Spärr:** E8 ignoreras (returnerar 0, ingen ändring) när autoWarnBits har AH_MISSING eller NOMV_INVALID.
- **Ingen EEPROM:**
  - Bara RAM-kopian ändras, och ändringen sparas först vid B.
  - Byte 0–3 (startMode/restartMode/startDelayMin) rörs inte. Sida 5 (ovr, man_*) och chargeMode rörs inte.
  - E8 är tillåten under laddning, eftersom tabellen inte används av Auto eller Enkel.
- **pageRefreshReq:**
  - Returnerar 0x02.
  - main håller bitarna i 1 000 ms via `PageCommands::RefreshHold` (refreshSet/refreshTick), som ersätter de tidigare fälten i AppState och har samma beteende. Hjälparen är värdtestad, men inte main-flödet.
- **Definition för chargeMode 1** (inte implementerat, nr 34): maxTime och endAhPct ger alltid fel (ErrMaxTime/ErrBulkAh), och onEnd gäller bara U/I-villkoren. totalMaxTime ska fortsätta skydda. minTime-mappningen är ungefärlig [U].
- **Flash:** cirka +1,3 kB (copyEffectiveToSteps 578 B + writeStep 594 B), mätt med avr-nm. Förslagets uppskattning 250–350 B [U] var för låg. Det ligger fortfarande långt under gränsen (se rapport/PR).
- **Tester** (test_pagecmd):
  - standard → tabell + byte 0–3 och övriga sidor orörda + idempotent
  - manuell bulkAmp 25 A → 15 A i s1/s2
  - restartMode av/på
  - klampning
  - AH_MISSING och NOMV_INVALID
  - ingen EEPROM-skrivning
  - refresh-hållning inklusive millis-överslag
  - matris med 30 720 körningar, alla fält inom ini-intervall
- **Mutationer som fälls:** override ignoreras, ingen spärr vid AH_MISSING, ingen klampning, s4 alltid Av, fel basoffset, hållning `>` i stället för `>=`, temperatur inte klampad.
- Inte verifierat i TunerStudio (sidomläsning via triggeredPageRefresh) eller på hårdvara.

## 41. E8 tar med manuella override-värden (2026-10-25)
Ola bekräftade 2026-10-25 12:28 (via Max) att E8 ska kopiera de **effektiva** värdena **med** manuella override-värden, alltså `man_X` där `ovr_X` = Manuell, begränsade av hårdvarutaken. Det är så nr 40 är implementerat (`AutoParams::compute(page5, in, r)` utan ignoreOverrides).
- Kodskissen i docs/forslag-2-3-4-6.md är rättad så att den stämmer med implementationen.
- Ingen kodändring. Frågan är struken ur PR-beskrivningen.

## 42. Alla strömfält 0–15 A, E8-klampning 1500 cA (2026-10-25)
Olas beslut 2026-10-25 (punkt 2, via Max): alla strömfält begränsas till 0–15 A. Skälet är att hårdvarutaket är 15 A (MaxChargeAmp max 15 A, `AutoParams` begränsar alla effektiva strömmar till 15 A). Ini-fält som tillät 30 A gav värden som laddaren aldrig kan använda, och mätare som varnade först över taket. Underlag: JAMFORELSE_orig_vs_v2.2.md avsnitt 3.4 och D5. Granskas av CISO.

**Firmware** (`shared/pageCommands.cpp`):
- `AmpMaxCa` 3000 → **1500** cA. Gäller E8:s klampning av s1–s4 `current`, `endAmpMax` och `endAmpMin`.
- Inga andra gränser i firmware ändras. Spänningsklampningen i E8 är kvar på 6000 cV (60 V), se öppen fråga nedan.

**tunerStudio_v2.2.ini** (via tools/make_ini22.py, bara övre gräns `hi` ändras; typ, offset, skala och layout är oförändrade):

| Fält | Före | Efter |
|---|---|---|
| s1–s4 `_current`, `_endAmpMax`, `_endAmpMin` (12 fält, sida 2) | 0..30 A | 0..15 A |
| `man_bulkAmp`, `man_tailAmp`, `man_floatAmp`, `man_prechargeAmp` (sida 5) | 0..30 A | 0..15 A |
| `ampTimeLimit` (sida 4, tabell, inte aktiv) | 0..30 A | 0..15 A |
| `MaxInputAmp` (sida 1, inte aktiv, nr 34) | 0..30 A | 0..15 A |
| Oförändrade (redan 0..15): `ChargeAmp`, `MaxChargeAmp`, `autoAmpCap` | 0..15 A | 0..15 A |
| `tailAmpFloor` | 0..5 A | 0..5 A |

Mätare: `outAmpGauge`, `ampSetGauge`, `inAmpGauge` 0..30 → 0..20 A, varning 14 A, fara 15 A. Effektiva/PC-strömmätare och `ampTimeCurve` y-axel 0..30 → 0..15 A.

**Påverkan:**
- `AutoParams` och golden (44 137 kontroller) är oförändrade. De effektiva strömmarna är redan begränsade till 1500 cA, så klampningen i E8 ändrar bara utfallet för syntetiska värden över 15 A.
- Ingen ändring av EEPROM-layout, tsLayout eller standardvärden. Alla standardvärden ligger inom 0..15 A (störst: s1/s2_current och autoAmpCap 15,00 A).
- Ett värde över 15 A som redan finns i EEPROM (skrivet med äldre ini) ändras inte av firmware. TS visar det som utanför intervallet [U].
- `MaxInputAmp` räknas som strömfält och har fått 0..15 A. Fältet används inte av firmware (nr 34).
- Sensorernas mätområden i formeln `pa_sensMax` (5/20/30 A) är givarvärden och inte fältgränser. De är oförändrade.

**Tester:** `testE8Clamp` och `testE8Matrix` i test_pagecmd ändrade 3000 → 1500, och gränsvärdena 1499/1500/1501 cA är tillagda. Mutationskontroll: `AmpMaxCa` = 3000 och = 1501 fälls båda av testE8Clamp (3 fel). Matrisen fäller dem inte, eftersom AutoParams redan begränsar till 15 A.

**Öppen fråga:** utspänningsfälten i ini har nu 0..55 V (nr 43), men E8 klampar spänning till 6000 cV. Ett effektivt värde över 55 V (t.ex. 48 V bly, absorption 58,80 V) skrivs därför utanför ini-intervallet. Ändras inte utan Olas beslut.


## 43. Utspänningsfält 0–55 V, driverVolt 0–20 V (2026-10-25)
Olas beslut 2026-10-25 (punkt 4, via Max): MaxInputVolt 65 V, MaxChargeVolt 55 V, MOSFET 125 °C, utspänning 0..55 V och driverVolt 0..20 V enligt JAMFORELSE_orig_vs_v2.2.md avsnitt 3.4. Granskas av CISO tillsammans med nr 42.

**tunerStudio_v2.2.ini** (bara övre gräns `hi`; typ, offset, skala, layout och standardvärden oförändrade):

| Fält | Före | Efter |
|---|---|---|
| s1–s4 `_volt`, `_endVoltMax`, `_endVoltMin` (12 fält, sida 2) | 0..60 V | 0..55 V |
| `man_absorbVolt`, `man_floatVolt`, `man_restartVolt`, `man_prechargeVolt`, `man_voltLimit`, `man_batteryPresentVolt` (sida 5) | 0..60 V | 0..55 V |
| `driverVoltMin`, `driverVoltMax` (sida 3, inte aktiva) | 0..25 V | 0..20 V |
| Oförändrade (redan rätt): `MaxChargeVolt` 0..55, `MaxInputVolt` 0..65, `minInputVolt` 0..65, `MaxTemp` 0..125 °C | – | – |

**Inte ändrat:**
- Firmware. E8 klampar fortfarande spänning till 6000 cV (60 V), se öppen fråga i nr 42. Med MaxChargeVolt = 0 (utgång av) ger 48 V bly typ 0/3/4 absorption 58,80 V och voltLimit 60,00 V (kontrollerat med ref/auto_params_v2.2.py). E8 kan då skriva värden över 55 V i stegtabellen. Med MaxChargeVolt > 0 begränsas allt till MaxChargeVolt (≤ 55 V).
- Spänningsmätarna (effektiva/PC och outVolt) har kvar 0..60 V, eftersom effektiva värden kan vara upp till 60 V enligt ovan.
- AutoParams, golden (44 137) och EEPROM-layout. Alla standardvärden ligger inom de nya gränserna (störst 14,70 V).
- Ett värde över 55 V (eller 20 V) som redan finns i EEPROM ändras inte av firmware. TS visar det som utanför intervallet [U].

## 44. Presentation i tunerStudio_v2.2.ini: engelska, framsida, hjälp och menyer (2026-10-25)
Olas beslut 2026-10-25 (punkt 1, 3, 5, 6, 7, via Max). Originalets presentation tas över och förbättras enligt JAMFORELSE_orig_vs_v2.2.md. TunerStudios ini-spec är facit. Genereras av `tools/make_ini22.py` + `tools/ini22_ui.py`.

**Språk och kodning:**
- Alla etiketter, menyer, mätare, indikatorer, hjälptexter och loggrubriker är på engelska. Fält-, kanal- och dialognamn är oförändrade (engelska camelCase).
- Undantag: alternativen för `ovr_X` är kvar som "Auto"/"Manuell". Den vendorade `test/ref/validate_ini_v2.2.py` kräver exakt de strängarna, och den filen ändras inte. Hjälptexten förklarar "Manuell (manual)".
- En teckenkodning: ISO-8859-1. Filen innehåller bara ASCII plus grad-tecknet (0xB0). Generatorn kontrollerar detta med assert. Källans UTF-8-rester ("Â°", "–") är ersatta med "°" och "-".
- Enheter: C → °C, celler → cells, steg → steps, mV/C/cell → mV/°C/cell, "% kap" → %, "ggr"/"x"/"st" → tomt, accelerationDelay "x10 ms" → ms (skalan 10 fanns redan, nr 15), mosfetTempADCTable "ADC" → "ADC/4" (nr 16). Inga enheter i etiketter (TS visar enheten själv).

**Alternativtexter (bits):** översatta på samma position, så lagrade värden och .msq-index är oförändrade. `defaultValue` i ConstantsExtensions mappas automatiskt till samma position. Funktioner som inte finns i firmware har kvar sina alternativ (punkt 6) men märks "(not active)": chargeMode 1, diagMode 1, temperaturgivarna, tempCompEnable På. currentSensor 3 märks "(not supported)".

**Framsida (punkt 5):**
- 8 mätare: outVolt, outAmp, outPower, voltSet, inVolt, driverVolt, duty, stage.
- 12 indikatorer: utgång på, CC, CV, batteri anslutet, stegindikatorer 0–4 (0 = `stage == 0` och statusBits b7, 1–3 = tillstånd 3 Laddar, 4 = tillstånd 4 Klar, enligt nr 31), anyError, mcvOutputOff och eepromWarnBits > 0. De tre sista har röd bakgrund.
- Alla övriga indikatorer (alla errorBits- och restrictionBits-bitar, statusBits b3/b5/b6, autoWarnBits, eepromWarnBits per bit, lagrade fel, PC ≠ firmware, Auto/Enkel) är `indicatorTemplate` (spec kap. 16, högerklicksmenyn). Bitar som firmware aldrig sätter är märkta "(not active)".
- `test/check_ini_layout.py` räknar nu både `indicator` och `indicatorTemplate` när Err*/Restr*-bitarna jämförs mot tsLayout.hpp. Mutationskontroll: en borttagen errorBits-mall och en felaktig restrictionBits-bit fälls.
- Tolkning: specen beskriver `!` (röd) för fältetiketter i dialoger, inte för indikatorer. Varningsrader i dialoger har `!`, informationsrader `#`. På framsidan används röd på-färg för varningar [U].

**Hjälp, dialoger och menyer (punkt 7):**
- Hjälpblock med `<br>` mellan stycken: safetyHelp (ny, ersätter originalets "Warning"), accelerationDelayHelp (mjukstart, märkt inte aktiv), chargingStatesHelp (med cykeln och exemplet 9,00 A tills 14,40 V), autoHelp, protectionHelp, calibrationHelp och diagHelp. Texterna lovar inga funktioner som saknas i firmware. Utvecklarnoteringar (HEAD/offset) är borttagna ur SettingContextHelp, och hjälp finns för de 8 tabellerna.
- topicHelp finns på alla dialoger med fält eller knappar.
- dlgBasic heter "Charger max values (hardware)", har topicHelp accelerationDelayHelp och fältet minInputVolt.
- Ny dialog dlgBatteryLimits ("Battery limits") av befintliga fält: batteryType, batteryNomVolt, capacityAh, autoAmpCap, ovr/man voltLimit, chargeTempMin/Max och totalMaxTime med effektiva mätare, samt MaxChargeVolt/MaxChargeAmp. Inga nya konstanter.
- Menyer: &Charger, &Protection, &Driver, &Calibration, D&iagnostics. `menu = "Tools"` får std_realtime ("Realtime display"), `menu = "Help"` får en post per hjälpblock.
- Avvikelse: Help-posterna pekar på dialoger `dlgHelp_<namn>` (texten som fältrader plus topicHelp), inte direkt på help-namnen. Specen tillåter `subMenu = helpNamn`, men den vendorade validatorn godtar bara dialog/kurva/tabell/std_* som mål. Om direkt `subMenu = helpNamn` ska användas måste validatorn uppdateras av Ola [U].
- Alla dialognamn, villkor, kommandoknappar, kurvor, mätarnamn och loggkanaler är oförändrade. Generatorn kontrollerar detta med assert.

**Inte verifierat i TunerStudio:** grad-tecknet i ISO-8859-1, `<br>`, topicHelp mot help-namn, indicatorTemplate i högerklicksmenyn, std_realtime i Tools-menyn och tillägg till Help-menyn. Se docs/hil-testprotokoll.md.

## 45. 48 V stöds inte; tak 55 V ut och 65 V in (2026-10-25)
Olas beslut 2026-10-25 14:11 (via Max, uttryckligen godkänt med konsekvenserna): 48 V-stöd stryks. Taken 55 V utspänning och 65 V inspänning gäller. 48 V-batterier (24 celler bly, 16S LiFePO4) tas bort ur autoläget.

**Skäl:** utspänningsmätningen slutar vid 55 V, och det finns inget schema eller BOM som visar att hårdvaran klarar mer.

**En enda konstant:** `shared/hwLimits.hpp`, `HwLimits::HwVoltMaxCv = 5500` (55,00 V). Den används av:
- E8-klampningen (`VoltMaxCv` i pageCommands.cpp sänkt från 6000 till 5500) för s1–s4 volt/endVoltMax/endVoltMin.
- `PageCommands::clampHwLimits()`: efter varje godkänd W/X (main.cpp `onPageWritten`) och efter `load()` vid uppstart begränsas `MaxChargeVolt` (sida 1) och `man_absorbVolt`, `man_floatVolt`, `man_restartVolt`, `man_prechargeVolt`, `man_voltLimit`, `man_batteryPresentVolt` (sida 5) till 5500 i RAM. Ett U16-fält klampas bara om skrivningen täckte hela fältet. W skriver en byte åt gången (spec 3.3.7), och en halv skrivning ska inte ge ett blandat värde. Inget bränns automatiskt.
- Läsning: `PageCommands::maxChargeVoltCv()` begränsar `MaxChargeVolt` till 5500 där firmware läser det (Charger, `autoInput`, `autoInputBoot`). 0 betyder fortfarande att utgången är av.
- ini-max: `tools/ini22_ui.py` läser `hwLimits.hpp` och sätter `hi` för s1–s4- och man_*-spänningsfälten. Den kontrollerar också att `MaxChargeVolt` har samma värde (assert). `test/check_ini_layout.py` fäller om ini och hwLimits.hpp skiljer sig.
- Klampningen sker inte inne i `AutoParams::compute` (det skulle ändra golden för fall som inte gäller 48 V).

**Autoläget:** `NomvCount` 5 → 4, `kCells` = bly {3, 6, 12, 18} och LiFePO4 {2, 4, 8, 12}. `batteryNomVolt` index 4 (tidigare 48 V) ger nu `NOMV_INVALID` som index 5–7: alla man_*-värden används och E8 blockeras. Borttagna konfigurationer (värden från Drive-originalet, 100 Ah):

| Typ | Celler | Absorption | Float | Restart | voltLimit |
|---|---|---|---|---|---|
| 0 Bly SLI, 3 Bly djupcykel, 4 Bly kalcium/EFB | 24 | 58,80 V | 54,00 V | 50,00 V | 60,00 V |
| 1 Bly AGM | 24 | 57,60 V | 54,40 V | 50,00 V | 60,00 V |
| 2 Bly GEL | 24 | 57,00 V | 54,80 V | 50,00 V | 60,00 V |
| 5 LiFePO4 | 16 | 56,80 V | 54,00 V | 52,80 V | 58,40 V |

Kvar: 6/12/24/36 V. Högsta voltLimit är 45,00 V (bly 18 celler) [kontrolleras i selftest: alla ≤ 55,00 V].

**Python-förlagan och golden:** `test/ref/auto_params_v2.2.py` (repots kopia, md5 36477a1af53ac70082480ed36e2e9eb2) har NOMV/CELLS utan 48 V och två nya selftest-kontroller. Drive-originalet (md5 ef56d1755987754a3a591b8dd32f4c74) är inte ändrat. Ola behöver ladda upp repots kopia till Drive. Golden genereras från repots kopia (`test/gen_auto_golden.py`, ny REF_MD5). Fallen och slumpsekvensen är oförändrade, och antalet kontroller är fortfarande 44 137. Diff mellan gammal och ny golden:
- 844 rader i kGoldenAuto ändrade, alla med nomv = 4 och identisk indata.
- 876 rader i kGoldenComp ändrade, alla för fall med nomv = 4. Spänningen v härleds ur fallets effektiva värden.
- kGoldenMan och kGoldenCompSyn är oförändrade. Inga rader för andra fall är ändrade.

**ini:** `batteryNomVolt` alternativ 4 = "INVALID", `pa_typeOk` kräver `batteryNomVolt < 4` och 48 V-grenarna i `pa_cellCount` är borttagna. check_ini_layout jämför alternativen, `pa_typeOk` och `pa_cellCount` mot `NomvCount` och `kCells` i autoParams.cpp.

**Inte ändrat:** EEPROM-layout, format, offset och standardvärden (standard är 12 V). En lagrad 4:a i batteryNomVolt tolkas som ogiltig.

**Ersätter:** den öppna frågan i nr 42 och "Inte ändrat"-punkten om E8 och 60 V i nr 43. Dessa står kvar som historik.

**[U]:**
- Att RAM-klampningen efter W/X gör att TS och firmware har olika värde tills sidan läses om. ini-max hindrar normalt inmatning över 55 V.
- Om TS skriver U16 byte för byte med W. Då klampas inte fältet vid skrivning, men läsningen av MaxChargeVolt är begränsad och klampningen vid uppstart gäller.

## 46. driverVolt max 16 V (2026-10-25)
Olas beslut 2026-10-25 14:13 (via Max): driverVolt max sätts till 16 V. Ersätter driverVolt 0..20 V i nr 43.

- En konstant: `HwLimits::HwDriverVoltMaxDv = 160` (16,0 V, 0,1 V-steg) i `shared/hwLimits.hpp`.
- Firmware: `PageCommands::clampHwLimits()` begränsar `driverVoltMin` och `driverVoltMax` (sida 3, offset 4/5, nya konstanter `TsLayout::P3DriverVoltMin/Max`) till 160 efter varje W/X som täcker byten och efter `load()` vid uppstart. Bara i RAM, ingen burn. Fälten är U08, så även en W på en byte klampas.
- ini: `tools/ini22_ui.py` hämtar konstanten och sätter `hi` = 16 för båda fälten. Hjälptexten anger 0-16 V. `test/check_ini_layout.py` fäller om ini-max eller offset skiljer sig från firmware.
- Standardvärden oförändrade: `eepromDefaults` har redan driverVoltMin 100 (10,0 V) och driverVoltMax 160 (16,0 V), och ini-default är 16. Ingen ändring av EEPROM-layout, format eller golden.
- Inte ändrat: mätaren `driverVoltGauge` (0..20 V) och övriga mätarskalor. Fälten driverVoltMin/Max används fortfarande inte av firmware för någon felkontroll (inte aktiva).

## 47. Framsidans spänningsmätare 0–60 V ut och 0–70 V in (2026-10-25)
Olas beslut 2026-10-25 14:14 (via Max). Ersätter CISO:s öppna fråga 3 om mätarskalor. Fråga 2 (validerare) är fortfarande öppen.

| Mätare | Före (lo, hi, loDanger, loWarning, hiWarning, hiDanger) | Efter |
|---|---|---|
| `outVoltGauge` "Output voltage" | 0, 60, 0, 0, 55, 58 | 0, 60, 0, 0, 55, 55 (röd från 55 V) |
| `inVoltGauge` "Input voltage" | 0, 65, 9, 10, 60, 63 | 0, 70, 9, 10, 65, 65 (röd från 65 V) |

- hiWarning = hiDanger, så ingen gul zon. Det röda börjar exakt vid taket. Hur TS ritar en zon med samma start och slut är inte provat [U]. Lågzonen för inVolt (9/10 V) är oförändrad.
- Genereras i `tools/ini22_ui.py` (GLIM). Utmätarens röda gräns hämtas ur `HwVoltMaxCv` (nr 45). `test/check_ini_layout.py` kontrollerar skalorna och att den röda gränsen är lika med `HwVoltMaxCv` respektive `MaxInputVolt`-max (65 V).
- Inte ändrat: `voltSetGauge`, `driverVoltGauge`, de effektiva spänningsmätarna och övriga mätare. Ingen ändring av firmware, layout eller EEPROM.

## 48. Temperaturskydd saknas tills givare finns; WarnVoltClamped på framsidan (2026-10-25)
Olas beslut 2026-10-25 14:15 (via Max). Ola accepterar uttryckligen att temperaturskydd saknas tills givare finns (etapp 6). Kylning och `totalMaxTime` är det enda skyddet, och laddaren lämnas inte obevakad.

**125 °C** (`MaxTemp`, ini-max 125) anges som Tj-max, alltså MOSFET-gränsen. Det är INTE ett aktivt skydd: firmware läser ingen temperatur och stoppar inte på temperatur.

**ini (genereras av tools/ini22_ui.py), ändrade poster:**
- `tempCompEnable` (bits, sida 5): alternativ 1 heter "On - NOT ACTIVE, NO EFFECT (no battery temp sensor)" (tidigare "On (needs battery temp sensor, not active)"). Standardvärdet är fortfarande 1 (eepromDefaults oförändrad), och `defaultValue` följer med samma position. Dialogfältet i dlgAutoMainSet heter "Temperature compensation (not active, read-only)", har villkoret `{ 0 }` (alltid inaktivt i TS) och följs av en informationsrad om att funktionen saknar effekt.
- `MaxTemp`: fältet heter "MOSFET Tj max (reference only, not a protection)" i dlgBasic och "MOSFET Tj max (reference only)" i dlgProtection (under "Not active"). Hjälptexten är omskriven: Tj-max 125 °C, inte ett aktivt skydd, kylning och total laddtid är enda skyddet, lämna inte laddaren obevakad.
- safetyHelp: samma två stycken tillagda.
- Indikatorer (indicatorTemplate) vars av-text är märkt "(not active)" har nu också "(NOT ACTIVE)" i på-texten: statusBits 8 (derating), restrictionBits 2/4/8/16/128 och errorBits 8/16/32/64/128/256/1024/2048. Firmware sätter aldrig de bitarna.
- WarnVoltClamped (autoWarnBits b4): flyttad från indicatorTemplate (gul) till framsidan som 13:e `indicator`, röd, med texten "WARNING: CHARGE VOLTAGE CLAMPED (VOLTAGE LIMIT / MAX CHARGE VOLTAGE)". Av-text "Voltage not clamped". Avviker från nr 44 (12 indikatorer).

**Tester:** `test/check_ini_layout.py` kontrollerar: WarnVoltClamped-indikatorn (bit från autoParams.hpp, röd, etikett), att alla "(not active)"-indikatorer har märkt på-text, tempCompEnable (märkt alternativ, `{ 0 }` i alla dialoger) och MaxTemp-etiketterna. Mutationskontroll: gul färg, fel bit, omärkt på-text, villkoret borttaget, "On" omärkt, gammal MaxTemp-etikett och ändrad WarnVoltClamped i autoParams.hpp fälls alla.

**Inte ändrat:** EEPROM-layout, format, standardvärden, firmware, golden och fältnamn. Mätare, kurvor och dialoger för temperatur var redan märkta "(not active)".

**[U]:**
- Att TS gråmarkerar ett fält med `{ 0 }`. Det är inte provat i TunerStudio.
- Att ett befintligt "On" inte går att stänga av i TS när fältet är skrivskyddat. Det har ingen effekt i firmware, men ger autovarning b6 (gul, i högerklicksmenyn).
