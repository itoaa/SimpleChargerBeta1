/*
 * sSerial.hpp
 *
 * UART. Hastigheten kommer från anroparen. Mottagna byte hamnar i en ringbuffert.
 */

#ifndef SMEOS_ATMEL328P_INCLUDE_SSERIAL_HPP_
#define SMEOS_ATMEL328P_INCLUDE_SSERIAL_HPP_

#include <stdint.h>

class sSerial
{
  private:
	void uartInit(uint32_t baud);

  public:
	sSerial();
	void init(uint32_t baud);
	bool dataRecived();
	/* false när bufferten är tom. */
	bool readByte(uint8_t* out);
	void putChar(uint8_t c);
};

#endif /* SMEOS_ATMEL328P_INCLUDE_SSERIAL_HPP_ */
