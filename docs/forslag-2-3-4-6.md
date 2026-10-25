# Förslag 2, 3, 4 och 6 (godkända och implementerade)

Status: **godkänt och implementerat.** Ola godkände förslag 2, 3, 4 och 6 som de står (via Max, 2026-10-25 11:48). Implementering: förslag 3 = BESLUT nr 37, förslag 4 = nr 38, förslag 6 = nr 39, förslag 2 = nr 40. Texten nedan är förslaget som det godkändes. Avvikelser vid implementeringen:

- **Förslag 2:** den ursprungliga kodskissen anropade `compute(..., true)`, alltså ignorerade overrides, medan texten och testfall 2 kräver effektiva värden med override. Implementerat utan ignoreOverrides enligt texten. Ola bekräftade 2026-10-25 12:28 att E8 ska ta med manuella override-värden (BESLUT nr 41). Kodskissen nedan är rättad så att den stämmer med implementationen. Flash ökade med cirka 1,3 kB mot uppskattade 250–350 B [U].
- **Förslag 4:**
  - Bulk-slutgränsen blir min(absorb, MCV − 0,10 V), annars når Auto aldrig bulk-slut när MCV = absorption.
  - Spärrtiden nollas inte av stopp eller start.
  - Marginalen hjälper bara om ett regleringssteg är mindre än marginalen [U].
- **Förslag 6:**
  - En pågående laddning avbryts inte, och omstart från övervakning nekas.
  - Starttimern stannar på 0 tills omstart.

Numreringen följer Olas uppdrag 2026-10-25. [U] betyder uppskattning eller antagande som inte är verifierat. Inget här är provat på hårdvara eller i TunerStudio.

Källor:
- `SPEC_v2.2.md` och `SPEC_tabeller_v2.2.md` (Drive)
- `tunerStudio_v2.2.ini`
- `shared/charger.cpp`, `shared/pageCommands.cpp`, `shared/scale.cpp`, `SmeOS/Middleware/src/sCurrent.cpp` och `main.cpp` på grenen

---

## 2. E8: effektiva värden → stegtabell s1–s4 (sida 2)

### Nuläge
- E8 gör ingenting (`PageCommands::apply`, BESLUT nr 28).
- SPEC kräver bara: "Efter E\x08: skriv sida 2 i RAM och sätt `pageRefreshReq |= 0x02`", håll biten cirka 1 s och bränn inte (SPEC_v2.2 avsnitt 3).
- Mappningen mellan fälten är inte specificerad.

### Sida 2-layout
Varje steg är 20 B. Steg n börjar på offset 4 + 20·(n−1), alltså s1 = 4, s2 = 24, s3 = 44, s4 = 64.

| +off | Fält | Typ/enhet |
|---|---|---|
| 0 | method [0:1] (0 CC, 1 CV, 2 Monitor, 3 Av), onEnd [2:3] (0 Nästa, 1 Gå till steg 1, 2 Stopp klar, 3 Stopp med fel) | bits |
| 1 | tempMax | S08 °C, −40..80 |
| 2 | current | U16 cA, 0..3000 |
| 4 | volt | U16 cV, 0..6000 |
| 6 | endVoltMax (U ≥, 0 = av) | U16 cV |
| 8 | endVoltMin (U ≤, 0 = av; Monitor → omstart) | U16 cV |
| 10 | endAmpMax (I ≥) | U16 cA |
| 12 | endAmpMin (I ≤) | U16 cA |
| 14 | maxTime (0 = obegränsad) | U16 min, 0..6000 |
| 16 | minTime | U16 s |
| 18 | tempMin | S08 °C, −40..60 |
| 19 | endAhPct (0 = av) | U08 %, 0..200 |

Byte 0–3 på sidan 2 (startMode/restartMode och startDelayMin) rörs **inte** av E8.

### Föreslagen mappning (alternativ A, rekommenderas)
Auto har fem steg (0 förladdning, 1 bulk, 2 absorption, 3 float, 4 övervaka), men sidan har bara fyra platser. Förslaget följer SPEC:ens standardvärden för sida 2 (s1 CC 15 A/14,4 V, s2 CV med slutström 1 A, s3 CV 13,6 V, s4 Av/Stopp klar), alltså bulk → absorption → float → övervaka. **Förladdningen (steg 0) får ingen plats** och följer inte med.

| Fält | s1 = Bulk | s2 = Absorption | s3 = Float | s4 = Övervaka |
|---|---|---|---|---|
| method | 0 CC | 1 CV | 1 CV | 2 Monitor om restartMode = på, annars 3 Av |
| onEnd | 0 Nästa | 0 Nästa | 0 Nästa | 1 Gå till steg 1 (om Monitor), annars 2 Stopp klar |
| current | eff_bulkAmp | eff_bulkAmp (CV-strömtak) | eff_floatAmp | 0 |
| volt | eff_absorbVolt (CC-spänningstak) | eff_absorbVolt | eff_floatVolt | 0 |
| endVoltMax | eff_absorbVolt | 0 | 0 | 0 |
| endVoltMin | 0 | 0 | 0 | eff_restartVolt |
| endAmpMax | 0 | 0 | 0 | 0 |
| endAmpMin | 0 | eff_tailAmp | 0 | 0 |
| maxTime | eff_bulkMaxTime | eff_absorbMaxTime | eff_floatMaxTime | 0 |
| minTime | 0 | eff_absorbMinTime | 0 | 0 |
| endAhPct | min(eff_bulkAhPct, 200) | 0 | 0 | 0 |
| tempMax / tempMin | eff_chargeTempMax / eff_chargeTempMin, begränsade till −40..80 respektive −40..60 | samma | samma | samma |

Enheterna är desamma på båda sidor (cA, cV, min, s, %), så ingen omräkning behövs. Bara klampning till fältens intervall.

### Det som går förlorat (måste godkännas)
1. **Förladdningen** (eff_prechargeAmp/Volt/MaxTime) följer inte med. Alternativ B är s1 = förladdning, s2 = bulk, s3 = absorption och s4 = float, men då försvinner övervakning och omstart i stället.
2. **onEnd gäller alla avslutsvillkor i steget.** I Auto ger bulkMaxTime och bulkAhPct *fel* (ErrMaxTime/ErrBulkAh), medan U ≥ absorb ger *nästa steg*. Ett enda onEnd-fält kan inte uttrycka båda. Förslag: i chargeMode 1 ger maxTime och endAhPct alltid fel, och onEnd gäller bara U/I-villkoren. Det är en definition för chargeMode 1, som inte är implementerat. **Beslut krävs.**
3. **minTime betyder olika saker.** I Auto är absorbMinTime den tid slutströmmen måste hållas. På sida 2 är minTime den minsta tid innan avslut tillåts. Mappningen är därför ungefärlig [U].
4. **eff_totalMaxTime** saknar plats på sida 2. Förslag: totalMaxTime fortsätter att skydda även i chargeMode 1. Beslut krävs när chargeMode 1 implementeras.
5. **Spänningarna** i eff_* gäller vid 25 °C. Temperaturkompensering finns inte, eftersom givare saknas (etapp 6).

### Manuell override
E8 kopierar **effektiva** värden. De innehåller redan `man_X` där `ovr_X` = Manuell (SPEC avsnitt 4, steg 2) och är begränsade av hårdvarutaken. Override respekteras alltså automatiskt utan särskild logik.
- E8 rör inte sida 5: varken `ovr`-bitar eller `man_*` ändras.
- E8 ändrar inte heller chargeMode. Användaren väljer själv "Manuell" när tabellen ska användas.

### Ingen EEPROM och ingen burn
- E8 skriver bara RAM-kopian av sida 2 (`gStore.page(PageIdx2)`), precis som E7 och E9.
- Ändringen sparas först när användaren trycker B. Den blir permanent bara om B godkänns av BurnGuard (avvisad B syns som restrictionBits b8, BESLUT nr 35).

### pageRefreshReq
Efter E8 sätts `refresh = RefreshPage2` (0x02). main håller biten i `RefreshHoldMs` = 1000 ms och nollar den sedan, med samma mekanism som 0x10 för E7/E9. ini har redan `triggeredPageRefresh = 2, { (pageRefreshReq & 2) > 0 }`.

### Villkor
- E8 ignoreras och ingen biten sätts om autoWarnBits har AH_MISSING eller NOMV_INVALID. Effektiva värden är då inte meningsfulla.
- E8 tillåts under laddning. Sida 2-tabellen används inte av Auto eller Enkel, och chargeMode 1 är inte implementerat. När chargeMode 1 implementeras bör E8 ignoreras om `chargingActive` och chargeMode = 1 [U].

### Kodskiss (rättad 2026-10-25 efter BESLUT nr 41, stämmer med implementationen)
```cpp
/* pageCommands.cpp: apply(code, page5, page2, in) */
else if (code == 8U)
{
    /* effektiva varden MED override (man_* dar ovr = Manuell), BESLUT nr 41 */
    AutoParams::Result r;
    AutoParams::compute(page5, in, r);
    if ((r.warn & (AutoParams::WarnAhMissing | AutoParams::WarnNomvInvalid)) == 0U)
    {
        copyEffectiveToSteps(r, page2);   /* bara byte 4..83 */
        refresh = RefreshPage2;
    }
}
```
Uppskattad kostnad: cirka 250–350 B flash och 0 B statiskt RAM [U]. Det finns gott om utrymme, eftersom 18 350 B av 28 672 B är använt.

### Testfall (värd)
1. Standard-EEPROM och E8: s1–s4 enligt tabellen. Byte 0–3 och sidorna 1, 3, 4 och 5 är oförändrade byte för byte. Returvärdet är 0x02.
2. Manuell `man_bulkAmp` med `ovr` = Manuell: s1.current och s2.current blir det effektiva (begränsade) manuella värdet, alltså 25 A → 15 A.
3. restartMode av: s4 = Av/Stopp klar. restartMode på: s4 = Monitor/Gå till steg 1 med endVoltMin = eff_restartVolt.
4. bulkAhPct 250: s1.endAhPct = 200 (klampat).
5. AH_MISSING: E8 ändrar ingenting och returnerar 0.
6. E8 gör ingen EEPROM-skrivning: mock-backendens skrivräknare är oförändrad.
7. main-flöde: pageRefreshReq & 0x02 är satt i ungefär 1 s och sedan 0.
8. Golden-matrisen: kör E8 för hela matrisen och kontrollera att varje fält ligger inom ini-intervallet.

---

## 3. Strömgivare typ 3 (30 A) över 32,767 A

### Nuläge (uppmätt och beräknat, inte prövat på hårdvara)
`scaleAcs712Ma()` returnerar `(int)(amps * 1000.0)`. På AVR är `int` 16 bitar. Med konstruktorns värden (ref 4 980 mV, offset 2 483 mV) blir intervallen:

| Typ | mV/A | ADC 1023 | ADC 0 | ADC 1023 med offset 0 (felaktig nollställning) |
|---|---|---|---|---|
| 1 (5 A) | 121 | +20,64 A | −20,52 A | +41,16 A |
| 2 (20 A) | 100 | +24,97 A | −24,83 A | +49,80 A |
| 3 (30 A) | 66 | **+37,83 A** | **−37,62 A** | **+75,45 A** |

- Typ 3 kan alltså gå utanför ±32 767 mA redan vid fullt ADC-utslag. Alla typer kan göra det om nollpunkten blir fel, eftersom `zeroAmpCalibrate` tar offset direkt ur ADC:n.
- Omvandlingen double → int är då odefinierad.
- main.cpp klampar efteråt (mA/10 till ±32 767 cA), men det hjälper inte, eftersom felet redan har skett i drivrutinen.
- Värdtesterna ser inte felet: där är `int` 32 bitar, och `-fsanitize=undefined` i GCC omfattar inte `float-cast-overflow`.

### Förslag
1. **Heltal i 32 bitar hela vägen och ingen double.** Ny funktion i `shared/scale.cpp`. Mellanresultaten ryms med god marginal: |mV| ≤ 1023·5000 och |mA| ≤ 10⁷.
2. **Klampa före omvandling** till ±32 767 mA i `Current::readCurrent()`, som fortfarande returnerar `int`. API:et är då oförändrat.
3. **Mättnadsflagga** `Current::saturated()`. main kan visa den som en varning i **restrictionBits b9** (förslag; inte layoutändring, samma princip som nr 35).
   - Laddaren behöver ingen ny felbit. ±32 767 mA blir ±3 276 cA, och `absCa()` ≥ MaxChargeAmp (högst 1 500 cA) ger redan ErrOverAmpOut och hård brytning i samma varv.
4. Explicita typer (`int16_t`/`int32_t`) gör att värdtesterna beter sig som på AVR.

```cpp
/* scale.hpp */
int32_t scaleAcs712Ma32(uint16_t adc, int16_t refMv, int16_t offsetMv, int16_t mVperAmp);

/* scale.cpp */
int32_t scaleAcs712Ma32(uint16_t adc, int16_t refMv, int16_t offsetMv, int16_t mVperAmp)
{
    int32_t mv;
    if (adc > 1023U) { adc = 1023U; }
    if (mVperAmp <= 0) { return 0; }
    mv = (((int32_t)adc * (int32_t)refMv) + 511L) / 1023L;          /* <= 5,1e6 */
    return ((mv - (int32_t)offsetMv) * 1000L) / (int32_t)mVperAmp;  /* |.| <= 1e7 */
}

/* sCurrent.cpp: readCurrent() */
int32_t ma = scaleAcs712Ma32(adc, pRefVoltage, pACOffset, mVperAmp);
pSaturated = (ma > 32767L) || (ma < -32767L);
if (ma > 32767L)  { ma = 32767L; }
if (ma < -32767L) { ma = -32767L; }
return (int)ma;
```

- Avrundningen skiljer sig från double-versionen med högst cirka ±15 mA för typ 3 [U] (1 mV/66 mV/A).
- Flash troligen oförändrad eller mindre om double-rutinerna inte används på annat håll [U]. `scaleAdcToVolt` och `zeroAmpCalibrate` använder också double. RAM +1 B (flaggan).

### Testfall (värd, nytt `test_scale.cpp`)
1. Tabell med ADC {0, 1, 511, 512, 1023} × typ {121, 100, 66} × offset {2 483, 0, 4 980}. Resultatet ska ligga inom ±15 mA från en double-referens som är mättad till ±32 767.
2. Typ 3, ADC 1023, offset 2 483: mättat till 32 767 och `saturated()` sann. ADC 0 ger −32 767 och sann.
3. ADC vid nollpunkten (offset 2 483 ≙ ADC 510) ger |mA| < 66.
4. Resultatet är monotont i ADC för alla typer.
5. mVperAmp 0 ger 0 utan division med noll.
6. Testbygget får `-fsanitize=float-cast-overflow`. Mot den gamla funktionen med `int16_t`-retur ska testet fallera, vilket bevisar att det fångar felet.
7. Laddaren: outCa = 3 276 och −3 276 ger ErrOverAmpOut och duty 0 i samma varv. Ett sådant test finns delvis redan.

---

## 4. MaxChargeVolt lika med absorption: av/på-pendling

### Nuläge (värdsimulerat, inte på hårdvara)
- Hårdgränsen i `Charger::update()` (charger.cpp, beräkningen av `err` ungefär rad 288–293) är `m.outCv >= mcv`.
- Regulatorns mål begränsas till samma värde: `vSet_ = minU16(vSet_, mcv)` (ungefär rad 415). Strömmen behandlas på samma sätt (`a >= mca` och `iSet_ = min(iSet_, mca)`).
- När regulatorn når sitt mål slår hårdgränsen alltså ifrån i samma varv (duty 0). Därefter rampar regulatorn upp igen från 0.

Simulering med `Sim`-modellen i `test_charger.cpp` (tillfälligt program, inte incheckat). Modellen ger 1 A per duty-steg, 0,05 Ω inre resistans och en kapacitet på 3 Ah, och körs i 1 timme med 100 ms per varv. Modellen är grov, så antalet varv är bara en indikation [U]:

| Fall | Varv med hård brytning | duty > 0 → 0 |
|---|---|---|
| Auto, MCV 15,00 V > absorb 14,70 V | 0 | 0 |
| Auto, MCV = absorb = 14,70 V | 5 431 | 5 431 |
| **Enkel**, MCV 14,40 V, 8 A, batteri som når 14,40 V | 17 490 | 3 456 |
| **Enkel**, ChargeAmp 0 (= MCA 6 A) | 5 142 | 5 142 |

**Viktigt:** i Enkel-läge är spänningsmålet *alltid* MaxChargeVolt (`vSet_ = mcv`). Där pendlar den alltså i varje CV-fas, inte bara när MCV = absorb. Samma sak gäller strömmen när ChargeAmp är 0 eller ≥ MaxChargeAmp. Det finns i grenen sedan etapp 4. Om master beter sig likadant är inte kontrollerat [U].

### Förslag: regleringsmarginal under gränsen, plus spärrtid efter brytning
1. **Marginal (regleringen hamnar under hårdgränsen):**
   - `vSet_ = min(vSet_, mcv − VoltRegMarginCv)` med **VoltRegMarginCv = 10 cV (0,10 V)**.
   - `iSet_ = min(iSet_, mca − AmpRegMarginCa)` med **AmpRegMarginCa = 20 cA (0,20 A)**.
   - Vald därför att mätupplösningen för utspänning är 5 500/1 023 ≈ 5,4 cV per ADC-steg, så 10 cV ≈ 2 steg. Det räcker om reglersteget (±1 duty per 100 ms) ger mindre än 10 cV per steg, vilket inte är verifierat [U].
   - Strömmens upplösning är grövre: 4 980 mV/1 023 ≈ 4,87 mV per ADC-steg. Det ger cirka 40 mA/steg för 5 A-givaren (121 mV/A) och 74 mA/steg för 30 A-givaren (66 mV/A). 20 cA motsvarar alltså ungefär 2,7–5 ADC-steg (före 13-punktsfiltret) [U].
   - Hårdgränsen ändras **inte** (≥ MCV eller ≥ MCA → av), så säkerheten är densamma som nu.
2. **Hysteres och minsta frånslagstid efter hård brytning:**
   - Efter `hardCut` hålls duty 0 i minst **HardCutHoldMs = 1 000 ms** (10 varv).
   - Därefter krävs **V < mcv − 20 cV** och **I < mca − 40 cA** innan rampen får börja igen från 0, eller från MinPWM enligt dödzonsregeln.
   - Felbitarna i `errorBits` visar som nu det aktuella varvets tillstånd, och `latched_` samlar dem.
   - Ny medlem `holdMs_` (uint16_t): RAM +2 B, flash cirka +60–100 B [U].
3. **Placering:** `Charger::update()` i shared/charger.cpp.
   - Konstanterna läggs i `charger.hpp` bredvid `AbsMaxChargeAmp`.
   - Marginalen läggs in i regulatorblocket där `vSet_`/`iSet_` begränsas.
   - Hysteres och spärrtid läggs in efter `hardCut = (err != 0U);`: om hardCut eller `holdMs_` > 0, sätt duty 0.

### Effekt på säkerhet och SPEC
- **Säkerhet:** oförändrad eller bättre. Gränsen ligger kvar på samma värde, frånslaget sker i samma varv, och spärrtiden minskar antalet tillslag mot en gräns.
- **SPEC:** SPEC tillåter absorb = voltLimit = MaxChargeVolt (VOLT_CLAMPED, exemplet "MaxChargeVolt = 14,00 V: absorption 14,00 V").
  - Med förslaget regleras utgången i stället till 13,90 V medan `eff_absorbVolt` fortfarande visar 14,00. Det är en **dokumenterad avvikelse i laddaren**.
  - `eff_*` och golden-testerna påverkas inte, så bitexaktheten mot auto_params_v2.2.py behålls.
  - Enkel-läget laddar till MCV − 0,10 V i stället för MCV.
- Alternativet att flytta hårdgränsen *uppåt* (strikt `>` eller MCV + marginal) avråds. Det tillåter spänning över det användaren angett som max (BESLUT nr 34, öppen fråga).

### Testfall (värd)
1. Auto, MCV = absorb = 14,70 V, 1 h simulering: 0 varv med ErrOverVoltOut, och utgången når CV vid 14,60 V ± 1 reglersteg.
2. Enkel, MCV 14,40 V: 0 hårda brytningar och stabil CV under 14,40 V.
3. Enkel, ChargeAmp 0: iSet = MCA − 20 cA och inga ErrOverAmpOut.
4. Verklig överspänning (`extraCv` +100 cV i ett varv): duty 0 i samma varv och ErrOverVoltOut i errorBits. Duty förblir 0 i ≥ 10 varv och tills V < MCV − 20 cV.
5. Överström (forceCa ≥ MCA): samma beteende som 4.
6. MCV < 10 cV eller MCA < 20 cA: inget underflöde i uint16. Beslut 24 garanterar bara att MCV > 0, så skillnaden klampas mot 0.
7. Golden-testet (test_autoparams) oförändrat.

---

## 6. currentSensor gäller först efter omstart

### Nuläge
- `main.cpp` läser givartypen vid uppstart och skapar `Current` med den (BESLUT nr 29). En ändring kräver E5 eller strömavbrott.
- **Inkonsekvens:** `AutoParams` (`PageCommands::autoInput`) läser `currentSensor` direkt från sida 3 i RAM vid varje omräkning. Om användaren byter typ utan omstart räknar autoparametrarna `hwCap` från den nya givaren (givarmax 5/20/30 A), medan strömmätningen fortfarande använder den gamla skalan (mV/A).

### Bedömning
Att typen gäller först efter omstart är **OK och säkrast**. Det bör dock göras konsekvent och synligt.

| Risk | Vid direkt byte |
|---|---|
| Byte under laddning | Strömskalan ändras med en faktor upp till 121/66 ≈ 1,8 mitt i regleringen. Det ger ett språng i uppmätt ström, och regulatorn eller hårdgränsen reagerar på fel värde. |
| ampZeroAtBoot | Nollpunkten lagras i mV (`pACOffset`) och beror inte av typen, så den är formellt fortfarande giltig. Men ett givarbyte innebär i praktiken ny hårdvara, och den nollställs bara säkert med utgången av vid uppstart. |
| Säkerhet | Fel skala gör att MaxChargeAmp och hårdgränsen jämförs mot fel ström. Med 5 A-skala (121 mV/A) på en 30 A-givare (66 mV/A) visas strömmen cirka 45 % för låg [U: beräknat], och överström upptäcks för sent. |

### Förslag
1. **Lås typen för AutoParams också:** `autoInput()` får boot-värdet (`gBootSensorSel`) i stället för sidans värde. Då hör `hwCap` och strömmätningen alltid ihop.
2. **Synlig indikator:** om sidans värde skiljer sig från boot-värdet sätts **restrictionBits b10** "Givartyp ändrad – omstart krävs". Det är förslag och inte en layoutändring. Ini får en gul indikator.
3. **Start nekas** så länge typerna skiljer sig, så att ingen laddning sker med fel skala. E5 (omstart) är tillåten i vila.
4. Direkt byte i vila (inte `chargingActive`) via `Current::setType()` plus ny nollställning är möjligt men **avråds**. Mer kod och fler tillstånd ger liten nytta, eftersom givarbyte ändå kräver ingrepp i hårdvaran.

Kostnad cirka 40–80 B flash och 1 B RAM [U].

### Testfall (värd)
1. Ändra currentSensor på sida 3 i RAM utan omstart: `eff_bulkAmp` och `hwCap` är oförändrade (boot-värdet används), restrictionBits b10 är satt och E1 startar inte.
2. Återställ värdet: b10 nollas och E1 startar.
3. Simulera omstart med det nya värdet: b10 är 0 och hwCap följer den nya typen.
4. Golden-testet oförändrat, eftersom compute() bara får ett annat in-värde.

---

## Sammanfattning av föreslagna nya bitar (ingen layoutändring)

| Bit | Förslag | Status |
|---|---|---|
| restrictionBits b8 | Avvisad B | **Införd** (BESLUT nr 35) |
| restrictionBits b9 | Strömmätning mättad (förslag 3) | **Införd** (BESLUT nr 37) |
| restrictionBits b10 | Givartyp ändrad, omstart krävs (förslag 6) | **Införd** (BESLUT nr 39) |
| errorBits b12 | bulkAhPct (BulkAh) | **Införd** (BESLUT nr 36) |

`ochBlockSize` 115 och layouten i nr 21 påverkas inte av något förslag.
