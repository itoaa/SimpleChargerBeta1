/*
 * Skalning från ADC-steg till volt och milliampere.
 * Nästa kort läser sin egen ADC och anropar samma funktioner.
 */

#ifndef SHARED_SCALE_HPP_
#define SHARED_SCALE_HPP_

#include <stdint.h>

/* adc 1023 motsvarar maxVolt i den enhet anroparen skickar in. */
uint16_t scaleAdcToVolt(uint16_t adc, uint16_t maxVolt);

/* refMv och offsetMv är millivolt. mVperAmp är givarens millivolt per ampere. */
int scaleAcs712Ma(uint16_t adc, int refMv, int offsetMv, int mVperAmp);

#endif /* SHARED_SCALE_HPP_ */
