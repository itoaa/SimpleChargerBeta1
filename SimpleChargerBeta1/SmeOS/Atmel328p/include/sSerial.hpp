/*
 * UART. The caller chooses the baud rate. Received bytes sit in a ring buffer.
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
	bool dataReceived();
	/* False when the ring is empty. */
	bool readByte(uint8_t* out);
	void putChar(uint8_t c);
};

#endif /* SMEOS_ATMEL328P_INCLUDE_SSERIAL_HPP_ */
