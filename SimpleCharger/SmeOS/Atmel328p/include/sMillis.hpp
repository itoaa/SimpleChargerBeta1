/*
 * 1 ms-räknare på timer 0. PWM använder timer 1.
 */

#ifndef SMEOS_ATMEL328P_INCLUDE_SMILLIS_HPP_
#define SMEOS_ATMEL328P_INCLUDE_SMILLIS_HPP_

#include <stdint.h>

void millisInit(void);
uint32_t millis(void);

#endif /* SMEOS_ATMEL328P_INCLUDE_SMILLIS_HPP_ */
