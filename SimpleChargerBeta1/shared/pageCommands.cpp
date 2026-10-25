/*
 * pageCommands.cpp - se pageCommands.hpp.
 */
#include "pageCommands.hpp"
#include "eepromLayout.hpp"
#include "tsLayout.hpp"

namespace PageCommands {

static uint16_t rdU16(const uint8_t* p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

void autoInput(const uint8_t* page1, const uint8_t* page3, AutoParams::Input& in)
{
    in.maxChargeAmp = rdU16(page1 + EepromLayout::P1MaxChargeAmp);
    in.maxChargeVolt = rdU16(page1 + EepromLayout::P1MaxChargeVolt);
    in.currentSensor = (uint8_t)((page3[TsLayout::P3SensorBits] >> 4) & 0x03U);
    in.batSensor = false;   /* etapp 6 (givare) ej implementerad */
}

uint8_t apply(uint8_t code, uint8_t* page5, const AutoParams::Input& in)
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
    else
    {
        /* E8 och ovriga: ingen andring (se hpp) */
    }
    return refresh;
}

} /* namespace PageCommands */
