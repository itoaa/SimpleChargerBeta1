#include "tunerStudio.hpp"

#include <string.h>

/* TunerStudio matches these bytes. The trailing NUL is not sent. */
static const char codeString[] = "MSII s-Charger B-1  ";
static const char codeRev[] = "Smedby simple Charger Beta 1";
static const char protocolVersion[] = "001";

typedef char rtpsSizeCheck[(RTPS == sizeof(RPageVarsStruct)) ? 1 : -1];
typedef char pg1SizeCheck[(PG1S == sizeof(Page1DataStruct)) ? 1 : -1];

TunerStudio::TunerStudio()
	: state(Idle), pendingOffset(0), pendingCount(0), sendFn(0), sendCtx(0), burnFn(0), burnCtx(0)
{
	memset(&page1, 0, sizeof(page1));
	/* Small duty after reset. OCR = duty * 10, so 5 is not half scale. */
	page1.MaxPWM = 5;
	memset(&realtime, 0, sizeof(realtime));
	realtime.pulseWidth = page1.MaxPWM;
}

void TunerStudio::setSend(void (*send)(const uint8_t* data, uint8_t len, void* ctx), void* ctx)
{
	sendFn = send;
	sendCtx = ctx;
}

void TunerStudio::setOnBurn(void (*burn)(void* ctx), void* ctx)
{
	burnFn = burn;
	burnCtx = ctx;
}

void TunerStudio::sendBytes(const uint8_t* data, uint8_t len)
{
	if (sendFn != 0)
	{
		sendFn(data, len, sendCtx);
	}
}

void TunerStudio::writePageByte(uint16_t offset, uint8_t value)
{
	if (offset < PG1S)
	{
		reinterpret_cast<uint8_t*>(&page1)[offset] = value;
	}
}

void TunerStudio::handleCommand(uint8_t command)
{
	uint8_t seconds;

	switch (command)
	{
	case 'A':
		sendBytes(reinterpret_cast<const uint8_t*>(&realtime), RTPS);
		break;
	case 'c':
	case 'C':
		/* One raw byte, not ASCII. */
		seconds = realtime.seconds;
		sendBytes(&seconds, 1);
		break;
	case 'S':
		sendBytes(reinterpret_cast<const uint8_t*>(codeRev), (uint8_t)(sizeof(codeRev) - 1));
		break;
	case 'Q':
		sendBytes(reinterpret_cast<const uint8_t*>(codeString), (uint8_t)(sizeof(codeString) - 1));
		break;
	case 'F':
		sendBytes(reinterpret_cast<const uint8_t*>(protocolVersion), (uint8_t)(sizeof(protocolVersion) - 1));
		break;
	case 'W':
		state = WOffset;
		break;
	case 'X':
		state = XOffset;
		break;
	case 'B':
		if (burnFn != 0)
		{
			burnFn(burnCtx);
		}
		break;
	case 'V':
		sendBytes(reinterpret_cast<const uint8_t*>(&page1), PG1S);
		break;
	default:
		break;
	}
}

void TunerStudio::onByte(uint8_t byte)
{
	switch (state)
	{
	case WOffset:
		pendingOffset = byte;
		state = WValue;
		return;
	case WValue:
		writePageByte(pendingOffset, byte);
		state = Idle;
		return;
	case XOffset:
		pendingOffset = byte;
		state = XCount;
		return;
	case XCount:
		pendingCount = byte;
		/* A count of 0 is an empty block. */
		state = (pendingCount == 0) ? Idle : XData;
		return;
	case XData:
		writePageByte(pendingOffset, byte);
		pendingOffset++;
		pendingCount--;
		if (pendingCount == 0)
		{
			state = Idle;
		}
		return;
	case Idle:
	default:
		handleCommand(byte);
		return;
	}
}
