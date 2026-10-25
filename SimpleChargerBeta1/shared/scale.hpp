/*
 * ADC counts to volts and milliamps. The caller owns the ADC.
 */

#ifndef SHARED_SCALE_HPP_
#define SHARED_SCALE_HPP_

#include <stdint.h>

/* ADC 1023 maps to maxVolt in the unit the caller passes in. */
uint16_t scaleAdcToVolt(uint16_t adc, uint16_t maxVolt);

/* ACS712: ADC -> mA i 32-bitars heltal, ingen double (BESLUT nr 37).
 * refMv och offsetMv i mV, mVperAmp = givarens mV/A. adc > 1023 -> 1023,
 * mVperAmp <= 0 -> 0. Mellanvarden: |adc*refMv| <= 1023*32767,
 * |(mv - offset)*1000| <= 65534*1000, ryms i int32. Avrundning: mV
 * avrundas till narmaste, mA trunkeras mot 0 (som tidigare (int)-cast). */
int32_t scaleAcs712Ma32(uint16_t adc, int16_t refMv, int16_t offsetMv, int16_t mVperAmp);

/* Grans for strom i mA som ryms i 16 bit (int pa AVR). */
const int32_t ScaleMaLimit = 32767L;

/* Som scaleAcs712Ma32 men klampat till +-32767 mA FORE omvandlingen
 * till int16_t. *saturated = true om klampning skedde (restrictionBits
 * b9 via main). saturated far vara 0. */
int16_t scaleAcs712MaSat(uint16_t adc, int16_t refMv, int16_t offsetMv, int16_t mVperAmp,
                         bool* saturated);

#endif /* SHARED_SCALE_HPP_ */
