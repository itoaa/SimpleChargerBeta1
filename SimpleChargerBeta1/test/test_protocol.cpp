/*
 * test_protocol.cpp - vardtester (PC) for TunerStudio-protokollet v2.2.
 * Bygg och kor: make -C SimpleChargerBeta1/test
 */
#include <stdio.h>
#include <string.h>
#include "tunerStudio.hpp"

static int g_checks = 0;
static int g_fails = 0;

#define CHECK(cond) do { ++g_checks; if (!(cond)) { ++g_fails; \
    printf("FEL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

/* ---- Mock-EEPROM (bara RAM) och utdata -------------------------------- */
static uint8_t g_mem[EepromLayout::EepromCapacity];
static void memRead(uint16_t a, uint8_t* d, uint16_t n, void*) { memcpy(d, &g_mem[a], n); }
static void memUpdate(uint16_t a, const uint8_t* s, uint16_t n, void*) { memcpy(&g_mem[a], s, n); }

static uint8_t g_out[512];
static unsigned g_outLen = 0U;
static void sendFn(const uint8_t* d, uint8_t n, void*)
{
    if (g_outLen + n <= sizeof(g_out)) { memcpy(&g_out[g_outLen], d, n); }
    g_outLen += n;
}

static int g_burns = 0;
static uint8_t g_cmds[16];
static unsigned g_cmdCount = 0U;
static int g_written[6];
static void hookBurn(void*) { ++g_burns; }
static void hookCmd(uint8_t c, void*) { if (g_cmdCount < sizeof(g_cmds)) { g_cmds[g_cmdCount] = c; } ++g_cmdCount; }
static uint8_t g_wOff = 0xFFU, g_wLen = 0U;
static void hookWritten(uint8_t p, uint8_t off, uint8_t len, void*) { if (p < 6U) { ++g_written[p]; } g_wOff = off; g_wLen = len; }

static uint32_t g_now = 1000U;
static void feed(TunerStudio& ts, const uint8_t* b, unsigned n)
{
    for (unsigned i = 0U; i < n; ++i) { ts.onByte(b[i], g_now); }
}
static void feedStr(TunerStudio& ts, const char* s) { feed(ts, reinterpret_cast<const uint8_t*>(s), (unsigned)strlen(s)); }
static void clearOut() { g_outLen = 0U; memset(g_out, 0, sizeof(g_out)); }

static void reqPage(TunerStudio& ts, uint8_t page)
{
    const uint8_t cmd[2] = { 'P', page };
    feed(ts, cmd, 2U);
}

int main()
{
    EepromBackend be;
    be.read = memRead; be.update = memUpdate; be.ctx = 0;
    memset(g_mem, 0xFF, sizeof(g_mem));
    EepromStore store(be);
    store.loadDefaults();
    TunerStudio ts(store);
    ts.setSend(sendFn, 0);
    TunerStudio::Hooks h;
    h.burn = hookBurn; h.command = hookCmd; h.pageWritten = hookWritten; h.ctx = 0;
    ts.setHooks(h);

    /* Q: exakt 20 byte, tva avslutande mellanslag, ingen NUL */
    clearOut(); feedStr(ts, "Q");
    CHECK(g_outLen == 20U);
    CHECK(memcmp(g_out, "MSII s-Charger 2.2  ", 20) == 0);
    clearOut(); feedStr(ts, "F");
    CHECK(g_outLen == 3U && memcmp(g_out, "002", 3) == 0);
    clearOut(); feedStr(ts, "S");
    CHECK(g_outLen > 0U && g_out[g_outLen - 1U] != 0U);

    /* A: 115 byte, samma som realtime */
    for (unsigned i = 0U; i < TsLayout::RtBlockSize; ++i) { ts.realtime[i] = (uint8_t)(i * 7U + 1U); }
    clearOut(); feedStr(ts, "A");
    CHECK(g_outLen == 115U);
    CHECK(memcmp(g_out, ts.realtime, 115U) == 0);
    clearOut(); feedStr(ts, "C");
    CHECK(g_outLen == 1U && g_out[0] == ts.realtime[0]);
    clearOut(); feedStr(ts, "c");
    CHECK(g_outLen == 1U);

    /* P0..P4 + V: sidstorlek och innehall */
    const unsigned sizes[5] = { 38U, 84U, 38U, 68U, 64U };
    for (uint8_t p = 0U; p < 5U; ++p)
    {
        reqPage(ts, p);
        CHECK(ts.activePage() == p);
        clearOut(); feedStr(ts, "V");
        CHECK(g_outLen == sizes[p]);
        CHECK(memcmp(g_out, store.page(p), sizes[p]) == 0);
    }
    /* Ogiltig sida ignoreras */
    reqPage(ts, 2U); reqPage(ts, 5U);
    CHECK(ts.activePage() == 2U);
    reqPage(ts, 255U);
    CHECK(ts.activePage() == 2U);

    /* W: en byte i aktiv sida */
    reqPage(ts, 0U);
    memset(g_written, 0, sizeof(g_written));
    { const uint8_t w[3] = { 'W', 10U, 7U }; feed(ts, w, 3U); }
    CHECK(store.page(0)[10] == 7U);
    CHECK(g_written[0] == 1);
    CHECK(g_wOff == 10U && g_wLen == 1U);   /* nr 45: kroken far offset och langd */
    /* W utanfor sidan: inget andras, ingen krok */
    {
        uint8_t before[38]; memcpy(before, store.page(0), 38U);
        const uint8_t w[3] = { 'W', 38U, 9U }; feed(ts, w, 3U);
        CHECK(memcmp(before, store.page(0), 38U) == 0);
        CHECK(g_written[0] == 1);
    }

    /* X: block i sida 4 (index 3), 64 byte */
    reqPage(ts, 3U);
    {
        uint8_t x[3 + 64];
        x[0] = 'X'; x[1] = 4U; x[2] = 64U;
        for (unsigned i = 0U; i < 64U; ++i) { x[3 + i] = (uint8_t)(0xA0U + i); }
        feed(ts, x, sizeof(x));
        CHECK(memcmp(store.page(3) + 4, x + 3, 64U) == 0);
        CHECK(g_written[3] == 1);
        CHECK(g_wOff == 4U && g_wLen == 64U);
    }
    /* X med count 65 avvisas helt men datat las bort; nasta kommando fungerar */
    {
        uint8_t before[68]; memcpy(before, store.page(3), 68U);
        uint8_t x[3 + 65];
        x[0] = 'X'; x[1] = 0U; x[2] = 65U;
        memset(x + 3, 0x55, 65U);
        feed(ts, x, sizeof(x));
        CHECK(memcmp(before, store.page(3), 68U) == 0);
        CHECK(g_written[3] == 1);
        clearOut(); feedStr(ts, "Q");
        CHECK(g_outLen == 20U);
    }
    /* X som gar over sidslutet avvisas (sida 4 = 68 B: 60 + 9 > 68) */
    {
        uint8_t before[68]; memcpy(before, store.page(3), 68U);
        uint8_t x[3 + 9];
        x[0] = 'X'; x[1] = 60U; x[2] = 9U;
        memset(x + 3, 0x66, 9U);
        feed(ts, x, sizeof(x));
        CHECK(memcmp(before, store.page(3), 68U) == 0);
        /* exakt till slutet gar bra */
        x[2] = 8U;
        feed(ts, x, 3U + 8U);
        CHECK(store.page(3)[67] == 0x66U && store.page(3)[59] == before[59]);
        CHECK(g_written[3] == 2);
    }
    /* X med count 0: inget hander */
    { const uint8_t x[3] = { 'X', 0U, 0U }; feed(ts, x, 3U); CHECK(g_written[3] == 2); }

    /* Timeout: halvt X-kommando kastas efter > 500 ms */
    {
        const uint8_t x[4] = { 'X', 0U, 10U, 1U };
        feed(ts, x, 4U);
        g_now += 501U;
        clearOut(); feedStr(ts, "Q");
        CHECK(g_outLen == 20U);
        CHECK(store.page(3)[0] != 'Q');
    }
    /* Inom timeout fortsatter kommandot */
    {
        reqPage(ts, 0U);
        const uint8_t w1[2] = { 'W', 11U };
        feed(ts, w1, 2U);
        g_now += 400U;
        const uint8_t w2[1] = { 3U };
        feed(ts, w2, 1U);
        CHECK(store.page(0)[11] == 3U);
    }

    /* E1..E9 och B */
    g_cmdCount = 0U;
    for (uint8_t k = 1U; k <= 9U; ++k)
    {
        const uint8_t e[2] = { 'E', k };
        feed(ts, e, 2U);
    }
    CHECK(g_cmdCount == 9U);
    for (uint8_t k = 1U; k <= 9U; ++k) { CHECK(g_cmds[k - 1U] == k); }
    feedStr(ts, "B");
    CHECK(g_burns == 1);
    /* Okand byte ignoreras */
    clearOut(); feedStr(ts, "Z");
    CHECK(g_outLen == 0U);

    /* Utan send/krokar: inga krascher */
    {
        TunerStudio t2(store);
        feedStr(t2, "QAVBF");
        const uint8_t e[2] = { 'E', 1U }; feed(t2, e, 2U);
        const uint8_t w[3] = { 'W', 0U, 0U }; feed(t2, w, 3U);
        CHECK(true);
    }

    printf("test_protocol: %d kontroller, %d fel\n", g_checks, g_fails);
    return (g_fails == 0) ? 0 : 1;
}
