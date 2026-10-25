/*
 * test_scale.cpp - vardtester (PC) for stromskalningen (shared/scale),
 * BESLUT nr 37 (forslag 3). Byggs med -fsanitize=float-cast-overflow
 * utover ASan/UBSan. Inget har sager nagot om hardvaran (ADC, givare).
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "scale.hpp"

static int g_checks = 0;
static int g_fails = 0;

#define CHECK(cond) do { ++g_checks; if (!(cond)) { ++g_fails; \
    printf("FEL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

/* Givarna i sCurrent.cpp: typ 1 = 121 mV/A, 2 = 100, 3 = 66. */
static const int16_t kMvA[3] = { 121, 100, 66 };
static const int16_t RefMv = 4980;      /* konstruktorns varde i sCurrent.cpp */
static const int16_t OffMv = 2483;

/* double-referens (gamla formeln), mattad till +-32767 utan cast-overflow */
static long refMa(uint16_t adc, int ref, int off, int mva)
{
    double v = ((double)adc / 1023.0) * ref;
    double ma = ((v - off) / mva) * 1000.0;
    if (ma > 32767.0) { return 32767L; }
    if (ma < -32767.0) { return -32767L; }
    return (long)ma;
}

/* 1. tabell ADC x typ x offset inom +-15 mA fran referensen */
static void testTable()
{
    static const uint16_t adcs[5] = { 0U, 1U, 511U, 512U, 1023U };
    static const int16_t offs[3] = { 2483, 0, 4980 };
    int maxDiff = 0;
    for (int a = 0; a < 5; ++a)
    {
        for (int t = 0; t < 3; ++t)
        {
            for (int o = 0; o < 3; ++o)
            {
                bool sat = false;
                long got = scaleAcs712MaSat(adcs[a], RefMv, offs[o], kMvA[t], &sat);
                long exp = refMa(adcs[a], RefMv, offs[o], kMvA[t]);
                int d = (int)labs(got - exp);
                if (d > maxDiff) { maxDiff = d; }
                CHECK(d <= 15);
                double raw = ((((double)adcs[a] / 1023.0) * RefMv - offs[o]) / kMvA[t]) * 1000.0;
                CHECK(sat == ((raw > 32767.0) || (raw < -32767.0)));
            }
        }
    }
    /* hela ADC-intervallet, alla typer, standardoffset */
    for (uint16_t adc = 0U; adc <= 1023U; ++adc)
    {
        for (int t = 0; t < 3; ++t)
        {
            long got = scaleAcs712MaSat(adc, RefMv, OffMv, kMvA[t], 0);
            long exp = refMa(adc, RefMv, OffMv, kMvA[t]);
            if (labs(got - exp) > 15L) { ++g_fails; printf("FEL adc %u typ %d: %ld != %ld\n", adc, t + 1, got, exp); }
        }
    }
    ++g_checks;
    printf("  max avvikelse mot double (tabell): %d mA\n", maxDiff);
}

/* 2. typ 3, fullt utslag: mattat och flaggat */
static void testSaturation()
{
    bool sat = false;
    CHECK(scaleAcs712Ma32(1023U, RefMv, OffMv, 66) > 32767L);       /* 37,8 A */
    CHECK(scaleAcs712MaSat(1023U, RefMv, OffMv, 66, &sat) == 32767 && sat);
    sat = false;
    CHECK(scaleAcs712MaSat(0U, RefMv, OffMv, 66, &sat) == -32767 && sat);
    /* inom intervallet: flaggan nollas igen */
    CHECK(scaleAcs712MaSat(600U, RefMv, OffMv, 66, &sat) < 32767 && !sat);
    /* typ 1 och 2 ar inom intervallet med standardoffset ... */
    CHECK(scaleAcs712MaSat(1023U, RefMv, OffMv, 121, &sat) == 20636 && !sat);
    CHECK(scaleAcs712MaSat(1023U, RefMv, OffMv, 100, &sat) < 32767 && !sat);
    /* ... men inte med felaktig nollpunkt 0 mV */
    CHECK(scaleAcs712MaSat(1023U, RefMv, 0, 121, &sat) == 32767 && sat);
    CHECK(scaleAcs712MaSat(1023U, RefMv, 0, 100, &sat) == 32767 && sat);
    /* exakt granen */
    CHECK(scaleAcs712MaSat(1023U, RefMv, OffMv, 66, 0) == 32767);
}

/* 3. nollpunkten (2483 mV ~ ADC 510) ger |mA| < 66 */
static void testZero()
{
    for (int t = 0; t < 3; ++t)
    {
        long ma = scaleAcs712Ma32(510U, RefMv, OffMv, kMvA[t]);
        CHECK(labs(ma) < 66L);
    }
}

/* 4. monotont i ADC */
static void testMonotonic()
{
    for (int t = 0; t < 3; ++t)
    {
        int32_t prev = scaleAcs712Ma32(0U, RefMv, OffMv, kMvA[t]);
        int16_t prevS = scaleAcs712MaSat(0U, RefMv, OffMv, kMvA[t], 0);
        bool ok = true;
        for (uint16_t adc = 1U; adc <= 1023U; ++adc)
        {
            int32_t v = scaleAcs712Ma32(adc, RefMv, OffMv, kMvA[t]);
            int16_t vs = scaleAcs712MaSat(adc, RefMv, OffMv, kMvA[t], 0);
            if ((v < prev) || (vs < prevS)) { ok = false; }
            prev = v; prevS = vs;
        }
        CHECK(ok);
    }
}

/* 5. mVperAmp 0 eller negativ: 0, ingen division med noll; adc > 1023 klampas */
static void testDegenerate()
{
    bool sat = true;
    CHECK(scaleAcs712Ma32(1023U, RefMv, OffMv, 0) == 0);
    CHECK(scaleAcs712Ma32(1023U, RefMv, OffMv, -5) == 0);
    CHECK(scaleAcs712MaSat(1023U, RefMv, OffMv, 0, &sat) == 0 && !sat);
    CHECK(scaleAcs712Ma32(65535U, RefMv, OffMv, 66) == scaleAcs712Ma32(1023U, RefMv, OffMv, 66));
    /* extrema indata utan overflow (UBSan fangar signed overflow) */
    (void)scaleAcs712Ma32(1023U, 32767, -32768, 1);
    (void)scaleAcs712Ma32(1023U, -32768, 32767, 1);
    CHECK(scaleAcs712MaSat(1023U, 32767, -32768, 1, &sat) == 32767 && sat);
}

/* 6. Typerna ar explicita (int16_t/int32_t), sa vardbygget beter sig som AVR.
 *    scaleAdcToVolt (double) kors genom float-cast-overflow-sanitizern. */
static void testTypesAndVolt()
{
    CHECK(sizeof(scaleAcs712MaSat(0U, 0, 0, 1, 0)) == 2U);
    CHECK(sizeof(scaleAcs712Ma32(0U, 0, 0, 1)) == 4U);
    CHECK(scaleAdcToVolt(1023U, 5500U) == 5500U);
    CHECK(scaleAdcToVolt(0U, 6500U) == 0U);
    CHECK(scaleAdcToVolt(1023U, 65535U) == 65535U);
}

int main()
{
    testTable();
    testSaturation();
    testZero();
    testMonotonic();
    testDegenerate();
    testTypesAndVolt();
    printf("test_scale: %d kontroller, %d fel\n", g_checks, g_fails);
    return (g_fails == 0) ? 0 : 1;
}
