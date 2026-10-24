/*
 * ATmega328P charger. One main loop.
 * shared/tunerStudio does not open the UART. main does.
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
	yellow.setHigh();

	millisInit();

	TunerStudio tune;

	/* PWM hardware starts here. Duty is the page default, not full scale. */
	sPWM myPwm(pwmPort, pwmPin);
	myPwm.setDuty(tune.page1.MaxPWM);
	tune.realtime.pulseWidth = tune.page1.MaxPWM;

	sSerial serial;
	serial.init(SerialBaud);

	tune.setSend(uartSend, &serial);

	sei();

	Current outputCurrent(OutputCurrentPort, OutputCurrentPin, Current_Measure_type);
	outputCurrent.zeroAmpCalibrate();

	Current inputCurrent(InputCurrentPort, InputCurrentPin, InputCurrentMeasureType);
	/* Same settle time as before. Keep reading the UART while waiting. */
	while (millis() < 200)
	{
		pollSerial(serial, tune);
	}
	inputCurrent.zeroAmpCalibrate();

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
			last = now;
			green.setLow();

			/* int8 is the wire format. A value above 127 does not fit. */
			tune.realtime.OutputVolt = (int8_t)outputVolt.readVolt();
			tune.realtime.InputVolt = (int8_t)inputVolt.readVolt();
			tune.realtime.InputAmp = (int8_t)(inputCurrent.readCurrent() / 100);
			tune.realtime.OutputAmp = (int8_t)(outputCurrent.readCurrent() / 100);
			tune.realtime.MosfetDriverVolt = (int8_t)driverVolt.readVolt();

			if (tune.realtime.InputAmp < 0)
			{
				tune.realtime.InputAmp = (int8_t)(0 - tune.realtime.InputAmp);
			}
			if (tune.realtime.OutputAmp < 0)
			{
				tune.realtime.OutputAmp = (int8_t)(0 - tune.realtime.OutputAmp);
			}

			myPwm.setDuty(tune.page1.MaxPWM);
			tune.realtime.pulseWidth = tune.page1.MaxPWM;
			green.setHigh();
		}
		pollSerial(serial, tune);
	}
}
