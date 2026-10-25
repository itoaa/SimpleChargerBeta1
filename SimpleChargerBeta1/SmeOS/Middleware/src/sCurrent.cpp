/*
 * sCurrent.cpp
 */

#include "sCurrent.hpp"
#include "filter.hpp"
#include "scale.hpp"

Current::Current(int port, int pin, int acs712Type)
{
	pAcs712_type = acs712Type;
	pAnalogIn = pin;
	pRefVoltage = 4980;
	pACOffset = 2483;
	pACOffsetADC = 567;
	pLastADCValue = 0;
	pLastFilteredADCValue = 0;
	pSaturated = false;

	/* Port sets the pin direction. The pin value is the ADC channel. */
	pADCPin.init(port, pAnalogIn);
	pADCPin.setADCMode(MODE_10_BIT);
	pADCPin.setADCRef(EXTERNAL_REF);
}

int Current::readCurrent(void)
{
    int mVperAmp = 100;
    if       (pAcs712_type == 1)    { mVperAmp = 121; }   /* 5 A sensor. */
    else if  (pAcs712_type == 2)    { mVperAmp = 100; }   /* 20 A sensor. */
    else if  (pAcs712_type == 3)    { mVperAmp = 66;  }   /* 30 A sensor. */
    else {}

 	Filter rawValue(13);
 	for (int i = 0; i < 13; i++)
 	{
		pLastADCValue = pADCPin.ADCRead(pAnalogIn);
	 	rawValue.addValue(pLastADCValue);
 	}

      pLastFilteredADCValue = rawValue.getFilteredValue();
      /* 32-bitars heltal, klampat till +-32767 mA fore omvandling (BESLUT nr 37). */
      return (int)scaleAcs712MaSat((uint16_t)pLastFilteredADCValue, (int16_t)pRefVoltage,
                                   (int16_t)pACOffset, (int16_t)mVperAmp, &pSaturated);
}

/* Store the present reading as the 0 A offset. */
void Current::zeroAmpCalibrate()
{
    int RawValue = 0;
    Filter adc(13);
    for (int i = 0; i < 13; i++)
    {
		RawValue = pADCPin.ADCRead(pAnalogIn);
        adc.addValue(RawValue);
    }
    RawValue = adc.getFilteredValue();
    pACOffsetADC = RawValue;
    pACOffset = (RawValue / 1023.0) * pRefVoltage;
}

void Current::setRefVoltage(int refMv)
{
    pRefVoltage = refMv;
}

int Current::getLastADCValue()
{
  return (pLastADCValue);
}
