/*
 * avr_probe.cpp - INTE firmware. Minsta AVR-program som lankar in
 * EepromStore + sEeprom-backend for att mata flash/RAM-kostnaden med
 * avr-size och kontrollera att AVR-koden kompilerar utan varningar.
 * Bygg: make -C SimpleChargerBeta1/test avr-probe
 */
#include <avr/io.h>
#include "eepromStore.hpp"
#include "sEeprom.hpp"

static EepromStore store(sEepromBackend());

/* Attrapper for BurnGuard sa att kostnaden for krokarna kommer med i
 * matningen. I firmware: laddning pagar? / duty 0 / aterstall duty. */
static bool probeCharging(void*) { return GPIOR2 == 0x01U; }
static void probeOutputOff(void*) { GPIOR1 = 0U; }
static void probeOutputRestore(void*) { GPIOR1 = 1U; }

int main(void)
{
    BurnGuard guard;
    guard.chargingActive = &probeCharging;
    guard.outputOff = &probeOutputOff;
    guard.outputRestore = &probeOutputRestore;
    guard.ctx = 0;
    store.setBurnGuard(guard);
    uint8_t warn = store.load();
    /* Lasbar sidoeffekt sa att inget optimeras bort. */
    GPIOR0 = warn;
    GPIOR1 = store.page(0)[EepromLayout::P1MaxPWM];
    if (GPIOR2 == 0xB5U)  /* aldrig sant i praktiken: burn bara pa explicit villkor */
    {
        GPIOR0 = (uint8_t)store.burn();
    }
    for (;;)
    {
    }
}
