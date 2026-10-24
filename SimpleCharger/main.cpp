/*
 * main.cpp
 *
 * ATmega328P-laddare. En huvudloop, ingen FreeRTOS.
 * TunerStudio-protokollet ligger i shared/ och kan anropas från en FreeRTOS-task på ett annat kort.
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

sGPIO green(GreenLedPport, GreenLedPin, 1);
sGPIO yelow(YellowLedPort, YellowLedPin, 1);
sGPIO red(RedLedPort, RedLedPin, 1);

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

int main()
{
	red.setHigh();
	green.setHigh();
	yelow.setHigh();

	millisInit();

	/* PWM igång med duty 0 innan något annat, så utgången inte flyter. */
	sPWM myPwm(pwmPort, pwmPin);
	myPwm.setDuty(0);

	sSerial serial;
	serial.init();

	TunerStudio tune;
	tune.setSend(uartSend, &serial);

	sei();

	Current outputCurrent(OutputCurrentPin, Current_Measure_type);
	outputCurrent.zeroAmpCallibrate();

	Current inputCurrent(7, 1);
	/* Samma paus som tidigare, men serieporten läses under tiden. */
	while (millis() < 200)
	{
		pollSerial(serial, tune);
	}
	inputCurrent.zeroAmpCallibrate();

	Volt outputVolt(OutputVoltPin, 55);
	Volt inputVolt(InputVoltPin, 65);

	uint32_t last = millis();
	for (;;)
	{
		uint32_t now = millis();
		if ((uint32_t)(now - last) >= 100)
		{
			last = now;
			green.setLow();

			/* int8_t är trådformatet mot befintlig ini-fil. */
			tune.realtime.OutputVolt = (int8_t)outputVolt.readVolt();
			tune.realtime.InputVolt = (int8_t)inputVolt.readVolt();
			tune.realtime.InputAmp = (int8_t)(inputCurrent.readCurrent() / 100);
			tune.realtime.OutputAmp = (int8_t)(outputCurrent.readCurrent() / 100);

			if (tune.realtime.InputAmp < 0)
			{
				tune.realtime.InputAmp = (int8_t)(0 - tune.realtime.InputAmp);
			}
			if (tune.realtime.OutputAmp < 0)
			{
				tune.realtime.OutputAmp = (int8_t)(0 - tune.realtime.OutputAmp);
			}

			myPwm.setDuty(tune.page1.MaxPWM);
			green.setHigh();
		}
		pollSerial(serial, tune);
	}
}
