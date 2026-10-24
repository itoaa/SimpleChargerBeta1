/*
 * Volt
 *
 * readVolt() returns the caller unit: ADC 1023 maps to maxVolt.
 * port is 1 = B, 2 = C, 3 = D. The pin number is the ADC channel on port C.
 */

#ifndef SMEOS_MIDDLEWARE_INCLUDE_SVOLT_HPP_
#define SMEOS_MIDDLEWARE_INCLUDE_SVOLT_HPP_

#include "stdlib.h"
#include "sADC.hpp"

class Volt
{
private:
	uint16_t  	pMaxVolt;
	uint8_t  	pAnalogIn;
	uint16_t	pAnalogRefVolt;
	sADC		adcPin;

public:
	Volt(uint8_t port, uint8_t pin, uint16_t maxVolt);
	Volt(uint8_t port, uint8_t pin, uint16_t maxVolt, uint16_t refVolt);
	uint16_t  readVolt(void);
};

#endif /* SMEOS_MIDDLEWARE_INCLUDE_SVOLT_HPP_ */
