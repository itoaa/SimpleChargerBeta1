/*
 * test_eeprom.cpp - vardtester (PC) for EEPROM-modulen.
 * Bygg och kor: make -C SimpleChargerBeta1/test
 */
#include <stdio.h>
#include <string.h>
#include "eepromStore.hpp"
#include "crc16.hpp"

static int g_checks = 0;
static int g_fails = 0;

#define CHECK(cond) do { ++g_checks; if (!(cond)) { ++g_fails; \
    printf("FEL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

/* ---- Mock-EEPROM ------------------------------------------------------ */
struct MockEeprom
{
    uint8_t mem[EepromLayout::EepromCapacity];
    unsigned updateCalls;   /* antal anrop till update */
    unsigned bytesWritten;  /* byte som faktiskt andrades (som eeprom_update_*) */
    unsigned writeBudget;   /* simulerat stromavbrott: max antal skrivna byte */
    bool ignoreWrites;      /* simulerat trasigt EEPROM */
    bool outOfRange;        /* satt om modulen nagonsin gick utanfor 1024 B */

    void reset(uint8_t fill)
    {
        memset(mem, fill, sizeof(mem));
        updateCalls = 0U;
        bytesWritten = 0U;
        writeBudget = 0xFFFFFFFFU;
        ignoreWrites = false;
        outOfRange = false;
    }
};

static void mockRead(uint16_t address, uint8_t* dst, uint16_t len, void* ctx)
{
    MockEeprom* m = static_cast<MockEeprom*>(ctx);
    if (((uint32_t)address + len) > EepromLayout::EepromCapacity) { m->outOfRange = true; return; }
    memcpy(dst, &m->mem[address], len);
}

static void mockUpdate(uint16_t address, const uint8_t* src, uint16_t len, void* ctx)
{
    MockEeprom* m = static_cast<MockEeprom*>(ctx);
    ++m->updateCalls;
    if (((uint32_t)address + len) > EepromLayout::EepromCapacity) { m->outOfRange = true; return; }
    for (uint16_t i = 0U; i < len; ++i)
    {
        if (m->ignoreWrites || (m->writeBudget == 0U)) { return; }
        if (m->mem[address + i] != src[i])
        {
            m->mem[address + i] = src[i];
            ++m->bytesWritten;
            --m->writeBudget;
        }
    }
}

static MockEeprom g_mock;

static EepromBackend mockBackend()
{
    EepromBackend b;
    b.read = &mockRead;
    b.update = &mockUpdate;
    b.ctx = &g_mock;
    return b;
}

static const uint8_t golden[EepromLayout::PagesTotalSize] = {
#include "golden_defaults.inc"
};

static bool ramEqualsDefaults(const EepromStore& s, uint8_t page)
{
    return memcmp(s.page(page), &golden[EepromLayout::pageRamOffset(page)], EepromLayout::pageSize(page)) == 0;
}

static bool allDefaults(const EepromStore& s)
{
    for (uint8_t p = 0U; p < EepromLayout::PageCount; ++p)
    {
        if (!ramEqualsDefaults(s, p)) { return false; }
    }
    return true;
}

/* Satter igenkannbara icke-standardvarden pa alla sidor. */
static void fillPattern(EepromStore& s)
{
    for (uint8_t p = 0U; p < EepromLayout::PageCount; ++p)
    {
        uint8_t* d = s.page(p);
        for (uint16_t i = 0U; i < s.pageSize(p); ++i) { d[i] = (uint8_t)(0xA0U + p + i); }
    }
    /* diagMode sparas alltid som Normal: monstret ska kunna aterlasas exakt */
    s.page(2)[EepromLayout::P3DiagModeOffset] &= (uint8_t)~EepromLayout::P3DiagModeMask;
}

/* ---- Tester ------------------------------------------------------------ */
static void testCrc16()
{
    const uint8_t v[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    CHECK(crc16(v, 9U) == 0x29B1U);                 /* CRC-16/CCITT-FALSE kontrollvarde */
    CHECK(crc16(v, 0U) == 0xFFFFU);
    CHECK(crc16(0, 5U) == 0xFFFFU);
    uint16_t c = Crc16Init;
    for (int i = 0; i < 9; ++i) { c = crc16Update(c, v[i]); }
    CHECK(c == 0x29B1U);
}

static void testLayout()
{
    using namespace EepromLayout;
    CHECK(pageSize(0) == 38U); CHECK(pageSize(1) == 84U); CHECK(pageSize(2) == 38U);
    CHECK(pageSize(3) == 68U); CHECK(pageSize(4) == 64U); CHECK(pageSize(5) == 0U);
    CHECK(PagesTotalSize == 292U);
    CHECK(HeaderSize == 6U);
    CHECK(TotalSize == 308U);
    CHECK(BaseAddress + TotalSize <= 1024U);
    uint16_t ram = 0U, addr = BaseAddress + HeaderSize;
    for (uint8_t p = 0U; p < PageCount; ++p)
    {
        CHECK(pageRamOffset(p) == ram);
        CHECK(pageAddress(p) == addr);
        ram = (uint16_t)(ram + pageSize(p));
        addr = (uint16_t)(addr + pageSize(p) + CrcSize);
    }
    CHECK(addr == BaseAddress + TotalSize);
    /* Sida 1 v2.2 (SPEC_tabeller 1.1) */
    CHECK(P1ChargeAmp == 0U); CHECK(P1MaxChargeAmp == 2U); CHECK(P1MaxInputVolt == 4U);
    CHECK(P1MaxChargeVolt == 6U); CHECK(P1MaxInputAmp == 8U); CHECK(P1MaxPWM == 10U);
    CHECK(P1MinPWM == 11U); CHECK(P1MaxTemp == 12U); CHECK(P1AccelerationDelay == 13U);
    CHECK(P1MosfetTempADCTable == 14U); CHECK(P1MosfetTempTable == 26U);
    CHECK(P3DiagModeOffset == 16U);
}

static void testByteHelpers()
{
    uint8_t b[5] = { 0, 0, 0, 0, 0 };
    scPutU16le(&b[1], 0x1234U);                     /* ojusterad adress */
    CHECK(b[1] == 0x34U && b[2] == 0x12U);
    CHECK(scGetU16le(&b[1]) == 0x1234U);
    scPutS16le(&b[3], (int16_t)-1);
    CHECK(b[3] == 0xFFU && b[4] == 0xFFU);
    CHECK(scGetS16le(&b[3]) == -1);
    scPutS16le(&b[0], (int16_t)-32768);
    CHECK(b[0] == 0x00U && b[1] == 0x80U && scGetS16le(&b[0]) == -32768);
    scPutS16le(&b[0], 32767);
    CHECK(scGetS16le(&b[0]) == 32767);
}

static void testDefaults()
{
    g_mock.reset(0xFFU);
    EepromStore s(mockBackend());
    for (uint16_t i = 0U; i < EepromLayout::PagesTotalSize; ++i)
    {
        if (scReadFlashByte(&eepromDefaults[i]) != golden[i])
        {
            printf("  standardvarde skiljer: byte %u (0x%02X, ini 0x%02X)\n", (unsigned)i,
                   (unsigned)eepromDefaults[i], (unsigned)golden[i]);
        }
    }
    CHECK(memcmp(eepromDefaults, golden, sizeof(golden)) == 0);
    CHECK(allDefaults(s));
    /* Handkontroller mot SPEC_tabeller_v2.2.md */
    const uint8_t* p1 = s.page(0); const uint8_t* p2 = s.page(1);
    const uint8_t* p3 = s.page(2); const uint8_t* p5 = s.page(4);
    CHECK(scGetU16le(&p1[EepromLayout::P1MaxChargeVolt]) == 0U);   /* 0 = utgang av */
    CHECK(scGetU16le(&p1[EepromLayout::P1MaxChargeAmp]) == 0U);
    CHECK(p1[EepromLayout::P1MaxPWM] == 5U);
    CHECK(p2[0] == 0x04U);
    CHECK(p2[4] == 0x00U && (int8_t)p2[5] == 45 && scGetU16le(&p2[6]) == 1500U && scGetU16le(&p2[8]) == 1440U);
    CHECK(scGetU16le(&p2[4 + 14]) == 600U);
    CHECK(p2[24] == 0x01U && scGetU16le(&p2[24 + 12]) == 100U);
    CHECK(p2[44] == 0x01U && scGetU16le(&p2[44 + 2]) == 200U && scGetU16le(&p2[44 + 4]) == 1360U);
    CHECK(p2[64] == 0x0BU);
    CHECK(p3[0] == 31U && scGetU16le(&p3[1]) == 2000U && p3[4] == 100U && p3[5] == 160U && p3[6] == 0x50U);
    CHECK(scGetU16le(&p3[9]) == 1000U && p3[16] == 0U);
    CHECK(scGetU16le(&p3[18]) == 10000U && scGetS16le(&p3[20]) == 0);
    CHECK(p5[0] == 0x04U && p5[1] == 0x08U && scGetU16le(&p5[2]) == 600U && scGetU16le(&p5[4]) == 1500U);
    CHECK(p5[12] == 6U && scGetU16le(&p5[15]) == 1470U && (int8_t)p5[34] == -40);
    CHECK(scGetU16le(&p5[45]) == 3960U && scGetU16le(&p5[47]) == 300U);
}

static void testBlankAndOldEeprom()
{
    /* Tomt EEPROM (0xFF) */
    g_mock.reset(0xFFU);
    EepromStore s(mockBackend());
    fillPattern(s);
    uint8_t w = s.load();
    CHECK(w == EepromStore::WarnHeaderInvalid);
    CHECK(s.warnBits() == EepromStore::WarnHeaderInvalid);
    CHECK(allDefaults(s));
    CHECK(g_mock.updateCalls == 0U);

    /* B-2-firmware: 33 B gammal sida 1 i borjan av EEPROM ska inte tolkas */
    g_mock.reset(0x00U);
    const uint8_t oldPage1[33] = { 10, 15, 14, 0, 16, 25, 0, 80, 0 };
    memcpy(g_mock.mem, oldPage1, sizeof(oldPage1));
    EepromStore s2(mockBackend());
    CHECK(s2.load() == EepromStore::WarnHeaderInvalid);
    CHECK(allDefaults(s2));
    CHECK(g_mock.updateCalls == 0U);

    /* Nollat EEPROM */
    g_mock.reset(0x00U);
    EepromStore s3(mockBackend());
    CHECK(s3.load() == EepromStore::WarnHeaderInvalid);
    CHECK(allDefaults(s3));
}

static void testRoundTrip()
{
    g_mock.reset(0xFFU);
    EepromStore a(mockBackend());
    fillPattern(a);
    a.page(2)[EepromLayout::P3DiagModeOffset] = 0x00U;
    CHECK(a.burn());
    CHECK(a.warnBits() == 0U);
    CHECK(g_mock.bytesWritten > 0U);
    CHECK(!g_mock.outOfRange);

    EepromStore b(mockBackend());
    CHECK(b.load() == 0U);
    for (uint8_t p = 0U; p < EepromLayout::PageCount; ++p)
    {
        CHECK(memcmp(a.page(p), b.page(p), a.pageSize(p)) == 0);
    }
    /* Inget utanfor layouten (308 B) far roras */
    for (uint16_t i = EepromLayout::BaseAddress + EepromLayout::TotalSize; i < EepromLayout::EepromCapacity; ++i)
    {
        if (g_mock.mem[i] != 0xFFU) { CHECK(false); break; }
    }
}

static void testCorruptPage()
{
    for (uint8_t bad = 0U; bad < EepromLayout::PageCount; ++bad)
    {
        g_mock.reset(0xFFU);
        EepromStore a(mockBackend());
        fillPattern(a);
        a.page(2)[EepromLayout::P3DiagModeOffset] = 0x00U;
        CHECK(a.burn());
        /* Vant en bit i sidans data */
        g_mock.mem[EepromLayout::pageAddress(bad) + 3U] ^= 0x01U;
        EepromStore b(mockBackend());
        const unsigned calls = g_mock.updateCalls;
        const uint8_t w = b.load();
        CHECK(w == (uint8_t)(EepromStore::WarnPage1Crc << bad));
        CHECK(g_mock.updateCalls == calls);            /* load skriver aldrig */
        for (uint8_t p = 0U; p < EepromLayout::PageCount; ++p)
        {
            if (p == bad) { CHECK(ramEqualsDefaults(b, p)); }
            else { CHECK(memcmp(a.page(p), b.page(p), a.pageSize(p)) == 0); }
        }
    }
    /* Trasig CRC-byte (data hel) */
    g_mock.reset(0xFFU);
    EepromStore a(mockBackend());
    fillPattern(a);
    CHECK(a.burn());
    g_mock.mem[EepromLayout::pageAddress(4) + EepromLayout::Page5Size] ^= 0x80U;
    EepromStore b(mockBackend());
    CHECK(b.load() == EepromStore::WarnPage5Crc);
    CHECK(ramEqualsDefaults(b, 4));
}

static void testHeader()
{
    const uint16_t offs[4] = { 0U, 1U, 2U, 3U };
    for (int i = 0; i < 4; ++i)
    {
        g_mock.reset(0xFFU);
        EepromStore a(mockBackend());
        fillPattern(a);
        CHECK(a.burn());
        g_mock.mem[EepromLayout::BaseAddress + offs[i]] ^= 0x01U;  /* magic, version, antal */
        EepromStore b(mockBackend());
        CHECK(b.load() == EepromStore::WarnHeaderInvalid);
        CHECK(allDefaults(b));
    }
    /* Annan layoutversion med korrekt huvud-CRC -> standardvarden */
    g_mock.reset(0xFFU);
    EepromStore a(mockBackend());
    CHECK(a.burn());
    g_mock.mem[2] = (uint8_t)(EepromLayout::LayoutVersion + 1U);
    scPutU16le(&g_mock.mem[4], crc16(g_mock.mem, 4U));
    EepromStore b(mockBackend());
    CHECK(b.load() == EepromStore::WarnHeaderInvalid);
}

static void testDiagMode()
{
    /* burn sparar diagMode som Normal, ovriga bitar och diagDuty behalls */
    g_mock.reset(0xFFU);
    EepromStore a(mockBackend());
    a.page(2)[EepromLayout::P3DiagModeOffset] = 0xF1U;   /* diag 1 + ovriga bitar */
    a.page(2)[17] = 12U;
    CHECK(a.burn());
    CHECK(a.page(2)[EepromLayout::P3DiagModeOffset] == 0xF1U);  /* RAM orord */
    CHECK(g_mock.mem[EepromLayout::Page3Address + 16U] == 0xF0U);
    CHECK(g_mock.mem[EepromLayout::Page3Address + 17U] == 12U);

    /* EEPROM som (felaktigt) innehaller diagMode != Normal med giltig CRC */
    g_mock.mem[EepromLayout::Page3Address + 16U] = 0x02U;
    const uint16_t crc = crc16(&g_mock.mem[EepromLayout::Page3Address], EepromLayout::Page3Size);
    scPutU16le(&g_mock.mem[EepromLayout::Page3Address + EepromLayout::Page3Size], crc);
    EepromStore b(mockBackend());
    CHECK(b.load() == 0U);
    CHECK((b.page(2)[16] & EepromLayout::P3DiagModeMask) == 0U);
    CHECK(b.page(2)[17] == 12U);

    /* Aven vid standardvarden */
    g_mock.reset(0xFFU);
    EepromStore c(mockBackend());
    c.page(2)[16] = 0x03U;
    c.load();
    CHECK(c.page(2)[16] == 0U);
}

static void testWritesOnlyOnBurn()
{
    g_mock.reset(0xFFU);
    EepromStore s(mockBackend());
    s.loadDefaults();
    s.load();
    uint8_t v = 7U;
    CHECK(s.writeBytes(0, EepromLayout::P1MaxPWM, &v, 1U));
    s.load();
    CHECK(g_mock.updateCalls == 0U);
    CHECK(g_mock.bytesWritten == 0U);

    CHECK(s.burn());
    CHECK(g_mock.bytesWritten > 0U);
    CHECK(g_mock.bytesWritten <= EepromLayout::TotalSize);
    /* Andra burn utan andringar: inga byte andras (update-semantik) */
    g_mock.bytesWritten = 0U;
    CHECK(s.burn());
    CHECK(g_mock.bytesWritten == 0U);
    /* En andrad byte -> den byten + sidans CRC (max 3 byte) */
    v = 9U;
    CHECK(s.writeBytes(0, EepromLayout::P1MaxPWM, &v, 1U));
    CHECK(s.burn());
    CHECK(g_mock.bytesWritten >= 1U && g_mock.bytesWritten <= 3U);
    CHECK(!g_mock.outOfRange);
}

static void testBounds()
{
    g_mock.reset(0xFFU);
    EepromStore s(mockBackend());
    uint8_t buf[100];
    memset(buf, 0x5A, sizeof(buf));
    CHECK(s.page(5) == 0);
    CHECK(s.page(255) == 0);
    CHECK(s.pageSize(5) == 0U);
    CHECK(s.writeBytes(0, 37U, buf, 1U));
    CHECK(!s.writeBytes(0, 38U, buf, 1U));
    CHECK(!s.writeBytes(0, 30U, buf, 9U));
    CHECK(s.writeBytes(0, 30U, buf, 8U));
    CHECK(s.writeBytes(0, 38U, buf, 0U));
    CHECK(!s.writeBytes(0, 39U, buf, 0U));
    CHECK(!s.writeBytes(0, 0xFFFFU, buf, 2U));      /* ingen overslagning */
    CHECK(!s.writeBytes(0, 2U, buf, 0xFFFFU));
    CHECK(!s.writeBytes(5, 0U, buf, 1U));
    CHECK(!s.writeBytes(0, 0U, 0, 1U));
    CHECK(s.writeBytes(1, 0U, buf, 84U));
    CHECK(!s.writeBytes(1, 0U, buf, 85U));
    CHECK(s.readBytes(4, 0U, buf, 64U));
    CHECK(!s.readBytes(4, 1U, buf, 64U));
    CHECK(!s.readBytes(5, 0U, buf, 1U));
    CHECK(!s.readBytes(0, 0U, 0, 1U));
    /* Sidan efter (sida 3) far inte paverkas av skrivning till slutet av sida 2 */
    const uint8_t before = s.page(2)[0];
    memset(buf, 0x11, sizeof(buf));
    CHECK(s.writeBytes(1, 80U, buf, 4U));
    CHECK(s.page(2)[0] == before);
    uint8_t r[4];
    CHECK(s.readBytes(1, 80U, r, 4U) && r[0] == 0x11U && r[3] == 0x11U);
}

static void testFailures()
{
    /* Trasigt EEPROM: burn ska rapportera fel */
    g_mock.reset(0xFFU);
    g_mock.ignoreWrites = true;
    EepromStore s(mockBackend());
    CHECK(!s.burn());
    CHECK((s.warnBits() & EepromStore::WarnBurnFailed) != 0U);

    /* Ingen backend */
    EepromBackend none; none.read = 0; none.update = 0; none.ctx = 0;
    EepromStore n(none);
    CHECK(n.load() == (EepromStore::WarnNoBackend | EepromStore::WarnHeaderInvalid));
    CHECK(allDefaults(n));
    CHECK(!n.burn());

    /* Stromavbrott under forsta burn: huvudet skrivs sist -> standardvarden */
    for (unsigned budget = 0U; budget < 320U; budget += 7U)
    {
        g_mock.reset(0xFFU);
        EepromStore a(mockBackend());
        fillPattern(a);
        g_mock.writeBudget = budget;
        (void)a.burn();
        g_mock.writeBudget = 0xFFFFFFFFU;
        EepromStore b(mockBackend());
        const uint8_t w = b.load();
        if (w == 0U) { /* hela skrivningen hann klart */
            for (uint8_t p = 0U; p < EepromLayout::PageCount; ++p)
            { CHECK(memcmp(a.page(p), b.page(p), a.pageSize(p)) == 0); }
        } else {
            CHECK(w == EepromStore::WarnHeaderInvalid);
            CHECK(allDefaults(b));
        }
    }
    /* Stromavbrott nar giltigt innehall redan finns: berord sida -> standard + varning */
    g_mock.reset(0xFFU);
    EepromStore a(mockBackend());
    CHECK(a.burn());                                  /* giltiga standardvarden */
    fillPattern(a);
    g_mock.writeBudget = 10U;                         /* hinner bara en del av sida 1 */
    (void)a.burn();
    g_mock.writeBudget = 0xFFFFFFFFU;
    EepromStore b(mockBackend());
    CHECK(b.load() == EepromStore::WarnPage1Crc);
    CHECK(ramEqualsDefaults(b, 0));
}

int main()
{
    testCrc16();
    testLayout();
    testByteHelpers();
    testDefaults();
    testBlankAndOldEeprom();
    testRoundTrip();
    testCorruptPage();
    testHeader();
    testDiagMode();
    testWritesOnlyOnBurn();
    testBounds();
    testFailures();
    printf("%d kontroller, %d fel\n", g_checks, g_fails);
    return (g_fails == 0) ? 0 : 1;
}
