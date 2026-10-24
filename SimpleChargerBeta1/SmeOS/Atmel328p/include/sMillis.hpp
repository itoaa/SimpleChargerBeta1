/*
 * 1 ms counter on timer 0. PWM uses timer 1, so this tick must not take timer 1.
 */

#ifndef SMEOS_ATMEL328P_INCLUDE_SMILLIS_HPP_
#define SMEOS_ATMEL328P_INCLUDE_SMILLIS_HPP_

#include <stdint.h>

void millisInit(void);
uint32_t millis(void);

#endif /* SMEOS_ATMEL328P_INCLUDE_SMILLIS_HPP_ */
