/*
 * sSerial.hpp
 *
 * UART på 115200. Mottagna byte hamnar i en ringbuffert, inte i stdio.
 */

#ifndef SMEOS_ATMEL328P_INCLUDE_SSERIAL_HPP_
#define SMEOS_ATMEL328P_INCLUDE_SSERIAL_HPP_

#include <stdint.h>

class sSerial
{
  private:
	void uartInit(void);

  public:
	sSerial();
	void init(void);
	bool dataRecived();
	/* false när bufferten är tom. */
	bool readByte(uint8_t* out);
	void putChar(uint8_t c);
};

#endif /* SMEOS_ATMEL328P_INCLUDE_SSERIAL_HPP_ */
