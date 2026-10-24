# ATmega328P, 16 MHz. Kräver avr-g++ i PATH.
MCU = atmega328p
F_CPU = 16000000UL
CXX = avr-g++
CXXFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -ffunction-sections -fdata-sections -std=gnu++98 \
	-Ishared -ISmeOS/Atmel328p/include -ISmeOS/MidleWare/include
LDFLAGS = -mmcu=$(MCU) -Wl,--gc-sections

SRCS = main.cpp \
	shared/filter.cpp shared/scale.cpp shared/tunerStudio.cpp \
	SmeOS/Atmel328p/src/sADC.cpp SmeOS/Atmel328p/src/sGPIO.cpp \
	SmeOS/Atmel328p/src/sMillis.cpp SmeOS/Atmel328p/src/sPWM.cpp SmeOS/Atmel328p/src/sSerial.cpp \
	SmeOS/MidleWare/src/sCurrent.cpp SmeOS/MidleWare/src/sVolt.cpp

OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: charger.elf

charger.elf: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) charger.elf
