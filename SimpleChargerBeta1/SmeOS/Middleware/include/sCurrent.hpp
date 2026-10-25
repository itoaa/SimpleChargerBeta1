/*
 * Current
 *
 * readCurrent() returns milliamps, clamped to +-32767 mA before the
 * conversion to int (BESLUT nr 37). saturated() is true when the last
 * reading was clamped.
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
	bool pSaturated;
	sADC pADCPin;

public:
	Current(int port, int pin, int acs712Type);
	int   readCurrent(void);
	void  zeroAmpCalibrate();
	void  setRefVoltage(int refMv);
	int   getLastADCValue();
	bool  saturated() const { return pSaturated; }
};

#endif /* SMEOS_MIDDLEWARE_INCLUDE_SCURRENT_HPP_ */
