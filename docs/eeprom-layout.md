# EEPROM-layout (TunerStudio v2.2)

Status: modulen finns och är testad på PC, men den är **inte inkopplad** i `main.cpp`. Den kopplas in tillsammans med v2.2-protokollet ([BESLUT.md](../BESLUT.md) nr 6). Dagens firmware (B-2) läser och skriver alltså fortfarande inget i EEPROM.

Källor: `SPEC_tabeller_v2.2.md`, `SPEC_v2.2.md` och `SmedbyCharger_v2.2.ini`. De ligger i LTT-Swedens Drive-mapp, inte i repot.

## Adresser (ATmega328P, 1024 B EEPROM)

| Adress | Storlek | Innehåll |
|---|---|---|
| 0–15 | 16 | **reserverat**, skrivs aldrig av modulen |
| 16 | 2 | magic `0x53 0x43` ("SC") |
| 18 | 1 | layoutversion = 1 |
| 19 | 1 | antal sidor = 5 |
| 20 | 2 | CRC16 över byte 16–19 |
| 22 | 38 + 2 | sida 1 + CRC16 |
| 62 | 84 + 2 | sida 2 + CRC16 |
| 148 | 38 + 2 | sida 3 + CRC16 (diagMode på adress 164) |
| 188 | 68 + 2 | sida 4 + CRC16 |
| 258 | 64 + 2 | sida 5 + CRC16 |
| 324–1023 | 700 | oanvänt (rörs inte) |

- Sidorna tar 292 B och hela layouten 308 B.
- Alla fält på flera byte och alla CRC-värden är little-endian.
- Sidindex 0–4 i koden motsvarar sida 1–5 och ini-kommandona `P\000`–`P\004`.
- CRC är CRC-16/CCITT-FALSE: polynom 0x1021, startvärde 0xFFFF, ingen reflektion, ingen slut-XOR. Kontrollvärde: `"123456789"` → `0x29B1`.

### Basadress 16 (BESLUT.md nr 20)

- **Adress 0 undviks.** Vid brownout under en EEPROM-skrivning är adress 0 den cell som oftast skadas, eftersom adressregistret EEAR nollställs vid reset. [U] Det bygger på erfarenhet från AVR-communityt, inte på databladet. Databladet (DS40002061B, avsnitt 8.4.2) säger att man ska hålla RESET aktiv vid för låg spänning, alltså använda BOD.
- **0–15 är reserverade** för framtida bruk, till exempel serienummer eller startflagga. Modulen läser och skriver dem aldrig, vilket testet `testReservedArea` kontrollerar.
- **Marginal:** 16 + 308 = 324 av 1024 B, så 700 B är fria för senare layouter.
- **Skrivslitage:** databladet anger 100 000 skriv-/raderingscykler per EEPROM-cell. `burn()` skriver bara ändrade byte.

## Regler

- **Laddning vid boot** (`EepromStore::load()`):
  - Om huvudet är ogiltigt (magic, version, antal sidor eller CRC) laddas standardvärden på alla sidor och `WarnHeaderInvalid` sätts. Det gäller tomt EEPROM, B-2 och andra layouter. Gammal B-2-data på adress 0–32 tolkas aldrig.
  - Om huvudet är giltigt kontrolleras varje sida för sig. En sida med fel CRC får standardvärden och `WarnPage<n>Crc` sätts. Övriga sidor laddas från EEPROM.
  - Ingen migrering av B-2:s sida 1 görs (BESLUT.md nr 14). Användaren får standardvärden plus en varningsbit.
  - `diagMode` (sida 3, byte 16, bit 0–1) sätts alltid till Normal.
  - `load()` skriver aldrig till EEPROM.
- **Skrivning** sker bara i `EepromStore::burn()`, alltså TunerStudio-kommandot B. Inget skrivs automatiskt.
  - B bränner alla sidor.
  - Bara ändrade byte skrivs (`eeprom_update_block`).
  - Sidorna skrivs först och huvudet sist.
  - `diagMode` lagras alltid som Normal.
  - Efter skrivningen läses allt tillbaka och CRC kontrolleras.
- Kopieringsknapparna (E\x07–E\x09) bränner inte. `MaxChargeVolt` 0 betyder att utgången är av.

### Burn-villkor (BESLUT.md nr 7)

`burn()` kräver en `BurnGuard` som sätts med `setBurnGuard()`. Den har tre funktioner:

- `chargingActive`: krävs.
- `outputOff`: krävs.
- `outputRestore`: valfri.

Ordningen är följande:

1. Om backend saknas returneras `BurnErrorNoBackend` och `WarnNoBackend | WarnBurnFailed` sätts.
2. Om guard saknas, eller saknar `chargingActive`/`outputOff`, returneras `BurnRejectedNoGuard`. Inget skrivs. Det är det säkra standardläget.
3. Om `chargingActive()` är sant returneras `BurnRejectedCharging`. Inget skrivs och utgången rörs inte.
4. `outputOff()` anropas, sedan skrivs och verifieras sidorna, och sist anropas `outputRestore()`. Återställningen sker alltid, även vid verifieringsfel.
5. Resultatet blir `BurnOk`, som nollar varningsbitarna, eller `BurnErrorVerify`, som sätter `WarnBurnFailed`.

Vid avvisning ändras varken EEPROM, utgången eller varningsbitarna. Testet `testBurnGuard` kontrollerar ordningen med en mock: ingen skrivning sker medan utgången är på.

En skrivning av hela layouten tar ungefär 308 × 3,3 ms ≈ 1 s. Det gäller bara när alla byte ändras. Siffran 3,3 ms per byte kommer från databladets tabell 8-2. Utgången är av under hela den tiden.

Inkoppling sker med v2.2-protokollet:

- `chargingActive` blir sant när laddning pågår, alltså när utgången är aktiv (`statusBits` b0).
- `outputOff` sätter duty till 0.
- `outputRestore` låter regulatorn starta mjukt igen.

Tolkningen av "laddning pågår" och omstartsbeteendet bestäms vid inkopplingen.

## Statusbyte i realtidsdata (BESLUT.md nr 8 och 21)

Realtidsblocket i `SPEC_tabeller_v2.2.md` är 114 B, och alla offset 0–113 är upptagna. Beslutet är en ny byte efter blocket, offset 114. Den finns bara som konstanter i `EepromRealtime` (`eepromStore.hpp`). Ini och protokoll är **inte** ändrade.

| Offset | Namn | Typ | Innehåll |
|---|---|---|---|
| 114 | `eepromWarnBits` | U08 | = `EepromStore::warnBits()` |

`ochBlockSize` blir 114 → 115.

Bitar:

| Bit | Mask | Namn | Betydelse |
|---|---|---|---|
| b0 | 0x01 | HeaderInvalid | tomt, okänt eller gammalt EEPROM: alla sidor har standardvärden |
| b1 | 0x02 | Page1Crc | sida 1 har standardvärden (CRC-fel) |
| b2 | 0x04 | Page2Crc | sida 2 har standardvärden |
| b3 | 0x08 | Page3Crc | sida 3 har standardvärden |
| b4 | 0x10 | Page4Crc | sida 4 har standardvärden |
| b5 | 0x20 | Page5Crc | sida 5 har standardvärden |
| b6 | 0x40 | BurnFailed | senaste B-verifiering misslyckades |
| b7 | 0x80 | NoBackend | ingen EEPROM-backend (internt fel) |

En lyckad B nollar alla bitar. En avvisad B (laddning pågår) ändrar dem inte. Om TunerStudio ska visa "B avvisad" kan det läggas till senare, till exempel som ett svar på B. Det är inte bestämt.

## Standardvärden för tabeller (BESLUT.md nr 22)

Ini-filen saknar `defaultValue` för tabellerna på sida 1 och 4. Med nollor skulle axlarna inte vara stigande och tabellerna vore oanvändbara. Värdena nedan är godkända (BESLUT.md nr 22). **NTC-tabellerna (mosfetTempADCTable/mosfetTempTable och batNtcADC/batNtcTemp) är platshållare**, eftersom kortet saknar NTC. De beräknas i `test/table_proposals.py` och kontrolleras byte för byte mot `eepromDefaults.cpp` via `golden_defaults.inc`.

| Fält | Sida/offset | Förslag | Skäl |
|---|---|---|---|
| mosfetTempADCTable | 1/14, U08[12], ADC/4 | 8, 12, 16, 21, 28, 38, 51, 67, 88, 128, 197, 233 | NTC 10 kΩ/B3950 mot GND och 10 kΩ pull-up [U], 10 bit >> 2 |
| mosfetTempTable | 1/26, S08[12], °C | 125, 110, 100, 90, 80, 70, 60, 50, 40, 25, 0, −20 | täcker deratering 70–120 °C |
| batNtcADC | 4/0, U16[12], 10 bit | 204, 270, 310, 355, 404, 456, 512, 570, 685, 789, 874, 935 | samma NTC-modell [U] |
| batNtcTemp | 4/24, S08[12], °C | 60, 50, 45, 40, 35, 30, 25, 20, 10, 0, −10, −20 | tätare punkter i laddfönstret 0–45 °C (WET-raden i auto_params: tMin 0, tMax 45) |
| derateTempBins | 4/36, S08[6], °C | 70, 80, 90, 100, 110, 120 | börjar vid derateStartTemp = 70 (sida 3) |
| deratePct | 4/42, U08[6], % | 100, 80, 60, 40, 20, 0 | linjärt till 0 % vid 120 °C [U] |
| ampTimeBins | 4/48, U16[5], s | 0, 60, 300, 900, 3600 | stigande axel 0 s – 1 h (ini-kurvans x-axel 0–3600) |
| ampTimeLimit | 4/58, U16[5], 0,01 A | 1500 × 5 (15,00 A) | = hårdvarutaket (HW_AMP_MAX_CA, MaxChargeAmp max 15 A), så det begränsar inte mer än i dag |

- Kortet har i dag ingen NTC. `TemperaturePin` PD7 är digital och DS18B20 är inte länkad. NTC-kurvorna är därför platshållare tills en givare och dess delare är bestämda.
- Ett 60 Ah våtbatteri ger i autoläget bulk 6,00 A (`auto_params_v2.2.py`). Tid-ström-skyddet hänger på laddarens hårdvara och inte på batteriet, därför används 15 A.
- `autoReserved` (sida 5) och skalära fält utan standardvärde är fortfarande 0, enligt specen.

## Brown-out (BOD) och säkringar

Repot innehåller **inga** säkringsvärden. Det finns inga fuse- eller avrdude-mål i `Makefile`, inget i CI, README eller Eclipse-inställningarna, och ingen `.fuse`-sektion i `charger.elf`. Den faktiska BOD-nivån på korten är därför **inte verifierad**.

| Uppgift | Värde | Status |
|---|---|---|
| Arduino Uno (Optiboot) lfuse/hfuse/efuse | 0xFF / 0xDE / 0xFD | läst i `ArduinoCore-avr/boards.txt` (uno), inte läst från något kort |
| efuse 0xFD → BODLEVEL 101 | 2,7 V typ (2,5–2,9 V) | avkodat mot databladets tabell 29-12 |
| hfuse 0xDE → BOOTSZ 11, BOOTRST på | 256 ord = 512 B boot | stämmer med beslutet Optiboot 512 B |
| hfuse 0xDE → EESAVE av | chip erase via ISP raderar EEPROM | databladet tabell 28-8. Uppladdning via Optiboot raderar inte EEPROM |
| 16 MHz säkert område | ≥ ca 3,8 V | linjärt 10 MHz @ 2,7 V → 20 MHz @ 4,5 V (avsnitt 29.3), uträknat |

- Med BOD 2,7 V och 16 MHz finns ett spänningsområde, ungefär 2,7–3,8 V, där CPU:n inte garanterat räknar rätt. Där kan ett felaktigt EEPROM-anrop i teorin ske. CRC och huvudet-sist skyddar mot att trasiga data *används*, men inte mot att de skrivs.
- **Beslut (BESLUT.md nr 23): BOD 4,3 V, efuse 0xFC** (BODLEVEL[2:0] = 100, tabell 29-12). lfuse och hfuse ändras inte. Ola sätter säkringen själv med ISP-programmerare. De exakta avrdude-kommandona för att läsa av och sätta säkringen, och vad som inte är verifierat (bland annat MCU:ns VCC), står i BESLUT.md nr 23.
- BurnGuard kompletterar BOD men ersätter den inte.

## MaxChargeVolt-standardvärde (FÖRSLAG, inte godkänt)

Standardvärdet är 0 = utgång av (`test_eeprom.cpp` kontrollerar det). Förslaget är att behålla det. Motivering och alternativ finns i BESLUT.md under "FÖRSLAG".

## Filer

| Fil | Innehåll |
|---|---|
| `shared/eepromLayout.hpp` | storlekar, adresser, offset, statiska kontroller och LE-hjälpare |
| `shared/eepromStore.hpp/.cpp` | load, burn (med BurnGuard) och gränskontrollerad RAM-åtkomst |
| `shared/eepromDefaults.cpp` | standardvärden (i flash via PROGMEM) |
| `shared/crc16.hpp/.cpp` | CRC16 |
| `SmeOS/Atmel328p/*/sEeprom.*` | AVR-backend (avr-libc) |
| `test/` | värdtester med mock-EEPROM och mock-guard, tabellförslag och AVR-storleksprov |

## Tester

```
make -C SimpleChargerBeta1/test             # värdtester (g++, -Wall -Wextra -Werror, ASan/UBSan)
make -C SimpleChargerBeta1/test avr-probe   # avr-size för modulen
make -C SimpleChargerBeta1/test golden INI=/sökväg/SmedbyCharger_v2.2.ini
```

`golden_defaults.inc` genereras ur ini-filen, och tabellförslagen kommer från `table_proposals.py`. Testet jämför filen byte för byte mot `eepromDefaults.cpp`.
