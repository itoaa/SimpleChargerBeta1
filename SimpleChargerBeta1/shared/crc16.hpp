/*
 * crc16.hpp
 *
 * CRC-16/CCITT-FALSE: polynom 0x1021, startvarde 0xFFFF, ingen reflektion,
 * ingen slut-XOR. Kontrollvarde: "123456789" -> 0x29B1.
 * Bitvis berakning (ingen tabell) for att spara flash pa ATmega328P.
 */
#ifndef CRC16_HPP_
#define CRC16_HPP_

#include <stdint.h>

const uint16_t Crc16Init = 0xFFFFU;

uint16_t crc16Update(uint16_t crc, uint8_t data);
uint16_t crc16(const uint8_t* data, uint16_t len);

#endif /* CRC16_HPP_ */
