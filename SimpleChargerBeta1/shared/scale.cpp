#include "scale.hpp"

uint16_t scaleAdcToVolt(uint16_t adc, uint16_t maxVolt)
{
	return (uint16_t)(adc * (maxVolt / 1023.0));
}

int32_t scaleAcs712Ma32(uint16_t adc, int16_t refMv, int16_t offsetMv, int16_t mVperAmp)
{
	int32_t mv;
	if (adc > 1023U)
	{
		adc = 1023U;
	}
	if (mVperAmp <= 0)
	{
		return 0;
	}
	/* (adc*ref + 511)/1023: avrundning for icke-negativ produkt, trunkering
	 * annars (ref < 0 ar inte meningsfullt men ger inget overflow). */
	mv = (((int32_t)adc * (int32_t)refMv) + 511L) / 1023L;
	return ((mv - (int32_t)offsetMv) * 1000L) / (int32_t)mVperAmp;
}

int16_t scaleAcs712MaSat(uint16_t adc, int16_t refMv, int16_t offsetMv, int16_t mVperAmp,
                         bool* saturated)
{
	int32_t ma = scaleAcs712Ma32(adc, refMv, offsetMv, mVperAmp);
	bool sat = false;
	if (ma > ScaleMaLimit)
	{
		ma = ScaleMaLimit;
		sat = true;
	}
	else if (ma < -ScaleMaLimit)
	{
		ma = -ScaleMaLimit;
		sat = true;
	}
	else
	{
	}
	if (saturated != 0)
	{
		*saturated = sat;
	}
	return (int16_t)ma;
}
