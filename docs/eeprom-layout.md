# EEPROM-layout (TunerStudio v2.2)

Status: modulen finns och är testad på PC, men den är **inte inkopplad** i `main.cpp`.
Dagens firmware (B-2) läser och skriver alltså fortfarande inget i EEPROM.
Se PR-beskrivningen för skälen.

Källor: `SPEC_tabeller_v2.2.md`, `SPEC_v2.2.md` och `SmedbyCharger_v2.2.ini`. De ligger i LTT-Swedens Drive-mapp, inte i repot.

## Adresser (ATmega328P, 1024 B EEPROM)

| Adress | Storlek | Innehåll |
|---|---|---|
| 0 | 2 | magic `0x53 0x43` ("SC") |
| 2 | 1 | layoutversion = 1 |
| 3 | 1 | antal sidor = 5 |
| 4 | 2 | CRC16 över byte 0–3 |
| 6 | 38 + 2 | sida 1 + CRC16 |
| 46 | 84 + 2 | sida 2 + CRC16 |
| 132 | 38 + 2 | sida 3 + CRC16 |
| 172 | 68 + 2 | sida 4 + CRC16 |
| 242 | 64 + 2 | sida 5 + CRC16 |
| 308–1023 | 716 | oanvänt (rörs inte) |

- Sidorna tar 292 B och hela layouten 308 B.
- Alla fält på flera byte och alla CRC-värden är little-endian.
- Sidindex 0–4 i koden motsvarar sida 1–5 och ini-kommandona `P\000`–`P\004`.
- CRC är CRC-16/CCITT-FALSE: polynom 0x1021, startvärde 0xFFFF, ingen reflektion, ingen slut-XOR. Kontrollvärde: `"123456789"` → `0x29B1`.

## Regler

- **Laddning vid boot** (`EepromStore::load()`):
  - Om huvudet är ogiltigt (magic, version, antal sidor eller CRC) laddas standardvärden på alla sidor och `WarnHeaderInvalid` sätts. Det gäller tomt EEPROM, B-2 och andra layouter. En gammal 33 B-sida 1 tolkas aldrig.
  - Om huvudet är giltigt kontrolleras varje sida för sig. En sida med fel CRC får standardvärden och `WarnPage<n>Crc` sätts. Övriga sidor laddas från EEPROM.
  - `diagMode` (sida 3, byte 16, bit 0–1) sätts alltid till Normal.
  - `load()` skriver aldrig till EEPROM.
- **Skrivning** sker bara i `EepromStore::burn()`, alltså TunerStudio-kommandot B när modulen kopplas in. Inget skrivs automatiskt.
  - Skrivningen använder `eeprom_update_block`, så bara ändrade byte skrivs.
  - Sidorna skrivs först och huvudet sist.
  - `diagMode` lagras alltid som Normal.
  - Efter skrivningen läses allt tillbaka och CRC kontrolleras. Vid fel sätts `WarnBurnFailed`.
- Kopieringsknapparna (E\x07–E\x09) bränner inte (Olas beslut). `MaxChargeVolt` 0 betyder att utgången är av.
- Fält som saknar standardvärde i spec och ini är 0. Det gäller tabellerna på sida 1 och 4.

## Filer

| Fil | Innehåll |
|---|---|
| `shared/eepromLayout.hpp` | storlekar, adresser, offset, statiska kontroller och LE-hjälpare |
| `shared/eepromStore.hpp/.cpp` | load, burn och gränskontrollerad RAM-åtkomst |
| `shared/eepromDefaults.cpp` | standardvärden (i flash via PROGMEM) |
| `shared/crc16.hpp/.cpp` | CRC16 |
| `SmeOS/Atmel328p/*/sEeprom.*` | AVR-backend (avr-libc) |
| `test/` | värdtester med mock-EEPROM och AVR-storleksprov |

## Tester

```
make -C SimpleChargerBeta1/test             # värdtester (g++, -Wall -Wextra -Werror, ASan/UBSan)
make -C SimpleChargerBeta1/test avr-probe   # avr-size för modulen
make -C SimpleChargerBeta1/test golden INI=/sökväg/SmedbyCharger_v2.2.ini
```

`golden_defaults.inc` genereras direkt ur ini-filen. Testet jämför den byte för byte mot `eepromDefaults.cpp`.
