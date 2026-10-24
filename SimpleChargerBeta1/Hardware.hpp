/*
 * Hardware.hpp
 *
 *  Created on: 11 dec. 2018
 *      Author: Ola Andersson
 */

#ifndef HARDWARE_HPP_
#define HARDWARE_HPP_

#include <avr/io.h>
#include <stdint.h>

// Define for witch hardware to compile
#define SmedbyCharger11				1
#define DCC							0

#define Serial						1
#define CAN							0

#if SmedbyCharger11 == 1

// Define what pins and ports to use on the MPU
// in reality, PD6 is really just '6'

	const int  InputVoltPin  		= PC2;				// Pin used to measure Battery-volt PC2 = A2
	const int  InputVoltPort  		= 2;				// Port used to measure Battery-volt
	const int  OutputCurrentPin 	= PC0;				// Pin used to measure Motor-volt PC0 = A0
	const int  OutputCurrentPort 	= 2;				// Port used to measure Motor-volt
	const int  OutputVoltPin 		= PC1;				// Pin used  to measure output-volt PC1 = A1
	const int  OutputVoltPort 		= 2;				// Port used to measure output-volt
	const int  Pin12V 				= PC3;				// Pin used to measure 12 volt rail PC3 = A3
	const int  Port12V 				= 2;				// Port used to measure 12 volt rail
	const int  TemperaturPin 		= PD7;				// Pin used to measure mosfet-temperatur PD7 = D7
	const int  TemperaturPort 		= 3;				// Port used to measure mosfet-temperatur
	const int  pwmPin 				= PB1;				// Pin used for PWM-charge-output PB1 = D9
	const int  pwmPort 				= 1;				// Port used for PWM-charge-output

	const int  Temp_Measure_type 	= 2;				// Type 1 = NTC-resistor, 2 = DS1820 temp sensor.
	const int  Current_Measure_type = 2;				// Type 1 = 5A, 2 = 20A, 3 = 30A (Defines witch type of current sense sensor is used.
	const int  RedLedPin	 		= PD5;				// Red LED pin
	const int  RedLedPort 			= 3;				// Red LED port
	const int  YellowLedPin			= PD4;				// Yellow LED pin
	const int  YellowLedPort		= 3;				// Yellow LED port
	const int  GreenLedPin 			= PD3;				// Green LED pin
	const int  GreenLedPport		= 3;				// Green LED port
	const int  ButtonPin 			= PD2;				// Button pin
	const int  ButtonPort 			= 3;				// Button port
	const uint32_t SerialBaud		= 115200;			// UART baud. main skickar värdet till sSerial.

#endif	// SmedbyCharger11 = 1

#if DCC == 1

// Define what pins and ports to use on the MPU
// in reality, PD6 is really just '6'

	const int  InputVoltPin  		= PC4;				// Pin used to measure inputVolt PC4 = A4
	const int  InputVoltPort  		= 2;				// Port used to measure inputvolt
	const int  OutputCurrentPin 	= PC1;				// Pin used to measure Output current PC1 = A1
	const int  OutputCurrentPort 	= 2;				// Port used to measure Output current
	const int  OutputVoltPin 		= PC2;				// Pin used  to measure output-volt PC2 = A2
	const int  OutputVoltPort 		= 2;				// Port used to measure output-volt
	const int  Pin12V 				= PC3;				// Pin used to measure 12 volt rail PC3 = A3
	const int  Port12V 				= 2;				// Port used to measure 12 volt rail
	const int  TemperaturPin 		= PD4;				// Pin used to measure mosfet-temperatur PD4 = D4
	const int  TemperaturPort 		= 3;				// Port used to measure mosfet-temperatur
	const int  pwmPin 				= PD6;				// Pin used for PWM-output PD6 = D6
	const int  pwmPort 				= 3;				// Port used for PWM-output

	const int  Temp_Measure_type 	= 2;				// Type 1 = NTC-resistor, 2 = DS1820 temp sensor.
	const int  Current_Measure_type = 3;				// Type 1 = 5A, 2 = 20A, 3 = 30A (Defines witch type of current sense sensor is used.
	const int  RedLedPin	 		= PB0;				// Red LED pin
	const int  RedLedPort 			= 1;				// Red LED port
	const int  YellowLedPin			= PB1;				// Yellow LED pin
	const int  YellowLedPort		= 1;				// Yellow LED port
	const int  GreenLedPin 			= PB2;				// Green LED pin
	const int  GreenLedPport		= 1;				// Green LED port
	const int  ButtonPin 			= PD5;				// Button pin
	const int  ButtonPort 			= 3;				// Button port

	const int  PotPin	 			= PD0;				// Potentiometer pin
	const int  PotPort 				= 2;				// Potentiometer port
	const uint32_t SerialBaud		= 9600;				// UART baud. main skickar värdet till sSerial.

#endif	// DCC = 1


#endif /* HARDWARE_HPP_ */
