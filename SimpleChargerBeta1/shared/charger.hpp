/*
 * charger.hpp
 *
 * Laddlogik v2.2 utan hardvara: tillstand, stegmaskin (auto 0..4),
 * enkel CC/CV, regulator, harda granser och raknare. main matar in
 * matvarden och tid; Charger returnerar PWM-steg (duty). Allt i heltal
 * (cV, cA, ms), ingen heap. Host-testas med simulerad tid.
 *
 * chargingActive (BESLUT nr 25) = chargerState i {Timer, SoftStart,
 * Charging, PauseTemp} ELLER duty > 0 ELLER diagMode == 1.
 *
 * Start: aldrig automatiskt vid boot. Laddning startar pa E1, eller vid
 * startMode Auto/Timer nar ett batteri ansluts (franvarande -> narvarande
 * efter boot). Start kraver MaxChargeVolt > 0, chargeMode Auto/Enkel,
 * errorBits == 0, inte Error-lage och (auto) inga AH_MISSING/NOMV_INVALID.
 */
#ifndef SHARED_CHARGER_HPP_
#define SHARED_CHARGER_HPP_

#include <stdint.h>
#include "autoParams.hpp"

class Charger
{
public:
    struct Measure
    {
        uint16_t outCv;     /* utgangsspanning cV */
        uint16_t inCv;      /* ingangsspanning cV */
        int16_t  outCa;     /* utgangsstrom cA (tecken som matt) */
    };

    struct Pages
    {
        const uint8_t* p1;
        const uint8_t* p2;
        const uint8_t* p3;
        const uint8_t* p5;
    };

    enum { AbsMaxChargeAmp = 1500U };   /* cA; 0 eller > 1500 -> 1500 */
    enum { DutyHardMax = 25U };         /* MaxPWM-tak (ini 0..25) */

    Charger();

    /* Ett regulatorvarv (main: var 100 ms). Returnerar duty 0..25. */
    uint8_t update(const Measure& m, const Pages& pg, const AutoParams::Result& ar, uint32_t nowMs);

    /* E1..E4. Returnerar true om koden hanterades har. */
    bool command(uint8_t code);
    /* Utgang av direkt (burn, E6). Tillstand andras inte. */
    void forceOff() { duty_ = 0U; }

    bool chargingActive() const;
    uint8_t duty() const { return duty_; }
    uint8_t state() const { return state_; }
    uint8_t step() const { return step_; }
    uint16_t errorBits() const { return (uint16_t)(errorBits_ | stickyErr_); }
    uint16_t latchedErrorBits() const { return latched_; }

    /* Skriver offset 2..11, 28..44, 60..63, 67, 111..112. */
    void writeRealtime(uint8_t* rt) const;

private:
    uint8_t  state_;
    uint8_t  step_;         /* auto 0..4, enkel 0 */
    uint8_t  duty_;
    uint8_t  statusBits_;
    uint8_t  runMode_;      /* chargeMode vid start */
    uint8_t  diagMode_;
    bool     startReq_;
    bool     armed_;        /* batteri saknats sedan boot/stopp */
    bool     battery_;
    bool     started_;      /* forsta update gjord */
    uint16_t errorBits_;
    uint16_t stickyErr_;
    uint16_t latched_;
    uint16_t restr_;
    uint16_t vSet_;
    uint16_t iSet_;
    int16_t  compDelta_;
    uint16_t cAh_;
    uint16_t stageCAh0_;
    uint32_t ampAcc_;       /* cA*ms, 3 600 000 = 1 cAh */
    uint32_t whAcc_;        /* (cV*cA/10)*ms */
    uint32_t wh_;           /* 0,1 Wh */
    uint32_t chargeMs_;     /* visad laddtid (E3 nollstaller) */
    uint32_t totalMs_;      /* skyddstid totalMaxTime (E3 ror den inte) */
    uint32_t stageMs_;
    uint32_t tailMs_;
    uint32_t timerMs_;
    uint32_t lastMs_;

    void beginCharge(uint8_t mode, uint16_t outCv, const AutoParams::Result& ar);
    void enterStep(uint8_t s);
    void stopTo(uint8_t state);
    void fail(uint16_t err);
    void autoStep(const Measure& m, uint16_t absCv, uint16_t floatCv, const AutoParams::Result& ar,
                  uint16_t capDAh, uint32_t dt);
    void accumulate(const Measure& m, uint32_t dt);
};

#endif /* SHARED_CHARGER_HPP_ */
