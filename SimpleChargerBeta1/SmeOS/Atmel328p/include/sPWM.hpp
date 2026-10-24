/*
 * sPWM.hpp
 *
 *  Created on: 27 okt. 2018
 *      Author: Ola Andersson
 	 	 ver 1.0
 */

#include "stdint.h"
#include "avr/io.h"
#include "sGPIO.hpp"

#ifndef SMEOS_ATMEL328P_INCLUDE_SPWM_HPP_
#define SMEOS_ATMEL328P_INCLUDE_SPWM_HPP_

class sPWM
{
  private:
  int	pPin;											// Pin number 1,2,3...
  int	pPort;											// 1 portB, 2 portC, 3 portD
  int	pPrescaler;										// 1, 2, 4, 8, 16 ... Dependent on port and pin
  int	pMode;
  int	pMaxCount;										// How far PWM is counting. Normaly 255 for 8 bit and 1024 for 16bit, but can be different value.
  bool	pInverse;
  sGPIO	pPWMPin;

  public:
  sPWM(int port, int pin);								// pin and port. port, 1 = PortB, 2 = PortC, 3 = PortD

  void	init(int port, int pin);						// Init the right pins.
  void	setDuty(double);								// Set duty cycle for PWM. 0 - 100.0
  void	setDutyValue(int);								// Set exact counter match. Depends on PWM resolution
  void	start();										// Start PWM
  void	stop();											// Stop PWM
  void	setMode(int);									// Set PWM mode
  bool	setPrescaler(int);								// Set prescaler. 1, 8, 256 ... Returns true if prescaler is possible.
  bool	setFrequensy(int);								// Set PWM frequensy. changes both prescaler and max count. Returns true if frequensy is possible
  void	inverse(bool);									// Inverse output? 1 = yes, 0 = no
  int	getFrequensy();									// Get PWM frequensy.
  int	getResolution();								// Get count number from 0 to 100% DS.

 };

#endif /* SMEOS_ATMEL328P_INCLUDE_SPWM_HPP_ */
