/*
 * sSerial.cpp
 */

#include "sSerial.hpp"

#include <avr/io.h>
#include <avr/interrupt.h>

/* Avvikelse mellan önskad och verklig baud. u2x är 0 eller 1. */
static uint32_t baudError(uint32_t baud, uint16_t ubrr, uint8_t u2x)
{
	uint32_t divisor = (uint32_t)(16UL >> u2x) * ((uint32_t)ubrr + 1UL);
	uint32_t actual = F_CPU / divisor;
	if (actual > baud)
	{
		return actual - baud;
	}
	return baud - actual;
}

#define RX_SIZE 64

static volatile uint8_t rxBuf[RX_SIZE];
static volatile uint8_t rxHead;
static volatile uint8_t rxTail;

ISR(USART_RX_vect)
{
	uint8_t data = UDR0;
	uint8_t next = (uint8_t)((rxHead + 1) & (RX_SIZE - 1));
	/* Full buffert: släng nya byten så loopen inte blockeras. */
	if (next != rxTail)
	{
		rxBuf[rxHead] = data;
		rxHead = next;
	}
}

sSerial::sSerial()
{
}

void sSerial::init(uint32_t baud)
{
	uartInit(baud);
}

void sSerial::uartInit(uint32_t baud)
{
	if (baud == 0)
	{
		return;
	}

	/* Samma avrundning som setbaud.h, med och utan dubbel hastighet. */
	uint32_t normal = (F_CPU + 8UL * baud) / (16UL * baud);
	uint32_t doubled = (F_CPU + 4UL * baud) / (8UL * baud);
	uint32_t ubrrNormal = (normal > 0UL) ? (normal - 1UL) : 0UL;
	uint32_t ubrrDouble = (doubled > 0UL) ? (doubled - 1UL) : 0UL;
	if (ubrrNormal > 4095UL)
	{
		ubrrNormal = 4095UL;
	}
	if (ubrrDouble > 4095UL)
	{
		ubrrDouble = 4095UL;
	}

	uint16_t ubrr = (uint16_t)ubrrNormal;
	uint8_t u2x = 0;
	if (baudError(baud, (uint16_t)ubrrDouble, 1) < baudError(baud, ubrr, 0))
	{
		ubrr = (uint16_t)ubrrDouble;
		u2x = 1;
	}

	if (u2x)
	{
		UCSR0A |= _BV(U2X0);
	}
	else
	{
		UCSR0A &= ~_BV(U2X0);
	}
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;

	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
	UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
}

bool sSerial::dataRecived()
{
	return rxHead != rxTail;
}

bool sSerial::readByte(uint8_t* out)
{
	if (rxHead == rxTail)
	{
		return false;
	}
	*out = rxBuf[rxTail];
	rxTail = (uint8_t)((rxTail + 1) & (RX_SIZE - 1));
	return true;
}

void sSerial::putChar(uint8_t c)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
}
