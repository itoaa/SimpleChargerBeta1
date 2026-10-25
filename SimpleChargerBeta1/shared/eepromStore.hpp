/*
 * eepromStore.hpp
 *
 * Lagring av TunerStudio v2.2-sidorna (1-5) i EEPROM.
 *
 *  - RAM-kopia av alla sidor (292 B), statiskt allokerad i objektet.
 *  - load(): last vid boot. Giltigt huvud + CRC -> EEPROM-varden, annars
 *    standardvarden + varningsbit. diagMode tvingas till Normal. Skriver ALDRIG.
 *  - burn(): ENDA vagen till EEPROM-skrivning (TunerStudio "B"). Anvander
 *    backendens update (skriver bara andrade byte). Huvudet skrivs sist.
 *  - Ingen heap, ingen STL, inga virtuella funktioner.
 *
 * Hardvaran abstraheras med EepromBackend (funktionspekare) sa att logiken
 * kan testas pa PC med en mock (test/). AVR-backend: sEeprom.hpp.
 *
 * OBS: modulen ar INTE inkopplad i main.cpp/regulatorn i denna version.
 */
#ifndef EEPROM_STORE_HPP_
#define EEPROM_STORE_HPP_

#include <stdint.h>
#include "eepromLayout.hpp"

struct EepromBackend
{
    /* Las len byte fran EEPROM-adress till dst. */
    void (*read)(uint16_t address, uint8_t* dst, uint16_t len, void* ctx);
    /* Skriv len byte fran src till EEPROM-adress; bara andrade byte ska skrivas. */
    void (*update)(uint16_t address, const uint8_t* src, uint16_t len, void* ctx);
    void* ctx;
};

/* Standardvarden for alla sidor i foljd (292 B). Ligger i flash pa AVR. */
extern const uint8_t eepromDefaults[EepromLayout::PagesTotalSize] SC_PROGMEM;

class EepromStore
{
public:
    /* Varningsbitar efter load()/burn(). 0 = allt last fran EEPROM. */
    enum
    {
        WarnHeaderInvalid = 0x01U, /* tomt/okant/gammalt (B-2) innehall: alla sidor = standard */
        WarnPage1Crc      = 0x02U, /* sida 1 hade fel CRC -> standardvarden */
        WarnPage2Crc      = 0x04U,
        WarnPage3Crc      = 0x08U,
        WarnPage4Crc      = 0x10U,
        WarnPage5Crc      = 0x20U,
        WarnBurnFailed    = 0x40U, /* aterlasning efter burn stammer inte */
        WarnNoBackend     = 0x80U  /* backend saknar read/update */
    };

    explicit EepromStore(const EepromBackend& backend);

    /* Fyll RAM-kopian med standardvarden. Skriver inte EEPROM. */
    void loadDefaults();

    /* Las EEPROM till RAM. Returnerar varningsbitarna. Skriver aldrig EEPROM. */
    uint8_t load();

    /* Spara alla sidor + huvud till EEPROM och verifiera genom aterlasning.
     * diagMode sparas alltid som Normal. Returnerar true om verifieringen gick bra. */
    bool burn();

    uint8_t warnBits() const { return warn_; }

    /* Sidans RAM-data, 0 om index ar utanfor 0..4. */
    uint8_t*       page(uint8_t index);
    const uint8_t* page(uint8_t index) const;
    uint16_t       pageSize(uint8_t index) const;

    /* Gransskontrollerad lasning/skrivning i RAM-kopian (inte EEPROM).
     * false om sida, offset eller langd ar utanfor sidan. */
    bool readBytes(uint8_t index, uint16_t offset, uint8_t* dst, uint16_t len) const;
    bool writeBytes(uint8_t index, uint16_t offset, const uint8_t* src, uint16_t len);

private:
    EepromStore(const EepromStore&);            /* ej kopierbar */
    EepromStore& operator=(const EepromStore&);

    bool backendOk() const;
    bool headerValid() const;
    bool loadPage(uint8_t index);
    void defaultsForPage(uint8_t index);
    void forceDiagNormal();
    uint8_t storedByte(uint8_t index, uint16_t offset) const;
    uint16_t storedCrc(uint8_t index) const;
    bool rangeOk(uint8_t index, uint16_t offset, uint16_t len) const;

    EepromBackend backend_;
    uint8_t data_[EepromLayout::PagesTotalSize];
    uint8_t warn_;
};

#endif /* EEPROM_STORE_HPP_ */
