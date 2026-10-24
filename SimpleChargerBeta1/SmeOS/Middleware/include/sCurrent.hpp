/*
 * Current
 *
 * readCurrent() returns milliamps.
 * Type 1 = 5 A, 2 = 20 A, 3 = 30 A.
 */

#ifndef SMEOS_MIDDLEWARE_INCLUDE_SCURRENT_HPP_
#define SMEOS_MIDDLEWARE_INCLUDE_SCURRENT_HPP_

#include "sADC.hpp"

class Current
{
private:
	int  pAcs712_type;
	int  pAnalogIn;
	int  pRefVoltage;
	int  pACOffset;
	int  pACOffsetADC;
	int	 pLastADCValue;
	int	 pLastFilteredADCValue;
	sADC pADCPin;

public:
	Current(int port, int pin, int acs712Type);
	int   readCurrent(void);
	void  zeroAmpCalibrate();
	void  setRefVoltage(int refMv);
	int   getLastADCValue();
};

#endif /* SMEOS_MIDDLEWARE_INCLUDE_SCURRENT_HPP_ */
