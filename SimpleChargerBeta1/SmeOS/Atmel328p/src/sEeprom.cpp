/*
 * sEeprom.cpp - AVR-backend for EepromStore, se sEeprom.hpp.
 */
#include <avr/eeprom.h>
#include "sEeprom.hpp"

namespace {

void avrRead(uint16_t address, uint8_t* dst, uint16_t len, void* ctx)
{
    (void)ctx;
    /* avr-libc tar EEPROM-adressen som pekare. */
    eeprom_read_block(dst, (const void*)(uintptr_t)address, len);
}

void avrUpdate(uint16_t address, const uint8_t* src, uint16_t len, void* ctx)
{
    (void)ctx;
    eeprom_update_block(src, (void*)(uintptr_t)address, len);
}

} /* namespace */

EepromBackend sEepromBackend(void)
{
    EepromBackend b;
    b.read = &avrRead;
    b.update = &avrUpdate;
    b.ctx = 0;
    return b;
}
