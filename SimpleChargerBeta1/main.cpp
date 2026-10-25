/*
 * ATmega328P-laddare, TunerStudio v2.2 (tunerStudio_v2.2.ini).
 *
 * En huvudloop. main oppnar UART och matar TunerStudio; sidorna ligger i
 * EepromStore (RAM-kopia). Laddlogiken ligger i shared/charger (host-
 * testad), autoparametrarna i shared/autoParams.
 *
 *  - Boot: sidorna laddas fran EEPROM (giltiga) annars standardvarden +
 *    eepromWarnBits. diagMode tvingas till Normal. Laddaren startar AV:
 *    duty 0 tills E1 (eller batterianslutning vid startMode Auto/Timer).
 *  - EEPROM skrivs BARA pa B (burn) via BurnGuard: avvisas om laddning
 *    pagar (chargingActive, BESLUT nr 25), annars utgang av -> skriv ->
 *    verifiera -> aterstall. Kopieringsknapparna E7..E9 branner aldrig.
 *  - Regulatorn gar var 100 ms i heltal (cV, cA).
 * Inget av detta ar verifierat pa hardvara eller i TunerStudio.
 */

#include "sGPIO.hpp"
#include "sPWM.hpp"
#include "sVolt.hpp"
#include "sCurrent.hpp"
#include "sSerial.hpp"
#include "sMillis.hpp"
#include "sEeprom.hpp"
#include "eepromStore.hpp"
#include "tunerStudio.hpp"
#include "tsLayout.hpp"
#include "autoParams.hpp"
#include "pageCommands.hpp"
#include "charger.hpp"
#include "Hardware.hpp"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

/* MCUSR sparas innan nagot annat kor och nollas (annars kan WDT-flaggan
 * ge en reset-loop). OBS [U]: Optiboot kan sjalv ha nollat MCUSR innan
 * applikationen startar; da blir resetCause 0. Ej verifierat. */
static uint8_t gResetFlags __attribute__((section(".noinit")));
extern "C" void earlyInit(void) __attribute__((naked, used, section(".init3")));
extern "C" void earlyInit(void)
{
	gResetFlags = MCUSR;
	MCUSR = 0U;
	wdt_disable();
}

/* resetCause (realtime 66): b0 power-on, b1 extern, b2 brown-out, b3 watchdog
 * = samma bitordning som MCUSR (PORF, EXTRF, BORF, WDRF). */
static const uint8_t ResetCauseMask = 0x0FU;

/* PWM: Timer1, WGM10 (8 bit phase correct), CS10 -> 16 MHz / 510 = 31,4 kHz.
 * Beraknat fran registren i sPWM, inte matt. */
static const uint8_t PwmFreqKhz = 31U;
/* OCR = steg x 10 (som tidigare setDuty). */
static const uint8_t OcrPerStep = 10U;
static const uint32_t LoopMs = 100U;
static const uint32_t RefreshHoldMs = 1000U;   /* pageRefreshReq ca 1 s (otestat i TS) */
/* Spanningsdelarnas fullskala i cV (ADC 1023), samma som tidigare 55/65/20,1 V. */
static const uint16_t OutVoltFullCv = 5500U;
static const uint16_t InVoltFullCv = 6500U;
static const uint16_t DriverVoltFullCv = (uint16_t)(DriverVoltFullScale * 10);

sGPIO green(GreenLedPort, GreenLedPin, 1);
sGPIO yellow(YellowLedPort, YellowLedPin, 1);
sGPIO red(RedLedPort, RedLedPin, 1);

static EepromStore gStore(sEepromBackend());
static TunerStudio gTs(gStore);
static Charger gCharger;
static AutoParams::Result gAuto;

struct AppState
{
	sPWM*    pwm;
	uint32_t refreshMs;
	uint8_t  refresh;      /* pageRefreshReq */
	bool     zeroReq;      /* E6 */
	bool     rebootReq;    /* E5 */
};
static AppState gApp;

static void putU16(uint8_t* rt, uint16_t off, uint16_t v)
{
	rt[off] = (uint8_t)(v & 0xFFU);
	rt[off + 1U] = (uint8_t)(v >> 8);
}

static void setOutput(uint8_t duty)
{
	if (gApp.pwm != 0)
	{
		gApp.pwm->setDutyValue((int)duty * (int)OcrPerStep);
	}
}

/* Raknar om eff_* (boot, efter sidskrivning, efter E7..E9). */
static void computeAuto(void)
{
	AutoParams::Input in;
	PageCommands::autoInput(gStore.page(TsLayout::PageIdx1), gStore.page(TsLayout::PageIdx3), in);
	AutoParams::compute(gStore.page(TsLayout::PageIdx5), in, gAuto);
}

/* ---- BurnGuard ---- */
static bool guardCharging(void*)
{
	return gCharger.chargingActive();
}
static void guardOff(void*)
{
	setOutput(0U);
}
static void guardRestore(void*)
{
	setOutput(gCharger.duty());
}

/* ---- TunerStudio-krokar ---- */
static void uartSend(const uint8_t* data, uint8_t len, void* ctx)
{
	sSerial* serial = static_cast<sSerial*>(ctx);
	for (uint8_t i = 0U; i < len; i++)
	{
		serial->putChar(data[i]);
	}
}

static void onBurn(void*)
{
	/* Resultatet syns via eepromWarnBits (BurnFailed) i realtime 114. */
	(void)gStore.burn();
}

static void onCommand(uint8_t code, void*)
{
	if (gCharger.command(code))
	{
		return;
	}
	if (code == 5U)
	{
		if (!gCharger.chargingActive())
		{
			gApp.rebootReq = true;
		}
	}
	else if (code == 6U)
	{
		if (!gCharger.chargingActive())
		{
			gApp.zeroReq = true;
		}
	}
	else if ((code >= 7U) && (code <= 9U))
	{
		AutoParams::Input in;
		uint8_t bits;
		PageCommands::autoInput(gStore.page(TsLayout::PageIdx1), gStore.page(TsLayout::PageIdx3), in);
		bits = PageCommands::apply(code, gStore.page(TsLayout::PageIdx5), in);
		if (bits != 0U)
		{
			computeAuto();
			gApp.refresh |= bits;
			gApp.refreshMs = millis();
		}
	}
	else
	{
		/* okand kod: ignoreras */
	}
}

static void onPageWritten(uint8_t, void*)
{
	computeAuto();
}

static void pollSerial(sSerial& serial)
{
	uint8_t byte;
	while (serial.readByte(&byte))
	{
		gTs.onByte(byte, millis());
	}
	gTs.poll(millis());
}

int main()
{
	red.setHigh();
	green.setHigh();
	yellow.setHigh();

	millisInit();

	/* Pinnen lag innan allt annat. Duty 0 ar inget laddkommando. */
	sPWM myPwm(pwmPort, pwmPin);
	myPwm.setDutyValue(0);
	gApp.pwm = &myPwm;
	gApp.refresh = 0U;
	gApp.refreshMs = 0U;
	gApp.zeroReq = false;
	gApp.rebootReq = false;

	BurnGuard guard;
	guard.chargingActive = &guardCharging;
	guard.outputOff = &guardOff;
	guard.outputRestore = &guardRestore;
	guard.ctx = 0;
	gStore.setBurnGuard(guard);
	(void)gStore.load();          /* ogiltig sida -> standard + eepromWarnBits */
	computeAuto();

	uint8_t* rt = gTs.realtime;
	for (uint8_t i = 0U; i < TsLayout::RtBlockSize; i++)
	{
		rt[i] = 0U;
	}
	rt[TsLayout::RtResetCause] = (uint8_t)(gResetFlags & ResetCauseMask);
	rt[TsLayout::RtPwmFreqNow] = PwmFreqKhz;

	sSerial serial;
	serial.init(SerialBaud);
	gTs.setSend(uartSend, &serial);
	TunerStudio::Hooks hooks;
	hooks.burn = &onBurn;
	hooks.command = &onCommand;
	hooks.pageWritten = &onPageWritten;
	hooks.ctx = 0;
	gTs.setHooks(hooks);

	sei();

	/* Stromgivare enligt sida 3 (lases vid boot; andring kraver omstart).
	 * 0 = 5 A, 1 = 20 A, 2 = 30 A -> Current-typ 1..3. 3 (shunt) stods
	 * inte av Current -> Hardware.hpp-typen anvands. */
	const uint8_t sensorBits = gStore.page(TsLayout::PageIdx3)[TsLayout::P3SensorBits];
	const uint8_t sensorSel = (uint8_t)((sensorBits >> 4) & 0x03U);
	const int sensorType = (sensorSel < 3U) ? (int)sensorSel + 1 : Current_Measure_type;
	Current outputCurrent(OutputCurrentPort, OutputCurrentPin, sensorType);
	if ((sensorBits & 0x40U) != 0U)
	{
		/* ampZeroAtBoot: stabilisera, nollstall med utgangen av. */
		while (millis() < 200U)
		{
			pollSerial(serial);
		}
		outputCurrent.zeroAmpCalibrate();
	}

	Volt outputVolt(OutputVoltPort, OutputVoltPin, OutVoltFullCv);
	Volt inputVolt(InputVoltPort, InputVoltPin, InVoltFullCv);
	Volt driverVolt(Port12V, Pin12V, DriverVoltFullCv);

	uint32_t last = millis();
	for (;;)
	{
		const uint32_t now = millis();
		if ((uint32_t)(now - last) >= LoopMs)
		{
			Charger::Measure m;
			Charger::Pages pg;
			int32_t ma;
			uint16_t driverCv;
			uint8_t duty;

			last = now;
			green.setLow();

			m.outCv = outputVolt.readVolt();
			m.inCv = inputVolt.readVolt();
			/* mA -> cA i 32 bit (ingen (int)amp*1000-overflow). */
			ma = (int32_t)outputCurrent.readCurrent();
			ma = ma / 10;
			if (ma > 32767L)
			{
				ma = 32767L;
			}
			if (ma < -32767L)
			{
				ma = -32767L;
			}
			m.outCa = (int16_t)ma;
			driverCv = driverVolt.readVolt();

			pg.p1 = gStore.page(TsLayout::PageIdx1);
			pg.p2 = gStore.page(TsLayout::PageIdx2);
			pg.p3 = gStore.page(TsLayout::PageIdx3);
			pg.p5 = gStore.page(TsLayout::PageIdx5);
			duty = gCharger.update(m, pg, gAuto, now);
			setOutput(duty);

			if (gApp.zeroReq)
			{
				gApp.zeroReq = false;
				if (!gCharger.chargingActive())
				{
					outputCurrent.zeroAmpCalibrate();
				}
			}

			if ((gApp.refresh != 0U) && ((uint32_t)(now - gApp.refreshMs) >= RefreshHoldMs))
			{
				gApp.refresh = 0U;
			}

			/* Realtidsblocket. Temperaturer: ingen givare i v2.2 (etapp 6)
			 * -> 0 och ADC 0, dvs. "ej tillganglig". Instrom saknas -> 0. */
			putU16(rt, TsLayout::RtSeconds, (uint16_t)(now / 1000UL));
			putU16(rt, TsLayout::RtInVolt, m.inCv);
			putU16(rt, TsLayout::RtOutVolt, m.outCv);
			putU16(rt, TsLayout::RtInAmp, 0U);
			putU16(rt, TsLayout::RtOutAmp, (uint16_t)m.outCa);
			putU16(rt, TsLayout::RtDriverVolt, driverCv);
			putU16(rt, TsLayout::RtOutVoltAdc, outputVolt.getLastADCValue());
			putU16(rt, TsLayout::RtOutAmpAdc, (uint16_t)outputCurrent.getLastADCValue());
			putU16(rt, TsLayout::RtInVoltAdc, inputVolt.getLastADCValue());
			putU16(rt, TsLayout::RtDriverVoltAdc, driverVolt.getLastADCValue());
			gCharger.writeRealtime(rt);
			AutoParams::writeRealtime(gAuto, rt);
			rt[TsLayout::RtPageRefreshReq] = gApp.refresh;
			rt[TsLayout::RtEepromWarnBits] = gStore.warnBits();

			green.setHigh();

			if (gApp.rebootReq && !gCharger.chargingActive())
			{
				/* E5: utgang av, WDT-reset efter ca 15 ms. */
				setOutput(0U);
				cli();
				wdt_enable(WDTO_15MS);
				for (;;)
				{
				}
			}
		}
		pollSerial(serial);
	}
}
