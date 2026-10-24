/*
 * sVolt.cpp
 *
 *  Created on: 10 dec. 2018
 *      Author: Ola Andersson
 *      Dokumentation in h-file
 *
 */


#include "sVolt.hpp"
#include "filter.hpp"
#include "scale.hpp"


Volt::Volt(uint8_t pin, uint16_t MaxVolt)
{
	pMaxVolt = MaxVolt;
	pAnalogIn = pin;
	pAnalogRefVolt = 5;		// Set Analog reference volt to 5V (default setting)
	adcPin.init(2,pAnalogIn);
	adcPin.setADCMode(MODE_10_BIT);
	adcPin.setADCRef(EXTERNAL_REF);
}

Volt::Volt(uint8_t pin, uint16_t MaxVolt, uint16_t refVolt)
{
	pMaxVolt = MaxVolt;
	pAnalogIn = pin;
	pAnalogRefVolt = refVolt;
	adcPin.init(2,pAnalogIn);
	adcPin.setADCMode(MODE_10_BIT);
	adcPin.setADCRef(EXTERNAL_REF);
}

// Subrutine to measure inputvoltagePin (multiple times for normalization)
uint16_t Volt::readVolt(void)
{
	Filter volt(13);                                                    // Setuf filter with 13 positions
	for(int i=0;i<13;i++)                                               // Fill Filter buffer with reads.
	{
//		volt.addValue(map(analogRead(panalogIn),0,1023,0,pmaxVolt));      // Read ADC-Value, remap to 0 to pmaxVolt and put in to filter buffer
		volt.addValue(adcPin.ADCRead(pAnalogIn));      // Read ADC-Value and put in to filter buffer
	}
//	return (volt.getFilteredValue());                                   // Return average value


	//return(map(analogRead(panalogIn),0,1023,0,pmaxVolt));
	return scaleAdcToVolt((uint16_t)volt.getFilteredValue(), pMaxVolt);
}



