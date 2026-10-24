/*
 * TunerStudio protocol without a UART.
 * main feeds onByte() and sends replies through setSend().
 * Command letters stay fixed. The realtime block grew when amps and the 12 V rail became int16.
 * tunerStudio.ini must use the same signature and the same offsets.
 */

#ifndef SHARED_TUNERSTUDIO_HPP_
#define SHARED_TUNERSTUDIO_HPP_

#include <stdint.h>

struct __attribute__((packed,aligned(1))) Page1DataStruct {
    uint8_t ChargeAmp;
    uint8_t MaxChargeAmp;
    uint8_t MaxChargeVolt;
    uint8_t MaxInputAmp;
    uint8_t MaxInputVolt;
    uint8_t MaxPWM;					/* Cap, default 5. The output stays off until volt and current limits are set. */
    uint8_t MinPWM;
    uint8_t MaxTemp;
    uint8_t accelerationDelay;
    uint8_t mosfetTempADCTable[12];
    int8_t  mosfetTempTable[12];
};

struct __attribute__((packed,aligned(1))) RPageVarsStruct
{
    uint8_t seconds;				/* millis() / 1000, wraps at 255. */
    uint8_t pulseWidth;				/* Duty last written. 0 until the regulator allows it. */
    uint8_t unusedRpm;				/* No RPM sensor. Stays 0. */
    uint8_t throttlePosition;		/* No TPS sensor. Stays 0. */
    int16_t InputAmp;				/* 0.1 A per count. No input sensor on this board, so this stays 0. */
    int8_t  InputVolt;
    int16_t OutputAmp;				/* 0.1 A per count. int16 so a 20 A sensor does not wrap. */
    int8_t  OutputVolt;
    int8_t  MosfetTempCor;			/* Not measured. OneWire is not in this build. */
    int8_t  MosfetTempADC;			/* Not measured. */
    int16_t MosfetDriverVolt;		/* 12 V rail, 0.1 V per count. int16 so a rail above 12.7 V fits. */
};

#define RTPS  14
#define PG1S  33

class TunerStudio
{
public:
	RPageVarsStruct realtime;
	Page1DataStruct page1;

	TunerStudio();
	/* send gets raw bytes. ctx is the pointer passed to setSend. */
	void setSend(void (*send)(const uint8_t* data, uint8_t len, void* ctx), void* ctx);
	/* Command B. A null function means nothing is stored. */
	void setOnBurn(void (*burn)(void* ctx), void* ctx);
	void onByte(uint8_t byte);

private:
	enum State { Idle, WOffset, WValue, XOffset, XCount, XData };

	State state;
	uint16_t pendingOffset;
	uint8_t pendingCount;
	void (*sendFn)(const uint8_t* data, uint8_t len, void* ctx);
	void* sendCtx;
	void (*burnFn)(void* ctx);
	void* burnCtx;

	void sendBytes(const uint8_t* data, uint8_t len);
	void writePageByte(uint16_t offset, uint8_t value);
	void handleCommand(uint8_t command);
};

#endif /* SHARED_TUNERSTUDIO_HPP_ */
