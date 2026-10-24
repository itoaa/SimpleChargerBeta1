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
	pMode = 1;										// mode = ??? fix
//	printf("Setting up PWM on port %d and pin %d\n\r",pPort, pPin);
	pPWMPin.init(pPort, pPin,1);
	pMaxCount = 254;
	pPrescaler = 0;

	setMode(1);
	setPrescaler(1);

	pInverse = false;
}



void	sPWM::setDuty(double duty)
{
	/* Samma skala som den gamla raden OCR1A = MaxPWM * 10. */
	setDutyValue((int)(duty * 10.0));
}

void	sPWM::setDutyValue(int value)
{
	if (value < 0)
	{
		value = 0;
	}
	/* Timer 0 är 1 ms-tick, så PD5/PD6 lämnas orörda. */
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
	if (mode ==1)											// mode 1 = fast PWM, fixed frequensy
	{
		if (pPort == 1)										// PB1 and PB2, Timer 1, 16 bit. PB3 Timer 2, 8 bit
		{
			if (pPin == 1)
			{
				TCCR1A = (1 << COM1A1)	|	(1 << WGM10);	// Clear on match, set at bottom. Fast PWM 8bit
//				TCCR1B = (1 << WGM12);						// Fast PWM 8bit
				pMaxCount = 254;							// Max count for PWM to be 100 DS.
//				puts("Setting up FastPWM mode for PB1");
			}
			if (pPin == 2)
			{
				TCCR1A = (1 << COM1B1)	|	(1 << WGM10);	// Clear on match, set at bottom. Fast PWM 8bit
//				TCCR1B = (1 << WGM12);						// Fast PWM 8bit
				pMaxCount = 254;							// Max count for PWM to be 100 DS.
				//puts("Setting up FastPWM mode for PB2");
			}
			if (pPin == 3)
			{
				TCCR2A = (1 << COM2A1)	|	(1 << WGM20)|	(1 << WGM21);	// Clear on match, set at bottom. Fast PWM 8bit
//				TCCR2B = (1 << WGM12);						// Fast PWM 8bit
				pMaxCount = 254;							// Max count for PWM to be 100 DS.
//				puts("Setting up FastPWM mode for PB3");
			}

		}
		if (pPort == 3)										// PD5, PD6, Timer 0, 8 bit. PD3 timer 3 8 bit
		{
			if (pPin == 3)
			{
				TCCR2B = (1 << COM2A1)	|	(1 << WGM20) |	(1 << WGM21);	// Clear on match, set at bottom. Fast PWM 8bit
	//			TCCR2B = (1 << WGM12);						// Fast PWM 8bit
				pMaxCount = 254;							// Max count for PWM to be 100 DS.
//				puts("Setting up FastPWM mode for PD3");
			}
			if (pPin == 5)
			{
				TCCR0A = (1 << COM0B1)	|	(1 << WGM00) |	(1 << WGM01);	// Clear on match, set at bottom. Fast PWM 8bit
	//			TCCR0B = (1 << WGM12);						// Fast PWM 8bit
				pMaxCount = 254;							// Max count for PWM to be 100 DS.
//				puts("Setting up FastPWM mode for PD5");
			}
			if (pPin == 6)
			{
				TCCR0A = (1 << COM0A1)	|	(1 << WGM00) |	(1 << WGM01);	// Clear on match, set at bottom. Fast PWM 8bit
	//			TCCR0B = (1 << WGM12);						// Fast PWM 8bit
				pMaxCount = 254;							// Max count for PWM to be 100 DS.
//				puts("Setting up FastPWM mode for PD6");
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
			TCCR1B = (1 << CS10);						// timer 1 16 bit, No prescaler. only support no prescaler right now :-(
//			puts("Setting up prescaler for PB1");
			if (prescaler == 1) { return(1); }
			else return(0);
		}
		if (pPin == 2)
		{
			TCCR1B = (1 << CS10);						// timer 1 16 bit, No prescaler. only support no prescaler right now :-(
//			puts("Setting up prescaler for PB2");
			if (prescaler == 1) { return(1); }
			else return(0);
		}
		if (pPin == 3)
		{
			TCCR2B = (1 << CS20);						// timer 2 8 bit No prescaler. only support no prescaler right now :-(
//			puts("Setting up prescaler for PB3");
			if (prescaler == 1) { return(1); }
			else return(0);
		}

	}
	if (pPort == 3)										// PD5, PD6, Timer 0, 8 bit. PD3 timer 2 8 bit
	{
		if (pPin == 3)
		{
			TCCR2B = (1 << CS20);						// No prescaler. only support no prescaler right now :-(
//			puts("Setting up prescaler dor PD3");
			if (prescaler == 1) { return(1); }
			else return(0);
		}
		if (pPin == 5)
		{
			TCCR0B = (1 << CS00);						// No prescaler. only support no prescaler right now :-(
//			puts("Setting up prescaler dor PD5");
			if (prescaler == 1) { return(1); }
			else return(0);
		}
		if (pPin == 6)
		{
			TCCR0B = (1 << CS00);						// No prescaler. only support no prescaler right now :-(
//			puts("Setting up prescaler dor PD6");
			if (prescaler == 1) { return(1); }
			else return(0);
		}

	}
	return 0;
}
void	sPWM::inverse(bool inverse)
{

}
int		sPWM::getFrequensy()
{
	return(0);
}

