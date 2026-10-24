/*
 * ADC counts to volts and milliamps. The caller owns the ADC.
 */

#ifndef SHARED_SCALE_HPP_
#define SHARED_SCALE_HPP_

#include <stdint.h>

/* ADC 1023 maps to maxVolt in the unit the caller passes in. */
uint16_t scaleAdcToVolt(uint16_t adc, uint16_t maxVolt);

/* refMv and offsetMv are millivolts. mVperAmp is the sensor millivolts per amp. */
int scaleAcs712Ma(uint16_t adc, int refMv, int offsetMv, int mVperAmp);

#endif /* SHARED_SCALE_HPP_ */
