/*
 * sADC.hpp
 *
 *  Created on: 25 okt. 2018
 *      Author: Ola Andersson
 */


#ifndef SMEOS_ATMEL328P_INCLUDE_SADC_HPP_
#define SMEOS_ATMEL328P_INCLUDE_SADC_HPP_

#include "stdint.h"
#include "avr/io.h"
#include "sGPIO.hpp"

#define EXTERNAL_REF	0
#define VCC_REF			1
#define INTERNAL_REF	2

#define MODE_8_BIT		1
#define MODE_10_BIT		0

#define ADC_CONVERTING	1
#define ADC_DONE		0


class sADC
{
  private:
  uint8_t pPin;
  uint8_t pPort;
  uint16_t pValue;

  void setInput();

  public:
  sADC(uint8_t port, uint8_t pin);						// port and pin
  sADC();										// Constructor. declare port and pin in init

  void 	init(uint8_t port, uint8_t pin);		// port and pin
  void	setADCRef(uint8_t ref);					// Set the analog reference for the ADC, EXTERNAL_REF, VCC_REF or INTERNAL_REF
  void	setADCMode(uint8_t mode);				// Set ADC to run either 8 or 10 bit mode (MODE_8_BIT or MODE_10_BIT)
  uint8_t getADCMode(void);						// Returns 8 or 10 bit mode, (returns MODE_8_BIT or MODE_10_BIT)
  												// For example: if (getMode() == MODE_8_BIT) ...
  uint8_t ADCIsConverting(void);					// returns ADC_CONVERTONG if the ADC is converting, otherwise ADC_DONE
  												// For example: if (analogIsConverting() == ADC_CONVERTING) ...
  // the following method can be used to initiate an ADC conversion
  // that runs in the background, allowing the CPU to perform other tasks
  // while the conversion is in progress.  The procedure is to start a
  // conversion on a channel with startConversion(channel), and then
  // poll isConverting in your main loop.  Once isConverting() returns
  // a zero, the result can be obtained through a call to conversionResult().

  void	startADC( uint8_t channel);

  // returns the result of the previous ADC conversion.
  uint16_t ADCResult(void);

  // Starts ADC conversion and wait for, and return the result.
  uint16_t ADCRead( uint8_t channel);

 };

#endif /* SMEOS_ATMEL328P_INCLUDE_SADC_HPP_ */
