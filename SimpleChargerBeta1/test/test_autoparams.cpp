/*
 * test_autoparams.cpp - vardtester for AutoParams (port av auto_params_v2.2.py).
 * Golden-vektorerna (build/golden_auto.inc) genereras av gen_auto_golden.py
 * fran referensen vid varje testbygge. Alla falt jamfors bit-exakt.
 */
#include <stdio.h>
#include <string.h>
#include "autoParams.hpp"
#include "tsLayout.hpp"
#include "eepromStore.hpp"
#include "golden_auto.inc"

static int g_checks = 0;
static int g_fails = 0;
static int g_reported = 0;

#define CHECK(cond) do { ++g_checks; if (!(cond)) { ++g_fails; \
    if (g_reported++ < 20) { printf("FEL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } } while (0)

using namespace AutoParams;

/* Bygg sida 5 som TS skulle skicka den. */
static void buildPage5(uint8_t* p5, const int32_t* c)
{
    memset(p5, 0, EepromLayout::Page5Size);
    p5[TsLayout::P5ModeBits] = (uint8_t)((c[5] != 0) ? 0x04U : 0x00U);
    p5[TsLayout::P5TypeBits] = (uint8_t)((c[0] & 0x07) | ((c[1] & 0x07) << 3));
    scPutU16le(p5 + TsLayout::P5CapacityAh, (uint16_t)c[2]);
    scPutU16le(p5 + TsLayout::P5AutoAmpCap, (uint16_t)c[7]);
    scPutU16le(p5 + TsLayout::P5TailAmpFloor, (uint16_t)c[8]);
    p5[TsLayout::P5OvrMask]      = (uint8_t)(c[3] & 0xFF);
    p5[TsLayout::P5OvrMask + 1U] = (uint8_t)((c[3] >> 8) & 0xFF);
    p5[TsLayout::P5OvrMask + 2U] = (uint8_t)((c[3] >> 16) & 0xFF);
    const int32_t* man = kGoldenMan[c[4]];
    for (uint8_t i = 0U; i < ParamCount; ++i) {
        uint8_t* q = p5 + manOffset(i);
        if (manType(i) == 2U) { scPutU16le(q, (uint16_t)man[i]); }
        else { q[0] = (uint8_t)(man[i] & 0xFF); }
    }
}

static Result g_res[kGoldenAutoCount];

static void testGoldenEffective()
{
    uint8_t p5[EepromLayout::Page5Size];
    unsigned bad = 0U;
    for (unsigned k = 0U; k < kGoldenAutoCount; ++k) {
        const int32_t* c = kGoldenAuto[k];
        buildPage5(p5, c);
        Input in;
        in.maxChargeAmp  = (uint16_t)c[9];
        in.maxChargeVolt = (uint16_t)c[10];
        in.currentSensor = (uint8_t)c[11];
        in.batSensor     = (c[6] != 0);
        Result& r = g_res[k];
        compute(p5, in, r);
        bool ok = (r.manualMask == (uint32_t)c[33]) && (r.warn == (uint16_t)c[34]);
        for (uint8_t i = 0U; i < ParamCount; ++i) {
            if (r.eff[i] != c[12 + i]) { ok = false; }
        }
        if (!ok && bad++ < 5U) {
            printf("golden %u: typ %d nomv %d ah %d ovr 0x%X warn 0x%X/0x%X mask 0x%X/0x%X\n", k,
                   (int)c[0], (int)c[1], (int)c[2], (unsigned)c[3], r.warn, (unsigned)c[34],
                   (unsigned)r.manualMask, (unsigned)c[33]);
            for (uint8_t i = 0U; i < ParamCount; ++i) {
                if (r.eff[i] != c[12 + i]) { printf("  param %u: %ld != %ld\n", i, (long)r.eff[i], (long)c[12 + i]); }
            }
        }
        CHECK(ok);
    }
}

static void testGoldenComp()
{
    for (unsigned k = 0U; k < kGoldenCompCount; ++k) {
        const int32_t* c = kGoldenComp[k];
        CHECK(compTarget(c[1], g_res[c[0]], (int16_t)c[2], c[3] != 0) == c[4]);
    }
    for (unsigned k = 0U; k < kGoldenCompSynCount; ++k) {
        const int32_t* c = kGoldenCompSyn[k];
        Result r;
        memset(&r, 0, sizeof(r));
        r.eff[TempCompMv] = c[0];
        r.eff[CellCount] = c[1];
        r.eff[VoltLimit] = 65535;
        CHECK(compTarget(c[2], r, (int16_t)c[3], true) == c[4]);
    }
}

/* Selftest-vardena i auto_params_v2.2.py ska aven galla via standardsidan. */
static void testDefaultsPage()
{
    EepromBackend none = { 0, 0, 0 };
    EepromStore s(none);
    s.loadDefaults();
    Input in;
    in.maxChargeAmp = scGetU16le(s.page(0) + EepromLayout::P1MaxChargeAmp);
    in.maxChargeVolt = 0U;
    in.currentSensor = 1U;
    in.batSensor = false;
    Result r;
    compute(s.page(4), in, r);
    /* Standard: AGM? typ enligt ini-default, 60,0 Ah, 12 V. Kontrollera bara egenskaper. */
    CHECK(r.eff[CellCount] == 6);
    CHECK(r.manualMask == 0UL);
    CHECK((r.warn & WarnTcNoSensor) != 0U);      /* tempkomp pa, ingen givare (etapp 6) */
    CHECK(r.eff[BulkAmp] <= 1500);
}

static void testRealtimeAndCopy()
{
    Result r;
    for (uint8_t i = 0U; i < ParamCount; ++i) { r.eff[i] = (int32_t)(1000 + i); }
    r.eff[TempCompMv] = -40;
    r.eff[CellCount] = 6;
    r.manualMask = 0x12345UL;
    r.warn = 0x0441U;
    uint8_t rt[TsLayout::RtBlockSize];
    memset(rt, 0xEE, sizeof(rt));
    writeRealtime(r, rt);
    CHECK(rt[68] == 6U);
    CHECK(scGetU16le(rt + 69) == 1001U);
    CHECK(scGetS8(rt + 90) == -40);
    CHECK(scGetU16le(rt + 103) == 1020U);
    CHECK(scGetU32le(rt + 105) == 0x12345UL);
    CHECK(scGetU16le(rt + 109) == 0x0441U);
    CHECK(rt[67] == 0xEEU && rt[111] == 0xEEU);   /* inget utanfor 68..110 */

    uint8_t p5[EepromLayout::Page5Size];
    memset(p5, 0xAA, sizeof(p5));
    copyEffectiveToManual(r, p5);
    CHECK(p5[12] == 6U);
    CHECK(scGetU16le(p5 + 13) == 1001U);
    CHECK(scGetS8(p5 + 34) == -40);
    CHECK(scGetU16le(p5 + 47) == 1020U);
    CHECK(p5[8] == 0xAAU && p5[11] == 0xAAU && p5[49] == 0xAAU);   /* ovr/reserv orort */
}

/* E7-semantik: ignoreOverrides ger autovardena aven nar ovr-bitar ar satta. */
static void testIgnoreOverrides()
{
    uint8_t p5[EepromLayout::Page5Size];
    memset(p5, 0, sizeof(p5));
    p5[TsLayout::P5TypeBits] = (uint8_t)(1U | (1U << 3));   /* AGM 12 V */
    scPutU16le(p5 + TsLayout::P5CapacityAh, 400U);
    scPutU16le(p5 + TsLayout::P5AutoAmpCap, 1500U);
    scPutU16le(p5 + TsLayout::P5TailAmpFloor, 30U);
    p5[TsLayout::P5OvrMask] = 0x02U;                       /* bulkAmp manuell */
    scPutU16le(p5 + 13, 500U);
    Input in = { 0U, 0U, 1U, false };
    Result a, b;
    compute(p5, in, a);
    compute(p5, in, b, true);
    CHECK(a.eff[BulkAmp] == 500 && a.eff[BulkMaxTime] == 576);
    CHECK(b.eff[BulkAmp] == 800 && b.manualMask == 0UL && (b.warn & WarnOverride) == 0U);
    CHECK(b.eff[AbsorbVolt] == 1440 && b.eff[TailAmp] == 80 && b.eff[FloatVolt] == 1360);
}

int main()
{
    testGoldenEffective();
    testGoldenComp();
    testDefaultsPage();
    testRealtimeAndCopy();
    testIgnoreOverrides();
    printf("test_autoparams: %d kontroller (%u golden effective, %u golden comp), %d fel\n",
           g_checks, kGoldenAutoCount, kGoldenCompCount + kGoldenCompSynCount, g_fails);
    return (g_fails == 0) ? 0 : 1;
}
