/*
 * autoParams.hpp
 *
 * Autoparametrar v2.2: ren C++-port av auto_params_v2.2.py (Drive,
 * md5 ef56d175..., kopia i test/ref/). Ingen hardvara, ingen heap,
 * bara heltal (int32). Golden-testerna i test/ kraver bit-exakt lika
 * resultat (eff_*, autoWarnBits, effManualMask) som Python-referensen.
 *
 * Enheter som i ini v2.2: spanning cV (0,01 V), strom cA (0,01 A),
 * kapacitet dAh (0,1 Ah), tid min (absorbMinTime s), temp C (komp: 0,1 C).
 */
#ifndef AUTO_PARAMS_HPP_
#define AUTO_PARAMS_HPP_

#include <stdint.h>

namespace AutoParams {

/* Parameterordning = PARAMS i auto_params_v2.2.py = bit i ovr-masken. */
enum ParamId {
    CellCount = 0, BulkAmp, AbsorbVolt, FloatVolt, TailAmp, FloatAmp,
    BulkMaxTime, BulkAhPct, AbsorbMaxTime, AbsorbMinTime, FloatMaxTime,
    RestartVolt, TempCompMv, ChargeTempMin, ChargeTempMax, PrechargeVolt,
    PrechargeAmp, PrechargeMaxTime, VoltLimit, TotalMaxTime, BatteryPresentVolt,
    ParamCount
};

const uint32_t AllMask = (1UL << ParamCount) - 1UL;

/* autoWarnBits (WARN i auto_params_v2.2.py) */
const uint16_t WarnAhMissing      = 0x0001U;  /* b0 */
const uint16_t WarnCustomType     = 0x0002U;  /* b1 */
const uint16_t WarnAmpCapped      = 0x0004U;  /* b2 */
const uint16_t WarnTailFloor      = 0x0008U;  /* b3 */
const uint16_t WarnVoltClamped    = 0x0010U;  /* b4 */
const uint16_t WarnNomvInvalid    = 0x0020U;  /* b5 */
const uint16_t WarnTcNoSensor     = 0x0040U;  /* b6 */
const uint16_t WarnInconsistent   = 0x0080U;  /* b7 */
const uint16_t WarnOverride       = 0x0100U;  /* b8 */
const uint16_t WarnTimeClamped    = 0x0200U;  /* b9 */
const uint16_t WarnFloatUnlimited = 0x0400U;  /* b10 */

/* Indata som inte ligger i sida 5 (sida 1/3) eller ar firmware-tillstand. */
struct Input {
    uint16_t maxChargeAmp;   /* sida 1 MaxChargeAmp, cA */
    uint16_t maxChargeVolt;  /* sida 1 MaxChargeVolt, cV (0 = av) */
    uint8_t  currentSensor;  /* sida 3 byte 6 [4:5] */
    bool     batSensor;      /* batteritempgivare tillganglig (etapp 6: false) */
};

struct Result {
    int32_t  eff[ParamCount];
    uint32_t manualMask;     /* effManualMask */
    uint16_t warn;           /* autoWarnBits */
};

/* effective(): sida 5 (64 B, man_* och ovr-bitar) + Input -> Result.
 * ignoreOverrides = true: som om alla ovr-bitar vore 0, dvs. rena
 * autovarden (auto_values) efter granserna - anvands av E7. */
void compute(const uint8_t* page5, const Input& in, Result& out, bool ignoreOverrides = false);

/* comp_target(): temperaturkompenserat borvarde (cV), tDc i 0,1 C. */
int32_t compTarget(int32_t vCv, const Result& r, int16_t tDc, bool enabled);

/* man_X: offset i sida 5 och typ (0 U08, 1 S08, 2 U16). */
uint8_t manOffset(uint8_t id);
uint8_t manType(uint8_t id);

/* Skriv eff_*, effManualMask och autoWarnBits till realtidsblocket. */
void writeRealtime(const Result& r, uint8_t* rt);

/* Kopiera r.eff till man_* (sida 5). Andrar inte ovr-bitar. E7 anropar
 * med ett resultat fran compute(..., true). */
void copyEffectiveToManual(const Result& r, uint8_t* page5);

} /* namespace AutoParams */

#endif /* AUTO_PARAMS_HPP_ */
