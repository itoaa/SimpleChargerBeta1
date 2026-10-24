/*
 * ATmega328P charger. One main loop.
 * shared/tunerStudio does not open the UART. main does.
 *
 * The PWM output stays at duty 0 until page1 has a voltage limit and a current limit.
 * Duty then steps toward MaxPWM, and drops when voltage, current, or temperature is over the limit.
 */

#include "sGPIO.hpp"
#include "sPWM.hpp"
#include "sVolt.hpp"
#include "sCurrent.hpp"
#include "sSerial.hpp"
#include "sMillis.hpp"
#include "tunerStudio.hpp"
#include "Hardware.hpp"

#include <avr/interrupt.h>

sGPIO green(GreenLedPort, GreenLedPin, 1);
sGPIO yellow(YellowLedPort, YellowLedPin, 1);
sGPIO red(RedLedPort, RedLedPin, 1);

/* setDuty() writes OCR = duty * 10. 25 * 10 = 250, which still fits the 8-bit PWM top. */
static const uint8_t DutyFullScale = 25;

static void uartSend(const uint8_t* data, uint8_t len, void* ctx)
{
	sSerial* serial = static_cast<sSerial*>(ctx);
	for (uint8_t i = 0; i < len; i++)
	{
		serial->putChar(data[i]);
	}
}

static void pollSerial(sSerial& serial, TunerStudio& tune)
{
	uint8_t byte;
	while (serial.readByte(&byte))
	{
		tune.onByte(byte);
	}
}

static int16_t clampVoltByte(uint16_t volts)
{
	if (volts > 127)
	{
		return 127;
	}
	return (int16_t)volts;
}

/* One step per call. 0 means off. Limits of 0 are "not set", not "allow anything". */
static uint8_t nextDuty(uint8_t duty, const TunerStudio& tune, int outVolt, int inVolt, int outMilliamp)
{
	const Page1DataStruct& page = tune.page1;
	uint8_t cap = page.MaxPWM;
	bool ready;
	bool fault;
	int setpointAmp;

	if (cap > DutyFullScale)
	{
		cap = DutyFullScale;
	}

	ready = (cap > 0) && (page.MaxChargeVolt > 0) && (page.ChargeAmp > 0 || page.MaxChargeAmp > 0);
	/* MinPWM above the cap leaves no legal on-duty. */
	if (!ready || page.MinPWM > cap)
	{
		return 0;
	}

	fault = (outVolt >= (int)page.MaxChargeVolt);
	if (page.MaxChargeAmp > 0 && outMilliamp >= (int)page.MaxChargeAmp * 1000)
	{
		fault = true;
	}
	if (page.MaxInputVolt > 0 && inVolt >= (int)page.MaxInputVolt)
	{
		fault = true;
	}
	/* MosfetTempCor stays 0 until OneWire is linked, so an unset sensor does not trip this. */
	if (page.MaxTemp > 0 && (int)tune.realtime.MosfetTempCor > (int)page.MaxTemp)
	{
		fault = true;
	}
	if (fault)
	{
		return 0;
	}

	setpointAmp = (page.ChargeAmp > 0) ? (int)page.ChargeAmp : (int)page.MaxChargeAmp;
	if (outMilliamp >= setpointAmp * 1000)
	{
		if (duty > 0)
		{
			duty--;
		}
	}
	else if (duty < cap)
	{
		duty++;
	}

	if (duty > cap)
	{
		duty = cap;
	}
	/* Skip the dead zone under MinPWM. At the current limit, turn fully off instead. */
	if (page.MinPWM > 0 && duty > 0 && duty < page.MinPWM)
	{
		if (outMilliamp >= setpointAmp * 1000)
		{
			duty = 0;
		}
		else
		{
			duty = page.MinPWM;
			if (duty > cap)
			{
				duty = cap;
			}
		}
	}
	return duty;
}

int main()
{
	uint8_t duty = 0;

	red.setHigh();
	green.setHigh();
	yellow.setHigh();

	millisInit();

	TunerStudio tune;

	/* Drive the pin low before any calibration. Duty 0 is not a charge command. */
	sPWM myPwm(pwmPort, pwmPin);
	myPwm.setDuty(0);
	tune.realtime.pulseWidth = 0;

	sSerial serial;
	serial.init(SerialBaud);

	tune.setSend(uartSend, &serial);

	sei();

	Current outputCurrent(OutputCurrentPort, OutputCurrentPin, Current_Measure_type);
	/* Settle, then learn 0 A while the switch is still off. */
	while (millis() < 200)
	{
		pollSerial(serial, tune);
	}
	outputCurrent.zeroAmpCalibrate();

	Volt outputVolt(OutputVoltPort, OutputVoltPin, 55);
	Volt inputVolt(InputVoltPort, InputVoltPin, 65);
	Volt driverVolt(Port12V, Pin12V, (uint16_t)DriverVoltFullScale);

	uint32_t last = millis();
	for (;;)
	{
		/* uint8 wrap is enough for the seconds command. */
		tune.realtime.seconds = (uint8_t)(millis() / 1000UL);

		uint32_t now = millis();
		if ((uint32_t)(now - last) >= 100)
		{
			int outMilliamp;
			int outVolt;
			int inVolt;
			int driverCount;

			last = now;
			green.setLow();

			outVolt = (int)outputVolt.readVolt();
			inVolt = (int)inputVolt.readVolt();
			outMilliamp = outputCurrent.readCurrent();
			if (outMilliamp < 0)
			{
				outMilliamp = -outMilliamp;
			}
			driverCount = (int)driverVolt.readVolt();

			tune.realtime.OutputVolt = (int8_t)clampVoltByte((uint16_t)outVolt);
			tune.realtime.InputVolt = (int8_t)clampVoltByte((uint16_t)inVolt);
			/* No input-current sensor on this board. */
			tune.realtime.InputAmp = 0;
			tune.realtime.OutputAmp = (int16_t)(outMilliamp / 100);
			tune.realtime.MosfetDriverVolt = (int16_t)driverCount;

			duty = nextDuty(duty, tune, outVolt, inVolt, outMilliamp);
			myPwm.setDuty(duty);
			tune.realtime.pulseWidth = duty;
			green.setHigh();
		}
		pollSerial(serial, tune);
	}
}
