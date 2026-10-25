/*
 * pageCommands.hpp
 *
 * Kopieringsknapparna E7..E9 (SPEC_v2.2 avsnitt "pageRefresh") och
 * AutoParams-indata fran sida 1/3. Ren C++ utan hardvara, host-testas.
 *
 *   E7: autovarden (compute med ignoreOverrides) -> man_* i sida 5.
 *       ovr-bitarna rors inte. Returnerar pageRefreshReq-bit 0x10.
 *   E8: effektiva varden (compute med override) -> stegtabell s1..s4 pa
 *       sida 2, byte 4..83 (BESLUT nr 40, forslag 2): s1 bulk, s2
 *       absorption, s3 float, s4 overvaka (Monitor/Ga till steg 1 om
 *       restartMode pa, annars Av/Stopp klar). Forladdningen far ingen
 *       plats. Byte 0..3 och sida 5 rors inte. Ignoreras (returnerar 0,
 *       ingen andring) vid autoWarnBits AH_MISSING eller NOMV_INVALID.
 *       Returnerar 0x02.
 *   E9: alla parametrar -> Auto, dvs. ovr-bitarna (sida 5 byte 8..10)
 *       nollas. man_* lamnas. Returnerar 0x10.
 * Ingen funktion branner EEPROM (bara RAM-kopian andras).
 */
#ifndef SHARED_PAGECOMMANDS_HPP_
#define SHARED_PAGECOMMANDS_HPP_

#include <stdint.h>
#include "autoParams.hpp"

namespace PageCommands {

/* pageRefreshReq: bit n-1 = sida n */
const uint8_t RefreshPage2 = 0x02U;
const uint8_t RefreshPage5 = 0x10U;

/* AutoParams::Input fran sida 1 och 3 (batSensor = false i v2.2, etapp 6). */
void autoInput(const uint8_t* page1, const uint8_t* page3, AutoParams::Input& in);

/* currentSensor-faltet (sida 3 byte 6 bit 4..5), 0..3. */
uint8_t sensorSel(const uint8_t* page3);

/* Som autoInput, men givartypen ar last till boot-vardet (BESLUT nr 39),
 * sa att hwCap och strommatningens skala alltid hor ihop. */
void autoInputBoot(const uint8_t* page1, uint8_t bootSensor, AutoParams::Input& in);

/* Utfor E7/E9 pa RAM-sidan page5 och E8 pa RAM-sidan page2. Returnerar
 * pageRefreshReq-bitar (0 om inget andrades eller koden inte ar 7..9). */
uint8_t apply(uint8_t code, uint8_t* page5, uint8_t* page2, const AutoParams::Input& in);

/* E8:s mappning (exponerad for test): skriver bara page2 byte 4..83. */
void copyEffectiveToSteps(const AutoParams::Result& r, uint8_t* page2);

/* pageRefreshReq-hallning: bitar satta vid E7..E9 halls i holdMs och
 * nollas sedan (SPEC: ca 1 s). Ren logik sa att den kan vardtestas. */
struct RefreshHold
{
    uint8_t  bits;
    uint32_t sinceMs;
};
void refreshInit(RefreshHold& h);
void refreshSet(RefreshHold& h, uint8_t bits, uint32_t nowMs);
/* Nollar bitarna nar holdMs har gatt; returnerar aktuella bitar. */
uint8_t refreshTick(RefreshHold& h, uint32_t nowMs, uint32_t holdMs);

} /* namespace PageCommands */

#endif /* SHARED_PAGECOMMANDS_HPP_ */
