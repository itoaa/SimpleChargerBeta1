/*
 * TunerStudio protocol without a UART.
 * main feeds onByte() and sends replies through setSend().
 * Command letters, RTPS and PG1S stay fixed so the byte layout does not move.
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
    uint8_t MaxPWM;					/* Default 5. setDuty() writes OCR = duty * 10. */
    uint8_t MinPWM;
    uint8_t MaxTemp;
    uint8_t accelerationDelay;
    uint8_t mosfetTempADCTable[12];
    int8_t  mosfetTempTable[12];
};

struct __attribute__((packed,aligned(1))) RPageVarsStruct
{
    uint8_t seconds;				/* millis() / 1000, wraps at 255. */
    uint8_t pulseWidth;				/* MaxPWM duty last applied. */
    uint8_t unusedRpm;				/* No RPM sensor. Stays 0. */
    uint8_t throttlePosition;		/* No TPS sensor. Stays 0. */
    int8_t  InputAmp;
    int8_t  InputVolt;
    int8_t  OutputAmp;
    int8_t  OutputVolt;
    int8_t  MosfetTempCor;			/* Not measured. OneWire is not in this build. */
    int8_t  MosfetTempADC;			/* Not measured. */
    int8_t  MosfetDriverVolt;		/* 12 V rail, 0.1 V per count. */
};

#define RTPS  11
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
