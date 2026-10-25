/*
 * test_charger.cpp - vardtester (PC) for laddlogiken v2.2 med simulerad
 * tid och en enkel batterimodell. Modellen ar en grov [U]-approximation
 * (Voc linjar i laddning, inre resistans 0,05 ohm, 1 A per PWM-steg) och
 * ar bara till for att driva stegmaskinen; den sager inget om hardvaran.
 */
#include <stdio.h>
#include <string.h>
#include "charger.hpp"
#include "eepromStore.hpp"
#include "tsLayout.hpp"

using namespace TsLayout;

static int g_checks = 0;
static int g_fails = 0;

#define CHECK(cond) do { ++g_checks; if (!(cond)) { ++g_fails; \
    printf("FEL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static void nopRead(uint16_t, uint8_t* d, uint16_t n, void*) { memset(d, 0xFF, n); }
static void nopUpdate(uint16_t, const uint8_t*, uint16_t, void*) {}

/* ---- testmiljo -------------------------------------------------------- */
struct Sim
{
    EepromStore* store;
    Charger ch;
    AutoParams::Result ar;
    uint32_t now;
    bool connected;      /* batteri anslutet */
    int32_t vocBase;     /* cV vid 0 laddning */
    int32_t vocMax;      /* cV vid full */
    int32_t capCAh;      /* modellkapacitet */
    int64_t chargeCAhMs; /* laddning i cA*ms */
    int16_t forceCa;     /* >= 0: tvinga matt strom */
    int32_t extraCv;     /* adderas till matt spanning */
    uint16_t inCv;

    explicit Sim(EepromStore& s)
        : store(&s), ch(), now(1000U), connected(true), vocBase(1200), vocMax(1436), capCAh(6000),
          chargeCAhMs(0), forceCa(-1), extraCv(0), inCv(2000U)
    {
        memset(&ar, 0, sizeof(ar));
    }

    void recompute()
    {
        const uint8_t* p1 = store->page(0);
        AutoParams::Input in;
        in.maxChargeAmp = scGetU16le(p1 + EepromLayout::P1MaxChargeAmp);
        in.maxChargeVolt = scGetU16le(p1 + EepromLayout::P1MaxChargeVolt);
        in.currentSensor = scBits(store->page(2)[P3SensorBits], 4U, 3U);
        in.batSensor = false;
        AutoParams::compute(store->page(4), in, ar);
    }

    int32_t voc() const
    {
        int32_t soc = (int32_t)(chargeCAhMs / 3600000LL);
        int32_t v = vocBase + (int32_t)(((int64_t)(vocMax - vocBase) * soc) / capCAh);
        return (v > vocMax) ? vocMax : v;
    }

    Charger::Measure measure() const
    {
        Charger::Measure m;
        int32_t ca = connected ? (int32_t)ch.duty() * 100 : 0;
        int32_t v = connected ? voc() + (ca * 5) / 100 : (ch.duty() > 0U ? 1500 : 0);
        if (forceCa >= 0) { ca = forceCa; }
        v += extraCv;
        m.outCv = (uint16_t)((v < 0) ? 0 : v);
        m.inCv = inCv;
        m.outCa = (int16_t)ca;
        return m;
    }

    void tick(uint32_t dt = 100U)
    {
        Charger::Measure m = measure();
        if (connected) { chargeCAhMs += (int64_t)m.outCa * dt; }
        now += dt;
        Charger::Pages pg;
        pg.p1 = store->page(0); pg.p2 = store->page(1); pg.p3 = store->page(2); pg.p5 = store->page(4);
        ch.update(m, pg, ar, now);
    }

    void run(uint32_t ms) { for (uint32_t t = 0U; t < ms; t += 100U) { tick(); } }

    /* kor tills villkoret ar sant eller tiden slut; returnerar forbrukad tid */
    template <typename F> uint32_t runUntil(F f, uint32_t maxMs)
    {
        uint32_t t = 0U;
        while (!f(*this) && t < maxMs) { tick(); t += 100U; }
        return t;
    }
};

static void setU16(EepromStore& s, uint8_t page, uint16_t off, uint16_t v) { scPutU16le(s.page(page) + off, v); }

static void setManual(EepromStore& s, uint8_t id, uint16_t v)
{
    uint8_t* p5 = s.page(4);
    p5[P5OvrMask + id / 8U] = (uint8_t)(p5[P5OvrMask + id / 8U] | (1U << (id % 8U)));
    if (AutoParams::manType(id) == 2U) { scPutU16le(p5 + AutoParams::manOffset(id), v); }
    else { p5[AutoParams::manOffset(id)] = (uint8_t)v; }
}

static void setupStore(EepromStore& s, uint16_t mcv)
{
    s.loadDefaults();
    setU16(s, 0, EepromLayout::P1MaxChargeVolt, mcv);
    s.page(0)[EepromLayout::P1MaxPWM] = 25U;
}

struct StateIs { uint8_t st; bool operator()(const Sim& s) const { return s.ch.state() == st; } };
struct StepIs { uint8_t sp; bool operator()(const Sim& s) const { return s.ch.state() == StateCharging && s.ch.step() == sp; } };

static uint16_t rt16(const Sim& s, uint16_t off)
{
    uint8_t rt[RtBlockSize]; memset(rt, 0, sizeof(rt)); s.ch.writeRealtime(rt);
    return scGetU16le(rt + off);
}
static uint8_t rt8(const Sim& s, uint16_t off)
{
    uint8_t rt[RtBlockSize]; memset(rt, 0, sizeof(rt)); s.ch.writeRealtime(rt);
    return rt[off];
}

/* ---- tester ----------------------------------------------------------- */
static EepromBackend be() { EepromBackend b; b.read = nopRead; b.update = nopUpdate; b.ctx = 0; return b; }

static void testBootOffAndMcvZero()
{
    EepromStore st(be());
    setupStore(st, 0U);           /* standard: MaxChargeVolt 0 = utgang av */
    Sim s(st); s.recompute();
    s.run(5000U);
    CHECK(s.ch.state() == StateWaiting && s.ch.duty() == 0U);
    CHECK(!s.ch.chargingActive());
    s.ch.command(1U);             /* E1 nekas: MCV = 0 */
    s.run(2000U);
    CHECK(s.ch.state() == StateWaiting && s.ch.duty() == 0U);
    /* Med MCV satt startar den inte av sig sjalv (startMode Manuell) */
    setU16(st, 0, EepromLayout::P1MaxChargeVolt, 1500U); s.recompute();
    s.run(5000U);
    CHECK(s.ch.state() == StateWaiting && s.ch.duty() == 0U);
    /* Gammal E1 ar forbrukad: ingen fordrojd start */
    CHECK(rt8(s, RtStatusBits) == StatusBattery);
}

static void testSimpleMode()
{
    EepromStore st(be());
    setupStore(st, 1500U);
    st.page(4)[P5ModeBits] = (uint8_t)((st.page(4)[P5ModeBits] & ~3U) | ChargeModeSimple);
    setU16(st, 0, EepromLayout::P1MaxChargeAmp, 0U);      /* 0 -> 1500 */
    setU16(st, 0, EepromLayout::P1ChargeAmp, 800U);
    Sim s(st); s.recompute();
    s.ch.command(1U);
    s.tick();
    CHECK(s.ch.state() == StateCharging && s.ch.chargingActive());
    CHECK(rt8(s, RtStage) == 0U);
    CHECK(rt16(s, RtAmpSetpoint) == 800U && rt16(s, RtVoltSetpoint) == 1500U);
    /* ramp ett steg per varv till 8 A (8 steg) */
    s.run(3000U);
    CHECK(s.ch.duty() >= 7U && s.ch.duty() <= 9U);
    CHECK((rt8(s, RtStatusBits) & StatusOutputOn) != 0U);
    /* ChargeAmp 0 -> MCA (0 -> 1500) */
    setU16(st, 0, EepromLayout::P1ChargeAmp, 0U);
    s.tick();
    CHECK(rt16(s, RtAmpSetpoint) == 1500U);
    /* MCA > 1500 -> 1500 */
    setU16(st, 0, EepromLayout::P1MaxChargeAmp, 3000U);
    s.tick();
    CHECK(rt16(s, RtAmpSetpoint) == 1500U);
    /* MCA 500 och ChargeAmp 800 -> 500 */
    setU16(st, 0, EepromLayout::P1MaxChargeAmp, 500U);
    setU16(st, 0, EepromLayout::P1ChargeAmp, 800U);
    s.tick();
    CHECK(rt16(s, RtAmpSetpoint) == 500U);
    /* strom 6 A >= MCA 5 A -> hard brytning, felbit, ej latchande */
    s.forceCa = 600;
    s.tick();
    CHECK(s.ch.duty() == 0U);
    CHECK((s.ch.errorBits() & ErrOverAmpOut) != 0U);
    s.forceCa = -1;
    s.tick();
    CHECK(s.ch.errorBits() == 0U);
    CHECK((s.ch.latchedErrorBits() & ErrOverAmpOut) != 0U);
    CHECK(s.ch.state() == StateCharging);
    s.ch.command(4U); s.tick();
    CHECK(s.ch.latchedErrorBits() == 0U);
    /* overspanning */
    s.extraCv = 400;
    s.tick();
    CHECK(s.ch.duty() == 0U && (s.ch.errorBits() & ErrOverVoltOut) != 0U);
    s.extraCv = 0;
    /* hog inspanning (MIV satt) */
    setU16(st, 0, EepromLayout::P1MaxInputVolt, 1900U);
    s.tick();
    CHECK((s.ch.errorBits() & ErrHighInVolt) != 0U && s.ch.duty() == 0U);
    setU16(st, 0, EepromLayout::P1MaxInputVolt, 0U);
    /* MCV -> 0 under laddning: utgang av och Vantar */
    s.run(1000U);
    setU16(st, 0, EepromLayout::P1MaxChargeVolt, 0U); s.recompute();
    s.tick();
    CHECK(s.ch.state() == StateWaiting && s.ch.duty() == 0U && !s.ch.chargingActive());
    /* E2 stoppar */
    setU16(st, 0, EepromLayout::P1MaxChargeVolt, 1500U); s.recompute();
    s.ch.command(1U); s.run(1000U);
    CHECK(s.ch.duty() > 0U);
    s.ch.command(2U);
    CHECK(s.ch.duty() == 0U && s.ch.state() == StateWaiting);
    s.run(1000U);
    CHECK(s.ch.duty() == 0U);
}

static void testMinPwmAndCap()
{
    EepromStore st(be());
    setupStore(st, 1500U);
    st.page(4)[P5ModeBits] = (uint8_t)((st.page(4)[P5ModeBits] & ~3U) | ChargeModeSimple);
    st.page(0)[EepromLayout::P1MaxPWM] = 4U;
    st.page(0)[EepromLayout::P1MinPWM] = 3U;
    Sim s(st); s.recompute();
    s.ch.command(1U);
    s.tick();
    CHECK(s.ch.duty() == 3U);           /* hoppar over dodzonen */
    s.run(2000U);
    CHECK(s.ch.duty() == 4U);           /* tak MaxPWM */
    CHECK((rt16(s, RtRestrictionBits) & RestrPwmCap) != 0U);
    st.page(0)[EepromLayout::P1MaxPWM] = 40U;  /* > 25 -> 25 */
    s.run(5000U);
    CHECK(s.ch.duty() <= 25U);
    st.page(0)[EepromLayout::P1MinPWM] = 30U;  /* MinPWM > tak -> av */
    s.tick();
    CHECK(s.ch.duty() == 0U);
}

static void testAutoFullCycle()
{
    EepromStore st(be());
    setupStore(st, 1500U);
    setManual(st, AutoParams::FloatMaxTime, 30U);   /* kort float i testet */
    Sim s(st); s.recompute();
    CHECK(s.ar.warn == AutoParams::WarnTcNoSensor || (s.ar.warn & ~(AutoParams::WarnTcNoSensor | AutoParams::WarnOverride)) == 0U);
    s.vocBase = 1000;                                /* djupurladdat: forladdning */
    s.capCAh = 1000;                                 /* liten modell: snabbare test */
    s.vocMax = 1466;                                 /* under absorb 14,70 V: strommen avtar i CV */
    s.ch.command(1U);
    s.tick();
    CHECK(s.ch.state() == StateCharging && s.ch.step() == 0U);
    CHECK((rt8(s, RtStatusBits) & StatusPrecharge) != 0U);
    CHECK(rt8(s, RtStage) == 0U);
    CHECK(rt16(s, RtAmpSetpoint) == (uint16_t)s.ar.eff[AutoParams::PrechargeAmp]);
    StepIs bulk = { 1U };
    uint32_t t = s.runUntil(bulk, 4U * 3600000U);
    CHECK(s.ch.step() == 1U && s.ch.state() == StateCharging);
    CHECK(t < (uint32_t)s.ar.eff[AutoParams::PrechargeMaxTime] * 60000U);
    s.tick();
    CHECK(rt16(s, RtAmpSetpoint) == (uint16_t)s.ar.eff[AutoParams::BulkAmp]);
    StepIs absorb = { 2U };
    s.runUntil(absorb, 12U * 3600000U);
    CHECK(s.ch.step() == 2U);
    CHECK(rt16(s, RtVoltSetpoint) == (uint16_t)s.ar.eff[AutoParams::AbsorbVolt]);
    StepIs flt = { 3U };
    s.runUntil(flt, 5U * 3600000U);
    s.tick();
    CHECK(s.ch.step() == 3U);
    CHECK(rt16(s, RtVoltSetpoint) == (uint16_t)s.ar.eff[AutoParams::FloatVolt]);
    CHECK(rt16(s, RtAmpSetpoint) == (uint16_t)s.ar.eff[AutoParams::FloatAmp]);
    StateIs done = { StateDone };
    t = s.runUntil(done, 2U * 3600000U);
    CHECK(s.ch.state() == StateDone && s.ch.step() == 4U);
    CHECK(t >= 30U * 60000U - 200U && t <= 30U * 60000U + 200U);
    CHECK(rt8(s, RtStage) == 4U && s.ch.duty() == 0U && !s.ch.chargingActive());
    CHECK(s.ch.errorBits() == 0U);
    CHECK(rt16(s, RtAmpHours) > 500U);
    {
        uint8_t rt[RtBlockSize]; s.ch.writeRealtime(rt);
        CHECK(scGetU32le(rt + RtWattHours) > 0U);
        CHECK(scGetU32le(rt + RtChargeTime) > 3600U);
    }
    /* Overvaka: ingen omstart over restartVolt */
    s.run(60000U);
    CHECK(s.ch.state() == StateDone);
    /* sjalvurladdning under 12,50 V -> omstart fran bulk */
    s.chargeCAhMs = 0; s.vocBase = 1240;
    s.tick(); s.tick();
    CHECK(s.ch.state() == StateCharging && s.ch.step() == 1U);
    /* restartMode av: stannar i Overvaka */
    StateIs done2 = { StateDone };
    s.runUntil(done2, 20U * 3600000U);
    CHECK(s.ch.state() == StateDone);
    st.page(1)[P2StartBits] = (uint8_t)(st.page(1)[P2StartBits] & ~0x04U);
    s.chargeCAhMs = 0; s.vocBase = 1200;
    s.run(1000U);
    CHECK(s.ch.state() == StateDone && s.ch.duty() == 0U);
    /* chargeMode-byte avbryter */
    st.page(4)[P5ModeBits] = (uint8_t)((st.page(4)[P5ModeBits] & ~3U) | ChargeModeSimple);
    s.tick();
    CHECK(s.ch.state() == StateWaiting);
}

static void testAutoTimeouts()
{
    /* totalMaxTime -> FEL 9 (ErrMaxTime), Error-lage, start nekas tills E4 */
    {
        EepromStore st(be());
        setupStore(st, 1500U);
        setManual(st, AutoParams::TotalMaxTime, 10U);
        Sim s(st); s.recompute();
        s.vocMax = 1300;               /* nar aldrig absorption */
        s.ch.command(1U);
        StateIs err = { StateError };
        uint32_t t = s.runUntil(err, 3600000U);
        CHECK(s.ch.state() == StateError);
        CHECK(t >= 10U * 60000U - 200U && t <= 10U * 60000U + 200U);
        CHECK((s.ch.errorBits() & ErrMaxTime) != 0U && (s.ch.latchedErrorBits() & ErrMaxTime) != 0U);
        CHECK(s.ch.duty() == 0U && !s.ch.chargingActive());
        s.ch.command(1U); s.run(1000U);
        CHECK(s.ch.state() == StateError);
        s.ch.command(4U); s.tick();
        CHECK(s.ch.state() == StateWaiting && s.ch.errorBits() == 0U);
        s.ch.command(1U); s.tick();
        CHECK(s.ch.state() == StateCharging);
    }
    /* bulkMaxTime */
    {
        EepromStore st(be());
        setupStore(st, 1500U);
        setManual(st, AutoParams::BulkMaxTime, 5U);
        Sim s(st); s.recompute();
        s.vocMax = 1300;
        s.ch.command(1U);
        StateIs err = { StateError };
        uint32_t t = s.runUntil(err, 3600000U);
        CHECK(s.ch.state() == StateError && t <= 5U * 60000U + 200U);
        CHECK((s.ch.errorBits() & ErrMaxTime) != 0U && (s.ch.errorBits() & ErrBulkAh) == 0U);
    }
    /* bulkAhPct: 10 % av 60 Ah = 6 Ah */
    {
        EepromStore st(be());
        setupStore(st, 1500U);
        setManual(st, AutoParams::BulkAhPct, 10U);
        Sim s(st); s.recompute();
        s.vocMax = 1300;
        s.ch.command(1U);
        StateIs err = { StateError };
        s.runUntil(err, 5U * 3600000U);
        CHECK(s.ch.state() == StateError);
        CHECK(rt16(s, RtAmpHours) >= 600U && rt16(s, RtAmpHours) <= 610U);
        /* BESLUT nr 36: egen bit b12, inte ErrMaxTime */
        CHECK(s.ch.errorBits() == ErrBulkAh);
        CHECK(s.ch.latchedErrorBits() == ErrBulkAh);
        CHECK((rt16(s, RtErrorBits) & ErrBulkAh) != 0U && (rt16(s, RtErrorBits) & ErrMaxTime) == 0U);
        CHECK((rt16(s, RtLatchedErrorBits) & ErrMaxTime) == 0U);
        /* E4 kvitterar som for ovriga fel */
        s.ch.command(4U); s.tick();
        CHECK(s.ch.state() == StateWaiting && s.ch.errorBits() == 0U);
    }
    /* forladdning max */
    {
        EepromStore st(be());
        setupStore(st, 1500U);
        Sim s(st); s.recompute();
        s.vocBase = 500; s.vocMax = 600;
        s.ch.command(1U);
        StateIs err = { StateError };
        uint32_t t = s.runUntil(err, 5U * 3600000U);
        CHECK(s.ch.state() == StateError);
        CHECK(t <= (uint32_t)s.ar.eff[AutoParams::PrechargeMaxTime] * 60000U + 200U);
    }
}

static void testAutoRefusals()
{
    /* AH_MISSING (kapacitet 0) -> start nekas i Auto */
    EepromStore st(be());
    setupStore(st, 1500U);
    setU16(st, 4, P5CapacityAh, 0U);
    Sim s(st); s.recompute();
    CHECK((s.ar.warn & AutoParams::WarnAhMissing) != 0U);
    s.ch.command(1U); s.run(1000U);
    CHECK(s.ch.state() == StateWaiting && s.ch.duty() == 0U);
    /* chargeMode Manuell (tabell ej implementerad) -> utgang av */
    setU16(st, 4, P5CapacityAh, 600U); s.recompute();
    st.page(4)[P5ModeBits] = (uint8_t)((st.page(4)[P5ModeBits] & ~3U) | ChargeModeManual);
    s.ch.command(1U); s.run(1000U);
    CHECK(s.ch.state() == StateWaiting && s.ch.duty() == 0U);
    /* Auto utan batteri -> nekas */
    st.page(4)[P5ModeBits] = (uint8_t)(st.page(4)[P5ModeBits] & ~3U);
    s.connected = false;
    s.ch.command(1U); s.run(1000U);
    CHECK(s.ch.state() == StateWaiting);
    CHECK((rt8(s, RtStatusBits) & StatusBattery) == 0U);
}

static void testAutoStartOnConnect()
{
    EepromStore st(be());
    setupStore(st, 1500U);
    st.page(1)[P2StartBits] = (uint8_t)((st.page(1)[P2StartBits] & ~3U) | StartAuto);
    /* batteri anslutet vid boot: ingen start */
    Sim s(st); s.recompute();
    s.run(3000U);
    CHECK(s.ch.state() == StateWaiting);
    /* ur och i: start */
    s.connected = false; s.run(500U);
    CHECK(s.ch.state() == StateWaiting);
    s.connected = true; s.run(200U);
    CHECK(s.ch.state() == StateCharging);
    /* E2: ingen ny autostart forran ur/i igen */
    s.ch.command(2U); s.run(3000U);
    CHECK(s.ch.state() == StateWaiting);
    s.connected = false; s.run(500U); s.connected = true; s.run(200U);
    CHECK(s.ch.state() == StateCharging);
}

static void testTimerStart()
{
    EepromStore st(be());
    setupStore(st, 1500U);
    st.page(1)[P2StartBits] = (uint8_t)((st.page(1)[P2StartBits] & ~3U) | StartTimer);
    setU16(st, 1, P2StartDelayMin, 2U);
    Sim s(st); s.recompute();
    s.ch.command(1U); s.tick();
    CHECK(s.ch.state() == StateTimer && s.ch.chargingActive() && s.ch.duty() == 0U);
    CHECK((rt8(s, RtStatusBits) & StatusTimer) != 0U);
    CHECK(rt16(s, RtTimerRemaining) == 120U);
    s.run(60000U);
    CHECK(rt16(s, RtTimerRemaining) == 60U || rt16(s, RtTimerRemaining) == 61U);
    StateIs ch = { StateCharging };
    uint32_t t = s.runUntil(ch, 120000U);
    CHECK(s.ch.state() == StateCharging && t <= 60100U);
    /* E2 under timer */
    s.ch.command(2U); s.ch.command(1U); s.tick();
    CHECK(s.ch.state() == StateTimer);
    s.ch.command(2U); s.tick();
    CHECK(s.ch.state() == StateWaiting && !s.ch.chargingActive());
}

static void testCountersAndDiag()
{
    EepromStore st(be());
    setupStore(st, 1500U);
    st.page(4)[P5ModeBits] = (uint8_t)((st.page(4)[P5ModeBits] & ~3U) | ChargeModeSimple);
    setU16(st, 0, EepromLayout::P1ChargeAmp, 1000U);
    Sim s(st); s.recompute();
    s.ch.command(1U); s.tick();
    s.vocBase = 1300; s.vocMax = 1300;   /* konstant Voc 13,00 V */
    s.run(5000U);
    s.forceCa = 999;    /* under granen, duty > 0 behalls */
    s.ch.command(3U);
    s.run(3600000U);
    {
        uint8_t rt[RtBlockSize]; s.ch.writeRealtime(rt);
        uint16_t ah = scGetU16le(rt + RtAmpHours);
        uint32_t wh = scGetU32le(rt + RtWattHours);
        CHECK(ah >= 997U && ah <= 1000U);                     /* 9,99 A x 1 h */
        /* duty gar till 25 (matt 9,99 A < 10 A): U = 13,00 + 1,25 = 14,25 V
         * -> 14,25 V x 9,99 A x 1 h = 142,36 Wh = 1423 x 0,1 Wh */
        CHECK(s.measure().outCv == 1425U);
        CHECK(wh >= 1422U && wh <= 1424U);
        CHECK(scGetU32le(rt + RtChargeTime) >= 3599U && scGetU32le(rt + RtChargeTime) <= 3601U);
    }
    s.ch.command(3U); s.tick();
    CHECK(rt16(s, RtAmpHours) == 0U);
    /* diagMode != 0 -> Diag, utgang av, chargingActive endast for diag 1 */
    s.forceCa = -1;
    st.page(2)[P3DiagMode] = 2U; s.tick();
    CHECK(s.ch.state() == StateDiag && s.ch.duty() == 0U && !s.ch.chargingActive());
    CHECK((rt8(s, RtStatusBits) & StatusDiag) != 0U);
    st.page(2)[P3DiagMode] = 1U; s.tick();
    CHECK(s.ch.chargingActive());
    st.page(2)[P3DiagMode] = 0U; s.tick();
    CHECK(s.ch.state() == StateWaiting && !s.ch.chargingActive());
    /* okanda kommandon hanteras inte har */
    CHECK(!s.ch.command(5U) && !s.ch.command(9U) && !s.ch.command(0U));
}

/* BESLUT nr 35: B avvisad under laddning -> restrictionBits b8 */
static bool g_simCharging(void* c) { return static_cast<Sim*>(c)->ch.chargingActive(); }
static void g_simOff(void* c) { static_cast<Sim*>(c)->ch.forceOff(); }

static uint16_t rtRestr(const Sim& s, const EepromStore& st)
{
    uint8_t rt[RtBlockSize]; memset(rt, 0, sizeof(rt)); s.ch.writeRealtime(rt);
    tsRtSetBurnRejected(rt, st.burnRejectedCharging());
    return scGetU16le(rt + RtRestrictionBits);
}

static void testBurnRejectedIndicator()
{
    CHECK(RestrBurnRejected == 0x0100U && ErrBulkAh == 0x1000U);
    CHECK((RestrBurnRejected & 0x00FFU) == 0U);       /* SPEC b0..b7 orords */
    CHECK((ErrBulkAh & 0x0FFFU) == 0U);               /* SPEC b0..b11 orords */

    /* hjalparen andrar bara b8 */
    {
        uint8_t rt[RtBlockSize]; memset(rt, 0xA5, sizeof(rt));
        tsRtSetBurnRejected(rt, true);
        CHECK(scGetU16le(rt + RtRestrictionBits) == (uint16_t)(0xA5A5U | 0x0100U));
        tsRtSetBurnRejected(rt, false);
        CHECK(scGetU16le(rt + RtRestrictionBits) == (uint16_t)(0xA5A5U & ~0x0100U));
        CHECK(rt[RtRestrictionBits - 1U] == 0xA5U && rt[RtErrorBits] == 0xA5U);
    }

    EepromStore st(be());
    setupStore(st, 1500U);
    st.page(4)[P5ModeBits] = (uint8_t)((st.page(4)[P5ModeBits] & ~3U) | ChargeModeSimple);
    Sim s(st); s.recompute();
    BurnGuard g; g.chargingActive = g_simCharging; g.outputOff = g_simOff; g.outputRestore = 0; g.ctx = &s;
    st.setBurnGuard(g);
    CHECK(!st.burnRejectedCharging() && (rtRestr(s, st) & RestrBurnRejected) == 0U);

    /* laddning pagar: B avvisas, b8 satts, laddningen fortsatter */
    s.ch.command(1U); s.run(1000U);
    CHECK(s.ch.chargingActive());
    CHECK(st.burn() == EepromStore::BurnRejectedCharging);
    CHECK(st.burnRejectedCharging());
    CHECK((rtRestr(s, st) & RestrBurnRejected) != 0U);
    CHECK(s.ch.chargingActive() && s.ch.duty() > 0U);
    CHECK(st.warnBits() == 0U);                      /* layout nr 21: eepromWarnBits orord */
    s.run(1000U);
    CHECK((rtRestr(s, st) & RestrBurnRejected) != 0U);   /* kvar tills ny burn */

    /* laddning stoppad: biten ligger kvar (andringarna ar fortfarande osparade) */
    s.ch.command(2U); s.tick();
    CHECK(!s.ch.chargingActive());
    CHECK((rtRestr(s, st) & RestrBurnRejected) != 0U);
    (void)s.ch.command(1U);                          /* b8 blockerar inte start */
    s.tick();
    CHECK(s.ch.state() == StateCharging);
    s.ch.command(2U); s.tick();

    /* nasta burn som kommer forbi villkoren nollar biten */
    /* (nop-backend: verifieringsfel; b8 nollas anda, WarnBurnFailed tar over) */
    CHECK(st.burn() == EepromStore::BurnErrorVerify);
    CHECK(!st.burnRejectedCharging() && (rtRestr(s, st) & RestrBurnRejected) == 0U);

    /* avvisning utan guard (BurnRejectedNoGuard) satter inte biten */
    EepromStore ng(be());
    CHECK(ng.burn() == EepromStore::BurnRejectedNoGuard && !ng.burnRejectedCharging());
}

int main()
{
    testBootOffAndMcvZero();
    testSimpleMode();
    testMinPwmAndCap();
    testAutoFullCycle();
    testAutoTimeouts();
    testAutoRefusals();
    testAutoStartOnConnect();
    testTimerStart();
    testCountersAndDiag();
    testBurnRejectedIndicator();
    printf("test_charger: %d kontroller, %d fel\n", g_checks, g_fails);
    return (g_fails == 0) ? 0 : 1;
}
