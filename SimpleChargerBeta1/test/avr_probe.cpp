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

int main(void)
{
    uint8_t warn = store.load();
    /* Lasbar sidoeffekt sa att inget optimeras bort. */
    GPIOR0 = warn;
    GPIOR1 = store.page(0)[EepromLayout::P1MaxPWM];
    if (GPIOR2 == 0xB5U)  /* aldrig sant i praktiken: burn bara pa explicit villkor */
    {
        (void)store.burn();
    }
    for (;;)
    {
    }
}
