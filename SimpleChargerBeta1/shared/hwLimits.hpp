/*
 * hwLimits.hpp
 *
 * Hardvarutak som delas av firmware och ini-generatorn
 * (tools/ini22_ui.py laser vardena har ur filen och satter ini-faltens
 * ovre grans; test/check_ini_layout.py faller om ini och firmware skiljer).
 *
 *   HwVoltMaxCv       55,00 V utspanning (BESLUT nr 45: 48 V stods inte,
 *                     utspanningsmatningen slutar vid 55 V). Anvands av
 *                     E8-klampningen, klampning av MaxChargeVolt och man_*
 *                     spanningsfalt vid skrivning, och ini-max for
 *                     MaxChargeVolt, man_* och s1..s4 spanningsfalt.
 */
#ifndef SHARED_HWLIMITS_HPP_
#define SHARED_HWLIMITS_HPP_

#include <stdint.h>

namespace HwLimits {

const uint16_t HwVoltMaxCv = 5500U;

} /* namespace HwLimits */

#endif /* SHARED_HWLIMITS_HPP_ */
