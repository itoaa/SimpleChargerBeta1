/*
 * sPWM.cpp
 *
 *  Created on: 27 okt. 2018
 *      Author: Ola Andersson
 */

#include "sPWM.hpp"
#include "stdio.h"
#include "util/delay.h"


sPWM::sPWM(int port, int pin)						// port, 1 = PortB, 2 = PortC, 3 = portD
{
	pPin = pin;										// Store pin in private variable
	pPort = port;									// 1 = PortB, 2 = PortC, 3 = PortD
	pMode = 1;
	pPWMPin.init(pPort, pPin,1);
	pMaxCount = 254;
	pPrescaler = 0;

	setMode(1);
	setPrescaler(1);

	pInverse = false;
}



void	sPWM::setDuty(double duty)
{
	/* OCR = duty * 10. Leave this scale alone. */
	setDutyValue((int)(duty * 10.0));
}

void	sPWM::setDutyValue(int value)
{
	if (value < 0)
	{
		value = 0;
	}
	/* Timer 0 is the 1 ms tick, so PD5 and PD6 stay with that timer. */
	if (pPort == 1 && pPin == 1)
	{
		OCR1A = (uint16_t)value;
	}
	else if (pPort == 1 && pPin == 2)
	{
		OCR1B = (uint16_t)value;
	}
	else if (pPort == 1 && pPin == 3)
	{
		OCR2A = (uint8_t)value;
	}
}

void	sPWM::start()
{
}
void	sPWM::stop()
{

}
void	sPWM::setMode(int mode)
{
	if (mode ==1)											// mode 1 = fast PWM, fixed frequency
	{
		if (pPort == 1)										// PB1 and PB2, Timer 1, 16 bit. PB3 Timer 2, 8 bit
		{
			if (pPin == 1)
			{
				TCCR1A = (1 << COM1A1)	|	(1 << WGM10);	// Clear on match, set at bottom. Fast PWM 8 bit
//				TCCR1B = (1 << WGM12);						// Fast PWM 8 bit
				pMaxCount = 254;							// Top for full duty.
			}
			if (pPin == 2)
			{
				TCCR1A = (1 << COM1B1)	|	(1 << WGM10);	// Clear on match, set at bottom. Fast PWM 8 bit
//				TCCR1B = (1 << WGM12);						// Fast PWM 8 bit
				pMaxCount = 254;							// Top for full duty.
			}
			if (pPin == 3)
			{
				TCCR2A = (1 << COM2A1)	|	(1 << WGM20)|	(1 << WGM21);	// Clear on match, set at bottom. Fast PWM 8 bit
//				TCCR2B = (1 << WGM12);						// Fast PWM 8 bit
				pMaxCount = 254;							// Top for full duty.
			}

		}
		if (pPort == 3)										// PD5, PD6, Timer 0, 8 bit. PD3 timer 3 8 bit
		{
			if (pPin == 3)
			{
				TCCR2B = (1 << COM2A1)	|	(1 << WGM20) |	(1 << WGM21);	// Clear on match, set at bottom. Fast PWM 8 bit
	//			TCCR2B = (1 << WGM12);						// Fast PWM 8 bit
				pMaxCount = 254;							// Top for full duty.
			}
			if (pPin == 5)
			{
				TCCR0A = (1 << COM0B1)	|	(1 << WGM00) |	(1 << WGM01);	// Clear on match, set at bottom. Fast PWM 8 bit
	//			TCCR0B = (1 << WGM12);						// Fast PWM 8 bit
				pMaxCount = 254;							// Top for full duty.
			}
			if (pPin == 6)
			{
				TCCR0A = (1 << COM0A1)	|	(1 << WGM00) |	(1 << WGM01);	// Clear on match, set at bottom. Fast PWM 8 bit
	//			TCCR0B = (1 << WGM12);						// Fast PWM 8 bit
				pMaxCount = 254;							// Top for full duty.
			}

		} // mode 1, fast PWM
	}
}
bool	sPWM::setPrescaler(int prescaler)
{
	if (pPort == 1 )					// Timer 1, 16 bit.
	{
		if (pPin == 1)
		{
			TCCR1B = (1 << CS10);						// Timer 1, no prescaler. Only prescaler 1 is supported.
			if (prescaler == 1) { return(1); }
			else return(0);
		}
		if (pPin == 2)
		{
			TCCR1B = (1 << CS10);						// Timer 1, no prescaler. Only prescaler 1 is supported.
			if (prescaler == 1) { return(1); }
			else return(0);
		}
		if (pPin == 3)
		{
			TCCR2B = (1 << CS20);						// Timer 2, no prescaler. Only prescaler 1 is supported.
			if (prescaler == 1) { return(1); }
			else return(0);
		}

	}
	if (pPort == 3)										// PD5, PD6, Timer 0, 8 bit. PD3 timer 2 8 bit
	{
		if (pPin == 3)
		{
			TCCR2B = (1 << CS20);						// No prescaler. Only prescaler 1 is supported.
			if (prescaler == 1) { return(1); }
			else return(0);
		}
		if (pPin == 5)
		{
			TCCR0B = (1 << CS00);						// No prescaler. Only prescaler 1 is supported.
			if (prescaler == 1) { return(1); }
			else return(0);
		}
		if (pPin == 6)
		{
			TCCR0B = (1 << CS00);						// No prescaler. Only prescaler 1 is supported.
			if (prescaler == 1) { return(1); }
			else return(0);
		}

	}
	return 0;
}
void	sPWM::inverse(bool inverse)
{

}
int		sPWM::getFrequency()
{
	return(0);
}

