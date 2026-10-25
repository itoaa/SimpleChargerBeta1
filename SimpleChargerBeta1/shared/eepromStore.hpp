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
 *    Kraver en BurnGuard (BESLUT.md nr 7): avvisas om laddning pagar, annars
 *    stangs utgangen fore skrivningen och aterstalls efter.
 *  - Ingen heap, ingen STL, inga virtuella funktioner.
 *
 * Hardvaran abstraheras med EepromBackend (funktionspekare) sa att logiken
 * kan testas pa PC med en mock (test/). AVR-backend: sEeprom.hpp.
 *
 * Inkopplad i main.cpp fran etapp 3: load() vid uppstart, burn() endast pa
 * TS-kommandot B (via BurnGuard), aldrig automatiskt.
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

/* Villkor och krokar for burn (BESLUT.md nr 7).
 * Firmware kopplar in dessa nar modulen kopplas in med v2.2-protokollet. */
struct BurnGuard
{
    /* true = laddning pagar -> burn avvisas utan att nagot skrivs. Kravs. */
    bool (*chargingActive)(void* ctx);
    /* Stang utgangen (duty 0) innan EEPROM skrivs. Kravs. */
    void (*outputOff)(void* ctx);
    /* Aterstall utgangen efter skrivningen (anropas alltid efter outputOff,
     * aven om verifieringen misslyckas). Far vara 0. */
    void (*outputRestore)(void* ctx);
    void* ctx;
};

/* Realtidsdata: ny statusbyte for EEPROM-varningarna (BESLUT.md nr 8 och 21).
 * Laggs efter 114 B-blocket i SPEC_tabeller_v2.2.md (offset 0..113 ar fullt
 * belagda), dvs ochBlockSize 114 -> 115. Bitarna = EepromStore::warnBits().
 * Fran etapp 2 ar 115 B infort i protokollet (TsLayout::RtBlockSize) och
 * i tunerStudio_v2.2.ini; BlockSizeV22 = blockstorleken enligt SPEC_tabeller. */
namespace EepromRealtime {
const uint16_t BlockSizeV22     = 114U;  /* blocket enligt SPEC_tabeller_v2.2.md */
const uint16_t WarnBitsOffset   = 114U;  /* eepromWarnBits, U08 */
const uint16_t BlockSizeProposed = 115U; /* infort i etapp 2 (A ger 115 B) */
}

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

    /* Resultat av burn(). */
    enum BurnResult
    {
        BurnOk               = 0, /* skrivet och verifierat */
        BurnRejectedCharging = 1, /* laddning pagar: inget skrivet, utgang orord */
        BurnRejectedNoGuard  = 2, /* ingen BurnGuard satt: inget skrivet (sakert standardlage) */
        BurnErrorNoBackend   = 3, /* backend saknas: inget skrivet */
        BurnErrorVerify      = 4  /* skrivet men aterlasningen stammer inte */
    };

    explicit EepromStore(const EepromBackend& backend);

    /* Satt villkor/krokar for burn. Utan guard (eller med chargingActive
     * eller outputOff = 0) avvisas burn med BurnRejectedNoGuard. */
    void setBurnGuard(const BurnGuard& guard);

    /* Fyll RAM-kopian med standardvarden. Skriver inte EEPROM. */
    void loadDefaults();

    /* Las EEPROM till RAM. Returnerar varningsbitarna. Skriver aldrig EEPROM. */
    uint8_t load();

    /* Spara alla sidor + huvud till EEPROM och verifiera genom aterlasning.
     * diagMode sparas alltid som Normal. Ordning: backend -> guard ->
     * chargingActive (avvisa) -> outputOff -> skriv + verifiera -> outputRestore.
     * Vid avvisning andras varken EEPROM, utgang eller varningsbitar.
     * Lyckad burn nollar varningsbitarna (BESLUT.md nr 8). */
    BurnResult burn();

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
    bool guardOk() const;
    bool writeAndVerify();
    bool headerValid() const;
    bool loadPage(uint8_t index);
    void defaultsForPage(uint8_t index);
    void forceDiagNormal();
    uint8_t storedByte(uint8_t index, uint16_t offset) const;
    uint16_t storedCrc(uint8_t index) const;
    bool rangeOk(uint8_t index, uint16_t offset, uint16_t len) const;

    EepromBackend backend_;
    BurnGuard guard_;
    uint8_t data_[EepromLayout::PagesTotalSize];
    uint8_t warn_;
};

#endif /* EEPROM_STORE_HPP_ */
