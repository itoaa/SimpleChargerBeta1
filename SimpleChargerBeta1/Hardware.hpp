/*
 * Hardware.hpp
 *
 * Pin map for the board selected below. Port numbers are 1 = B, 2 = C, 3 = D.
 */

#ifndef HARDWARE_HPP_
#define HARDWARE_HPP_

#include <avr/io.h>
#include <stdint.h>

/* Which board this build is for. */
#define SmedbyCharger11				1
#define DCC							0

#define Serial						1
#define CAN							0

#if SmedbyCharger11 == 1

	const int  InputVoltPin  		= PC2;		/* Battery voltage, PC2 = ADC2. */
	const int  InputVoltPort  		= 2;
	const int  OutputCurrentPin 	= PC0;		/* Output current, PC0 = ADC0. */
	const int  OutputCurrentPort 	= 2;
	const int  OutputVoltPin 		= PC1;		/* Output voltage, PC1 = ADC1. */
	const int  OutputVoltPort 		= 2;
	const int  Pin12V 				= PC3;		/* 12 V rail, PC3 = ADC3. */
	const int  Port12V 				= 2;
	/* ADC 1023 maps to this Volt reading. 201 is 20.1 V, so the byte is 0.1 V per count. */
	const int  DriverVoltFullScale	= 201;
	const int  TemperaturePin 		= PD7;		/* MOSFET temperature pin. No sensor in this build. */
	const int  TemperaturePort 		= 3;
	const int  pwmPin 				= PB1;		/* PWM charge output, PB1. */
	const int  pwmPort 				= 1;

	const int  Temp_Measure_type 	= 2;		/* 1 = NTC, 2 = DS18B20. OneWire is not linked. */
	const int  Current_Measure_type = 2;		/* Output sensor. 1 = 5 A, 2 = 20 A, 3 = 30 A. */
	/* No input-current pin on this board. ADC7 is not a DIP pin, so main does not sample it. */
	const int  RedLedPin	 		= PD5;
	const int  RedLedPort 			= 3;
	const int  YellowLedPin			= PD4;
	const int  YellowLedPort		= 3;
	const int  GreenLedPin 			= PD3;
	const int  GreenLedPort			= 3;
	const int  ButtonPin 			= PD2;
	const int  ButtonPort 			= 3;
	const uint32_t SerialBaud		= 115200;	/* Passed to sSerial by main. */

#endif	/* SmedbyCharger11 = 1 */

#if DCC == 1

	const int  InputVoltPin  		= PC4;
	const int  InputVoltPort  		= 2;
	const int  OutputCurrentPin 	= PC1;
	const int  OutputCurrentPort 	= 2;
	const int  OutputVoltPin 		= PC2;
	const int  OutputVoltPort 		= 2;
	const int  Pin12V 				= PC3;
	const int  Port12V 				= 2;
	const int  DriverVoltFullScale	= 201;		/* 0.1 V per count, ADC 1023 = 20.1 V. */
	const int  TemperaturePin 		= PD4;
	const int  TemperaturePort 		= 3;
	const int  pwmPin 				= PD6;
	const int  pwmPort 				= 3;

	const int  Temp_Measure_type 	= 2;		/* 1 = NTC, 2 = DS18B20. OneWire is not linked. */
	const int  Current_Measure_type = 3;		/* Output sensor. 1 = 5 A, 2 = 20 A, 3 = 30 A. */
	/* No input-current pin on this board. */
	const int  RedLedPin	 		= PB0;
	const int  RedLedPort 			= 1;
	const int  YellowLedPin			= PB1;
	const int  YellowLedPort		= 1;
	const int  GreenLedPin 			= PB2;
	const int  GreenLedPort			= 1;
	const int  ButtonPin 			= PD5;
	const int  ButtonPort 			= 3;

	const int  PotPin	 			= PD0;
	const int  PotPort 				= 2;
	const uint32_t SerialBaud		= 9600;

#endif	/* DCC = 1 */


#endif /* HARDWARE_HPP_ */
