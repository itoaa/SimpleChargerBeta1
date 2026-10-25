/*
 * pageCommands.hpp
 *
 * Kopieringsknapparna E7..E9 (SPEC_v2.2 avsnitt "pageRefresh") och
 * AutoParams-indata fran sida 1/3. Ren C++ utan hardvara, host-testas.
 *
 *   E7: autovarden (compute med ignoreOverrides) -> man_* i sida 5.
 *       ovr-bitarna rors inte. Returnerar pageRefreshReq-bit 0x10.
 *   E8: effektiva -> stegtabell s1..s4 (sida 2): INTE implementerad
 *       (mappningen ar inte specificerad, BESLUT nr 28). Ingen andring,
 *       returnerar 0.
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

/* Utfor E7/E8/E9 pa RAM-sidan page5. Returnerar pageRefreshReq-bitar
 * (0 om inget andrades eller koden inte ar 7..9). */
uint8_t apply(uint8_t code, uint8_t* page5, const AutoParams::Input& in);

} /* namespace PageCommands */

#endif /* SHARED_PAGECOMMANDS_HPP_ */
