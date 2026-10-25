/*
 * pageCommands.cpp - se pageCommands.hpp.
 */
#include "pageCommands.hpp"
#include "eepromLayout.hpp"
#include "tsLayout.hpp"
#include "hwLimits.hpp"

namespace PageCommands {

static uint16_t rdU16(const uint8_t* p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

uint16_t maxChargeVoltCv(const uint8_t* page1)
{
    const uint16_t v = rdU16(page1 + EepromLayout::P1MaxChargeVolt);
    return (v > HwLimits::HwVoltMaxCv) ? HwLimits::HwVoltMaxCv : v;
}

void autoInput(const uint8_t* page1, const uint8_t* page3, AutoParams::Input& in)
{
    in.maxChargeAmp = rdU16(page1 + EepromLayout::P1MaxChargeAmp);
    in.maxChargeVolt = maxChargeVoltCv(page1);
    in.currentSensor = sensorSel(page3);
    in.batSensor = false;   /* etapp 6 (givare) ej implementerad */
}

uint8_t sensorSel(const uint8_t* page3)
{
    return (uint8_t)((page3[TsLayout::P3SensorBits] >> 4) & 0x03U);
}

void autoInputBoot(const uint8_t* page1, uint8_t bootSensor, AutoParams::Input& in)
{
    in.maxChargeAmp = rdU16(page1 + EepromLayout::P1MaxChargeAmp);
    in.maxChargeVolt = maxChargeVoltCv(page1);
    in.currentSensor = (uint8_t)(bootSensor & 0x03U);
    in.batSensor = false;   /* etapp 6 (givare) ej implementerad */
}

static void wrU16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)(v >> 8);
}

static uint16_t clampU16(int32_t v, int32_t hi)
{
    int32_t x = v;
    if (x < 0) { x = 0; }
    if (x > hi) { x = hi; }
    return (uint16_t)x;
}

static uint8_t clampS8(int32_t v, int32_t lo, int32_t hi)
{
    int32_t x = v;
    if (x < lo) { x = lo; }
    if (x > hi) { x = hi; }
    return (uint8_t)(int8_t)x;
}

/* Faltens offset inom ett steg och intervall (tunerStudio_v2.2.ini). */
enum
{
    SMethod = 0U, STempMax = 1U, SCurrent = 2U, SVolt = 4U, SEndVoltMax = 6U,
    SEndVoltMin = 8U, SEndAmpMax = 10U, SEndAmpMin = 12U, SMaxTime = 14U,
    SMinTime = 16U, STempMin = 18U, SEndAhPct = 19U
};
const int32_t AmpMaxCa = 1500;   /* BESLUT nr 42: alla stromfalt 0..15 A */
const int32_t VoltMaxCv = (int32_t)HwLimits::HwVoltMaxCv;   /* BESLUT nr 45: 55,00 V */
const int32_t TimeMaxMin = 6000;
const int32_t MinTimeMaxS = 65535;
const int32_t AhPctMax = 200;
const int32_t TempLo = -40;
const int32_t TempMaxHi = 80;
const int32_t TempMinHi = 60;
/* method [0:1], onEnd [2:3] */
const uint8_t MethodCC = 0U, MethodCV = 1U, MethodMonitor = 2U, MethodOff = 3U;
const uint8_t EndNext = 0U, EndGoto1 = 1U, EndStopDone = 2U;

struct StepVals
{
    uint8_t method;
    uint8_t onEnd;
    int32_t current;
    int32_t volt;
    int32_t endVoltMax;
    int32_t endVoltMin;
    int32_t endAmpMin;
    int32_t maxTime;
    int32_t minTime;
    int32_t ahPct;
};

static void writeStep(uint8_t* s, const StepVals& v, int32_t tMax, int32_t tMin)
{
    uint8_t i;
    for (i = 0U; i < TsLayout::P2StepSize; i++)
    {
        s[i] = 0U;
    }
    s[SMethod] = (uint8_t)((v.method & 0x03U) | (uint8_t)((v.onEnd & 0x03U) << 2));
    s[STempMax] = clampS8(tMax, TempLo, TempMaxHi);
    wrU16(s + SCurrent, clampU16(v.current, AmpMaxCa));
    wrU16(s + SVolt, clampU16(v.volt, VoltMaxCv));
    wrU16(s + SEndVoltMax, clampU16(v.endVoltMax, VoltMaxCv));
    wrU16(s + SEndVoltMin, clampU16(v.endVoltMin, VoltMaxCv));
    wrU16(s + SEndAmpMax, 0U);
    wrU16(s + SEndAmpMin, clampU16(v.endAmpMin, AmpMaxCa));
    wrU16(s + SMaxTime, clampU16(v.maxTime, TimeMaxMin));
    wrU16(s + SMinTime, clampU16(v.minTime, MinTimeMaxS));
    s[STempMin] = clampS8(tMin, TempLo, TempMinHi);
    s[SEndAhPct] = (uint8_t)clampU16(v.ahPct, AhPctMax);
}

void copyEffectiveToSteps(const AutoParams::Result& r, uint8_t* page2)
{
    const int32_t* e = r.eff;
    const bool restartOn = ((page2[TsLayout::P2StartBits] & 0x04U) != 0U);
    const int32_t tMax = e[AutoParams::ChargeTempMax];
    const int32_t tMin = e[AutoParams::ChargeTempMin];
    StepVals v;
    uint8_t* s = page2 + TsLayout::P2StepBase;

    /* s1 bulk: CC bulkAmp, spanningstak absorb, slut U >= absorb */
    v.method = MethodCC; v.onEnd = EndNext;
    v.current = e[AutoParams::BulkAmp]; v.volt = e[AutoParams::AbsorbVolt];
    v.endVoltMax = e[AutoParams::AbsorbVolt]; v.endVoltMin = 0; v.endAmpMin = 0;
    v.maxTime = e[AutoParams::BulkMaxTime]; v.minTime = 0; v.ahPct = e[AutoParams::BulkAhPct];
    writeStep(s, v, tMax, tMin);
    s += TsLayout::P2StepSize;

    /* s2 absorption: CV absorb, strommtak bulk, slut I <= tail */
    v.method = MethodCV; v.onEnd = EndNext;
    v.current = e[AutoParams::BulkAmp]; v.volt = e[AutoParams::AbsorbVolt];
    v.endVoltMax = 0; v.endVoltMin = 0; v.endAmpMin = e[AutoParams::TailAmp];
    v.maxTime = e[AutoParams::AbsorbMaxTime]; v.minTime = e[AutoParams::AbsorbMinTime]; v.ahPct = 0;
    writeStep(s, v, tMax, tMin);
    s += TsLayout::P2StepSize;

    /* s3 float: CV float, strommtak floatAmp */
    v.method = MethodCV; v.onEnd = EndNext;
    v.current = e[AutoParams::FloatAmp]; v.volt = e[AutoParams::FloatVolt];
    v.endVoltMax = 0; v.endVoltMin = 0; v.endAmpMin = 0;
    v.maxTime = e[AutoParams::FloatMaxTime]; v.minTime = 0; v.ahPct = 0;
    writeStep(s, v, tMax, tMin);
    s += TsLayout::P2StepSize;

    /* s4 overvaka */
    v.method = restartOn ? MethodMonitor : MethodOff;
    v.onEnd = restartOn ? EndGoto1 : EndStopDone;
    v.current = 0; v.volt = 0; v.endVoltMax = 0;
    v.endVoltMin = restartOn ? e[AutoParams::RestartVolt] : 0;
    v.endAmpMin = 0; v.maxTime = 0; v.minTime = 0; v.ahPct = 0;
    writeStep(s, v, tMax, tMin);
}

void refreshInit(RefreshHold& h)
{
    h.bits = 0U;
    h.sinceMs = 0U;
}

void refreshSet(RefreshHold& h, uint8_t bits, uint32_t nowMs)
{
    if (bits != 0U)
    {
        h.bits = (uint8_t)(h.bits | bits);
        h.sinceMs = nowMs;
    }
}

uint8_t refreshTick(RefreshHold& h, uint32_t nowMs, uint32_t holdMs)
{
    if ((h.bits != 0U) && ((uint32_t)(nowMs - h.sinceMs) >= holdMs))
    {
        h.bits = 0U;
    }
    return h.bits;
}

/* Klampar U16 vid p+o om [off, off+len) tacker faltet helt. */
static bool clampField(uint8_t* p, uint16_t o, uint16_t off, uint16_t len, uint16_t hi)
{
    bool changed = false;
    if ((off <= o) && ((uint32_t)o + 2UL <= (uint32_t)off + (uint32_t)len))
    {
        if (rdU16(p + o) > hi)
        {
            wrU16(p + o, hi);
            changed = true;
        }
    }
    return changed;
}

bool clampHwLimits(uint8_t pageIdx, uint8_t* page, uint16_t off, uint16_t len)
{
    static const uint8_t voltIds[6] = {
        AutoParams::AbsorbVolt, AutoParams::FloatVolt, AutoParams::RestartVolt,
        AutoParams::PrechargeVolt, AutoParams::VoltLimit, AutoParams::BatteryPresentVolt
    };
    bool changed = false;
    uint8_t i;
    if (pageIdx == TsLayout::PageIdx1)
    {
        changed = clampField(page, EepromLayout::P1MaxChargeVolt, off, len, HwLimits::HwVoltMaxCv);
    }
    else if (pageIdx == TsLayout::PageIdx5)
    {
        for (i = 0U; i < 6U; ++i)
        {
            if (clampField(page, AutoParams::manOffset(voltIds[i]), off, len, HwLimits::HwVoltMaxCv))
            {
                changed = true;
            }
        }
    }
    else
    {
        /* ovriga sidor: inga hardvarutak har */
    }
    return changed;
}

uint8_t apply(uint8_t code, uint8_t* page5, uint8_t* page2, const AutoParams::Input& in)
{
    uint8_t refresh = 0U;
    if (code == 7U)
    {
        AutoParams::Result r;
        AutoParams::compute(page5, in, r, true);
        AutoParams::copyEffectiveToManual(r, page5);
        refresh = RefreshPage5;
    }
    else if (code == 9U)
    {
        page5[TsLayout::P5OvrMask] = 0U;
        page5[TsLayout::P5OvrMask + 1U] = 0U;
        page5[TsLayout::P5OvrMask + 2U] = 0U;
        refresh = RefreshPage5;
    }
    else if (code == 8U)
    {
        /* effektiva varden MED override (man_* dar ovr = Manuell) */
        AutoParams::Result r;
        AutoParams::compute(page5, in, r);
        if ((r.warn & (AutoParams::WarnAhMissing | AutoParams::WarnNomvInvalid)) == 0U)
        {
            copyEffectiveToSteps(r, page2);
            refresh = RefreshPage2;
        }
    }
    else
    {
        /* ovriga: ingen andring */
    }
    return refresh;
}

} /* namespace PageCommands */
