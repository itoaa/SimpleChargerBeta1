# HIL-testprotokoll: firmware v2.2 med TunerStudio (SimpleChargerBeta1)

Status: **ej genomfört.** Allt i v2.2 (etapp 2–5) är bara testat med värdtester på PC (`make -C SimpleChargerBeta1/test test`). Inget är verifierat på hårdvara eller i TunerStudio. Det här protokollet fylls i vid första provningen på bänk.

Firmware: grenen `feature/tunerstudio-v2.2`. Lokalt bygge (avr-gcc 14.2.0): `charger.hex` md5 `d16838da89f3485bae8ccc1d3cc72617` (efter BESLUT nr 35/36). CI-bygget (Ubuntus gcc-avr) ger en annan hex. Notera den hex som faktiskt flashas.
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
| 1.4 | Läs `resetCause` (realtime 66) efter strömpåslag | Noteras. Förväntat PORF (0x01) eller BORF (0x04). [U] Kan vara 0 om Optiboot har nollställt MCUSR (BESLUT nr 30). **Se avsnitt 10.1.** | |
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
| 5.1 | Ändra ett värde, starta laddning mot dummylast (steg 6.1) och tryck Burn under pågående laddning | B avvisas. Utgången påverkas inte (oscilloskop). Gul indikator "BURN AVVISAD" tänds (`restrictionBits` b8, BESLUT nr 35). `eepromWarnBits` oförändrad. | |
| 5.1a | Stoppa laddningen (E2) | Indikatorn lyser kvar, eftersom ändringen fortfarande är osparad. Laddning kan startas igen (biten blockerar inte). | |
| 5.1b | Tryck Burn i vila | Indikatorn släcks. `eepromWarnBits` = 0. Värdet finns kvar efter omstart. | |
| 5.1c | Upprepa 5.1, gör sedan omstart (E5 i vila eller strömavbrott) utan B | Indikatorn släckt efter omstart. Ändringen är borta (EEPROM oförändrad). | |
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
| 6.8 | E8 (Kopiera effektiva → stegtabell) i vila, standardvärden och kapacitet satt | Sida 2 s1 = CC/bulkAmp/absorb, s2 = CV/tail, s3 = CV/float, s4 = övervaka (restartMode på) eller av. Byte 0–3 och sida 5 oförändrade. `pageRefreshReq` bit 0x02 är 1 i ca 1 s och TS läser om sida 2. Ingen burn: värdena borta efter omstart utan B (BESLUT nr 40). | |
| 6.8a | E8 med en override (t.ex. ovr bulkAmp = Manuell, man 25 A) | s1/s2 ström = effektivt värde (begränsat av hårdvarutaket), inte autovärdet | |
| 6.8b | E8 med kapacitet 0 (AH_MISSING) | Ingen ändring på sida 2. Ingen pageRefreshReq-bit. | |
| 6.8c | E8 och sedan B i vila, omstart | Stegtabellen finns kvar | |
| 6.9 | E9 (alla → Auto) | ovr-bitarna = 0 och man_* oförändrade | |
| 6.10 | MaxChargeVolt = 0 | Utgången av och laddning kan inte startas. TS visar voltLimit 0 med röd indikator (BESLUT nr 33). | |
| 6.11 | Indikatorn "Max Ah i bulk" (`errorBits` b12, BESLUT nr 36) | Finns i TS och är släckt. Felet kräver Auto-laddning med batteri och testas inte här; det är bara värdtestat. | |
| 6.12 | Regleringsmarginal (BESLUT nr 38): Enkel-läge med dummylast, ChargeVolt = MaxChargeVolt | Utspänningen regleras mot MaxChargeVolt − 0,10 V. Ingen av/på-pendling: OverVoltOut blinkar inte och duty hoppar inte till 0. | |
| 6.12a | Samma med ström: ChargeAmp ≥ MaxChargeAmp | Strömmen regleras mot MaxChargeAmp − 0,20 A, utan hård brytning | |
| 6.12b | Spärrtid efter hård brytning: provocera överspänning (sänk MaxChargeVolt under utspänningen) | Duty 0 och felbit enligt 6.4. Efter kvittering och start: utgången hålls av i minst 1 s **och** tills U < MaxChargeVolt − 0,20 V (respektive I < MaxChargeAmp − 0,40 A). Mät med oscilloskop. | |
| 6.13 | Strömgivare typ 3 (30 A), `restrictionBits` b9 (BESLUT nr 37) | Indikatorn "STROMGIVARE MATTAD" släckt vid normal ström. Mättnad (>32,767 A) kräver extern strömkälla: utan den testas b9 inte, den är bara värdtestad. | |
| 6.14 | Byt `currentSensor` på sida 3 utan omstart (BESLUT nr 39) | Gul indikator "STROMGIVARTYP ANDRAD" (b10) tänds. E1 startar inte, och en väntande starttimer startar inte. outAmp skalas fortfarande med boot-typen. | |
| 6.14a | Samma under pågående laddning | Laddningen fortsätter med boot-typen. Efter stopp (E2) går den inte att starta. | |
| 6.14b | B och sedan E5 | b10 släckt. Ny typ används för skalning och autoparametrar. | |

## 7. pageRefresh (triggeredPageRefresh) – se även 10.2

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 7.1 | Ha sida 5-dialogen öppen och tryck E7 | pageRefreshReq bit 0x10 är 1 i ca 1 s, och TS läser om sida 5 automatiskt: fälten uppdateras utan att dialogen stängs | |
| 7.2 | Samma för E9 | Som 7.1 | |
| 7.3 | Om fälten inte uppdateras | Notera TS-version. Stäng och öppna dialogen (reservväg enligt ini). | |

## 8. Resurser och tidsåtgång

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 8.1 | Stack-painting: fyll RAM mellan `__heap_start` och SP med ett mönster vid uppstart (separat testbygge), kör 4.1, 6.1–6.9 (inkl. 6.8 E8) och 7.1, läs ut minsta orörda adress | Använd stack < 1 182 B. Statisk uppskattning 384 B [U] (BESLUT nr 32; längsta väg nu E8 via PageCommands::apply). | |
| 8.2 | Loopens tid med oscilloskop på gröna LED:n (låg under mätning och reglering) | Noteras. loopTimeUs i realtime är 0 (inte implementerat). | |
| 8.3 | A-svar (115 B vid 115 200 baud ≈ 10 ms) under laddning | Ingen märkbar störning i regleringen | |

## 9. Batteri (först när 0–8 är godkända)

Inte en del av det här protokollet. Batteri kopplas in först när spänningsgränserna är verifierade mot dummylast (plan avsnitt 6).

## 10. Öppna punkter som bara kan avgöras på bänk (Ola)

Båda är oprövade. Värdtesterna kan inte avgöra dem.

### 10.1 Optiboot och MCUSR (resetCause, realtime 66)

firmware läser `MCUSR & 0x0F` i `.init3` (BESLUT nr 30). [U] Optiboot kan läsa och nollställa MCUSR innan applikationen startar. Då visar resetCause 0 och orsaken går förlorad. Vissa Optiboot-versioner skickar i stället vidare MCUSR i register r2, men det är inte kontrollerat för den version som finns på kortet.

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 10.1.1 | Notera Optiboot-version (avrdude -v eller källa) | Noteras | |
| 10.1.2 | resetCause efter strömpåslag | PORF 0x01 (ev. med BORF 0x04) eller 0 = nollställd av Optiboot | |
| 10.1.3 | resetCause efter E5 (watchdog-omstart) | WDRF 0x08 eller 0 | |
| 10.1.4 | resetCause efter tryck på reset (extern) | EXTRF 0x02 eller 0 | |
| 10.1.5 | Om allt ovan ger 0 | Optiboot nollställer MCUSR. Rapportera. Möjlig åtgärd (inte gjord): läs r2 från Optiboot. | |

### 10.2 triggeredPageRefresh (pageRefreshReq, realtime 113)

Firmware håller biten i cirka 1 s (`RefreshHoldMs` 1000) efter E7/E9. Det är okänt om din TunerStudio-version läser om sidan direkt via `[EventTriggers] triggeredPageRefresh` (SPEC_v2.2 avsnitt 3, "ärligt förbehåll").

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 10.2.1 | Notera TunerStudio-version och utgåva | Noteras | |
| 10.2.2 | Kör 7.1 och 7.2 | Fälten uppdateras utan att dialogen stängs | |
| 10.2.3 | Logga `pageRefreshReq` i TS-loggern under E7 | Bit 0x10 är 1 i ungefär 1 s och sedan 0, och den sitter inte kvar | |
| 10.2.4 | Om ingen omläsning sker | Reserv: stäng och öppna dialogen, eller Ctrl+R. Tryck **inte** Burn innan TS visar de nya värdena, annars skrivs de gamla tillbaka. | |

### 10.3 Presentation i tunerStudio_v2.2.ini (BESLUT nr 42–44)

Kan kontrolleras i TunerStudio utan laddning (offline-projekt räcker för 10.3.1–10.3.8). Inget av detta är provat.

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 10.3.1 | Öppna projektet med tunerStudio_v2.2.ini. Titta på enheter med grad-tecken (t.ex. Charge temp min, MaxTemp) | "°C" visas, inte "Â°" eller "?" (filen är ISO-8859-1) [U] | |
| 10.3.2 | Menyraden | Menyerna Charger, Protection, Driver, Calibration och Diagnostics finns. Tools har "Realtime display" (std_realtime). Help har 7 poster tillagda efter TS egna | |
| 10.3.3 | Help → varje post | Dialogen visar texten. Knappen/länken för hjälp (topicHelp) öppnar help-blocket med radbrytningar (`<br>`) [U] | |
| 10.3.4 | Hjälpknappen i Charger max values, Auto mode, Stage 1–4, Protection, Sensors, Diagnostics | Rätt help-block öppnas [U: topicHelp mot help-namn är inte dokumenterat i specen] | |
| 10.3.5 | Framsidan | 8 mätare och 12 indikatorer. Högerklick på en indikator listar de övriga (indicatorTemplate) [U] | |
| 10.3.6 | Strömfält, t.ex. Stage 1 current och Manual value för bulk current | Går inte att mata in över 15,00 A | |
| 10.3.7 | Spänningsfält, t.ex. Stage 1 voltage, och driverVoltMax | Över 55,00 V respektive 20,00 V går inte att mata in | |
| 10.3.8 | Ladda en .msq eller EEPROM med äldre värden över de nya gränserna (t.ex. 20 A eller 58 V) | Notera hur TS visar och hanterar värdet. Strömvärden ändras inte av firmware. MaxChargeVolt och man_*-spänningar över 55,00 V begränsas till 55,00 V i RAM vid uppstart och vid hel skrivning (BESLUT nr 45), se 10.4.2 [U] | |
| 10.3.9 | Med kort: E8 (Copy effective values to stage table) med 36 V bly typ 0 och MaxChargeVolt = 0 | Inga spänningar över 55,00 V i stegtabellen (E8 klampar till `HwVoltMaxCv`, BESLUT nr 45) | |
| 10.3.10 | Battery limits | Dialogen visar fälten och de effektiva mätarna. Ändring i Battery limits syns i Auto mode-dialogerna (samma konstanter) | |

### 10.4 Spänningstak 55 V ut / 65 V in (BESLUT nr 45)

Inget av detta är provat.

| # | Steg | Förväntat | Resultat |
|---|---|---|---|
| 10.4.1 | Auto mode → Nominal voltage | Bara 6, 12, 24 och 36 V är giltiga, index 4–7 visas som INVALID. Väljs INVALID: autovarning "INVALID NOMINAL VOLTAGE" och E8 gör inget | |
| 10.4.2 | Med kort, utan batteri: skriv MaxChargeVolt = 55,00 V och läs tillbaka. Upprepa med en .msq som har 58,00 V | 55,00 V ligger kvar. 58,00 V läses tillbaka som 55,00 V efter skrivning eller omstart. Notera om TS rapporterar att sidan skiljer sig [U] | |
| 10.4.3 | Med kort, dummylast: MaxChargeVolt 55,00 V, mät utspänningen med extern multimeter | Utspänningen går inte över 55 V (ErrOverVoltOut vid ≥ MaxChargeVolt). Kräver att mätkretsen faktiskt når 55 V [U] | |

## Ej testbart i v2.2

Temperaturer (visas som 0 = ej tillgänglig), ingångsström, temperaturkompensering, SoftStart/PauseTemp och diagMode 1. Att stegtabellen *används* (chargeMode 1) är inte implementerat, men E8 fyller den (6.8). Se BESLUT nr 30, 34 och 40.
