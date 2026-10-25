/*
 * tsLayout.hpp
 *
 * Offset for TunerStudio v2.2: sida 2, 3 och 5 samt realtidsblocket (A).
 * Kalla: SPEC_tabeller_v2.2.md och SmedbyCharger_v2.2.ini (Drive, LTT-Sweden).
 * Sida 1 och 4: eepromLayout.hpp. Sidindex ar 0-baserat (P\000..P\004).
 *
 * Sidorna lagras som byte-arrayer (EepromStore). Fleroktetsfalt laser och
 * skrivs bara via scGet/scPut-hjalparna (little-endian), aldrig via
 * pekarcast. test/test_ini_layout.py jamfor offset har mot ini-filen.
 */
#ifndef TS_LAYOUT_HPP_
#define TS_LAYOUT_HPP_

#include <stdint.h>
#include "eepromLayout.hpp"

namespace TsLayout {

/* ---- Sidindex (0-baserat) ------------------------------------------- */
const uint8_t PageIdx1 = 0U;
const uint8_t PageIdx2 = 1U;
const uint8_t PageIdx3 = 2U;
const uint8_t PageIdx4 = 3U;
const uint8_t PageIdx5 = 4U;

/* ---- Sida 2: start + manuell stegtabell s1..s4 ----------------------- */
const uint16_t P2StartBits      = 0U;   /* [0:1] startMode, [2:2] restartMode */
const uint16_t P2StartDelayMin  = 2U;   /* U16 min */
const uint16_t P2StepBase       = 4U;   /* s1 borjar har */
const uint16_t P2StepSize       = 20U;  /* 20 B per steg */
const uint8_t  P2StepCount      = 4U;
/* Offset inom ett steg. */
const uint16_t StMethodBits     = 0U;   /* [0:1] metod, [2:3] onEnd */
const uint16_t StTempMax        = 1U;   /* S08 C */
const uint16_t StCurrent        = 2U;   /* U16 cA */
const uint16_t StVolt           = 4U;   /* U16 cV */
const uint16_t StEndVoltMax     = 6U;
const uint16_t StEndVoltMin     = 8U;
const uint16_t StEndAmpMax      = 10U;
const uint16_t StEndAmpMin      = 12U;
const uint16_t StMaxTime        = 14U;  /* U16 min */
const uint16_t StMinTime        = 16U;  /* U16 s */
const uint16_t StTempMin        = 18U;  /* S08 C */
const uint16_t StEndAhPct       = 19U;  /* U08 % */
const uint8_t  MethodCC = 0U, MethodCV = 1U, MethodMonitor = 2U, MethodOff = 3U;
const uint8_t  OnEndNext = 0U, OnEndGoto1 = 1U, OnEndStop = 2U, OnEndStopError = 3U;
const uint8_t  StartManual = 0U, StartAuto = 1U, StartTimer = 2U;

/* ---- Sida 3: skydd, drivsteg, kalibrering ---------------------------- */
const uint16_t P3SensorBits     = 6U;   /* [0:1] mosfet, [2:3] bat, [4:5] currentSensor, [6] ampZeroAtBoot, [7] inAmpSensor */
const uint16_t P3DiagMode       = 16U;  /* [0:1], se EepromLayout::P3DiagModeOffset */
const uint16_t P3DiagDuty       = 17U;

/* ---- Sida 5: autolage ------------------------------------------------ */
const uint16_t P5ModeBits       = 0U;   /* [0:1] chargeMode, [2:2] tempCompEnable */
const uint16_t P5TypeBits       = 1U;   /* [0:2] batteryType, [3:5] batteryNomVolt */
const uint16_t P5CapacityAh     = 2U;   /* U16 0,1 Ah */
const uint16_t P5AutoAmpCap     = 4U;   /* U16 cA */
const uint16_t P5TailAmpFloor   = 6U;   /* U16 cA */
const uint16_t P5OvrMask        = 8U;   /* 3 byte, bit i = parameter i (21 bitar) */
const uint16_t P5ManBase        = 12U;  /* man_cellCount; ovriga man_* i AutoParams-tabellen */
const uint16_t P5AutoReserved   = 49U;  /* U08[15] */
const uint8_t  ChargeModeAuto = 0U, ChargeModeManual = 1U, ChargeModeSimple = 2U;

/* ---- Realtidsblocket (kommando A) ------------------------------------ */
const uint16_t RtSeconds          = 0U;   /* U16 s */
const uint16_t RtPulseWidth       = 2U;   /* U08 steg */
const uint16_t RtChargerState     = 3U;   /* U08 */
const uint16_t RtStage            = 4U;   /* U08 */
const uint16_t RtStatusBits       = 5U;   /* U08 */
const uint16_t RtRestrictionBits  = 6U;   /* U16 */
const uint16_t RtErrorBits        = 8U;   /* U16 */
const uint16_t RtLatchedErrorBits = 10U;  /* U16 */
const uint16_t RtInVolt           = 12U;  /* U16 cV */
const uint16_t RtOutVolt          = 14U;  /* U16 cV */
const uint16_t RtInAmp            = 16U;  /* S16 cA */
const uint16_t RtOutAmp           = 18U;  /* S16 cA */
const uint16_t RtMosfetTemp       = 20U;  /* S16 0,1 C */
const uint16_t RtBatTemp          = 22U;  /* S16 0,1 C */
const uint16_t RtExtraTemp        = 24U;  /* S16 0,1 C */
const uint16_t RtDriverVolt       = 26U;  /* U16 cV */
const uint16_t RtAmpHours         = 28U;  /* U16 0,01 Ah */
const uint16_t RtWattHours        = 30U;  /* U32 0,1 Wh */
const uint16_t RtStageTime        = 34U;  /* U16 s */
const uint16_t RtChargeTime       = 36U;  /* U32 s */
const uint16_t RtVoltSetpoint     = 40U;  /* U16 cV */
const uint16_t RtAmpSetpoint      = 42U;  /* U16 cA */
const uint16_t RtDerateNow        = 44U;  /* U08 % */
const uint16_t RtPwmFreqNow       = 45U;  /* U08 kHz */
const uint16_t RtMosfetTempAdc    = 46U;
const uint16_t RtBatTempAdc       = 48U;
const uint16_t RtOutVoltAdc       = 50U;
const uint16_t RtOutAmpAdc        = 52U;
const uint16_t RtInVoltAdc        = 54U;
const uint16_t RtInAmpAdc         = 56U;
const uint16_t RtDriverVoltAdc    = 58U;
const uint16_t RtOverAmpTime      = 60U;
const uint16_t RtTimerRemaining   = 62U;  /* U16 s */
const uint16_t RtLoopTimeUs       = 64U;
const uint16_t RtResetCause       = 66U;
const uint16_t RtRetriesLeft      = 67U;
const uint16_t RtEffBase          = 68U;  /* eff_cellCount; eff_X = man_X + 56 */
const uint16_t RtEffManualMask    = 105U; /* U32 */
const uint16_t RtAutoWarnBits     = 109U; /* U16 */
const uint16_t RtTempCompDelta    = 111U; /* S16 cV */
const uint16_t RtPageRefreshReq   = 113U; /* U08, bit n-1 = sida n */
const uint16_t RtEepromWarnBits   = 114U; /* U08, BESLUT nr 21 */
const uint16_t RtBlockSize        = 115U; /* ochBlockSize (BESLUT nr 8/21) */

/* eff_X ligger 56 byte efter man_X (68 - 12). */
const uint16_t EffMinusMan = RtEffBase - P5ManBase;

/* chargerState */
const uint8_t StateWaiting = 0U, StateTimer = 1U, StateSoftStart = 2U, StateCharging = 3U,
              StateDone = 4U, StateError = 5U, StatePauseTemp = 6U, StateDiag = 7U;

/* statusBits */
const uint8_t StatusOutputOn = 0x01U, StatusCC = 0x02U, StatusCV = 0x04U, StatusDerate = 0x08U,
              StatusBattery = 0x10U, StatusDiag = 0x20U, StatusTimer = 0x40U, StatusPrecharge = 0x80U;

/* errorBits (SPEC_tabeller_v2.2.md realtime offset 8) */
const uint16_t ErrOverVoltOut   = 0x0001U;
const uint16_t ErrOverAmpOut    = 0x0002U;
const uint16_t ErrHighInVolt    = 0x0004U;
const uint16_t ErrLowInVolt     = 0x0008U;
const uint16_t ErrMosfetTemp    = 0x0010U;
const uint16_t ErrLowDriver     = 0x0020U;
const uint16_t ErrHighDriver    = 0x0040U;
const uint16_t ErrTempSensor    = 0x0080U;
const uint16_t ErrBatTemp       = 0x0100U;
const uint16_t ErrMaxTime       = 0x0200U;
const uint16_t ErrNoBattery     = 0x0400U;
const uint16_t ErrAmpZero       = 0x0800U;
/* b12: bulkAhPct overskriden (BESLUT nr 36). Egen bit, delas inte med
 * ErrMaxTime. Tillagg: SPEC_tabeller definierar b0..b11, b12..b15 lediga. */
const uint16_t ErrBulkAh        = 0x1000U;

/* restrictionBits (offset 6) */
const uint16_t RestrMaxOutAmp   = 0x0001U;
const uint16_t RestrMaxInAmp    = 0x0002U;
const uint16_t RestrMosfetTemp  = 0x0004U;
const uint16_t RestrBatTemp     = 0x0008U;
const uint16_t RestrLowInVolt   = 0x0010U;
const uint16_t RestrMaxOutVolt  = 0x0020U;
const uint16_t RestrPwmCap      = 0x0040U;
const uint16_t RestrAmpTime     = 0x0080U;
/* b8: senaste B avvisades (laddning pagick), andringar inte sparade.
 * Tillagg i den lediga hoga byten (BESLUT nr 35), satts av main, inte av Charger. */
const uint16_t RestrBurnRejected = 0x0100U;

} /* namespace TsLayout */

typedef char tsRtFitsU8Len[(TsLayout::RtBlockSize < 256U) ? 1 : -1];
typedef char tsRtWarnLast[(TsLayout::RtEepromWarnBits + 1U == TsLayout::RtBlockSize) ? 1 : -1];
typedef char tsP2StepsFit[((TsLayout::P2StepBase + TsLayout::P2StepCount * TsLayout::P2StepSize) == EepromLayout::Page2Size) ? 1 : -1];
typedef char tsP5ReservedEnd[((TsLayout::P5AutoReserved + 15U) == EepromLayout::Page5Size) ? 1 : -1];
typedef char tsEffMan56[(TsLayout::EffMinusMan == 56U) ? 1 : -1];

/* Bitfalt: (byte >> shift) & mask. */
static inline uint8_t scBits(uint8_t value, uint8_t shift, uint8_t mask)
{
    return (uint8_t)((uint8_t)(value >> shift) & mask);
}

static inline uint32_t scGetU32le(const uint8_t* p)
{
    return (uint32_t)scGetU16le(p) | ((uint32_t)scGetU16le(p + 2) << 16);
}

static inline void scPutU32le(uint8_t* p, uint32_t v)
{
    scPutU16le(p, (uint16_t)(v & 0xFFFFU));
    scPutU16le(p + 2, (uint16_t)(v >> 16));
}

static inline int8_t scGetS8(const uint8_t* p)
{
    int8_t s;
    memcpy(&s, p, 1);
    return s;
}

/* Lagg in RestrBurnRejected (b8) i restrictionBits i realtidsblocket efter
 * Charger::writeRealtime (BESLUT nr 35). Ovriga bitar orors. */
static inline void tsRtSetBurnRejected(uint8_t* rt, bool rejected)
{
    uint16_t r = scGetU16le(rt + TsLayout::RtRestrictionBits);
    if (rejected)
    {
        r = (uint16_t)(r | TsLayout::RestrBurnRejected);
    }
    else
    {
        r = (uint16_t)(r & (uint16_t)~TsLayout::RestrBurnRejected);
    }
    scPutU16le(rt + TsLayout::RtRestrictionBits, r);
}

#endif /* TS_LAYOUT_HPP_ */
