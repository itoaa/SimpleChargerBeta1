/*
 * eepromDefaults.cpp
 *
 * Standardvarden for TunerStudio v2.2 sida 1-5 (ravarden, little-endian).
 * Kalla: SPEC_tabeller_v2.2.md och defaultValue i SmedbyCharger_v2.2.ini.
 * Falt utan angivet standardvarde = 0 (bl.a. alla tabeller pa sida 1 och 4).
 * Testet test/test_eeprom.cpp jamfor byte for byte mot golden_defaults.inc,
 * som genereras direkt ur ini-filen.
 */
#include "eepromStore.hpp"

#define U16LE(v) (uint8_t)((uint16_t)(v) & 0xFFU), (uint8_t)(((uint16_t)(v) >> 8) & 0xFFU)
#define S08(v)   (uint8_t)(int8_t)(v)
#define Z4       0U, 0U, 0U, 0U
#define Z12      Z4, Z4, Z4

const uint8_t eepromDefaults[EepromLayout::PagesTotalSize] SC_PROGMEM =
{
    /* ---- Sida 1 (38 B) ---- */
    U16LE(0),       /*  0 ChargeAmp      0,01 A */
    U16LE(0),       /*  2 MaxChargeAmp   0,01 A */
    U16LE(0),       /*  4 MaxInputVolt   0,01 V */
    U16LE(0),       /*  6 MaxChargeVolt  0,01 V (0 = utgang av, beslut Ola) */
    U16LE(0),       /*  8 MaxInputAmp    0,01 A */
    5U,             /* 10 MaxPWM */
    0U,             /* 11 MinPWM */
    0U,             /* 12 MaxTemp (0 = av) */
    0U,             /* 13 accelerationDelay */
    Z12,            /* 14 mosfetTempADCTable[12] */
    Z12,            /* 26 mosfetTempTable[12] */

    /* ---- Sida 2 (84 B) ---- */
    0x04U,          /*  0 startMode=Manuell(0), restartMode=Omstart(bit2) */
    0U,             /*  1 (oanvand) */
    U16LE(0),       /*  2 startDelayMin */
    /* s1: CC, Nasta */
    0x00U, S08(45), U16LE(1500), U16LE(1440), U16LE(1440), U16LE(0), U16LE(0), U16LE(0),
    U16LE(600), U16LE(0), S08(0), 0U,
    /* s2: CV, Nasta */
    0x01U, S08(45), U16LE(1500), U16LE(1440), U16LE(0), U16LE(0), U16LE(0), U16LE(100),
    U16LE(240), U16LE(60), S08(0), 0U,
    /* s3: CV, Nasta */
    0x01U, S08(45), U16LE(200), U16LE(1360), U16LE(0), U16LE(0), U16LE(0), U16LE(0),
    U16LE(0), U16LE(0), S08(0), 0U,
    /* s4: Av(3), Stopp klar(2<<2) */
    0x0BU, S08(45), U16LE(0), U16LE(0), U16LE(0), U16LE(0), U16LE(0), U16LE(0),
    U16LE(0), U16LE(0), S08(0), 0U,

    /* ---- Sida 3 (38 B) ---- */
    31U,            /*  0 pwmFreqKHz */
    U16LE(2000),    /*  1 softStartMs */
    1U,             /*  3 dutyStepMax */
    100U,           /*  4 driverVoltMin 10,0 V (0,1 V) */
    160U,           /*  5 driverVoltMax 16,0 V (0,1 V) */
    0x50U,          /*  6 mosfetTemp=Ingen, batTemp=Ingen, ACS712-20A(1<<4), ampZeroAtBoot(bit6), inAmp=Ingen */
    S08(70),        /*  7 derateStartTemp */
    10U,            /*  8 tempHyst */
    U16LE(1000),    /*  9 minInputVolt 10,00 V */
    U16LE(1000),    /* 11 currentRestoreMs */
    1U,             /* 13 currentRestoreStep */
    3U,             /* 14 faultRetryCount */
    30U,            /* 15 faultRetryDelay */
    0U,             /* 16 diagMode = Normal (tvingas vid load) */
    0U,             /* 17 diagDuty */
    U16LE(10000), U16LE(0),   /* 18 calOutVolt  skala 1,0000 / offset 0 */
    U16LE(10000), U16LE(0),   /* 22 calInVolt */
    U16LE(10000), U16LE(0),   /* 26 calOutAmp */
    U16LE(10000), U16LE(0),   /* 30 calInAmp */
    U16LE(10000), U16LE(0),   /* 34 calDriver */

    /* ---- Sida 4 (68 B): inga standardvarden i spec/ini -> 0 ---- */
    Z12, Z12,       /*  0 batNtcADC U16[12] */
    Z12,            /* 24 batNtcTemp S08[12] */
    0U, 0U, 0U, 0U, 0U, 0U,  /* 36 derateTempBins S08[6] */
    0U, 0U, 0U, 0U, 0U, 0U,  /* 42 deratePct U08[6] */
    Z4, Z4, 0U, 0U,          /* 48 ampTimeBins U16[5] */
    Z4, Z4, 0U, 0U,          /* 58 ampTimeLimit U16[5] */

    /* ---- Sida 5 (64 B) ---- */
    0x04U,          /*  0 chargeMode=Auto(0), tempCompEnable=Pa(bit2) */
    0x08U,          /*  1 batteryType=SLI(0), batteryNomVolt=12 V(1<<3) */
    U16LE(600),     /*  2 capacityAh 60,0 Ah */
    U16LE(1500),    /*  4 autoAmpCap 15,00 A */
    U16LE(30),      /*  6 tailAmpFloor 0,30 A */
    0U, 0U, 0U,     /*  8 ovr_* = Auto */
    0U,             /* 11 (oanvand) */
    6U,             /* 12 man_cellCount */
    U16LE(600),     /* 13 man_bulkAmp 6,00 A */
    U16LE(1470),    /* 15 man_absorbVolt 14,70 V */
    U16LE(1350),    /* 17 man_floatVolt 13,50 V */
    U16LE(180),     /* 19 man_tailAmp 1,80 A */
    U16LE(300),     /* 21 man_floatAmp 3,00 A */
    U16LE(720),     /* 23 man_bulkMaxTime min */
    120U,           /* 25 man_bulkAhPct % */
    U16LE(240),     /* 26 man_absorbMaxTime min */
    U16LE(60),      /* 28 man_absorbMinTime s */
    U16LE(2880),    /* 30 man_floatMaxTime min */
    U16LE(1250),    /* 32 man_restartVolt 12,50 V */
    S08(-40),       /* 34 man_tempCompMv -4,0 mV/C/cell */
    S08(0),         /* 35 man_chargeTempMin */
    S08(45),        /* 36 man_chargeTempMax */
    U16LE(1050),    /* 37 man_prechargeVolt 10,50 V */
    U16LE(180),     /* 39 man_prechargeAmp 1,80 A */
    U16LE(120),     /* 41 man_prechargeMaxTime min */
    U16LE(1500),    /* 43 man_voltLimit 15,00 V */
    U16LE(3960),    /* 45 man_totalMaxTime min */
    U16LE(300),     /* 47 man_batteryPresentVolt 3,00 V */
    Z12, 0U, 0U, 0U /* 49 autoReserved[15] */
};

#undef U16LE
#undef S08
#undef Z4
#undef Z12
