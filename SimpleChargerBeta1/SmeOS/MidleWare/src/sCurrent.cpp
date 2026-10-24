/*
 * sCurrent.cpp
 *
 *  Created on: 13 dec. 2018
 *      Author: Ola
 */

#include "sCurrent.hpp"
#include "filter.hpp"
#include "scale.hpp"

Current::Current(int pin, int ACS712_type)
{
	pAcs712_type = ACS712_type;
	pAnalogIn = pin;

	pADCPin.init(2,pAnalogIn);			// Set up ADC pin (alvays port c)
	pADCPin.setADCMode(MODE_10_BIT);	// Set 10 bit mode
	pADCPin.setADCRef(EXTERNAL_REF);	// Set external volt ref
	pLastADCValue = 0;
}


int Current::readCurrent(void)
{
 //   int i;
    int mVperAmp = 100.0;                  // use 185.0 for 5A module, 100 for 20A Module and 66 for 30A Module
//    if       (pAcs712_type == 1)    { mVperAmp = 185.0; }   // 5A.  using 185.0 for 5A module
    if       (pAcs712_type == 1)    { mVperAmp = 121.0; }   // 5A.  using 185.0 for 5A module
    else if  (pAcs712_type == 2)    { mVperAmp = 100.0; }   // 20A. using 100.0 for 20A Module
    else if  (pAcs712_type == 3)    { mVperAmp = 66.0;  }   // 30A  using 66.0mv/amp for 30A Module
 //       mAperADC = 73.2422;                           // 73.2422mA per ADC tick.
    else {}

 	Filter rawValue(13);
 	for(int i=0;i<13;i++)
 	{
		pLastADCValue = pADCPin.ADCRead(pAnalogIn);
	 	rawValue.addValue(pLastADCValue);
 	}

      pLastFilteredADCValue = rawValue.getFilteredValue();
      return scaleAcs712Ma((uint16_t)pLastFilteredADCValue, pRefVoltage, pACOffset, mVperAmp);
//	  return (pLastFilteredADCValue);
}

// Reads voltage on pin and set offset value to that.
void Current::zeroAmpCallibrate()
{
    int RawValue= 0;
    Filter adc(13);                                   // Use 13 position buffer
    for(int i=0;i<13;i++)                             // Fill all potitions with newly read values
    {
		RawValue = pADCPin.ADCRead(pAnalogIn);		// Read ADC-Value
        adc.addValue(RawValue);                       // put in to filter buffer
    }
    RawValue = adc.getFilteredValue();               // Get aveage value
    pACOffsetADC = RawValue;
    pACOffset = (RawValue / 1023.0) * pRefVoltage;       // Remap averaged reading to mV
}



void Current::setRefVoltage(int RV)
{
    pRefVoltage = RV;                           // Set ref voltage för ADC
}


int Current::getLastADCValue()
{
  return (pLastADCValue);                 // Get ADC value from last readCurrent();
}




