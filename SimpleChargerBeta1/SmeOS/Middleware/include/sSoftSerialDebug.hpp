/*
 * sSoftSerialDebug.hpp
 *
 *  Created on: 18 nov. 2018
 *      Author: Ola
 */

#ifndef SMEOS_ATMEL328P_INCLUDE_SSOFTSERIALDEBUG_HPP_
#define SMEOS_ATMEL328P_INCLUDE_SSOFTSERIALDEBUG_HPP_

#define BAUD 19200

#include "avr/io.h"
#include <stdio.h>

class sSoftSerialDebug
{
  private:
	void uartInit(void);
	static FILE uart_output;
	FILE uart_str;
  public:
	sSoftSerialDebug(int baud);							// set baud rate

};

#endif /* SMEOS_ATMEL328P_INCLUDE_SSOFTSERIALDEBUG_HPP_ */
