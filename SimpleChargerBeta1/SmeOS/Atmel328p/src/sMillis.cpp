#include "sMillis.hpp"

#include <avr/io.h>
#include <avr/interrupt.h>

static volatile uint32_t gMillis;
static uint32_t gCyclesPerTick;
static uint32_t gCredit;

/* Each interrupt adds the cycles that tick took. One millisecond is F_CPU / 1000 cycles. */
ISR(TIMER0_COMPA_vect)
{
	gCredit += gCyclesPerTick * 1000UL;
	while (gCredit >= F_CPU)
	{
		gCredit -= F_CPU;
		gMillis++;
	}
}

/* Prescaler whose tick fits in timer 0 and lands nearest 1 ms. */
static void chooseTick(uint16_t* prescaler, uint8_t* csBits, uint8_t* ocr)
{
	const uint16_t prescList[5] = { 1, 8, 64, 256, 1024 };
	const uint8_t csList[5] = {
		(1 << CS00),
		(1 << CS01),
		(uint8_t)((1 << CS01) | (1 << CS00)),
		(1 << CS02),
		(uint8_t)((1 << CS02) | (1 << CS00))
	};
	uint32_t bestErr = 0xFFFFFFFFUL;
	uint8_t best = 4;
	uint16_t bestCounts = 256;

	for (uint8_t i = 0; i < 5; i++)
	{
		uint32_t presc = prescList[i];
		uint32_t counts = (F_CPU + (presc * 500UL)) / (presc * 1000UL);
		if (counts < 1UL || counts > 256UL)
		{
			continue;
		}
		uint32_t actual = presc * counts * 1000UL;
		uint32_t err = (actual > F_CPU) ? (actual - F_CPU) : (F_CPU - actual);
		if (err < bestErr)
		{
			bestErr = err;
			best = i;
			bestCounts = (uint16_t)counts;
		}
	}

	*prescaler = prescList[best];
	*csBits = csList[best];
	*ocr = (uint8_t)(bestCounts - 1U);
}

void millisInit(void)
{
	uint16_t prescaler;
	uint8_t csBits;
	uint8_t ocr;

	chooseTick(&prescaler, &csBits, &ocr);
	gCyclesPerTick = (uint32_t)prescaler * ((uint32_t)ocr + 1UL);
	gCredit = 0;
	gMillis = 0;

	TCCR0A = (1 << WGM01);
	TCCR0B = csBits;
	OCR0A = ocr;
	TIMSK0 = (1 << OCIE0A);
}

uint32_t millis(void)
{
	uint32_t now;
	uint8_t sreg = SREG;
	/* A 32-bit read is not atomic on this core. */
	cli();
	now = gMillis;
	SREG = sreg;
	return now;
}
