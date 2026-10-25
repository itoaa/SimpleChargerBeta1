/*
 * sEeprom.hpp
 *
 * AVR-backend for EepromStore (shared/eepromStore.hpp) via avr-libc.
 * Lasning: eeprom_read_block. Skrivning: eeprom_update_block (skriver bara
 * byte som andrats -> mindre slitage). Varje skriven byte tar ca 3,3 ms och
 * funktionerna blockerar tills skrivningen ar klar.
 */
#ifndef SMEOS_ATMEL328P_INCLUDE_SEEPROM_HPP_
#define SMEOS_ATMEL328P_INCLUDE_SEEPROM_HPP_

#include "eepromStore.hpp"

/* Backend med avr-libc-funktionerna (ctx anvands inte). */
EepromBackend sEepromBackend(void);

#endif /* SMEOS_ATMEL328P_INCLUDE_SEEPROM_HPP_ */
