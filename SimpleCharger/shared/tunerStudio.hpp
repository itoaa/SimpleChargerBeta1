/*
 * TunerStudio/MegaTune-protokoll utan UART och utan operativsystem.
 * AVR-loopen och en framtida FreeRTOS-task matar onByte() och skickar svar via setSend().
 * Kommandon, RTPS och PG1S är oförändrade så befintlig ini-fil stämmer.
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
    uint8_t MaxPWM;
    uint8_t MinPWM;
    uint8_t MaxTemp;
    uint8_t accDelay;
    uint8_t mosfetTempADCTable[12];
    int8_t  mosfetTempTable[12];
};

struct __attribute__((packed,aligned(1))) RPageVarsStruct
{
    uint8_t seconds;
    uint8_t pw1;
    uint8_t rpm;
    uint8_t TPSPos;
    int8_t  InputAmp;
    int8_t  InputVolt;
    int8_t  OutputAmp;
    int8_t  OutputVolt;
    int8_t  MosfetTempCor;
    int8_t  MosfetTempADC;
    int8_t  MosfetDriverVolt;
};

#define RTPS  11
#define PG1S  33

class TunerStudio
{
public:
	RPageVarsStruct realtime;
	Page1DataStruct page1;

	TunerStudio();
	/* send får råa bytes. ctx är pekaren som skickades till setSend. */
	void setSend(void (*send)(const uint8_t* data, uint8_t len, void* ctx), void* ctx);
	/* Kommando B. Tom pekare betyder att inget sparas. */
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
