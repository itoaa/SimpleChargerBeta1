/*
 * sGPIO.hpp
 *
 *  Created on: 21 okt. 2018
 *      Author: Ola Andersson
 */

#include "stdint.h"
#include "avr/io.h"

#ifndef SMEOS_ATMEL328P_INCLUDE_SGPIO_HPP_
#define SMEOS_ATMEL328P_INCLUDE_SGPIO_HPP_

// Some macros that make the code more readable
#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

// Some macros i need to look in to.
// Not used yet but most interesting
// Found at:
// https://www.avrfreaks.net/forum/pass-ports-and-their-registers-functions

#define _xpaste(a,b)  a ## b
#define   paste(a,b)  _xpaste(a,b)

#define _xarg1(a, ...) a
#define _xarg2(a,b, ...) b

#define arg1(a, ...)  _xarg1(a)
#define arg2(a, ...)  _xarg2(a, __VA_ARGS__)

#define  bit(a, ...) arg2(a, __VA_ARGS__)
#define  let(a, ...) arg1(a)
#define port(pb, ...)  paste(PORT, let(pb))
#define  pin(pb, ...)  paste(PIN,  let(pb))
#define  ddr(pb, ...)  paste(DDR,  let(pb))

#define pset(pb)  (port(pb) |=  (1<<bit(pb)))
#define pclr(pb)  (port(pb) &= ~(1<<bit(pb)))
#define ptog(pb)  ( pin(pb) =   (1<<bit(pb)))
#define pval(pb)  ( pin(pb) &   (1<<bit(pb)))
#define dset(pb)  ( ddr(pb) |=  (1<<bit(pb)))
#define dclr(pb)  ( ddr(pb) &= ~(1<<bit(pb)))

#define  XB3 B,3

/* examples
pin(XB3)
ddr(XB3)
port(XB3)
bit(XB3)

pset(XB3)
pclr(XB3)
ptog(XB3)
pval(XB3)
dset(XB3)
dclr(XB3)
*/

class sGPIO
{
  private:
	uint8_t pPin;
	uint8_t pPort;
	uint8_t pPDD;
	uint8_t pState;

  public:
	sGPIO(uint8_t ddr, uint8_t pin, uint8_t state);			// pin and state. state, 1 = output, 0 = input
	sGPIO();

	void init(uint8_t ddr, uint8_t pin, uint8_t state);					// use if using default constructor, pin and state. state, 1 = output, 0 = input
	void	setInput();										// Set pin as input pin
	void	setOutput();									// Set pin as output
	void	setHigh();										// Set pin high
	void	setLow();										// Set pin low
	uint8_t	getState();										// Return state. 1 = high, 0 = low
	uint8_t	switchState();									// Switch state and return new state. 1 = high, 0 = low

 };

#endif /* SMEOS_ATMEL328P_INCLUDE_SGPIO_HPP_ */
