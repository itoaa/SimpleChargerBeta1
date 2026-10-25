/*
 * charger.cpp - laddlogik v2.2, se charger.hpp och BESLUT.md nr 25-27.
 */
#include "charger.hpp"
#include "eepromLayout.hpp"
#include "tsLayout.hpp"

using namespace TsLayout;

namespace {

const uint32_t MsPerMin = 60000UL;
const uint32_t MsPerCAh = 3600000UL;      /* 1 cA under 3 600 000 ms = 1 cAh */
const uint32_t WhAccPerUnit = 360000000UL;/* (cV*cA/10)*ms per 0,1 Wh */
const uint32_t EnergyDtMax = 1000UL;      /* ms per varv i raknarna, skydd mot overflow */

uint16_t clampU16(int32_t v)
{
    if (v < 0) { return 0U; }
    if (v > 65535L) { return 65535U; }
    return (uint16_t)v;
}

uint16_t absCa(int16_t v)
{
    return (v < 0) ? (uint16_t)(-(int32_t)v) : (uint16_t)v;
}

uint16_t minU16(uint16_t a, uint16_t b)
{
    return (a < b) ? a : b;
}

} /* namespace */

Charger::Charger()
    : state_(StateWaiting), step_(0U), duty_(0U), statusBits_(0U), runMode_(ChargeModeSimple),
      diagMode_(0U), startReq_(false), armed_(false), battery_(false), started_(false),
      errorBits_(0U), stickyErr_(0U), latched_(0U), restr_(0U), vSet_(0U), iSet_(0U),
      compDelta_(0), cAh_(0U), stageCAh0_(0U), ampAcc_(0U), whAcc_(0U), wh_(0U),
      chargeMs_(0U), totalMs_(0U), stageMs_(0U), tailMs_(0U), timerMs_(0U), lastMs_(0U)
{
}

bool Charger::chargingActive() const
{
    return (state_ == StateTimer) || (state_ == StateSoftStart) || (state_ == StateCharging) ||
           (state_ == StatePauseTemp) || (duty_ > 0U) || (diagMode_ == 1U);
}

bool Charger::command(uint8_t code)
{
    bool handled = true;
    switch (code)
    {
    case 1U:   /* Start */
        startReq_ = true;
        break;
    case 2U:   /* Stopp: tillbaka till Vantar, ingen autostart forran batteriet kopplats ur/in */
        startReq_ = false;
        armed_ = false;
        stopTo(StateWaiting);
        break;
    case 3U:   /* Nollstall visade raknare (inte skyddstiden totalMs_) */
        cAh_ = 0U;
        stageCAh0_ = 0U;
        ampAcc_ = 0U;
        whAcc_ = 0U;
        wh_ = 0U;
        chargeMs_ = 0U;
        break;
    case 4U:   /* Kvittera fel */
        stickyErr_ = 0U;
        latched_ = errorBits_;
        if (state_ == StateError)
        {
            state_ = StateWaiting;
        }
        break;
    default:
        handled = false;
        break;
    }
    return handled;
}

void Charger::stopTo(uint8_t state)
{
    state_ = state;
    duty_ = 0U;
    timerMs_ = 0U;
    vSet_ = 0U;
    iSet_ = 0U;
    compDelta_ = 0;
    if (state != StateDone)
    {
        step_ = 0U;
    }
}

void Charger::fail(uint16_t err)
{
    stickyErr_ |= err;
    latched_ |= err;
    stopTo(StateError);
}

void Charger::enterStep(uint8_t s)
{
    step_ = s;
    stageMs_ = 0U;
    tailMs_ = 0U;
    stageCAh0_ = cAh_;
}

void Charger::beginCharge(uint8_t mode, uint16_t outCv, const AutoParams::Result& ar)
{
    runMode_ = mode;
    state_ = StateCharging;
    totalMs_ = 0U;
    chargeMs_ = 0U;
    if ((mode == ChargeModeAuto) && ((int32_t)outCv < ar.eff[AutoParams::PrechargeVolt]))
    {
        enterStep(0U);
    }
    else if (mode == ChargeModeAuto)
    {
        enterStep(1U);
    }
    else
    {
        enterStep(0U);
    }
}

void Charger::accumulate(const Measure& m, uint32_t dt)
{
    uint32_t de;
    if ((duty_ == 0U) || (m.outCa <= 0))
    {
        return;
    }
    de = (dt > EnergyDtMax) ? EnergyDtMax : dt;
    ampAcc_ += (uint32_t)(uint16_t)m.outCa * de;
    while (ampAcc_ >= MsPerCAh)
    {
        ampAcc_ -= MsPerCAh;
        if (cAh_ < 65535U) { cAh_++; }
    }
    /* 6500 cV * 1500 cA / 10 = 975 000; * 1000 ms < 2^32 */
    whAcc_ += (((uint32_t)m.outCv * (uint32_t)(uint16_t)m.outCa) / 10UL) * de;
    while (whAcc_ >= WhAccPerUnit)
    {
        whAcc_ -= WhAccPerUnit;
        if (wh_ < 0xFFFFFFFFUL) { wh_++; }
    }
}

void Charger::autoStep(const Measure& m, uint16_t absCv, uint16_t floatCv,
                       const AutoParams::Result& ar, uint16_t capDAh, uint32_t dt)
{
    const int32_t* e = ar.eff;
    const uint16_t a = absCa(m.outCa);
    const uint32_t stageCAh = (uint32_t)(uint16_t)(cAh_ - stageCAh0_);

    switch (step_)
    {
    case 0U:   /* Forladdning: CC prechargeAmp tills V > prechargeVolt */
        iSet_ = clampU16(e[AutoParams::PrechargeAmp]);
        vSet_ = absCv;
        if ((int32_t)m.outCv > e[AutoParams::PrechargeVolt])
        {
            enterStep(1U);
        }
        else if ((e[AutoParams::PrechargeMaxTime] > 0) &&
                 (stageMs_ >= (uint32_t)e[AutoParams::PrechargeMaxTime] * MsPerMin))
        {
            fail(ErrMaxTime);
        }
        else {}
        break;
    case 1U:   /* Bulk: CC bulkAmp tills V >= absorb */
        iSet_ = clampU16(e[AutoParams::BulkAmp]);
        vSet_ = absCv;
        if (m.outCv >= absCv)
        {
            enterStep(2U);
        }
        else if ((e[AutoParams::BulkMaxTime] > 0) &&
                 (stageMs_ >= (uint32_t)e[AutoParams::BulkMaxTime] * MsPerMin))
        {
            fail(ErrMaxTime);
        }
        else if ((e[AutoParams::BulkAhPct] > 0) && (capDAh > 0U) &&
                 /* cAh * 10 >= dAh * pct (kapacitet 0,1 Ah, pct %) */
                 ((stageCAh * 10UL) >= ((uint32_t)capDAh * (uint32_t)e[AutoParams::BulkAhPct])))
        {
            fail(ErrMaxTime);
        }
        else {}
        break;
    case 2U:   /* Absorption: CV absorb tills I < tail i absorbMinTime s, eller absorbMaxTime */
        iSet_ = clampU16(e[AutoParams::BulkAmp]);
        vSet_ = absCv;
        if ((e[AutoParams::TailAmp] > 0) && ((int32_t)a < e[AutoParams::TailAmp]))
        {
            tailMs_ += dt;
        }
        else
        {
            tailMs_ = 0U;
        }
        if (((e[AutoParams::TailAmp] > 0) && (tailMs_ >= (uint32_t)e[AutoParams::AbsorbMinTime] * 1000UL) &&
             (tailMs_ > 0U)) ||
            ((e[AutoParams::AbsorbMaxTime] > 0) &&
             (stageMs_ >= (uint32_t)e[AutoParams::AbsorbMaxTime] * MsPerMin)))
        {
            enterStep(3U);
        }
        break;
    case 3U:   /* Float: CV float, strom floatAmp, tills floatMaxTime (0 = obegransat) */
        iSet_ = clampU16(e[AutoParams::FloatAmp]);
        vSet_ = floatCv;
        if ((e[AutoParams::FloatMaxTime] > 0) &&
            (stageMs_ >= (uint32_t)e[AutoParams::FloatMaxTime] * MsPerMin))
        {
            enterStep(4U);
            stopTo(StateDone);
        }
        break;
    default:
        break;
    }
}

uint8_t Charger::update(const Measure& m, const Pages& pg, const AutoParams::Result& ar, uint32_t nowMs)
{
    const uint8_t* p1 = pg.p1;
    const int32_t* e = ar.eff;
    const uint16_t a = absCa(m.outCa);
    uint32_t dt;
    uint16_t mca;
    uint16_t mcv;
    uint16_t miv;
    uint8_t cap;
    uint8_t minPwm;
    uint8_t chargeMode;
    uint8_t startMode;
    bool restartOn;
    bool ready;
    bool hardCut;
    bool atLimit;
    uint16_t err;
    uint16_t absCv;
    uint16_t floatCv;

    dt = started_ ? (uint32_t)(nowMs - lastMs_) : 0U;
    lastMs_ = nowMs;
    started_ = true;

    /* --- konfiguration --- */
    mca = scGetU16le(p1 + EepromLayout::P1MaxChargeAmp);
    if ((mca == 0U) || (mca > (uint16_t)AbsMaxChargeAmp))
    {
        mca = (uint16_t)AbsMaxChargeAmp;
    }
    mcv = scGetU16le(p1 + EepromLayout::P1MaxChargeVolt);
    miv = scGetU16le(p1 + EepromLayout::P1MaxInputVolt);
    cap = p1[EepromLayout::P1MaxPWM];
    if (cap > (uint8_t)DutyHardMax)
    {
        cap = (uint8_t)DutyHardMax;
    }
    minPwm = p1[EepromLayout::P1MinPWM];
    chargeMode = scBits(pg.p5[P5ModeBits], 0U, 3U);
    startMode = scBits(pg.p2[P2StartBits], 0U, 3U);
    restartOn = (scBits(pg.p2[P2StartBits], 2U, 1U) != 0U);
    diagMode_ = scBits(pg.p3[P3DiagMode], 0U, EepromLayout::P3DiagModeMask);

    /* --- batteri och levande fel (som tidigare: inte latchande, per varv) --- */
    battery_ = (e[AutoParams::BatteryPresentVolt] > 0) &&
               ((int32_t)m.outCv >= e[AutoParams::BatteryPresentVolt]);
    if (!battery_)
    {
        armed_ = true;
    }
    err = 0U;
    if ((mcv > 0U) && (m.outCv >= mcv)) { err |= ErrOverVoltOut; }
    if (a >= mca) { err |= ErrOverAmpOut; }
    if ((miv > 0U) && (m.inCv >= miv)) { err |= ErrHighInVolt; }
    errorBits_ = err;
    latched_ |= err;
    hardCut = (err != 0U);

    /* Utgang av-villkor (MaxChargeVolt 0 = utgang av, BESLUT nr 25) */
    ready = (mcv > 0U) && (cap > 0U) && (minPwm <= cap) &&
            ((chargeMode == ChargeModeAuto) || (chargeMode == ChargeModeSimple));
    if (ready && (chargeMode == ChargeModeAuto) &&
        ((ar.warn & (AutoParams::WarnAhMissing | AutoParams::WarnNomvInvalid)) != 0U))
    {
        ready = false;
    }

    /* --- diagnos: utgangen hanteras inte har (diagMode 1 ej implementerat) --- */
    if (diagMode_ != 0U)
    {
        if (state_ != StateDiag)
        {
            stopTo(StateDiag);
        }
        startReq_ = false;
    }
    else if (state_ == StateDiag)
    {
        stopTo(StateWaiting);
        armed_ = false;
    }
    else {}

    /* --- start --- */
    if ((state_ == StateWaiting) || (state_ == StateDone))
    {
        bool trigger = startReq_ ||
                       ((startMode != StartManual) && armed_ && battery_ && (state_ == StateWaiting));
        if (trigger && ready && (errorBits() == 0U) &&
            ((chargeMode != ChargeModeAuto) || battery_))
        {
            armed_ = false;
            if (startMode == StartTimer)
            {
                state_ = StateTimer;
                step_ = 0U;
                timerMs_ = (uint32_t)scGetU16le(pg.p2 + P2StartDelayMin) * MsPerMin;
            }
            else
            {
                beginCharge(chargeMode, m.outCv, ar);
            }
        }
    }

    /* --- pagaende: avbryt om forutsattningarna forsvinner --- */
    if ((state_ == StateTimer) || (state_ == StateCharging) || (state_ == StateDone))
    {
        if (!ready || ((state_ != StateTimer) && (chargeMode != runMode_)))
        {
            stopTo(StateWaiting);
        }
    }
    if (state_ == StateTimer)
    {
        timerMs_ = (timerMs_ > dt) ? (uint32_t)(timerMs_ - dt) : 0U;
        if (timerMs_ == 0U)
        {
            beginCharge(chargeMode, m.outCv, ar);
        }
    }

    /* --- raknare (forra varvets duty) --- */
    accumulate(m, dt);

    /* --- stegmaskin --- */
    absCv = clampU16(AutoParams::compTarget(e[AutoParams::AbsorbVolt], ar, 0, false));
    floatCv = clampU16(AutoParams::compTarget(e[AutoParams::FloatVolt], ar, 0, false));
    compDelta_ = 0;   /* ingen batteritempgivare (etapp 6): ingen kompensering */
    if (state_ == StateCharging)
    {
        chargeMs_ += dt;
        totalMs_ += dt;
        stageMs_ += dt;
        if (runMode_ == ChargeModeAuto)
        {
            if ((e[AutoParams::TotalMaxTime] > 0) &&
                (totalMs_ >= (uint32_t)e[AutoParams::TotalMaxTime] * MsPerMin))
            {
                fail(ErrMaxTime);
            }
            else
            {
                autoStep(m, absCv, floatCv, ar, scGetU16le(pg.p5 + P5CapacityAh), dt);
            }
        }
        else
        {
            const uint16_t ca = scGetU16le(p1 + EepromLayout::P1ChargeAmp);
            step_ = 0U;
            iSet_ = (ca > 0U) ? minU16(ca, mca) : mca;
            vSet_ = mcv;
        }
    }
    else if ((state_ == StateDone) && (runMode_ == ChargeModeAuto))
    {
        step_ = 4U;
        stageMs_ += dt;
        if (restartOn && battery_ && (e[AutoParams::RestartVolt] > 0) &&
            ((int32_t)m.outCv < e[AutoParams::RestartVolt]))
        {
            state_ = StateCharging;
            totalMs_ = 0U;   /* SPEC: timern nollas vid omstart fran Overvaka */
            enterStep(1U);
        }
    }
    else {}

    /* --- regulator: ett steg per varv, som tidigare --- */
    restr_ = 0U;
    statusBits_ = 0U;
    if ((state_ != StateCharging) || hardCut)
    {
        duty_ = 0U;
    }
    else
    {
        iSet_ = minU16(iSet_, mca);
        vSet_ = minU16(vSet_, mcv);
        atLimit = (a >= iSet_) || (m.outCv >= vSet_);
        if (a >= iSet_) { statusBits_ |= StatusCC; }
        if (m.outCv >= vSet_) { statusBits_ |= StatusCV; }
        if (iSet_ == mca) { restr_ |= RestrMaxOutAmp; }
        if (vSet_ == mcv) { restr_ |= RestrMaxOutVolt; }
        if (atLimit)
        {
            if (duty_ > 0U) { duty_--; }
        }
        else if (duty_ < cap)
        {
            duty_++;
        }
        else
        {
            restr_ |= RestrPwmCap;
        }
        if (duty_ > cap)
        {
            duty_ = cap;
        }
        /* Dodzon under MinPWM: vid granen helt av, annars hoppa till MinPWM. */
        if ((minPwm > 0U) && (duty_ > 0U) && (duty_ < minPwm))
        {
            duty_ = atLimit ? 0U : minPwm;
        }
    }

    if (duty_ > 0U) { statusBits_ |= StatusOutputOn; }
    if (battery_) { statusBits_ |= StatusBattery; }
    if (state_ == StateDiag) { statusBits_ |= StatusDiag; }
    if (state_ == StateTimer) { statusBits_ |= StatusTimer; }
    if ((state_ == StateCharging) && (runMode_ == ChargeModeAuto) && (step_ == 0U))
    {
        statusBits_ |= StatusPrecharge;
    }
    if (state_ != StateCharging)
    {
        vSet_ = 0U;
        iSet_ = 0U;
    }
    startReq_ = false;   /* E1 galler bara det varv da den kom (inte fordrojt) */
    return duty_;
}

void Charger::writeRealtime(uint8_t* rt) const
{
    const uint32_t chargeS = chargeMs_ / 1000UL;
    rt[RtPulseWidth] = duty_;
    rt[RtChargerState] = state_;
    rt[RtStage] = ((runMode_ == ChargeModeAuto) && ((state_ == StateCharging) || (state_ == StateDone))) ? step_ : 0U;
    rt[RtStatusBits] = statusBits_;
    scPutU16le(rt + RtRestrictionBits, restr_);
    scPutU16le(rt + RtErrorBits, errorBits());
    scPutU16le(rt + RtLatchedErrorBits, latched_);
    scPutU16le(rt + RtAmpHours, cAh_);
    scPutU32le(rt + RtWattHours, wh_);
    scPutU16le(rt + RtStageTime, clampU16((int32_t)(stageMs_ / 1000UL)));
    scPutU32le(rt + RtChargeTime, chargeS);
    scPutU16le(rt + RtVoltSetpoint, vSet_);
    scPutU16le(rt + RtAmpSetpoint, iSet_);
    rt[RtDerateNow] = 100U;
    scPutU16le(rt + RtOverAmpTime, 0U);
    scPutU16le(rt + RtTimerRemaining, clampU16((int32_t)((timerMs_ + 999UL) / 1000UL)));
    rt[RtRetriesLeft] = 0U;
    scPutS16le(rt + RtTempCompDelta, compDelta_);
}
