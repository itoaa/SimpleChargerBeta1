/*
 * tunerStudio.hpp
 *
 * TunerStudio-protokoll v2.2 (tunerStudio_v2.2.ini, signatur
 * "MSII s-Charger 2.2  "). Ingen UART har: main matar onByte() och skickar
 * svaren via setSend(). Sidorna ligger i EepromStore (RAM-kopian); W/X
 * andrar bara RAM. EEPROM skrivs enbart via B -> krok burn (BurnGuard i
 * EepromStore). Inga automatiska skrivningar.
 *
 * Kommandon (1 byte offset/antal, sidor <= 255 B):
 *   Q  signatur (20 B)            S  versionstext
 *   F  protokollversion "002"     A  realtidsblock (115 B)
 *   C/c sekunder (1 byte, lag byte av realtime.seconds)
 *   P<n>  aktiv sida n = 0..4 (ogiltigt n ignoreras, aktiv sida behalls)
 *   V     las hela aktiva sidan
 *   W<o><v>     skriv 1 byte i aktiva sidan (utanfor sidan ignoreras)
 *   X<o><c><d..> skriv c byte (c <= 64). Hela blocket tillampas atomart
 *                eller inte alls (c > 64 eller o + c > sidstorlek).
 *   B     burn (krok). E<k>  kommando k (krok), k = 1..9 enligt ini.
 * Ett pabarjat kommando som inte fullbordas inom TimeoutMs kastas.
 */
#ifndef SHARED_TUNERSTUDIO_HPP_
#define SHARED_TUNERSTUDIO_HPP_

#include <stdint.h>
#include "eepromStore.hpp"
#include "tsLayout.hpp"

class TunerStudio
{
public:
    enum { MaxChunk = 64U };          /* blockingFactor i ini */
    enum { TimeoutMs = 500U };        /* [U] vald marginal, ej fran spec */

    struct Hooks
    {
        void (*burn)(void* ctx);                     /* B */
        void (*command)(uint8_t code, void* ctx);    /* E<code> */
        /* efter godkand W/X: sida, forsta offset och antal byte (BESLUT nr 45:
         * main klampar hardvarutak bara i falt som skrivningen tackte helt) */
        void (*pageWritten)(uint8_t page, uint8_t offset, uint8_t len, void* ctx);
        void* ctx;
    };

    /* Realtidsblocket som A skickar. Fylls av main/Charger. */
    uint8_t realtime[TsLayout::RtBlockSize];

    explicit TunerStudio(EepromStore& store);

    void setSend(void (*send)(const uint8_t* data, uint8_t len, void* ctx), void* ctx);
    void setHooks(const Hooks& hooks);
    void onByte(uint8_t byte, uint32_t nowMs);
    /* Avbryt ett halvfardigt kommando efter TimeoutMs utan byte. */
    void poll(uint32_t nowMs);
    uint8_t activePage() const { return page_; }

private:
    TunerStudio(const TunerStudio&);
    TunerStudio& operator=(const TunerStudio&);

    enum State { Idle, PPage, EData, WOffset, WValue, XOffset, XCount, XData, XDiscard };

    EepromStore& store_;
    State state_;
    uint8_t page_;
    uint8_t offset_;
    uint8_t count_;
    uint8_t got_;
    uint32_t lastByteMs_;
    void (*sendFn_)(const uint8_t* data, uint8_t len, void* ctx);
    void* sendCtx_;
    Hooks hooks_;
    uint8_t chunk_[MaxChunk];

    void send(const uint8_t* data, uint8_t len);
    void handleCommand(uint8_t command);
    void applyWrite(const uint8_t* src, uint8_t len);
};

#endif /* SHARED_TUNERSTUDIO_HPP_ */
