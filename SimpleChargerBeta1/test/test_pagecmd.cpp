/*
 * test_pagecmd.cpp - vardtester (PC) for kopieringsknapparna E7..E9
 * (shared/pageCommands). Kontrollerar att bara RAM-sidan andras, att
 * ratt pageRefreshReq-bit returneras och att E8 inte gor nagot.
 */
#include <stdio.h>
#include <string.h>
#include "pageCommands.hpp"
#include "eepromStore.hpp"
#include "eepromLayout.hpp"
#include "tsLayout.hpp"

using namespace TsLayout;

static int g_checks = 0;
static int g_fails = 0;

#define CHECK(cond) do { ++g_checks; if (!(cond)) { ++g_fails; \
    printf("FEL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static int g_writes = 0;
static void nopRead(uint16_t, uint8_t* d, uint16_t n, void*) { memset(d, 0xFF, n); }
static void cntUpdate(uint16_t, const uint8_t*, uint16_t, void*) { ++g_writes; }

static EepromBackend be()
{
    EepromBackend b; b.read = nopRead; b.update = cntUpdate; b.ctx = 0; return b;
}

static void setU16(uint8_t* p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

static void setup(EepromStore& s)
{
    s.loadDefaults();
    setU16(s.page(PageIdx1) + EepromLayout::P1MaxChargeVolt, 1500U);
}

static void testAutoInput()
{
    EepromStore s(be());
    setup(s);
    setU16(s.page(PageIdx1) + EepromLayout::P1MaxChargeAmp, 1200U);
    s.page(PageIdx3)[P3SensorBits] = 0x20U;   /* currentSensor = 2 (30 A) */
    AutoParams::Input in;
    PageCommands::autoInput(s.page(PageIdx1), s.page(PageIdx3), in);
    CHECK(in.maxChargeAmp == 1200U);
    CHECK(in.maxChargeVolt == 1500U);
    CHECK(in.currentSensor == 2U);
    CHECK(!in.batSensor);
}

static void testE7CopiesAutoValues()
{
    EepromStore s(be());
    setup(s);
    uint8_t* p5 = s.page(PageIdx5);
    AutoParams::Input in;
    PageCommands::autoInput(s.page(PageIdx1), s.page(PageIdx3), in);

    /* En override (FloatMaxTime) med eget varde: E7 ska skriva over man_*
     * med autovardet men lamna ovr-biten. */
    const uint8_t id = AutoParams::FloatMaxTime;
    p5[P5OvrMask + id / 8U] = (uint8_t)(p5[P5OvrMask + id / 8U] | (1U << (id % 8U)));
    setU16(p5 + AutoParams::manOffset(id), 30U);
    uint8_t ovrBefore[3];
    memcpy(ovrBefore, p5 + P5OvrMask, 3);

    AutoParams::Result autoOnly;
    AutoParams::compute(p5, in, autoOnly, true);

    g_writes = 0;
    CHECK(PageCommands::apply(7U, p5, in) == PageCommands::RefreshPage5);
    CHECK(g_writes == 0);                                   /* ingen burn */
    CHECK(memcmp(ovrBefore, p5 + P5OvrMask, 3) == 0);       /* ovr orord */

    /* Med alla ovr satta ger man_* samma effektiva varden som auto. */
    p5[P5OvrMask] = 0xFFU; p5[P5OvrMask + 1U] = 0xFFU; p5[P5OvrMask + 2U] = 0x1FU;
    AutoParams::Result r;
    AutoParams::compute(p5, in, r);
    uint8_t i;
    int diff = 0;
    for (i = 0U; i < AutoParams::ParamCount; ++i)
    {
        if (r.eff[i] != autoOnly.eff[i]) { ++diff; printf("  param %u: %ld != %ld\n", i, (long)r.eff[i], (long)autoOnly.eff[i]); }
    }
    CHECK(diff == 0);
}

static void testE9ClearsOverrides()
{
    EepromStore s(be());
    setup(s);
    uint8_t* p5 = s.page(PageIdx5);
    AutoParams::Input in;
    PageCommands::autoInput(s.page(PageIdx1), s.page(PageIdx3), in);
    p5[P5OvrMask] = 0xA5U; p5[P5OvrMask + 1U] = 0x5AU; p5[P5OvrMask + 2U] = 0x11U;
    uint8_t before[EepromLayout::Page5Size];
    memcpy(before, p5, sizeof(before));
    g_writes = 0;
    CHECK(PageCommands::apply(9U, p5, in) == PageCommands::RefreshPage5);
    CHECK(g_writes == 0);
    CHECK(p5[P5OvrMask] == 0U && p5[P5OvrMask + 1U] == 0U && p5[P5OvrMask + 2U] == 0U);
    /* Allt annat i sidan oforandrat. */
    uint16_t k;
    int other = 0;
    for (k = 0U; k < EepromLayout::Page5Size; ++k)
    {
        if ((k >= P5OvrMask) && (k < P5OvrMask + 3U)) { continue; }
        if (p5[k] != before[k]) { ++other; }
    }
    CHECK(other == 0);
}

static void testE8AndOthersNoChange()
{
    EepromStore s(be());
    setup(s);
    AutoParams::Input in;
    PageCommands::autoInput(s.page(PageIdx1), s.page(PageIdx3), in);
    uint8_t before[EepromLayout::Page5Size];
    memcpy(before, s.page(PageIdx5), sizeof(before));
    uint8_t p2[EepromLayout::Page2Size];
    memcpy(p2, s.page(PageIdx2), sizeof(p2));
    CHECK(PageCommands::apply(8U, s.page(PageIdx5), in) == 0U);
    CHECK(PageCommands::apply(1U, s.page(PageIdx5), in) == 0U);
    CHECK(PageCommands::apply(0U, s.page(PageIdx5), in) == 0U);
    CHECK(PageCommands::apply(10U, s.page(PageIdx5), in) == 0U);
    CHECK(memcmp(before, s.page(PageIdx5), sizeof(before)) == 0);
    CHECK(memcmp(p2, s.page(PageIdx2), sizeof(p2)) == 0);
}

int main()
{
    testAutoInput();
    testE7CopiesAutoValues();
    testE9ClearsOverrides();
    testE8AndOthersNoChange();
    printf("test_pagecmd: %d kontroller, %d fel\n", g_checks, g_fails);
    return g_fails == 0 ? 0 : 1;
}
