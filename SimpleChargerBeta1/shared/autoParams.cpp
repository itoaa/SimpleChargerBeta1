/*
 * autoParams.cpp
 *
 * Port av auto_params_v2.2.py (auto_values, effective, comp_target).
 * Alla mellanvarden ar int32 och icke-negativa utom temperaturkompen-
 * seringen, dar tecknet hanteras explicit (Python // golvar, C trunkerar;
 * se divRound och compTarget). Max mellanvarde: 720 * 65535 < 2^31.
 */
#include "autoParams.hpp"
#include "tsLayout.hpp"

namespace AutoParams {

namespace {

/* Kolumner i typtabellen (TYPES i auto_params_v2.2.py, utan key/namn). */
enum Col {
    CCrate = 0, CAbsV, CTail, CFloatV, CFloatA, CAbsMax, CFloatMax, CTc,
    CRestV, CPrechV, CPrechA, CVMax, CPresV, CTMin, CTMax, CChem, ColCount
};

const uint8_t TypeCount = 6U;   /* index 6 Anpassad, 7 INVALID */
const uint8_t NomvCount = 4U;   /* 6/12/24/36 V; index 4..7 ogiltiga (BESLUT nr 45) */

/* Typtabell i flash. tc lagras som tvakomplement (uint16) och tolkas S16. */
const uint16_t kTypes[TypeCount][ColCount] SC_PROGMEM = {
    /* crate absV   tail floatV floatA absMax floatMax tc               restV  prechV prechA vMax   presV tMin tMax chem */
    { 100U, 24500U, 30U, 22500U, 50U,  240U,  2880U,  (uint16_t)0xFFD8U, 20833U, 17500U, 30U, 25000U, 5000U, 0U, 45U, 0U }, /* WET   */
    { 200U, 24000U, 20U, 22667U, 50U,  240U,  1440U,  (uint16_t)0xFFD8U, 20833U, 17500U, 30U, 25000U, 5000U, 0U, 45U, 0U }, /* AGM   */
    { 100U, 23750U, 15U, 22833U, 50U,  240U,  1440U,  (uint16_t)0xFFD8U, 20833U, 17500U, 30U, 25000U, 5000U, 0U, 45U, 0U }, /* GEL   */
    { 100U, 24500U, 20U, 22500U, 50U,  240U,  1440U,  (uint16_t)0xFFD8U, 20833U, 17500U, 30U, 25000U, 5000U, 0U, 45U, 0U }, /* DEEP  */
    { 100U, 24500U, 30U, 22500U, 50U,  240U,  1440U,  (uint16_t)0xFFD8U, 20833U, 17500U, 30U, 25000U, 5000U, 0U, 45U, 0U }, /* CAEFB */
    { 500U, 35500U, 0U,  33750U, 50U,  120U,  1440U,  0U,                33000U, 25000U, 30U, 36500U, 5000U, 5U, 50U, 1U }, /* LFP   */
};

const uint8_t kCells[2][NomvCount] SC_PROGMEM = {
    { 3U, 6U, 12U, 18U },   /* bly */
    { 2U, 4U, 8U, 12U },    /* LiFePO4 */
};

const uint16_t kSensorMaxCa[4] SC_PROGMEM = { 500U, 2000U, 3000U, 0U };

const int32_t HwAmpMaxCa     = 1500;
const int32_t TotalTimeMax   = 10080;
const int32_t BulkAhPctDef   = 120;
const int32_t BulkTimeFactor = 720;
const int32_t BulkTimeMin    = 60;
const int32_t BulkTimeMax    = 1200;
const int32_t AbsMinTimeS    = 60;
const int32_t PrechMaxMin    = 120;
const int32_t CompTMinDc     = -100;
const int32_t CompTMaxDc     = 450;
const int32_t TRefDc         = 250;

/* man_X offset i sida 5 och typ (0 U08, 1 S08, 2 U16), PARAMS-ordning. */
const uint8_t kManOff[ParamCount] SC_PROGMEM = {
    12U, 13U, 15U, 17U, 19U, 21U, 23U, 25U, 26U, 28U, 30U,
    32U, 34U, 35U, 36U, 37U, 39U, 41U, 43U, 45U, 47U
};
const uint8_t kManType[ParamCount] SC_PROGMEM = {
    0U, 2U, 2U, 2U, 2U, 2U, 2U, 0U, 2U, 2U, 2U,
    2U, 1U, 1U, 1U, 2U, 2U, 2U, 2U, 2U, 2U
};

uint16_t flashWord(const uint16_t* p)
{
#if defined(__AVR__)
    return pgm_read_word(p);
#else
    return *p;
#endif
}

int32_t typeCol(uint8_t t, uint8_t c)
{
    uint16_t raw = flashWord(&kTypes[t][c]);
    if (c == CTc) {
        int16_t s;
        memcpy(&s, &raw, 2);
        return (int32_t)s;
    }
    return (int32_t)raw;
}

/* _div_round: (a + b // 2) // b, bara for a >= 0, b > 0. */
int32_t divRound(int32_t a, int32_t b)
{
    return (a + (b / 2)) / b;
}

int32_t vpc(int32_t u01mV, int32_t cells)
{
    return divRound(u01mV * cells, 100);
}

int32_t min32(int32_t a, int32_t b) { return (a < b) ? a : b; }
int32_t max32(int32_t a, int32_t b) { return (a > b) ? a : b; }

int32_t hwAmpCap(int32_t ampCapCa, int32_t maxChargeAmp, uint8_t sensorIdx)
{
    int32_t cap = min32(ampCapCa, HwAmpMaxCa);
    if (maxChargeAmp > 0) {
        cap = min32(cap, maxChargeAmp);
    }
    int32_t smax = 0;
    if (sensorIdx < 4U) {
        smax = (int32_t)flashWord(&kSensorMaxCa[sensorIdx]);
    }
    if (smax > 0) {
        cap = min32(cap, smax);
    }
    return cap;
}

int32_t bulkTime(int32_t ahD, int32_t bulkCa, uint16_t& w)
{
    int32_t bt = BulkTimeMax;
    if (bulkCa > 0) {
        bt = ((BulkTimeFactor * ahD) + bulkCa - 1) / bulkCa;
    }
    if ((bt < BulkTimeMin) || (bt > BulkTimeMax)) {
        w = (uint16_t)(w | WarnTimeClamped);
    }
    return max32(BulkTimeMin, min32(BulkTimeMax, bt));
}

int32_t totalTime(int32_t prech, int32_t bulk, int32_t absorb, int32_t flt)
{
    return min32(TotalTimeMax, prech + bulk + absorb + flt);
}

int32_t floorCap(int32_t x, int32_t floorCa, int32_t bulk, uint16_t& w)
{
    if (x < floorCa) {
        x = floorCa;
        w = (uint16_t)(w | WarnTailFloor);
    }
    return min32(x, bulk);
}

int32_t vclamp(int32_t x, int32_t vlim, uint16_t& w)
{
    if (x > vlim) {
        w = (uint16_t)(w | WarnVoltClamped);
        return vlim;
    }
    return x;
}

int32_t readMan(const uint8_t* page5, uint8_t id)
{
    const uint8_t* p = page5 + manOffset(id);
    uint8_t t = manType(id);
    if (t == 0U) {
        return (int32_t)p[0];
    }
    if (t == 1U) {
        return (int32_t)scGetS8(p);
    }
    return (int32_t)scGetU16le(p);
}

/* auto_values(): false = Anpassad/ogiltig (inga autovarden). */
bool autoValues(const uint8_t* page5, const Input& in, int32_t* v, uint16_t& w)
{
    const uint8_t typeBits = page5[TsLayout::P5TypeBits];
    const uint8_t btype = scBits(typeBits, 0U, 0x07U);
    const uint8_t nomv  = scBits(typeBits, 3U, 0x07U);
    const int32_t ahD   = (int32_t)scGetU16le(page5 + TsLayout::P5CapacityAh);
    const int32_t ampCap = (int32_t)scGetU16le(page5 + TsLayout::P5AutoAmpCap);
    const int32_t floorCa = (int32_t)scGetU16le(page5 + TsLayout::P5TailAmpFloor);

    w = 0U;
    if (btype >= TypeCount) {
        w = WarnCustomType;
        return false;
    }
    if (nomv >= NomvCount) {
        w = WarnNomvInvalid;
        return false;
    }
    if (ahD == 0) {
        w = (uint16_t)(w | WarnAhMissing);
    }
    const int32_t chem  = typeCol(btype, CChem);
    const int32_t cells = (int32_t)scReadFlashByte(&kCells[chem][nomv]);
    const int32_t cap   = hwAmpCap(ampCap, (int32_t)in.maxChargeAmp, in.currentSensor);

    int32_t bulk = divRound(typeCol(btype, CCrate) * ahD, 100);
    if (bulk > cap) {
        bulk = cap;
        w = (uint16_t)(w | WarnAmpCapped);
    }
    int32_t tail = 0;
    if (typeCol(btype, CTail) != 0) {
        tail = floorCap(divRound(typeCol(btype, CTail) * ahD, 100), floorCa, bulk, w);
    }
    const int32_t flo = floorCap(divRound(typeCol(btype, CFloatA) * ahD, 100), floorCa, bulk, w);
    const int32_t pre = floorCap(divRound(typeCol(btype, CPrechA) * ahD, 100), floorCa, bulk, w);

    int32_t vlim = vpc(typeCol(btype, CVMax), cells);
    if (in.maxChargeVolt > 0U) {
        vlim = min32(vlim, (int32_t)in.maxChargeVolt);
    }
    const int32_t absV = vclamp(vpc(typeCol(btype, CAbsV), cells), vlim, w);
    const int32_t fltV = vclamp(vpc(typeCol(btype, CFloatV), cells), vlim, w);
    const int32_t bt = bulkTime(ahD, bulk, w);

    v[CellCount]          = cells;
    v[BulkAmp]            = bulk;
    v[AbsorbVolt]         = absV;
    v[FloatVolt]          = fltV;
    v[TailAmp]            = tail;
    v[FloatAmp]           = flo;
    v[BulkMaxTime]        = bt;
    v[BulkAhPct]          = BulkAhPctDef;
    v[AbsorbMaxTime]      = typeCol(btype, CAbsMax);
    v[AbsorbMinTime]      = AbsMinTimeS;
    v[FloatMaxTime]       = typeCol(btype, CFloatMax);
    v[RestartVolt]        = vpc(typeCol(btype, CRestV), cells);
    v[TempCompMv]         = typeCol(btype, CTc);
    v[ChargeTempMin]      = typeCol(btype, CTMin);
    v[ChargeTempMax]      = typeCol(btype, CTMax);
    v[PrechargeVolt]      = vpc(typeCol(btype, CPrechV), cells);
    v[PrechargeAmp]       = pre;
    v[PrechargeMaxTime]   = PrechMaxMin;
    v[VoltLimit]          = vlim;
    v[TotalMaxTime]       = totalTime(PrechMaxMin, bt, typeCol(btype, CAbsMax), typeCol(btype, CFloatMax));
    v[BatteryPresentVolt] = vpc(typeCol(btype, CPresV), cells);
    return true;
}

bool bit(uint32_t mask, uint8_t id)
{
    return ((mask >> id) & 1UL) != 0UL;
}

} /* namespace */

uint8_t manOffset(uint8_t id)
{
    return (id < ParamCount) ? scReadFlashByte(&kManOff[id]) : 0U;
}

uint8_t manType(uint8_t id)
{
    return (id < ParamCount) ? scReadFlashByte(&kManType[id]) : 0U;
}

void compute(const uint8_t* page5, const Input& in, Result& out, bool ignoreOverrides)
{
    int32_t* eff = out.eff;
    uint16_t w = 0U;
    const uint32_t ovr = ((uint32_t)page5[TsLayout::P5OvrMask]
                        | ((uint32_t)page5[TsLayout::P5OvrMask + 1U] << 8)
                        | ((uint32_t)page5[TsLayout::P5OvrMask + 2U] << 16)) & (ignoreOverrides ? 0UL : AllMask);
    uint32_t mask = ovr;
    uint8_t i;

    if (!autoValues(page5, in, eff, w)) {
        mask = AllMask;
        for (i = 0U; i < ParamCount; ++i) {
            eff[i] = 0;
        }
    }
    for (i = 0U; i < ParamCount; ++i) {
        if (bit(mask, i)) {
            eff[i] = readMan(page5, i);
        }
    }
    if (ovr != 0UL) {
        w = (uint16_t)(w | WarnOverride);
    }

    /* Hardvarugranser galler alltid, aven for manuella varden. */
    const int32_t mcv = (int32_t)in.maxChargeVolt;
    const int32_t mca = (int32_t)in.maxChargeAmp;
    if ((mcv > 0) && (eff[VoltLimit] > mcv)) {
        eff[VoltLimit] = mcv;
        w = (uint16_t)(w | WarnVoltClamped);
    }
    if (eff[AbsorbVolt] > eff[VoltLimit]) {
        eff[AbsorbVolt] = eff[VoltLimit];
        w = (uint16_t)(w | WarnVoltClamped);
    }
    if (eff[FloatVolt] > eff[VoltLimit]) {
        eff[FloatVolt] = eff[VoltLimit];
        w = (uint16_t)(w | WarnVoltClamped);
    }
    const int32_t hcap = hwAmpCap(HwAmpMaxCa, mca, in.currentSensor);
    static const uint8_t ampIds[4] = { BulkAmp, TailAmp, FloatAmp, PrechargeAmp };
    for (i = 0U; i < 4U; ++i) {
        if (eff[ampIds[i]] > hcap) {
            eff[ampIds[i]] = hcap;
            w = (uint16_t)(w | WarnAmpCapped);
        }
    }

    /* v2.2: harledda tider foljer de effektiva vardena. */
    const int32_t ahD = (int32_t)scGetU16le(page5 + TsLayout::P5CapacityAh);
    if (!bit(mask, BulkMaxTime) && bit(mask, BulkAmp)) {
        eff[BulkMaxTime] = bulkTime(ahD, eff[BulkAmp], w);
    }
    if (!bit(mask, TotalMaxTime)) {
        eff[TotalMaxTime] = totalTime(eff[PrechargeMaxTime], eff[BulkMaxTime],
                                      eff[AbsorbMaxTime], eff[FloatMaxTime]);
    }
    if (eff[FloatMaxTime] == 0) {
        w = (uint16_t)(w | WarnFloatUnlimited);
    }
    const bool tce = scBits(page5[TsLayout::P5ModeBits], 2U, 0x01U) != 0U;
    if (tce && (eff[TempCompMv] != 0) && !in.batSensor) {
        w = (uint16_t)(w | WarnTcNoSensor);
    }
    if ((eff[FloatVolt] >= eff[AbsorbVolt])
        || ((eff[TailAmp] != 0) && (eff[TailAmp] >= eff[BulkAmp]))
        || (eff[ChargeTempMin] >= eff[ChargeTempMax])
        || (eff[RestartVolt] >= eff[FloatVolt])) {
        w = (uint16_t)(w | WarnInconsistent);
    }
    out.manualMask = mask;
    out.warn = w;
}

int32_t compTarget(int32_t vCv, const Result& r, int16_t tDc, bool enabled)
{
    const int32_t vlim = r.eff[VoltLimit];
    if (!enabled || (r.eff[TempCompMv] == 0)) {
        return min32(vCv, vlim);
    }
    const int32_t t = max32(CompTMinDc, min32(CompTMaxDc, (int32_t)tDc));
    int32_t d = r.eff[TempCompMv] * r.eff[CellCount] * (t - TRefDc);   /* 0,01 mV */
    /* Symmetrisk avrundning: delas alltid som icke-negativt tal. */
    if (d < 0) {
        d = -divRound(-d, 1000);
    } else {
        d = divRound(d, 1000);
    }
    return max32(0, min32(vCv + d, vlim));
}

void writeRealtime(const Result& r, uint8_t* rt)
{
    uint8_t i;
    for (i = 0U; i < ParamCount; ++i) {
        uint8_t* p = rt + manOffset(i) + TsLayout::EffMinusMan;
        const int32_t v = r.eff[i];
        if (manType(i) == 2U) {
            scPutU16le(p, (uint16_t)(v & 0xFFFF));
        } else {
            p[0] = (uint8_t)(v & 0xFF);   /* U08/S08: tvakomplement */
        }
    }
    scPutU32le(rt + TsLayout::RtEffManualMask, r.manualMask);
    scPutU16le(rt + TsLayout::RtAutoWarnBits, r.warn);
}

void copyEffectiveToManual(const Result& r, uint8_t* page5)
{
    uint8_t i;
    for (i = 0U; i < ParamCount; ++i) {
        uint8_t* p = page5 + manOffset(i);
        const int32_t v = r.eff[i];
        if (manType(i) == 2U) {
            scPutU16le(p, (uint16_t)(v & 0xFFFF));
        } else {
            p[0] = (uint8_t)(v & 0xFF);
        }
    }
}

} /* namespace AutoParams */
