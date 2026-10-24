/*
 * sGPIO.cpp
 *
 *  Created on: 21 okt. 2018
 *      Author: Ola Andersson
 */

#include "sGPIO.hpp"

sGPIO::sGPIO()										// defaukr constructor. Dose nothing
{
	pPin = 0;										// Store pin in private variable
	pPDD = 0;										// Store PortDataDirection (1 = output) in local variable
	pPort = 0;									// 1 = PortB, 2 = PortC, 3 = PortD
	pState = 0;										// Set default state to low

}

sGPIO::sGPIO(uint8_t port, uint8_t pin, uint8_t dir)			// dir, 1 = output, 0 = input. port, 1 = PortB, 2 = PortC, 3 = portD
{
	pPin = pin;										// Store pin in private variable
	pPDD = dir;										// Store PortDataDirection (1 = output) in local variable
	pPort = port;									// 1 = PortB, 2 = PortC, 3 = PortD
	pState = 0;										// Set default state to low
	setLow();										// Set state to low
	if (dir == 1) {	setOutput();	}				// Set pin as output
	if (dir == 0) {	setInput();		}				// Set pin as input
};

void sGPIO::init(uint8_t port, uint8_t pin, uint8_t dir)			// dir, 1 = output, 0 = input. port, 1 = PortB, 2 = PortC, 3 = portD
{
	pPin = pin;										// Store pin in private variable
	pPDD = dir;										// Store PortDataDirection (1 = output) in local variable
	pPort = port;									// 1 = PortB, 2 = PortC, 3 = PortD
	pState = 0;										// Set default state to low
	setLow();										// Set state to low
	if (dir == 1) {	setOutput();	}				// Set pin as output
	if (dir == 0) {	setInput();		}				// Set pin as input
};

void	sGPIO::setInput()
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
void	sGPIO::setOutput()
{
	if (pPort == 1)
	{
		set_output(DDRB,pPin);						// Set pin as input
	}
	if (pPort == 2)
	{
		set_output(DDRC,pPin);						// Set pin as input
	}
	if (pPort == 3)
	{
		set_output(DDRD,pPin);						// Set pin as input
	}
}
void	sGPIO::setHigh()
{
	if (pPort == 1)
	{
		output_high(PORTB,pPin);					// Set pin high
	}
	if (pPort == 2)
	{
		output_high(PORTC,pPin);					// Set pin high
	}
	if (pPort == 3)
	{
		output_high(PORTD,pPin);					// Set pin high
	}
}
void	sGPIO::setLow()
{
	if (pPort == 1)
	{
		output_low(PORTB,pPin);						// Set pin low
	}
	if (pPort == 2)
	{
		output_low(PORTC,pPin);						// Set pin low
	}
	if (pPort == 3)
	{
		output_low(PORTD,pPin);						// Set pin low
	}

}
uint8_t		sGPIO::getState()
{
	if (pPort == 1)
	{
		pState = PINB & _BV(pPin);
	}
	if (pPort == 2)
	{
		pState = PINC & _BV(pPin);
	}
	if (pPort == 3)
	{
		pState = PIND & _BV(pPin);
	}

	return pState;								// Return current state. 1 = high, 0 = low
}
uint8_t		sGPIO::switchState()
{

	if(pState == 0)
	{
		setHigh();
		pState = 1;
	}
	else
	{
		pState = 0;
		setLow();
	}

	return(pState);									// Fix
}

