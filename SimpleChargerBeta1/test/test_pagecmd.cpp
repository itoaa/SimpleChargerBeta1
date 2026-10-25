/*
 * test_pagecmd.cpp - vardtester (PC) for kopieringsknapparna E7..E9
 * (shared/pageCommands). Kontrollerar att bara RAM-sidan andras, att
 * ratt pageRefreshReq-bit returneras, E8-mappningen (BESLUT nr 40) och
 * pageRefreshReq-hallningen.
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

/* BESLUT nr 39: givartypen fran boot-vardet, inte fran sida 3 */
static void testAutoInputBoot()
{
    EepromStore s(be());
    setup(s);
    setU16(s.page(PageIdx1) + EepromLayout::P1MaxChargeAmp, 1200U);
    s.page(PageIdx3)[P3SensorBits] = 0x10U;   /* sidan: 1 */
    CHECK(PageCommands::sensorSel(s.page(PageIdx3)) == 1U);
    s.page(PageIdx3)[P3SensorBits] = 0xFFU;
    CHECK(PageCommands::sensorSel(s.page(PageIdx3)) == 3U);
    AutoParams::Input in;
    PageCommands::autoInputBoot(s.page(PageIdx1), 2U, in);
    CHECK(in.currentSensor == 2U);
    CHECK(in.maxChargeAmp == 1200U && in.maxChargeVolt == 1500U && !in.batSensor);
    PageCommands::autoInputBoot(s.page(PageIdx1), 0xF1U, in);   /* bara bit 0..1 */
    CHECK(in.currentSensor == 1U);
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
    CHECK(PageCommands::apply(7U, p5, s.page(PageIdx2), in) == PageCommands::RefreshPage5);
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
    CHECK(PageCommands::apply(9U, p5, s.page(PageIdx2), in) == PageCommands::RefreshPage5);
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

static void testOthersNoChange()
{
    EepromStore s(be());
    setup(s);
    AutoParams::Input in;
    PageCommands::autoInput(s.page(PageIdx1), s.page(PageIdx3), in);
    uint8_t before[EepromLayout::Page5Size];
    memcpy(before, s.page(PageIdx5), sizeof(before));
    uint8_t p2[EepromLayout::Page2Size];
    memcpy(p2, s.page(PageIdx2), sizeof(p2));
    CHECK(PageCommands::apply(1U, s.page(PageIdx5), s.page(PageIdx2), in) == 0U);
    CHECK(PageCommands::apply(0U, s.page(PageIdx5), s.page(PageIdx2), in) == 0U);
    CHECK(PageCommands::apply(10U, s.page(PageIdx5), s.page(PageIdx2), in) == 0U);
    CHECK(memcmp(before, s.page(PageIdx5), sizeof(before)) == 0);
    CHECK(memcmp(p2, s.page(PageIdx2), sizeof(p2)) == 0);
}

/* ---- E8 (forslag 2, BESLUT nr 40) ---- */
static uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static const uint8_t* stepP(EepromStore& s, uint8_t n) { return s.page(PageIdx2) + P2StepBase + P2StepSize * (n - 1U); }

struct StepExp
{
    uint8_t method, onEnd;
    int32_t current, volt, endVoltMax, endVoltMin, endAmpMax, endAmpMin, maxTime, minTime, tempMax, tempMin, ahPct;
};

static int32_t clampI(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : (v > hi ? hi : v); }

static bool stepIs(const uint8_t* p, const StepExp& e)
{
    bool ok = true;
    ok = ok && ((p[0] & 3U) == e.method) && (((p[0] >> 2) & 3U) == e.onEnd) && ((p[0] & 0xF0U) == 0U);
    ok = ok && ((int8_t)p[1] == clampI(e.tempMax, -40, 80));
    ok = ok && (rd16(p + 2) == clampI(e.current, 0, 3000));
    ok = ok && (rd16(p + 4) == clampI(e.volt, 0, 6000));
    ok = ok && (rd16(p + 6) == clampI(e.endVoltMax, 0, 6000));
    ok = ok && (rd16(p + 8) == clampI(e.endVoltMin, 0, 6000));
    ok = ok && (rd16(p + 10) == clampI(e.endAmpMax, 0, 3000));
    ok = ok && (rd16(p + 12) == clampI(e.endAmpMin, 0, 3000));
    ok = ok && (rd16(p + 14) == clampI(e.maxTime, 0, 6000));
    ok = ok && (rd16(p + 16) == clampI(e.minTime, 0, 65535));
    ok = ok && ((int8_t)p[18] == clampI(e.tempMin, -40, 60));
    ok = ok && (p[19] == clampI(e.ahPct, 0, 200));
    return ok;
}

/* forvantad tabell direkt fran forslag 2, alternativ A */
static void expectTable(EepromStore& s, const AutoParams::Result& r, bool restartOn)
{
    const int32_t* e = r.eff;
    const int32_t tx = e[AutoParams::ChargeTempMax], tn = e[AutoParams::ChargeTempMin];
    StepExp s1 = { 0U, 0U, e[AutoParams::BulkAmp], e[AutoParams::AbsorbVolt], e[AutoParams::AbsorbVolt], 0, 0, 0,
                   e[AutoParams::BulkMaxTime], 0, tx, tn, e[AutoParams::BulkAhPct] };
    StepExp s2 = { 1U, 0U, e[AutoParams::BulkAmp], e[AutoParams::AbsorbVolt], 0, 0, 0, e[AutoParams::TailAmp],
                   e[AutoParams::AbsorbMaxTime], e[AutoParams::AbsorbMinTime], tx, tn, 0 };
    StepExp s3 = { 1U, 0U, e[AutoParams::FloatAmp], e[AutoParams::FloatVolt], 0, 0, 0, 0,
                   e[AutoParams::FloatMaxTime], 0, tx, tn, 0 };
    StepExp s4 = { (uint8_t)(restartOn ? 2U : 3U), (uint8_t)(restartOn ? 1U : 2U), 0, 0, 0,
                   restartOn ? e[AutoParams::RestartVolt] : 0, 0, 0, 0, 0, tx, tn, 0 };
    CHECK(stepIs(stepP(s, 1U), s1));
    CHECK(stepIs(stepP(s, 2U), s2));
    CHECK(stepIs(stepP(s, 3U), s3));
    CHECK(stepIs(stepP(s, 4U), s4));
}

/* 1 + 6: standard -> tabellen, byte 0..3 och sidorna 1,3,4,5 orord, 0x02, ingen EEPROM */
static void testE8Default()
{
    EepromStore s(be());
    setup(s);
    AutoParams::Input in;
    PageCommands::autoInputBoot(s.page(PageIdx1), 0U, in);
    uint8_t snap[EepromLayout::PagesTotalSize];
    memcpy(snap, s.page(PageIdx1), sizeof(snap));     /* sidorna ligger i foljd i RAM */
    AutoParams::Result r;
    AutoParams::compute(s.page(PageIdx5), in, r);
    CHECK((r.warn & (AutoParams::WarnAhMissing | AutoParams::WarnNomvInvalid)) == 0U);
    g_writes = 0;
    CHECK(PageCommands::apply(8U, s.page(PageIdx5), s.page(PageIdx2), in) == PageCommands::RefreshPage2);
    CHECK(g_writes == 0);
    const bool restartOn = (s.page(PageIdx2)[P2StartBits] & 4U) != 0U;
    expectTable(s, r, restartOn);
    /* allt utom sida 2 byte 4..83 oforandrat */
    uint16_t i;
    int other = 0, inSteps = 0;
    const uint16_t p2off = EepromLayout::Page2RamOffset - EepromLayout::Page1RamOffset;
    for (i = 0U; i < EepromLayout::PagesTotalSize; ++i)
    {
        const bool step = (i >= p2off + P2StepBase) && (i < p2off + EepromLayout::Page2Size);
        if (snap[i] != s.page(PageIdx1)[i]) { if (step) { ++inSteps; } else { ++other; } }
    }
    CHECK(other == 0);
    CHECK(inSteps > 0);
    /* idempotent: andra E8 ger samma sida */
    uint8_t p2[EepromLayout::Page2Size];
    memcpy(p2, s.page(PageIdx2), sizeof(p2));
    CHECK(PageCommands::apply(8U, s.page(PageIdx5), s.page(PageIdx2), in) == PageCommands::RefreshPage2);
    CHECK(memcmp(p2, s.page(PageIdx2), sizeof(p2)) == 0);
}

/* 2: manuell bulkAmp 25 A med ovr -> det effektiva (begransade) vardet i s1/s2 */
static void testE8ManualOverride()
{
    EepromStore s(be());
    setup(s);
    setU16(s.page(PageIdx1) + EepromLayout::P1MaxChargeAmp, 1500U);
    uint8_t* p5 = s.page(PageIdx5);
    const uint8_t id = AutoParams::BulkAmp;
    p5[P5OvrMask + id / 8U] = (uint8_t)(p5[P5OvrMask + id / 8U] | (1U << (id % 8U)));
    setU16(p5 + AutoParams::manOffset(id), 2500U);
    AutoParams::Input in;
    PageCommands::autoInputBoot(s.page(PageIdx1), 2U, in);   /* 30 A-givare */
    AutoParams::Result r;
    AutoParams::compute(p5, in, r);
    CHECK(r.eff[AutoParams::BulkAmp] == 1500);
    uint8_t ovr[3]; memcpy(ovr, p5 + P5OvrMask, 3);
    CHECK(PageCommands::apply(8U, p5, s.page(PageIdx2), in) == PageCommands::RefreshPage2);
    CHECK(rd16(stepP(s, 1U) + 2) == 1500U && rd16(stepP(s, 2U) + 2) == 1500U);
    CHECK(memcmp(ovr, p5 + P5OvrMask, 3) == 0 && rd16(p5 + AutoParams::manOffset(id)) == 2500U);
    /* utan override ar vardet ett annat (autovardet) -> override paverkade */
    AutoParams::Result ra;
    AutoParams::compute(p5, in, ra, true);
    printf("  E8 override: auto bulk %ld, eff %ld\n", (long)ra.eff[AutoParams::BulkAmp], (long)r.eff[AutoParams::BulkAmp]);
}

/* 3: restartMode av/pa -> s4 */
static void testE8Restart()
{
    EepromStore s(be());
    setup(s);
    AutoParams::Input in;
    PageCommands::autoInputBoot(s.page(PageIdx1), 0U, in);
    AutoParams::Result r;
    AutoParams::compute(s.page(PageIdx5), in, r);
    uint8_t* p2 = s.page(PageIdx2);
    p2[P2StartBits] = (uint8_t)(p2[P2StartBits] & ~4U);
    CHECK(PageCommands::apply(8U, s.page(PageIdx5), p2, in) == PageCommands::RefreshPage2);
    expectTable(s, r, false);
    CHECK((stepP(s, 4U)[0] & 0x0FU) == (3U | (2U << 2)) && rd16(stepP(s, 4U) + 8) == 0U);
    p2[P2StartBits] = (uint8_t)(p2[P2StartBits] | 4U);
    CHECK(PageCommands::apply(8U, s.page(PageIdx5), p2, in) == PageCommands::RefreshPage2);
    expectTable(s, r, true);
    CHECK((stepP(s, 4U)[0] & 0x0FU) == (2U | (1U << 2)));
    CHECK(rd16(stepP(s, 4U) + 8) == (uint16_t)r.eff[AutoParams::RestartVolt] && r.eff[AutoParams::RestartVolt] > 0);
    CHECK((p2[P2StartBits] & 4U) != 0U);                       /* byte 0 orord */
}

/* 4: klampning till faltens intervall (syntetiskt Result) */
static void testE8Clamp()
{
    uint8_t p2[EepromLayout::Page2Size];
    memset(p2, 0xA5, sizeof(p2));
    p2[P2StartBits] = 0x04U;
    AutoParams::Result r;
    memset(&r, 0, sizeof(r));
    r.eff[AutoParams::BulkAhPct] = 250;
    r.eff[AutoParams::BulkAmp] = 5000;
    r.eff[AutoParams::AbsorbVolt] = 7000;
    r.eff[AutoParams::FloatAmp] = -5;
    r.eff[AutoParams::FloatVolt] = -1;
    r.eff[AutoParams::TailAmp] = 4000;
    r.eff[AutoParams::BulkMaxTime] = 9000;
    r.eff[AutoParams::AbsorbMinTime] = 70000;
    r.eff[AutoParams::RestartVolt] = 6500;
    r.eff[AutoParams::ChargeTempMax] = 90;
    r.eff[AutoParams::ChargeTempMin] = 70;
    PageCommands::copyEffectiveToSteps(r, p2);
    const uint8_t* s1 = p2 + P2StepBase;
    const uint8_t* s2 = s1 + P2StepSize;
    const uint8_t* s3 = s2 + P2StepSize;
    const uint8_t* s4 = s3 + P2StepSize;
    CHECK(s1[19] == 200U);
    CHECK(rd16(s1 + 2) == 3000U && rd16(s1 + 4) == 6000U && rd16(s1 + 6) == 6000U && rd16(s1 + 14) == 6000U);
    CHECK(rd16(s2 + 12) == 3000U && rd16(s2 + 16) == 65535U);
    CHECK(rd16(s3 + 2) == 0U && rd16(s3 + 4) == 0U);
    CHECK(rd16(s4 + 8) == 6000U);
    CHECK((int8_t)s1[1] == 80 && (int8_t)s1[18] == 60);
    CHECK(p2[0] == 0x04U && p2[1] == 0xA5U && p2[2] == 0xA5U && p2[3] == 0xA5U);   /* byte 0..3 orord */
    r.eff[AutoParams::ChargeTempMax] = -60;
    r.eff[AutoParams::ChargeTempMin] = -50;
    PageCommands::copyEffectiveToSteps(r, p2);
    CHECK((int8_t)s4[1] == -40 && (int8_t)s4[18] == -40);
}

/* 5: AH_MISSING / NOMV_INVALID -> ingen andring, 0 */
static void testE8Blocked()
{
    EepromStore s(be());
    setup(s);
    AutoParams::Input in;
    PageCommands::autoInputBoot(s.page(PageIdx1), 0U, in);
    uint8_t* p5 = s.page(PageIdx5);
    uint8_t p2[EepromLayout::Page2Size];
    memcpy(p2, s.page(PageIdx2), sizeof(p2));
    setU16(p5 + P5CapacityAh, 0U);
    AutoParams::Result r;
    AutoParams::compute(p5, in, r);
    CHECK((r.warn & AutoParams::WarnAhMissing) != 0U);
    CHECK(PageCommands::apply(8U, p5, s.page(PageIdx2), in) == 0U);
    CHECK(memcmp(p2, s.page(PageIdx2), sizeof(p2)) == 0);

    setup(s);
    memcpy(p2, s.page(PageIdx2), sizeof(p2));
    p5[P5TypeBits] = (uint8_t)((p5[P5TypeBits] & 0x07U) | (7U << 3));   /* nomV utanfor tabellen */
    AutoParams::compute(p5, in, r);
    CHECK((r.warn & AutoParams::WarnNomvInvalid) != 0U);
    CHECK(PageCommands::apply(8U, p5, s.page(PageIdx2), in) == 0U);
    CHECK(memcmp(p2, s.page(PageIdx2), sizeof(p2)) == 0);
}

/* 7: pageRefreshReq-hallning ca 1 s (samma hjalpare som main anvander) */
static void testRefreshHold()
{
    PageCommands::RefreshHold h;
    PageCommands::refreshInit(h);
    CHECK(PageCommands::refreshTick(h, 500U, 1000U) == 0U);
    PageCommands::refreshSet(h, 0U, 600U);                       /* 0 -> ingen andring */
    CHECK(PageCommands::refreshTick(h, 700U, 1000U) == 0U);
    PageCommands::refreshSet(h, PageCommands::RefreshPage2, 1000U);
    CHECK(PageCommands::refreshTick(h, 1000U, 1000U) == 0x02U);
    CHECK(PageCommands::refreshTick(h, 1999U, 1000U) == 0x02U);
    CHECK(PageCommands::refreshTick(h, 2000U, 1000U) == 0U);
    CHECK(PageCommands::refreshTick(h, 2100U, 1000U) == 0U);
    /* E8 och sedan E7 inom 1 s: bada bitarna, tiden raknas fran den senaste */
    PageCommands::refreshSet(h, PageCommands::RefreshPage2, 3000U);
    PageCommands::refreshSet(h, PageCommands::RefreshPage5, 3500U);
    CHECK(PageCommands::refreshTick(h, 4000U, 1000U) == 0x12U);
    CHECK(PageCommands::refreshTick(h, 4499U, 1000U) == 0x12U);
    CHECK(PageCommands::refreshTick(h, 4500U, 1000U) == 0U);
    /* millis-overslag */
    PageCommands::refreshSet(h, PageCommands::RefreshPage2, 0xFFFFFF00UL);
    CHECK(PageCommands::refreshTick(h, 0x000002E7UL, 1000U) == 0x02U);
    CHECK(PageCommands::refreshTick(h, 0x000002E8UL, 1000U) == 0U);
}

/* 8: matris -> varje falt inom ini-intervallet, metod/onEnd giltiga, byte 0..3 orord */
static void testE8Matrix()
{
    static const uint16_t caps[] = { 1U, 50U, 1000U, 20000U, 65535U };
    static const uint16_t mcas[] = { 0U, 300U, 1500U, 3000U };
    static const uint16_t mcvs[] = { 0U, 1500U, 6000U };
    long runs = 0, bad = 0, blocked = 0;
    EepromStore s(be());
    for (uint8_t type = 0U; type < 8U; ++type)
    for (uint8_t nomv = 0U; nomv < 8U; ++nomv)
    for (uint8_t ci = 0U; ci < 5U; ++ci)
    for (uint8_t mi = 0U; mi < 4U; ++mi)
    for (uint8_t vi = 0U; vi < 3U; ++vi)
    for (uint8_t sens = 0U; sens < 4U; ++sens)
    for (uint8_t ovr = 0U; ovr < 2U; ++ovr)
    {
        s.loadDefaults();
        uint8_t* p5 = s.page(PageIdx5);
        p5[P5TypeBits] = (uint8_t)(type | (nomv << 3));
        setU16(p5 + P5CapacityAh, caps[ci]);
        setU16(s.page(PageIdx1) + EepromLayout::P1MaxChargeAmp, mcas[mi]);
        setU16(s.page(PageIdx1) + EepromLayout::P1MaxChargeVolt, mcvs[vi]);
        if (ovr != 0U)
        {
            /* alla override, extrema man_*: 0xFF.. */
            p5[P5OvrMask] = 0xFFU; p5[P5OvrMask + 1U] = 0xFFU; p5[P5OvrMask + 2U] = 0x1FU;
            for (uint8_t id = 0U; id < AutoParams::ParamCount; ++id)
            {
                if (AutoParams::manType(id) == 2U) { setU16(p5 + AutoParams::manOffset(id), (uint16_t)(0xFFFFU - id)); }
                else { p5[AutoParams::manOffset(id)] = (uint8_t)(0x80U + id); }
            }
        }
        uint8_t head[4]; memcpy(head, s.page(PageIdx2), 4);
        AutoParams::Input in;
        PageCommands::autoInputBoot(s.page(PageIdx1), sens, in);
        const uint8_t rb = PageCommands::apply(8U, p5, s.page(PageIdx2), in);
        ++runs;
        if (rb == 0U) { ++blocked; continue; }
        bool ok = (rb == PageCommands::RefreshPage2) && (memcmp(head, s.page(PageIdx2), 4) == 0);
        for (uint8_t n = 1U; n <= 4U; ++n)
        {
            const uint8_t* p = stepP(s, n);
            ok = ok && ((p[0] & 0xF0U) == 0U) && (((p[0] >> 2) & 3U) <= 2U);
            ok = ok && ((int8_t)p[1] >= -40) && ((int8_t)p[1] <= 80);
            ok = ok && (rd16(p + 2) <= 3000U) && (rd16(p + 4) <= 6000U) && (rd16(p + 6) <= 6000U);
            ok = ok && (rd16(p + 8) <= 6000U) && (rd16(p + 10) <= 3000U) && (rd16(p + 12) <= 3000U);
            ok = ok && (rd16(p + 14) <= 6000U);
            ok = ok && ((int8_t)p[18] >= -40) && ((int8_t)p[18] <= 60) && (p[19] <= 200U);
        }
        if (!ok) { ++bad; }
    }
    printf("  E8-matris: %ld korningar, %ld blockerade (AH_MISSING/NOMV_INVALID), %ld utanfor intervall\n", runs, blocked, bad);
    CHECK(bad == 0);
    CHECK(runs - blocked > 1000);
}

int main()
{
    testAutoInput();
    testAutoInputBoot();
    testE7CopiesAutoValues();
    testE9ClearsOverrides();
    testOthersNoChange();
    testE8Default();
    testE8ManualOverride();
    testE8Restart();
    testE8Clamp();
    testE8Blocked();
    testRefreshHold();
    testE8Matrix();
    printf("test_pagecmd: %d kontroller, %d fel\n", g_checks, g_fails);
    return g_fails == 0 ? 0 : 1;
}
