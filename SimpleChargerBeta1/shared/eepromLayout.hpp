/*
 * eepromLayout.hpp
 *
 * EEPROM-layout for TunerStudio v2.2 (sida 1-5 = 38/84/38/68/64 B = 292 B).
 * Kalla for sidstorlekar och offset: SPEC_tabeller_v2.2.md och
 * SmedbyCharger_v2.2.ini (Drive, LTT-Sweden). Se docs/eeprom-layout.md.
 *
 * Regler: C++98 (gnu++98), ingen heap, ingen STL. Sidorna lagras som
 * byte-arrayer. Fleroktetsfalt laser/skrivs ENDAST via byte-hjalparna nedan
 * (little-endian), aldrig via ojusterad pekarcast.
 *
 * Sidindex ar 0-baserat (0..4) och motsvarar ini-kommandona P\000..P\004,
 * dvs sida 1..5 i specen.
 */
#ifndef EEPROM_LAYOUT_HPP_
#define EEPROM_LAYOUT_HPP_

#include <stdint.h>
#include <string.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#define SC_PROGMEM PROGMEM
/* Laser en byte ur flash (standardvarden ligger i flash, inte i RAM). */
static inline uint8_t scReadFlashByte(const uint8_t* p) { return pgm_read_byte(p); }
#else
#define SC_PROGMEM
static inline uint8_t scReadFlashByte(const uint8_t* p) { return *p; }
#endif

namespace EepromLayout {

/* ---- Sidor ---------------------------------------------------------- */
const uint8_t  PageCount      = 5U;
const uint16_t Page1Size      = 38U;
const uint16_t Page2Size      = 84U;
const uint16_t Page3Size      = 38U;
const uint16_t Page4Size      = 68U;
const uint16_t Page5Size      = 64U;
const uint16_t PagesTotalSize = Page1Size + Page2Size + Page3Size + Page4Size + Page5Size;

/* Offset for varje sida i den sammanhangande RAM-bufferten (och i
 * standardvardestabellen). */
const uint16_t Page1RamOffset = 0U;
const uint16_t Page2RamOffset = Page1RamOffset + Page1Size;
const uint16_t Page3RamOffset = Page2RamOffset + Page2Size;
const uint16_t Page4RamOffset = Page3RamOffset + Page3Size;
const uint16_t Page5RamOffset = Page4RamOffset + Page4Size;

/* ---- EEPROM-adresser ------------------------------------------------- */
/* Basadress 16 (BESLUT.md nr 20, se docs/eeprom-layout.md):
 *  - adress 0 undviks: vid brownout under skrivning ar adress 0 den
 *    vanligaste korrumperade cellen (EEAR nollas vid reset) [U: erfarenhet
 *    fran AVR-communityt, inte databladet];
 *  - 0..15 reserveras (skrivs aldrig av modulen) for framtida bruk;
 *  - 16 + 308 = 324 <= 1024: 700 B marginal for senare layouter.
 * Huvud: magic[2] + layoutversion + antal sidor + CRC16 over de 4 forsta. */
const uint16_t ReservedBytes  = 16U;     /* 0..15 reserverade, ror ej */
const uint16_t BaseAddress    = ReservedBytes;
const uint8_t  Magic0         = 0x53U;   /* 'S' */
const uint8_t  Magic1         = 0x43U;   /* 'C' */
const uint8_t  LayoutVersion  = 1U;      /* hoj vid varje layoutandring */
const uint16_t HeaderDataSize = 4U;
const uint16_t CrcSize        = 2U;
const uint16_t HeaderSize     = HeaderDataSize + CrcSize;
/* Varje sida lagras som data + CRC16 (little-endian). */
const uint16_t TotalSize      = HeaderSize + PagesTotalSize + (PageCount * CrcSize);
const uint16_t EepromCapacity = 1024U;   /* ATmega328P */

const uint16_t Page1Address   = BaseAddress + HeaderSize;
const uint16_t Page2Address   = Page1Address + Page1Size + CrcSize;
const uint16_t Page3Address   = Page2Address + Page2Size + CrcSize;
const uint16_t Page4Address   = Page3Address + Page3Size + CrcSize;
const uint16_t Page5Address   = Page4Address + Page4Size + CrcSize;

/* ---- Sida 1 (index 0): falt-offset i v2.2 ------------------------------ */
const uint16_t P1ChargeAmp          = 0U;   /* U16 0,01 A */
const uint16_t P1MaxChargeAmp       = 2U;   /* U16 0,01 A */
const uint16_t P1MaxInputVolt       = 4U;   /* U16 0,01 V */
const uint16_t P1MaxChargeVolt      = 6U;   /* U16 0,01 V, 0 = utgang av */
const uint16_t P1MaxInputAmp        = 8U;   /* U16 0,01 A */
const uint16_t P1MaxPWM             = 10U;  /* U08 steg */
const uint16_t P1MinPWM             = 11U;  /* U08 steg */
const uint16_t P1MaxTemp            = 12U;  /* U08 C */
const uint16_t P1AccelerationDelay  = 13U;  /* U08, raa x 10 = ms (BESLUT.md nr 15: enhet ms, skala 10) */
const uint16_t P1MosfetTempADCTable = 14U;  /* U08[12], enhet ADC/4 (10-bit ADC >> 2, BESLUT.md nr 16) */
const uint16_t P1MosfetTempTable    = 26U;  /* S08[12] */
const uint16_t P1TableLength        = 12U;

/* ---- Sida 4 (index 3): tabeller --------------------------------------- */
const uint16_t P4BatNtcADC      = 0U;   /* U16[12] ADC 10 bit */
const uint16_t P4BatNtcTemp     = 24U;  /* S08[12] C */
const uint16_t P4DerateTempBins = 36U;  /* S08[6] C */
const uint16_t P4DeratePct      = 42U;  /* U08[6] % */
const uint16_t P4AmpTimeBins    = 48U;  /* U16[5] s */
const uint16_t P4AmpTimeLimit   = 58U;  /* U16[5] 0,01 A */
const uint16_t P4DerateLength   = 6U;
const uint16_t P4AmpTimeLength  = 5U;

/* ---- Sida 3 (index 2): diagnoslage ------------------------------------ */
/* diagMode = bits [0:1] i byte 16. Far aldrig overleva en omstart. */
const uint16_t P3DiagModeOffset = 16U;
const uint8_t  P3DiagModeMask   = 0x03U;

/* Storlek for sida (0-baserat index), 0 om index ar utanfor. */
uint16_t pageSize(uint8_t page);
/* Offset i RAM-bufferten, 0 om index ar utanfor. */
uint16_t pageRamOffset(uint8_t page);
/* EEPROM-adress for sidans data (CRC ligger direkt efter), 0 om utanfor. */
uint16_t pageAddress(uint8_t page);

} /* namespace EepromLayout */

/* ---- Statiska kontroller (C++98: negativ arraystorlek = kompileringsfel) */
typedef char eepromPagesTotalIs292[(EepromLayout::PagesTotalSize == 292U) ? 1 : -1];
typedef char eepromTotalIs308[(EepromLayout::TotalSize == 308U) ? 1 : -1];
typedef char eepromBaseNotZero[((EepromLayout::BaseAddress != 0U) && (EepromLayout::BaseAddress >= EepromLayout::ReservedBytes)) ? 1 : -1];
typedef char eepromFitsInDevice[((EepromLayout::BaseAddress + EepromLayout::TotalSize) <= EepromLayout::EepromCapacity) ? 1 : -1];
typedef char eepromPageSizesFitU08[((EepromLayout::Page1Size | EepromLayout::Page2Size | EepromLayout::Page3Size | EepromLayout::Page4Size | EepromLayout::Page5Size) < 256U) ? 1 : -1];
typedef char eepromP1TablesEndAtPageEnd[((EepromLayout::P1MosfetTempTable + EepromLayout::P1TableLength) == EepromLayout::Page1Size) ? 1 : -1];
typedef char eepromP4TablesEndAtPageEnd[((EepromLayout::P4AmpTimeLimit + (2U * EepromLayout::P4AmpTimeLength)) == EepromLayout::Page4Size) ? 1 : -1];
typedef char eepromP5EndsAtTotal[((EepromLayout::Page5Address + EepromLayout::Page5Size + EepromLayout::CrcSize) == (EepromLayout::BaseAddress + EepromLayout::TotalSize)) ? 1 : -1];

/* ---- Little-endian byte-hjalpare ------------------------------------- */
static inline uint16_t scGetU16le(const uint8_t* p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

static inline void scPutU16le(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)((v >> 8) & 0xFFU);
}

/* S16 via memcpy mellan lika stora heltal: bitmonstret behalls, ingen
 * implementationsdefinierad konvertering. */
static inline int16_t scGetS16le(const uint8_t* p)
{
    const uint16_t u = scGetU16le(p);
    int16_t s;
    memcpy(&s, &u, sizeof(s));
    return s;
}

static inline void scPutS16le(uint8_t* p, int16_t v)
{
    uint16_t u;
    memcpy(&u, &v, sizeof(u));
    scPutU16le(p, u);
}

#endif /* EEPROM_LAYOUT_HPP_ */
