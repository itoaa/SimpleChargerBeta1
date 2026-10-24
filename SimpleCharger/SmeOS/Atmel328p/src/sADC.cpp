/*
 * sADC.cpp
 *
 *  Created on: 25 okt. 2018
 *      Author: Ola Andersson
 */

#include "sADC.hpp"

/* Minsta delning som håller ADC-klockan på högst 200 kHz. */
static uint8_t adcPrescalerBits(void)
{
	const uint16_t divs[7] = { 2, 4, 8, 16, 32, 64, 128 };
	const uint8_t bits[7] = {
		(1 << ADPS0),
		(1 << ADPS1),
		(uint8_t)((1 << ADPS1) | (1 << ADPS0)),
		(1 << ADPS2),
		(uint8_t)((1 << ADPS2) | (1 << ADPS0)),
		(uint8_t)((1 << ADPS2) | (1 << ADPS1)),
		(uint8_t)((1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0))
	};

	for (uint8_t i = 0; i < 7; i++)
	{
		if ((F_CPU / divs[i]) <= 200000UL)
		{
			return bits[i];
		}
	}
	return bits[6];
}

sADC::sADC(uint8_t port, uint8_t pin)						// Pin and port.  1 = PortB, 2 = PortC, 3 = portD
{
	pPin = pin;										// Store pin in private variable
	pPort = port;									// 1 = PortB, 2 = PortC, 3 = PortD
	pValue = 0;										// Set default value to 0
	setInput();
}

sADC::sADC()										// Just construct
{
	pPin = 0;										// Store pin in private variable
	pPort = 2;										// 1 = PortB, 2 = PortC, 3 = PortD
	pValue = 0;										// Set default value to 0
}

void 	sADC::init(uint8_t port, uint8_t pin)				// Pin and port.  1 = PortB, 2 = PortC, 3 = portD
{
	pPin = pin;										// Store pin in private variable
	pPort = port;									// 1 = PortB, 2 = PortC, 3 = PortD
	pValue = 0;										// Set default value to 0
	setInput();
	/* Digital buffert av på ADC0-5, och komparatorn av. Ersätter FreeRTOS idle-hooken. */
	DIDR0 = 0x3F;
	ACSR &= ~_BV(ACIE);
	ACSR |= _BV(ACD);
}


void	sADC::setInput()
{
	if (pPort == 1)
	{
		set_input(DDRB,pPin);						// Set pin as input
	}
	if (pPort == 2)
	{
		set_input(DDRC,pPin);						// Set pin as input
	}
	if (pPort == 3)
	{
		set_input(DDRD,pPin);						// Set pin as input
	}
}
void 	sADC::setADCMode(uint8_t mode)
/*--------------------------------------------------------------------*/
// setADCMode sets the ADC to run in either 8-bit mode (MODE_8_BIT) or
// 10-bit mode (MODE_10_BIT)
{
	if (mode == MODE_10_BIT)
		ADMUX &= ~(1 << ADLAR);	// right-adjust result (ADC has result)
	else
		ADMUX |= 1 << ADLAR;	// left-adjust result (ADCH has result)
}
uint8_t sADC::getADCMode(void)
/*--------------------------------------------------------------------*/
// getADCMode returns 0 if in 10-bit mode, otherwise returns non-zero.  The return
// value of this method can be directly compared against the macros
// MODE_8_BIT and MODE_10_BIT:
// For example: if (getMode() == MODE_8_BIT) ...
{
	return (ADMUX >> ADLAR) & 1;
}
uint8_t sADC::ADCIsConverting(void)
/*--------------------------------------------------------------------*/
// ADCIsConverting returns 1 if the ADC is in the middle of an conversion, otherwise
// returns 0
{
	return (ADCSRA >> ADSC) & 1;
}
void 	sADC::startADC(uint8_t channel)
/*--------------------------------------------------------------------*/
// startADC method can be used to initiate an ADC conversion
// that runs in the background, allowing the CPU to perform other tasks
// while the conversion is in progress.  The procedure is to start a
// conversion on a channel with startADC(channel), and then
// poll ADCisConverting in your main loop.  Once ADCisConverting() returns
// a zero, the result can be obtained through a call to ADCResult().


{
	// Channel numbers greater than 15 on 328p are invalid.
	if (channel > 0x0F)
	{
		return;
	}

	ADCSRA = (uint8_t)(_BV(ADEN) | adcPrescalerBits());

	uint8_t tempADMUX = ADMUX;

	tempADMUX &= ~0x1F;		 // clear channel selection bits of ADMUX
	tempADMUX |= channel;    // we only get this far if channel is less than 32

	ADMUX = tempADMUX;
	ADCSRA |= _BV(ADSC); // start the conversion
}
uint16_t sADC::ADCResult(void)
// returns the result of the previous ADC conversion.
{
/*
 * Need fixing, only returns 16bit values now

	if (getADCMode())				// if left-adjusted (i.e. 8-bit mode)
	{
		return (uint16_t) ADCH;			// 8-bit result
	}
	else
	{
		return ADC;				// 10-bit result
	}
*/
	return ADC;				// 10-bit result

}
uint16_t sADC::ADCRead( uint8_t channel)
{
	startADC(channel);
	while (ADCIsConverting()) 	{}
	return(ADCResult());
}
void 	sADC::setADCRef(uint8_t ref)
{
	uint8_t tempADMUX = ADMUX;
	if(ref == EXTERNAL_REF)
	{
		tempADMUX &= ~0xC0;				// clear REFS0, REFS1 and REFS2 bits of ADMUX, use external reference
	}
	if(ref == VCC_REF)
	{
		tempADMUX &= ~0xC0;				// clear REFS0, REFS1 and REFS2 bits of ADMUX
		tempADMUX |= _BV(REFS0);		// Use external capacitor on ARef, USe Vcc as reference
	}
	if(ref == INTERNAL_REF)
	{
		tempADMUX &= ~0xC0;				// clear REFS0, REFS1 and REFS2 bits of ADMUX
		tempADMUX |= _BV(REFS0);		// Use internal 1.1V reference, Use external capacitor on ARef.
		tempADMUX |= _BV(REFS1);		// Do not connect ARef to Vcc in this mode, it will damage the internal 1.1V reference.
	}
	ADMUX = tempADMUX;
}
