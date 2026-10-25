/*
 * sVolt.cpp
 */

#include "sVolt.hpp"
#include "filter.hpp"
#include "scale.hpp"

static void voltInit(sADC& adc, uint8_t port, uint8_t pin)
{
	adc.init(port, pin);
	adc.setADCMode(MODE_10_BIT);
	adc.setADCRef(EXTERNAL_REF);
}

Volt::Volt(uint8_t port, uint8_t pin, uint16_t maxVolt)
{
	pMaxVolt = maxVolt;
	pAnalogIn = pin;
	pAnalogRefVolt = 5;
	pLastADCValue = 0;
	voltInit(adcPin, port, pAnalogIn);
}

Volt::Volt(uint8_t port, uint8_t pin, uint16_t maxVolt, uint16_t refVolt)
{
	pMaxVolt = maxVolt;
	pAnalogIn = pin;
	pAnalogRefVolt = refVolt;
	pLastADCValue = 0;
	voltInit(adcPin, port, pAnalogIn);
}

/* Several samples so one noisy conversion is not the whole reading. */
uint16_t Volt::readVolt(void)
{
	Filter volt(13);
	for (int i = 0; i < 13; i++)
	{
		volt.addValue(adcPin.ADCRead(pAnalogIn));
	}
	pLastADCValue = (uint16_t)volt.getFilteredValue();
	return scaleAdcToVolt(pLastADCValue, pMaxVolt);
}
