/*
 * sSerial.cpp
 */

#include "sSerial.hpp"

#include <avr/io.h>
#include <avr/interrupt.h>

/* Hastigheten den tidigare binären programmerade. F_CPU sätts av bygget. */
#define BAUD 115200
#define USE_2X
#define BAUD_TOL 4
#include <util/setbaud.h>

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

void sSerial::init(void)
{
	uartInit();
}

void sSerial::uartInit(void)
{
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;

#ifdef USE_2X
	UCSR0A |= _BV(U2X0);
#else
	UCSR0A &= ~(_BV(U2X0));
#endif

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
