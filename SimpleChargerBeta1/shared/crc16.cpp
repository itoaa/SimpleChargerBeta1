/*
 * crc16.cpp - CRC-16/CCITT-FALSE, se crc16.hpp.
 */
#include "crc16.hpp"

uint16_t crc16Update(uint16_t crc, uint8_t data)
{
    crc = (uint16_t)(crc ^ (uint16_t)((uint16_t)data << 8));
    for (uint8_t bit = 0U; bit < 8U; ++bit)
    {
        if ((crc & 0x8000U) != 0U)
        {
            crc = (uint16_t)((uint16_t)(crc << 1) ^ 0x1021U);
        }
        else
        {
            crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

uint16_t crc16(const uint8_t* data, uint16_t len)
{
    uint16_t crc = Crc16Init;
    if (data != 0)
    {
        for (uint16_t i = 0U; i < len; ++i)
        {
            crc = crc16Update(crc, data[i]);
        }
    }
    return crc;
}
