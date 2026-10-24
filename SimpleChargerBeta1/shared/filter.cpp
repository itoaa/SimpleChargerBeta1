#include "filter.hpp"

#include <string.h>

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

Filter::Filter(uint8_t bufferL)
{
	pBufferLength = bufferL;
	pBufferPosition = 0;
	/* A cleared buffer keeps the first average off leftover stack data. */
	memset(pSensArray1, 0, sizeof(pSensArray1));
	memset(pSensArray1Sort, 0, sizeof(pSensArray1Sort));
}

void Filter::addValue(int16_t value)
{
	int j;
	int16_t temp;
	bool done;

	pBufferPosition = (pBufferPosition + 1) % filterSamples;
	pSensArray1[pBufferPosition] = value;

	for (j=0; j<filterSamples; j++)
	{
		pSensArray1Sort[j] = pSensArray1[j];
	}
	done = 0;
	while(done != 1)
	{
		done = 1;
		for (j = 0; j < (filterSamples -1); j++)
		{
			if (pSensArray1Sort[j] > pSensArray1Sort[j + 1])
			{
				temp = pSensArray1Sort[j + 1];
				pSensArray1Sort [j+1] =  pSensArray1Sort[j] ;
				pSensArray1Sort [j] = temp;
				done = 0;
			}
		}
	}
}

int16_t Filter::getFilteredValue(void)
{
	int16_t  top, bottom, k, l;
	long total;
	bottom = max(((filterSamples * 15)  / 100), 1);
	top = min((((filterSamples * 85) / 100) + 1  ), (filterSamples - 1));
	k = 0;
	total = 0;
	/* Drop the low and high tails. Sum the middle that remains, not the start of the sorted buffer. */
	for (l = bottom; l < top; l++)
	{
		total += pSensArray1Sort[l];
		k++;
	}
	if (k == 0)
	{
		return 0;
	}
	return (int16_t)(total / k);
}

int16_t Filter::getValue(uint8_t index)
{
	return (pSensArray1[index]);
}

int16_t Filter::getSortValue(uint8_t index)
{
	return (pSensArray1Sort[index]);
}
