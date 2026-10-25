# HIL-testprotokoll: firmware v2.2 med TunerStudio (SimpleChargerBeta1)

Status: **ej genomfört.** Allt i v2.2 (etapp 2–5) är bara testat med värdtester på PC (`make -C SimpleChargerBeta1/test test`). Inget är verifierat på hårdvara eller i TunerStudio. Det här protokollet fylls i vid första provningen på bänk.

Firmware: grenen `feature/tunerstudio-v2.2`. Lokalt bygge (avr-gcc 14.2.0): `charger.hex` md5 `51ab51b96ce79729d0aaf7008ae8a826`. CI-bygget (Ubuntus gcc-avr) ger en annan hex. Notera den hex som faktiskt flashas.
Ini: `SimpleChargerBeta1/tunerStudio_v2.2.ini` (signatur `"MSII s-Charger 2.2  "`, 115 200 baud enligt `Hardware.hpp`, SmedbyCharger11).

## 0. Säkerhet och förutsättningar (före allt annat)

| # | Kontroll | Förväntat | Resultat | Sign/datum |
|---|---|---|---|---|
| 0.1 | **Inget batteri anslutet** under steg 1–8. Utgången belastas med en dummylast (motstånd dimensionerat för maxström) och matas från ett nätaggregat med strömbegränsning. | Uppkopplingen dokumenterad (last, aggregat, gräns) | | |
| 0.2 | BOD-säkring (BESLUT nr 23): läs lfuse/hfuse/efuse enligt BESLUT nr 23 och sätt efuse 0xFC bara om lfuse = 0xFF och hfuse = 0xDE. Använd inte `-e`. | efuse bit 2:0 = 100 (läses som 0xFC eller 0x04 beroende på avrdude) | | |
| 0.3 | MCU:ns VCC mätt med oscilloskop vid tomgång, vid lastsprång och när PWM startar | VCC > 4,5 V hela tiden (max VBOT för BOD 4,3 V), inklusive rippel | | |
| 0.4 | Lock-bitar avlästa | Noteras (förväntat 0x0F för Uno/Optiboot, inte verifierat) | | |
| 0.5 | Gate-drivning för PWM-utgången kontrollerad enligt schema (plan fråga 14) | Noteras | | |

Avbryt om 0.2 eller 0.3 inte är uppfyllt.

## 1. Uppstart och signatur

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 1.1 | Flasha via Optiboot och starta. Tom eller gammal EEPROM (B-2). | Utgången av (duty 0, PWM-pinne låg). Ingen laddning startar. | |
| 1.2 | Anslut TS med `tunerStudio_v2.2.ini` | TS accepterar signaturen utan varning om fel firmware. F ger `002`. | |
| 1.3 | Läs `eepromWarnBits` (realtime 114) | b0 = 1 (huvud ogiltigt, standardvärden) vid tom EEPROM | |
| 1.4 | Läs `resetCause` (realtime 66) | Noteras. [U] kan vara 0 om Optiboot nollställt MCUSR (BESLUT nr 30). | |
| 1.5 | Läs `pwmFreqNow` | 31 (beräknat värde). Mät frekvensen på PWM-pinnen med oscilloskop när duty > 0 (steg 6): förväntat ca 31,4 kHz. | |

## 2. Sidbyte och läsning

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 2.1 | Öppna dialoger för sida 1–5 i tur och ordning | Alla sidor läses utan timeout (P 0–4, pageActivationDelay 10 ms) | |
| 2.2 | Jämför värden mot standardvärdena i `eepromDefaults.cpp` | Lika | |
| 2.3 | Byt sida snabbt fram och tillbaka 10 gånger | Inga felaktiga värden, inga kommunikationsfel | |

## 3. Skrivning (W och X i block)

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 3.1 | Ändra ett enskilt värde på sida 1 | Värdet läses tillbaka lika. Utgången påverkas inte. | |
| 3.2 | Ändra många värden på sida 2 (84 B, blockingFactor 64 ger X-block) | Alla värden lika efter återläsning | |
| 3.3 | Ändra ett värde på sida 5 som påverkar autoparametrarna (t.ex. kapacitet) | eff_*-kanalerna uppdateras inom ca 100 ms | |
| 3.4 | Stäng av och starta utan B | Ändringarna är borta (ingen automatisk skrivning) | |

## 4. Burn och omstart

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 4.1 | Laddaren i vila. Ändra värden på alla sidor, tryck Burn. | PWM-utgången förblir 0 under skrivningen (oscilloskop). eepromWarnBits = 0 efter skrivningen. | |
| 4.2 | Bryt matningen och starta igen | Värdena från 4.1 finns kvar. eepromWarnBits = 0. diagMode = Normal. Laddaren startar AV. | |
| 4.3 | E5 (omstart) i vila | Omstart via watchdog. Värdena kvar. resetCause noteras. | |
| 4.4 | Bryt matningen **under** en burn (om möjligt, med dummylast) och starta igen | Antingen gamla eller nya värden per sida, aldrig blandat. Vid CRC-fel standardvärden och bit 1–5 satt. | |

## 5. BurnGuard: avvisning under laddning

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 5.1 | Starta laddning mot dummylast (steg 6.1) och tryck Burn under pågående laddning | B avvisas: EEPROM oförändrad (kontrollera efter omstart), utgången påverkas inte. **Ingen återkoppling i TS** (öppen fråga, BESLUT nr 26). | |
| 5.2 | E5 och E6 under laddning | Ignoreras (BESLUT nr 25/28) | |

## 6. E-kommandon E1–E9 (dummylast, inget batteri)

Inställning: MaxChargeVolt > 0 och lagom låg för dummylasten, chargeMode Enkel (Auto kräver batteridetektering), lågt MaxChargeAmp.

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 6.1 | E1 (Start) | Laddning startar och duty stiger ett steg per 100 ms mot gränsen. CC/CV-bitar rimliga. | |
| 6.2 | E3 (nollställ räknare) under laddning | Ah, Wh och laddtid = 0 | |
| 6.3 | E2 (Stopp) | Duty 0 inom ett varv (100 ms) | |
| 6.4 | Provocera ett fel (t.ex. sänk MaxChargeVolt under utspänningen) | Utgången av och felbit satt. Error-läge om felet är låsande. | |
| 6.5 | E1 i Error-läge, sedan E4 (kvittera) | E1 ignoreras. Efter E4: Waiting **utan** att laddning startar (BESLUT nr 27). | |
| 6.6 | E6 (nollställ strömgivare) i vila | outAmp ≈ 0 efteråt | |
| 6.7 | E7 (auto → manuell) | man_* på sida 5 = autovärden. ovr-bitar oförändrade. Ingen burn (kontrollera efter omstart). | |
| 6.8 | E8 | Ingen ändring (inte implementerad, BESLUT nr 28) | |
| 6.9 | E9 (alla → Auto) | ovr-bitarna = 0 och man_* oförändrade | |
| 6.10 | MaxChargeVolt = 0 | Utgången av och laddning kan inte startas. TS visar voltLimit 0 med röd indikator (BESLUT nr 33). | |

## 7. pageRefresh (triggeredPageRefresh)

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 7.1 | Ha sida 5-dialogen öppen och tryck E7 | pageRefreshReq bit 0x10 är 1 i ca 1 s, och TS läser om sida 5 automatiskt: fälten uppdateras utan att dialogen stängs | |
| 7.2 | Samma för E9 | Som 7.1 | |
| 7.3 | Om fälten inte uppdateras | Notera TS-version. Stäng och öppna dialogen (reservväg enligt ini). | |

## 8. Resurser och tidsåtgång

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 8.1 | Stack-painting: fyll RAM mellan `__heap_start` och SP med ett mönster vid uppstart (separat testbygge), kör 4.1, 6.1–6.9 och 7.1, läs ut minsta orörda adress | Använd stack < 1 182 B. Statisk uppskattning 368 B [U] (BESLUT nr 32). | |
| 8.2 | Loopens tid med oscilloskop på gröna LED:n (låg under mätning och reglering) | Noteras. loopTimeUs i realtime är 0 (inte implementerat). | |
| 8.3 | A-svar (115 B vid 115 200 baud ≈ 10 ms) under laddning | Ingen märkbar störning i regleringen | |

## 9. Batteri (först när 0–8 är godkända)

Inte en del av det här protokollet. Batteri kopplas in först när spänningsgränserna är verifierade mot dummylast (plan avsnitt 6).

## Ej testbart i v2.2

Temperaturer (visas som 0 = ej tillgänglig), ingångsström, temperaturkompensering, SoftStart/PauseTemp, diagMode 1 och E8. Se BESLUT nr 30 och 34.
