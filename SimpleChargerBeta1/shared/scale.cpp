#include "scale.hpp"

uint16_t scaleAdcToVolt(uint16_t adc, uint16_t maxVolt)
{
	return (uint16_t)(adc * (maxVolt / 1023.0));
}

int scaleAcs712Ma(uint16_t adc, int refMv, int offsetMv, int mVperAmp)
{
	double voltage = (adc / 1023.0) * refMv;
	double amps = (voltage - offsetMv) / mVperAmp;
	return (int)(amps * 1000.0);
}
