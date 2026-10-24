/*
 * sVolt.hpp
 *
 *  Created on: 10 dec. 2018
 *      Author: Ola Andersson
 *
 *      Ver 2.0 is ported to Arduino Eclipse and FreeRTOS
 *      Konstruktor av IVOLT. ex (IVOLT(8,50). sätter pin 8 som mätpinner för matningsspänning och högsta värde (1023) motsvarar 50V.
 *
 *		Ver	2.0
 *				Change name to Volt. Ready for OS48
 *		Ver	2.1
 *				Changed platform to AVR. Arduino no longer supported. Still usin OOP? think so, dependent on memory.
 *				Now using aADC FreeRTOS lib
 *
 */

#ifndef SMEOS_MIDLEWARE_INCLUDE_SVOLT_HPP_
#define SMEOS_MIDLEWARE_INCLUDE_SVOLT_HPP_

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
	Volt(uint8_t pin, uint16_t MaxVolt);						// pin = Arduino pin where Input Voltage is connected.
    														// MaxVolt = Maximum voltage thet is measurable. ADC-value 1023 vonverts to MaxVolt
	Volt(uint8_t pin, uint16_t MaxVolt, uint16_t refVolt);	// pin = Arduino pin where Input Voltage is connected.
															// MaxVolt = Maximum voltage thet is measurable. ADC-value 1023 vonverts to MaxVolt
															// refVolt = Analog Ref Volt. Set to tve value of analog reference volt.
	uint16_t  readVolt(void);		                   		// Performe a measurement
};

#endif /* SMEOS_MIDLEWARE_INCLUDE_SVOLT_HPP_ */
