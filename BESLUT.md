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
