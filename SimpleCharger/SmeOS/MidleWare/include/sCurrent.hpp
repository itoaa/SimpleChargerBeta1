/*
 * sCurrent.hpp
 *
 *  Created on: 13 dec. 2018
 *      Author: ola
 *
 *		Version 2.0
 *			Change name to Current.
 *			Changed output to mA. 30 000 = 30 Amp.
 *			FreeRTOS ready, witch sADC
 */

#ifndef SMEOS_MIDLEWARE_INCLUDE_SCURRENT_HPP_
#define SMEOS_MIDLEWARE_INCLUDE_SCURRENT_HPP_

#include "sADC.hpp"

class Current
{
private:
	int  pAcs712_type;							// 1=5A version, 2=20A and 3=30A
	int  pAnalogIn;
	int  pRefVoltage = 4980;					// Analog reference voltage, in mV.
	int  pACOffset = 2483;						// milli voltage for 0 A.
	int  pACOffsetADC = 567;					// ADC value for 0A.
	int	 pLastADCValue = 0; 					// Last ADC value read from ADC (Not filtered)
	int	pLastFilteredADCValue = 0;				// Last filtered ADC value read from ADC.
	sADC pADCPin;

public:
	Current(int pin, int ACS712_type);			// pin = arduino pin where AC712 is connected.
//	virtual ~Current();
	int   readCurrent(void);					// Performe a measurement
	void  zeroAmpCallibrate();					// Read and set ADC-value at 0A. (Zero amp calibration). Returns input voltage.
	void  setRefVoltage(int);					// define whitch reverece voltage used for ADC-convertion.
	int    getLastADCValue();					// Get ADC value from last getTPS();
};




#endif /* SMEOS_MIDLEWARE_INCLUDE_SCURRENT_HPP_ */
