#include "sMillis.hpp"

#include <avr/io.h>
#include <avr/interrupt.h>

static volatile uint32_t gMillis;

/* 16 MHz / 64 / 250 = 1 kHz. */
ISR(TIMER0_COMPA_vect)
{
	gMillis++;
}

void millisInit(void)
{
	TCCR0A = (1 << WGM01);
	TCCR0B = (1 << CS01) | (1 << CS00);
	OCR0A = 249;
	TIMSK0 = (1 << OCIE0A);
}

uint32_t millis(void)
{
	uint32_t now;
	uint8_t sreg = SREG;
	/* 32-bitars läsning är inte atomär på den här kärnan. */
	cli();
	now = gMillis;
	SREG = sreg;
	return now;
}
