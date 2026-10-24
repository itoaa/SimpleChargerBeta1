/*
 * Median-style filter with no hardware dependency.
 */

#ifndef SHARED_FILTER_HPP_
#define SHARED_FILTER_HPP_

#include <stdint.h>

#define filterSamples   13

class Filter {
	private:
		int16_t pSensArray1 [filterSamples];
		int16_t pSensArray1Sort [filterSamples];
		uint8_t pBufferPosition;
		uint8_t	pBufferLength;
	public:
		Filter(uint8_t bufferLength);
		int16_t getValue(uint8_t index);
		int16_t getSortValue(uint8_t index);
		int16_t getFilteredValue(void);
		void addValue(int16_t value);
};

#endif /* SHARED_FILTER_HPP_ */
